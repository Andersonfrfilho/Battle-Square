// Copyright 2026 Anderson. All Rights Reserved.

#include "Data/PetCrypto.h"
#include "Misc/AutomationTest.h"
#include "Misc/Base64.h"

// T15/T16 (tasks.md): prova de interoperabilidade real entre o backend
// TypeScript (apps/api-battle-pets, apps/worker-pet-sync) e o OpenSSL da
// Unreal. As fixtures abaixo foram geradas RODANDO o código TS de
// verdade (mirror-crypto.ts / pet-signing.ts), não inventadas — se a
// interoperabilidade quebrar (framing, ordem de campo no JSON canônico,
// etc.), este teste falha, não um teste que só verifica OpenSSL contra
// si mesmo.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPetCryptoAesGcmInteropTest,
	"BattleSquare.PetCrypto.DecryptsBufferEncryptedByTypeScriptSidecar",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPetCryptoAesGcmInteropTest::RunTest(const FString& Parameters)
{
	// Fixture gerada por apps/worker-pet-sync/src/mirror/mirror-crypto.ts
	const FString KeyHex = FString(TEXT("aabbccddeeff00112233445566778899aabbccddeeff00112233445566778899")).Left(64);
	const FString EncryptedBase64 = TEXT("JIRlQaCAq/teGj7eGfHUaOR903i1SowVzpL2e4wPUOVB5FJUuDgv/UcpclnDx3HQj3UzdAsQEwwGlvQ2ucDx0g==");
	const FString ExpectedPlaintext = TEXT("BATTLE_SQUARE_INTEROP_TEST_PLAINTEXT");

	TArray<uint8> Key;
	Key.SetNumUninitialized(32);
	for (int32 Index = 0; Index < 32; ++Index)
	{
		Key[Index] = static_cast<uint8>(FParse::HexNumber(*KeyHex.Mid(Index * 2, 2)));
	}

	TArray<uint8> Encrypted;
	if (!TestTrue(TEXT("Base64 do fixture decodifica"), FBase64::Decode(EncryptedBase64, Encrypted)))
	{
		return false;
	}

	TArray<uint8> Plaintext;
	const bool bDecrypted = BattlePetCrypto::DecryptMirrorBuffer(Encrypted, Key, Plaintext);
	TestTrue(TEXT("Decifra com sucesso usando a chave correta"), bDecrypted);

	if (bDecrypted)
	{
		// ANSI_TO_TCHAR presume string terminada em '\0' — Plaintext é um
		// buffer de bytes crus, SEM terminador, então essa macro leria
		// memória além do fim do buffer. FUTF8ToTCHAR com comprimento
		// explícito é o conversor correto aqui.
		const FUTF8ToTCHAR Converter(reinterpret_cast<const UTF8CHAR*>(Plaintext.GetData()), Plaintext.Num());
		const FString ActualPlaintext(Converter.Length(), Converter.Get());
		TestEqual(TEXT("Plaintext bate com o que o TypeScript cifrou"), ActualPlaintext, ExpectedPlaintext);
	}

	// Chave errada — o mesmo GCM que detectou adulteração do lado
	// TypeScript precisa detectar aqui também.
	TArray<uint8> WrongKey;
	WrongKey.SetNumUninitialized(32);
	for (int32 Index = 0; Index < 32; ++Index) { WrongKey[Index] = 0xFF; }

	TArray<uint8> ShouldFail;
	TestFalse(TEXT("Chave errada é rejeitada pela autenticação do GCM"), BattlePetCrypto::DecryptMirrorBuffer(Encrypted, WrongKey, ShouldFail));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPetCryptoEd25519InteropTest,
	"BattleSquare.PetCrypto.VerifiesSignatureFromTypeScriptBackend",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPetCryptoEd25519InteropTest::RunTest(const FString& Parameters)
{
	// Fixture gerada por apps/api-battle-pets/src/pet/pet-signing.ts
	const FString PublicKeyPem =
		TEXT("-----BEGIN PUBLIC KEY-----\n")
		TEXT("MCowBQYDK2VwAyEASmCzzcPySYHgKJgbH2uuAjtP4gXGRl2jP4ynBxOF2K0=\n")
		TEXT("-----END PUBLIC KEY-----\n");

	const FString CanonicalPayload =
		TEXT("{\"id\":\"fixture-001\",\"name\":\"Fluffy\",\"type\":\"Cat\",\"attack\":10,")
		TEXT("\"defense\":5,\"speed\":8,\"maxHealth\":50,\"updatedAt\":\"2026-08-25T00:00:00.000Z\"}");

	const FString SignatureBase64 = TEXT("e1jAOs8kmoiV7JhiQB0MZ/D+OhaVbgg2LHlhdavXWjo9C4riK2URrEKMiNICW+w0Jbh0X+ZY6aK55EIqAJogDA==");

	TArray<uint8> Signature;
	if (!TestTrue(TEXT("Base64 da assinatura decodifica"), FBase64::Decode(SignatureBase64, Signature)))
	{
		return false;
	}

	TestTrue(
		TEXT("Assinatura genuína do backend TypeScript verifica com sucesso no OpenSSL da Unreal"),
		BattlePetCrypto::VerifyEd25519Signature(CanonicalPayload, Signature, PublicKeyPem));

	// Payload adulterado — 1 caractere alterado — precisa falhar.
	const FString TamperedPayload = CanonicalPayload.Replace(TEXT("\"attack\":10"), TEXT("\"attack\":99"));
	TestFalse(
		TEXT("Payload adulterado é rejeitado"),
		BattlePetCrypto::VerifyEd25519Signature(TamperedPayload, Signature, PublicKeyPem));

	return true;
}

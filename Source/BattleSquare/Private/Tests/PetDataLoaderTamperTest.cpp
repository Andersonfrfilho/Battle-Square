// Copyright 2026 Anderson. All Rights Reserved.

#include "Data/PetDataLoader.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"

// T22 (tasks.md, PETDB-10): espelho local válido do ponto de vista da
// CHAVE DE CRIPTOGRAFIA (decifra corretamente), mas com 1 registro
// adulterado DIRETO no arquivo, fora do fluxo de sync — a assinatura
// Ed25519 daquele registro não bate mais. Diferente de T17's
// WrongKeyFailsExplicitly (chave errada, decriptação falha inteira):
// aqui a decriptação TEM sucesso, e é a reverificação por registro que
// precisa pegar o problema.
//
// Fixture gerada rodando o código real: espelho sincronizado de verdade
// (mesmo da T17), decifrado, 1 campo alterado direto no SQLite, recifrado
// — nunca passou pelo fluxo de assinatura de novo.

namespace
{
	TArray<uint8> HexStringToBytes(const FString& HexString)
	{
		TArray<uint8> Bytes;
		Bytes.SetNumUninitialized(HexString.Len() / 2);
		for (int32 Index = 0; Index < Bytes.Num(); ++Index)
		{
			Bytes[Index] = static_cast<uint8>(FParse::HexNumber(*HexString.Mid(Index * 2, 2)));
		}
		return Bytes;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPetDataLoaderTamperedRecordTest,
	"BattleSquare.PetDataLoader.TamperedRecordIsRejectedOthersSurvive",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPetDataLoaderTamperedRecordTest::RunTest(const FString& Parameters)
{
	// PetDataLoader loga em nível Warning (não Error) para registro
	// adulterado — não precisa de AddExpectedError (L-014 só se aplica a
	// UE_LOG(Error), verificado antes de adicionar a chamada aqui).
	const FString MirrorPath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("PetMirrorFixture"), TEXT("pets-mirror-tampered.sqlite"));
	if (!TestTrue(TEXT("Fixture adulterada existe"), FPlatformFileManager::Get().GetPlatformFile().FileExists(*MirrorPath)))
	{
		return false;
	}

	const TArray<uint8> Key = HexStringToBytes(TEXT("ad2f50e7781fcca5148299d249825f117ff78cc0b1758de30ce9832f01918e39"));
	const FString PublicKeyPem =
		TEXT("-----BEGIN PUBLIC KEY-----\n")
		TEXT("MCowBQYDK2VwAyEASmCzzcPySYHgKJgbH2uuAjtP4gXGRl2jP4ynBxOF2K0=\n")
		TEXT("-----END PUBLIC KEY-----\n");

	TArray<FLoadedPetRecord> LoadedPets;
	int32 RejectedCount = 0;
	const bool bLoaded = FPetDataLoader::LoadVerifiedPets(MirrorPath, Key, PublicKeyPem, LoadedPets, RejectedCount);

	// A chave estava certa — decriptação do ARQUIVO teve sucesso. O
	// carregamento geral não falha por causa de 1 registro ruim.
	TestTrue(TEXT("Carregamento prossegue apesar de 1 registro adulterado"), bLoaded);
	TestEqual(TEXT("Exatamente 1 registro rejeitado"), RejectedCount, 1);

	// O pet adulterado (id conhecido da geração da fixture) NÃO deve
	// aparecer entre os carregados.
	const FString TamperedPetId = TEXT("4f953fcc-7fc3-4052-916e-fee6af466f4e");
	bool bTamperedPetLoaded = false;
	for (const FLoadedPetRecord& Pet : LoadedPets)
	{
		if (Pet.Id == TamperedPetId)
		{
			bTamperedPetLoaded = true;
		}
	}
	TestFalse(TEXT("Pet adulterado não está entre os carregados"), bTamperedPetLoaded);

	// Os outros pets do mesmo espelho continuam presentes — a
	// adulteração de 1 registro não derruba o espelho inteiro.
	TestTrue(TEXT("Outros pets (não adulterados) foram carregados normalmente"), LoadedPets.Num() > 0);

	return true;
}

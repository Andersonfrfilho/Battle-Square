// Copyright 2026 Anderson. All Rights Reserved.

#include "Data/PetDataLoader.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"

// T17 (tasks.md): prova de ponta a ponta contra um espelho REAL, gerado
// rodando a cadeia de verdade — apps/api-battle-pets criou 2 pets,
// apps/worker-pet-sync sincronizou e assinou. Nada aqui é simulado.

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
	FPetDataLoaderRealMirrorTest,
	"BattleSquare.PetDataLoader.LoadsAndVerifiesRealSyncedMirror",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPetDataLoaderRealMirrorTest::RunTest(const FString& Parameters)
{
	const FString MirrorPath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("PetMirrorFixture"), TEXT("pets-mirror.sqlite"));
	if (!TestTrue(TEXT("Arquivo de fixture do espelho existe"), FPlatformFileManager::Get().GetPlatformFile().FileExists(*MirrorPath)))
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

	TestTrue(TEXT("Carregamento bem-sucedido"), bLoaded);
	TestTrue(TEXT("Ao menos 1 pet carregado do espelho real"), LoadedPets.Num() > 0);
	TestEqual(TEXT("Nenhum registro rejeitado — espelho não foi adulterado"), RejectedCount, 0);

	// Pet dedicado a este teste — nunca reusa "Fluffy" nem outros pets
	// mutados por verificação manual em fases anteriores da sessão
	// (aquele "attack":10 vira 15 depois de um PUT, e o teste ficaria
	// dependente de estado externo mutável).
	bool bFoundFixturePet = false;
	for (const FLoadedPetRecord& Pet : LoadedPets)
	{
		if (Pet.Name == TEXT("UnrealTestFixturePet"))
		{
			bFoundFixturePet = true;
			TestEqual(TEXT("FixturePet.Type"), Pet.Type, FString(TEXT("Cat")));
			TestEqual(TEXT("FixturePet.Attack"), Pet.Attack, 10);
			TestEqual(TEXT("FixturePet.Defense"), Pet.Defense, 5);
			TestEqual(TEXT("FixturePet.Speed"), Pet.Speed, 8);
			TestEqual(TEXT("FixturePet.MaxHealth"), Pet.MaxHealth, 50);
		}
	}
	TestTrue(TEXT("Pet de fixture dedicado (criado via API real) está entre os pets carregados"), bFoundFixturePet);

	return true;
}

// PETDB-06: espelho ausente é erro explícito, nunca lista vazia silenciosa.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPetDataLoaderMissingMirrorTest,
	"BattleSquare.PetDataLoader.MissingMirrorFailsExplicitly",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPetDataLoaderMissingMirrorTest::RunTest(const FString& Parameters)
{
	// O log de erro AQUI é o comportamento correto e esperado do
	// PetDataLoader (PETDB-06) — sem isto, o framework de automação
	// conta qualquer UE_LOG(Error) como falha de teste automaticamente.
	AddExpectedError(TEXT("espelho local ausente"), EAutomationExpectedErrorFlags::Contains, 1);

	const FString NonExistentPath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("nao-existe.sqlite"));
	const TArray<uint8> Key = HexStringToBytes(TEXT("ad2f50e7781fcca5148299d249825f117ff78cc0b1758de30ce9832f01918e39"));

	TArray<FLoadedPetRecord> LoadedPets;
	int32 RejectedCount = 0;
	const bool bLoaded = FPetDataLoader::LoadVerifiedPets(NonExistentPath, Key, TEXT(""), LoadedPets, RejectedCount);

	TestFalse(TEXT("Carregamento falha explicitamente com espelho ausente"), bLoaded);
	TestTrue(TEXT("Nenhum pet retornado"), LoadedPets.IsEmpty());

	return true;
}

// Chave errada: decriptação falha, carregamento falha — nunca dado
// corrompido silencioso.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPetDataLoaderWrongKeyTest,
	"BattleSquare.PetDataLoader.WrongKeyFailsExplicitly",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPetDataLoaderWrongKeyTest::RunTest(const FString& Parameters)
{
	// Idem: "falha ao decifrar" é o resultado CORRETO com chave errada
	// (AD-018 — GCM detecta e recusa). Precisa ser declarado esperado.
	AddExpectedError(TEXT("falha ao decifrar"), EAutomationExpectedErrorFlags::Contains, 1);

	const FString MirrorPath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("PetMirrorFixture"), TEXT("pets-mirror.sqlite"));
	const TArray<uint8> WrongKey = HexStringToBytes(TEXT("0000000000000000000000000000000000000000000000000000000000000000"));

	TArray<FLoadedPetRecord> LoadedPets;
	int32 RejectedCount = 0;
	const bool bLoaded = FPetDataLoader::LoadVerifiedPets(MirrorPath, WrongKey, TEXT(""), LoadedPets, RejectedCount);

	TestFalse(TEXT("Chave errada falha a decriptação — carregamento recusado"), bLoaded);

	return true;
}

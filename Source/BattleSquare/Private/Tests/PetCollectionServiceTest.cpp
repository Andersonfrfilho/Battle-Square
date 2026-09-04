// Copyright 2026 Anderson. All Rights Reserved.

#include "Meta/CaptureQueueRules.h"
#include "Meta/PetCollectionService.h"
#include "Meta/TrainerSpecialtyRules.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"

namespace
{
	FOwnedPetInstance MakeInstance(const FString& CatalogId, const FString& Name, const FString& Type)
	{
		FOwnedPetInstance Instance;
		Instance.CatalogId = CatalogId;
		Instance.Name = Name;
		Instance.Type = Type;
		return Instance;
	}

	// Slot dedicado a teste — nunca o de produção (design.md, Riscos).
	const FString TestSlotName = TEXT("PetCollectionTestSlot");

	void CleanupTestSlot()
	{
		if (UGameplayStatics::DoesSaveGameExist(TestSlotName, 0))
		{
			UGameplayStatics::DeleteGameInSlot(TestSlotName, 0);
		}
	}
}

// T2: pet novo captura; pet repetido não duplica; tipos iguais com
// CatalogId diferente são capturas independentes.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPetCollectionServiceCaptureIfNewInMemoryTest,
	"BattleSquare.Meta.PetCollectionService.CaptureIfNewInMemory",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPetCollectionServiceCaptureIfNewInMemoryTest::RunTest(const FString& Parameters)
{
	UPetCollectionSaveGame* SaveGame = NewObject<UPetCollectionSaveGame>();

	const bool bFirstCapture = FPetCollectionService::CaptureIfNewInMemory(SaveGame, MakeInstance(TEXT("id-fogo-1"), TEXT("Brasa"), TEXT("Fogo")));
	TestTrue(TEXT("Primeira captura de um CatalogId novo retorna true"), bFirstCapture);
	TestEqual(TEXT("Coleção tem 1 instância"), SaveGame->OwnedPets.Num(), 1);

	const bool bDuplicateCapture = FPetCollectionService::CaptureIfNewInMemory(SaveGame, MakeInstance(TEXT("id-fogo-1"), TEXT("Brasa"), TEXT("Fogo")));
	TestFalse(TEXT("Capturar o mesmo CatalogId de novo retorna false"), bDuplicateCapture);
	TestEqual(TEXT("Coleção continua com 1 instância — sem duplicata"), SaveGame->OwnedPets.Num(), 1);

	// Mesmo Type, CatalogId diferente — captura independente.
	const bool bSameTypeDifferentId = FPetCollectionService::CaptureIfNewInMemory(SaveGame, MakeInstance(TEXT("id-fogo-2"), TEXT("Chama"), TEXT("Fogo")));
	TestTrue(TEXT("Mesmo Type, CatalogId diferente, é captura independente"), bSameTypeDifferentId);
	TestEqual(TEXT("Coleção agora tem 2 instâncias"), SaveGame->OwnedPets.Num(), 2);

	return true;
}

// T3: captura persiste de verdade através de um ciclo salvar/carregar;
// slot nunca usado retorna coleção vazia, sem erro.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPetCollectionServicePersistsAcrossLoadCycleTest,
	"BattleSquare.Meta.PetCollectionService.PersistsAcrossLoadCycle",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPetCollectionServicePersistsAcrossLoadCycleTest::RunTest(const FString& Parameters)
{
	CleanupTestSlot();

	// Slot nunca usado — coleção vazia, sem erro.
	const TArray<FOwnedPetInstance> EmptyCollection = FPetCollectionService::LoadCollection(TestSlotName);
	TestEqual(TEXT("Slot nunca usado retorna coleção vazia"), EmptyCollection.Num(), 0);

	const bool bCaptured = FPetCollectionService::CaptureIfNew(TestSlotName, MakeInstance(TEXT("id-agua-1"), TEXT("Gotinha"), TEXT("Agua")));
	TestTrue(TEXT("Captura real (com persistência) retorna true para pet novo"), bCaptured);

	const TArray<FOwnedPetInstance> Reloaded = FPetCollectionService::LoadCollection(TestSlotName);
	TestEqual(TEXT("Coleção recarregada tem a instância persistida"), Reloaded.Num(), 1);
	if (Reloaded.Num() == 1)
	{
		TestEqual(TEXT("CatalogId persistido corretamente"), Reloaded[0].CatalogId, FString(TEXT("id-agua-1")));
	}

	const bool bDuplicateCapture = FPetCollectionService::CaptureIfNew(TestSlotName, MakeInstance(TEXT("id-agua-1"), TEXT("Gotinha"), TEXT("Agua")));
	TestFalse(TEXT("Captura real repetida não duplica"), bDuplicateCapture);
	TestEqual(TEXT("Coleção continua com 1 instância após tentativa duplicada"), FPetCollectionService::LoadCollection(TestSlotName).Num(), 1);

	CleanupTestSlot();
	return true;
}

// GRAVAR A COLEÇÃO NÃO APAGA O TREINADOR, e vice-versa.
//
// As duas funções montam um save NOVO e gravam por cima do slot. Sem cada uma
// reler a metade que não é dela, ganhar experiência apagaria as especialidades
// — uma escolha que não se refaz, sumindo sem nada indicar quando nem por quê.
//
// Foi o defeito que este commit quase introduziu, e ele não apareceria em
// nenhum teste de treino: só cruzando as duas gravações.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSavingOneHalfKeepsTheOtherTest,
	"BattleSquare.Meta.PetCollectionService.SavingOneHalfKeepsTheOther",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSavingOneHalfKeepsTheOtherTest::RunTest(const FString& Parameters)
{
	CleanupTestSlot();

	FPetCollectionService::CaptureIfNew(TestSlotName,
		MakeInstance(TEXT("id-fogo-1"), TEXT("Faísca"), TEXT("Fogo")));

	FTrainerProfile Perfil;
	Perfil.Specialties.Add(TEXT("flight"));
	FPetCollectionService::SaveTrainerProfile(TestSlotName, Perfil);

	// Gravar o TREINADOR manteve a coleção.
	TestEqual(TEXT("O pet sobreviveu à gravação do treinador"),
		FPetCollectionService::LoadCollection(TestSlotName).Num(), 1);

	// Agora o caminho que a batalha percorre: mexer num pet e salvar tudo.
	TArray<FOwnedPetInstance> Colecao = FPetCollectionService::LoadCollection(TestSlotName);
	Colecao[0].Experience += 50;
	FPetCollectionService::SaveCollection(TestSlotName, Colecao);

	const FTrainerProfile Recarregado = FPetCollectionService::LoadTrainerProfile(TestSlotName);
	TestEqual(TEXT("A especialidade sobreviveu à gravação da coleção"),
		Recarregado.Specialties.Num(), 1);
	TestEqual(TEXT("E é a mesma"),
		Recarregado.Specialties.Num() == 1 ? Recarregado.Specialties[0] : FString(),
		FString(TEXT("flight")));
	TestEqual(TEXT("E a experiência foi gravada"),
		FPetCollectionService::LoadCollection(TestSlotName)[0].Experience, 50);

	CleanupTestSlot();
	return true;
}

// PS9: quem já joga migra, e o save antigo SOBREVIVE — a invariante 14.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOwnershipMigrationKeepsTheSaveTest,
	"BattleSquare.Meta.Posse.MigracaoNaoApagaOSave",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FOwnershipMigrationKeepsTheSaveTest::RunTest(const FString& Parameters)
{
	CleanupTestSlot();

	// Um save "ANTIGO": a coleção existe, e os campos da posse estão como um
	// UPROPERTY ausente desserializa — vazios e falso. É exatamente o estado
	// de quem gravou o save antes desta feature.
	{
		UPetCollectionSaveGame* Antigo = Cast<UPetCollectionSaveGame>(
			UGameplayStatics::CreateSaveGameObject(UPetCollectionSaveGame::StaticClass()));
		Antigo->OwnedPets.Add(MakeInstance(TEXT("id-fogo-1"), TEXT("Brasa"), TEXT("Fogo")));
		Antigo->OwnedPets.Add(MakeInstance(TEXT("id-agua-2"), TEXT("Gota"), TEXT("Agua")));
		// Não tocamos em PendingCaptures nem bOwnershipMigrated: eles têm de
		// nascer vazios sozinhos.
		UGameplayStatics::SaveGameToSlot(Antigo, TestSlotName, 0);
	}

	// O save antigo carrega, e a posse nasce "não migrada, nada pendente".
	FTrainerProfile Perfil = FPetCollectionService::LoadTrainerProfile(TestSlotName);
	TestFalse(TEXT("save antigo nasce NAO migrado"), Perfil.bOwnershipMigrated);
	TestEqual(TEXT("e sem captura pendente"), Perfil.PendingCaptures.Num(), 0);

	const TArray<FOwnedPetInstance> Colecao = FPetCollectionService::LoadCollection(TestSlotName);
	TestEqual(TEXT("a colecao antiga carregou inteira"), Colecao.Num(), 2);

	// A MIGRAÇÃO enfileira a coleção inteira (sem servidor: usa as regras
	// puras que o GameMode usa) e MARCA — e o save continua legível depois.
	for (const FOwnedPetInstance& Pet : Colecao)
	{
		CaptureQueueRules::Enqueue(Perfil.PendingCaptures, Pet.CatalogId,
			FString::Printf(TEXT("conta:%s"), *Pet.CatalogId));
	}
	Perfil.bOwnershipMigrated = true;
	FPetCollectionService::SaveTrainerProfile(TestSlotName, Perfil);

	// RELÊ do disco: a coleção NÃO foi apagada (contrapeso da task), e a fila
	// gravou junto.
	const TArray<FOwnedPetInstance> DepoisDaMigracao =
		FPetCollectionService::LoadCollection(TestSlotName);
	TestEqual(TEXT("migracao NAO apaga o save local"), DepoisDaMigracao.Num(), 2);

	const FTrainerProfile Remigravel = FPetCollectionService::LoadTrainerProfile(TestSlotName);
	TestTrue(TEXT("a marca de migrado persistiu"), Remigravel.bOwnershipMigrated);
	TestEqual(TEXT("os dois pets estao na fila para subir"),
		Remigravel.PendingCaptures.Num(), 2);

	// REPETÍVEL: enfileirar de novo não duplica — a fila deduplica por
	// catálogo (e o servidor tem a unicidade de PS1 por baixo).
	FTrainerProfile SegundaPassada = Remigravel;
	for (const FOwnedPetInstance& Pet : DepoisDaMigracao)
	{
		CaptureQueueRules::Enqueue(SegundaPassada.PendingCaptures, Pet.CatalogId,
			FString::Printf(TEXT("conta:%s"), *Pet.CatalogId));
	}
	TestEqual(TEXT("a segunda passada NAO duplica"),
		SegundaPassada.PendingCaptures.Num(), 2);

	CleanupTestSlot();
	return true;
}

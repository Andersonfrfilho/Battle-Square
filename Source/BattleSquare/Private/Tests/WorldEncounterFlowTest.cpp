// Copyright 2026 Anderson. All Rights Reserved.

#include "World/WorldEncounterFlow.h"
#include "World/WorldBattleTransitionService.h"
#include "World/EncounterDetectionComponent.h"
#include "World/WorldEncounterActor.h"
#include "Battle/BattleArena.h"
#include "Misc/AutomationTest.h"
#include "GameFramework/Actor.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

namespace
{
	UWorld* CreateFlowTestWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
		FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
		WorldContext.SetCurrentWorld(World);
		World->InitializeActorsForPlay(FURL());
		return World;
	}

	void DestroyFlowTestWorld(UWorld* World)
	{
		if (!World)
		{
			return;
		}
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
	}

	FLoadedPetRecord MakeRecord(const FString& Id, const FString& Name, const FString& Type)
	{
		FLoadedPetRecord Record;
		Record.Id = Id;
		Record.Name = Name;
		Record.Type = Type;
		Record.Attack = 20;
		Record.Defense = 10;
		Record.Speed = 12;
		Record.MaxHealth = 60;
		return Record;
	}

	FEncounterMatchParams MakeMatchParams()
	{
		FEncounterMatchParams Params;
		Params.AvailablePets.Add(MakeRecord(TEXT("pet-jogador"), TEXT("Jogador"), TEXT("Fogo")));
		Params.AvailablePets.Add(MakeRecord(TEXT("pet-do-mundo"), TEXT("DoMundo"), TEXT("Agua")));
		Params.PlayerCatalogId = TEXT("pet-jogador");
		// Slot dedicado: nunca poluir o slot de produção em teste.
		Params.PetCollectionSlotName = TEXT("EncounterFlowTestCollection");
		return Params;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEncounterMatchAssemblerBuildsBothSidesTest,
	"BattleSquare.World.EncounterMatchAssembler.BuildsBothSidesFromCatalogIds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEncounterMatchAssemblerBuildsBothSidesTest::RunTest(const FString& Parameters)
{
	FEncounterMatchParams Params = MakeMatchParams();
	Params.EncounterCatalogId = TEXT("pet-do-mundo");

	FBattleState InitialState;
	TArray<FPetPresentationInfo> Presentations;
	TestTrue(TEXT("montagem sucede com os dois CatalogId presentes"),
		FEncounterMatchAssembler::AssembleFromEncounter(Params, InitialState, Presentations));

	TestEqual(TEXT("dois pets no estado inicial"), InitialState.Pets.Num(), 2);
	TestEqual(TEXT("lado 0 é o pet do jogador"), Presentations[0].CatalogId, FString(TEXT("pet-jogador")));
	TestEqual(TEXT("lado 1 é o pet do encontro"), Presentations[1].CatalogId, FString(TEXT("pet-do-mundo")));
	TestEqual(TEXT("o pet do jogador entra no lado 0"), static_cast<int32>(InitialState.Pets[0].Side), 0);
	TestEqual(TEXT("o pet do encontro entra no lado 1"), static_cast<int32>(InitialState.Pets[1].Side), 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEncounterMatchAssemblerRejectsUnknownCatalogIdTest,
	"BattleSquare.World.EncounterMatchAssembler.RejectsUnknownCatalogId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEncounterMatchAssemblerRejectsUnknownCatalogIdTest::RunTest(const FString& Parameters)
{
	FEncounterMatchParams Params = MakeMatchParams();
	Params.EncounterCatalogId = TEXT("pet-que-nao-existe");

	FBattleState InitialState;
	TArray<FPetPresentationInfo> Presentations;

	// Erro de configuração do nível é recusa explícita, nunca um pet
	// substituído em silêncio.
	TestFalse(TEXT("CatalogId ausente no espelho recusa a montagem"),
		FEncounterMatchAssembler::AssembleFromEncounter(Params, InitialState, Presentations));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorldEncounterFlowEndToEndTest,
	"BattleSquare.World.WorldEncounterFlow.PawnReachingEncounterAssemblesArena",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldEncounterFlowEndToEndTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateFlowTestWorld();

	AActor* Pawn = World->SpawnActor<AActor>();
	USceneComponent* Root = NewObject<USceneComponent>(Pawn);
	Pawn->SetRootComponent(Root);
	Root->RegisterComponent();
	Pawn->SetActorLocation(FVector(1000.0, 0.0, 0.0));

	UEncounterDetectionComponent* Detection = NewObject<UEncounterDetectionComponent>(Pawn);
	Detection->RegisterComponent();

	AWorldEncounterActor* Encounter = World->SpawnActor<AWorldEncounterActor>();
	Encounter->SetActorLocation(FVector(1100.0, 0.0, 0.0));
	Encounter->EncounterRadiusUnits = 400.0f;
	Encounter->CatalogId = TEXT("pet-do-mundo");

	UWorldEncounterFlow* Flow = NewObject<UWorldEncounterFlow>();
	Flow->Initialize(Pawn, Detection, ABattleArena::StaticClass(), MakeMatchParams());

	// O pawn "chega" ao encontro: a detecção avalia e o flow encadeia o resto.
	Detection->EvaluateAndTrigger({ Encounter });

	UWorldBattleTransitionService* Service = Flow->GetTransitionService();
	TestTrue(TEXT("a transição está ativa depois do encontro"), Service->IsTransitionActive());

	ABattleArena* Arena = Service->GetActiveArena();
	TestNotNull(TEXT("uma arena foi montada a partir do encontro"), Arena);
	TestEqual(TEXT("a arena carrega os dois pets"), Arena->GetCurrentState().Pets.Num(), 2);
	TestFalse(TEXT("a detecção fica desligada durante a batalha"), Detection->IsDetectionEnabled());

	DestroyFlowTestWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorldEncounterFlowUnknownPetDoesNotTrapPlayerTest,
	"BattleSquare.World.WorldEncounterFlow.UnassemblableEncounterReenablesDetection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldEncounterFlowUnknownPetDoesNotTrapPlayerTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateFlowTestWorld();

	AActor* Pawn = World->SpawnActor<AActor>();
	USceneComponent* Root = NewObject<USceneComponent>(Pawn);
	Pawn->SetRootComponent(Root);
	Root->RegisterComponent();

	UEncounterDetectionComponent* Detection = NewObject<UEncounterDetectionComponent>(Pawn);
	Detection->RegisterComponent();

	AWorldEncounterActor* Encounter = World->SpawnActor<AWorldEncounterActor>();
	Encounter->EncounterRadiusUnits = 400.0f;
	Encounter->CatalogId = TEXT("pet-que-nao-existe");

	UWorldEncounterFlow* Flow = NewObject<UWorldEncounterFlow>();
	Flow->Initialize(Pawn, Detection, ABattleArena::StaticClass(), MakeMatchParams());

	Detection->EvaluateAndTrigger({ Encounter });

	// Sem isto, um CatalogId errado no nível deixaria o jogador preso: a
	// detecção desligada pelo disparo e nenhuma batalha para religá-la.
	TestFalse(TEXT("nenhuma transição começa"), Flow->GetTransitionService()->IsTransitionActive());
	TestTrue(TEXT("a detecção volta a ligar, o jogador não fica preso"), Detection->IsDetectionEnabled());

	DestroyFlowTestWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAssemblerSeedsRandomTest,
	"BattleSquare.World.EncounterMatchAssembler.SeedsRandomPerMatch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAssemblerSeedsRandomTest::RunTest(const FString& Parameters)
{
	FEncounterMatchParams Params = MakeMatchParams();
	Params.EncounterCatalogId = TEXT("pet-do-mundo");

	FBattleState ComSemente;
	TArray<FPetPresentationInfo> Ignorado;
	Params.RandomSeed = 4242;
	FEncounterMatchAssembler::AssembleFromEncounter(Params, ComSemente, Ignorado);
	TestEqual(TEXT("semente explícita é respeitada — teste que precisa de resultado fixo continua possível"),
		ComSemente.Random.State, static_cast<uint64>(4242));

	// Sem semente, a partida NÃO pode começar sempre no mesmo ponto: era isso
	// que fazia o primeiro turno do oponente ser idêntico em toda batalha.
	FBattleState SemSemente;
	Params.RandomSeed = 0;
	FEncounterMatchAssembler::AssembleFromEncounter(Params, SemSemente, Ignorado);
	TestNotEqual(TEXT("sem semente explícita, o gerador não começa em zero"),
		SemSemente.Random.State, static_cast<uint64>(0));

	return true;
}

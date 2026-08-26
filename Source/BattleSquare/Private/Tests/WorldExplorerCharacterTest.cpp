// Copyright 2026 Anderson. All Rights Reserved.

#include "World/WorldExplorerCharacter.h"
#include "World/EncounterDetectionComponent.h"
#include "World/WorldEncounterActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

namespace
{
	UWorld* CreateExplorerTestWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
		FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
		WorldContext.SetCurrentWorld(World);
		World->InitializeActorsForPlay(FURL());
		return World;
	}

	void DestroyExplorerTestWorld(UWorld* World)
	{
		if (!World)
		{
			return;
		}
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorldExplorerCharacterCameraRigTest,
	"BattleSquare.World.WorldExplorerCharacter.CameraRigMatchesDesign",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldExplorerCharacterCameraRigTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateExplorerTestWorld();
	AWorldExplorerCharacter* Explorer = World->SpawnActor<AWorldExplorerCharacter>();

	USpringArmComponent* Boom = Explorer->GetCameraBoom();
	TestNotNull(TEXT("o personagem tem braço de câmera"), Boom);
	TestEqual(TEXT("o braço usa o comprimento da constante nomeada"),
		Boom->TargetArmLength, WorldTraversal::CameraArmLengthUnits);
	TestTrue(TEXT("o teste de colisão do braço está ligado — é o que impede a câmera de entrar na parede"),
		Boom->bDoCollisionTest);
	TestTrue(TEXT("o braço usa a rotação do controlador"), Boom->bUsePawnControlRotation);

	UCameraComponent* Camera = Explorer->GetFollowCamera();
	TestNotNull(TEXT("o personagem tem câmera"), Camera);
	TestFalse(TEXT("a câmera NÃO usa rotação de controlador — a rotação vive só no braço"),
		Camera->bUsePawnControlRotation);

	DestroyExplorerTestWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorldExplorerCharacterRotationSetupTest,
	"BattleSquare.World.WorldExplorerCharacter.BodyTurnsTowardMovement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldExplorerCharacterRotationSetupTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateExplorerTestWorld();
	AWorldExplorerCharacter* Explorer = World->SpawnActor<AWorldExplorerCharacter>();

	// DP-trav-04: esta combinação é a diferença entre "explorador" e "shooter
	// andando de lado".
	TestFalse(TEXT("o ator não herda o yaw do controlador"), Explorer->bUseControllerRotationYaw);
	TestTrue(TEXT("o corpo vira na direção do movimento"),
		Explorer->GetCharacterMovement()->bOrientRotationToMovement);

	DestroyExplorerTestWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorldExplorerCharacterHasEncounterDetectionTest,
	"BattleSquare.World.WorldExplorerCharacter.ReusesEncounterDetectionComponent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldExplorerCharacterHasEncounterDetectionTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateExplorerTestWorld();
	AWorldExplorerCharacter* Explorer = World->SpawnActor<AWorldExplorerCharacter>();
	Explorer->SetActorLocation(FVector::ZeroVector);

	UEncounterDetectionComponent* Detection = Explorer->GetEncounterDetection();
	TestNotNull(TEXT("o personagem carrega o componente de encontro da feature anterior"), Detection);

	AWorldEncounterActor* Encounter = World->SpawnActor<AWorldEncounterActor>();
	Encounter->SetActorLocation(FVector(100.0, 0.0, 0.0));
	Encounter->EncounterRadiusUnits = 400.0f;
	Encounter->CatalogId = TEXT("pet-do-mundo");

	// O pawn de jogador dispara encontros pelo MESMO caminho do pawn de
	// debug — nenhuma detecção nova (P1 da spec).
	TestEqual(TEXT("o pawn de jogador dispara o encontro"),
		Detection->EvaluateAndTrigger({ Encounter }), Encounter);

	DestroyExplorerTestWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorldExplorerCharacterSurvivesMissingInputAssetsTest,
	"BattleSquare.World.WorldExplorerCharacter.MissingInputAssetsDoNotCrash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldExplorerCharacterSurvivesMissingInputAssetsTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateExplorerTestWorld();

	// DP-trav-06: os assets de input vivem fora do C++, então a ausência
	// deles é um estado REAL — e precisa degradar para "não anda", nunca
	// para queda.
	AWorldExplorerCharacter* Explorer = World->SpawnActor<AWorldExplorerCharacter>();

	// BeginPlay sem controller: o registro do mapping context precisa sair
	// cedo em vez de assumir que há um jogador local.
	Explorer->DispatchBeginPlay();

	// Um UInputComponent real, mas com MoveAction/LookAction/Contexto nulos —
	// que é o estado de quem ainda não autorou os assets no Editor.
	UEnhancedInputComponent* InputComponent = NewObject<UEnhancedInputComponent>(Explorer);
	Explorer->SetupPlayerInputComponent(InputComponent);

	TestTrue(TEXT("montar o personagem sem nenhum asset de input não crasha"), IsValid(Explorer));

	DestroyExplorerTestWorld(World);
	return true;
}

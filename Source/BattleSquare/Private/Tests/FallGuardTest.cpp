// Copyright 2026 Anderson. All Rights Reserved.

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "World/WorldExplorerCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"

namespace
{
	struct FScopedWorld
	{
		UWorld* World = nullptr;

		FScopedWorld()
		{
			World = UWorld::CreateWorld(EWorldType::Game, false);
			FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
			Context.SetCurrentWorld(World);
			World->InitializeActorsForPlay(FURL());
		}

		~FScopedWorld()
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
		}
	};
}

// O usuário atravessou os blocos e caiu ETERNAMENTE. Sem chão infinito e sem
// um piso autorado perfeito, cair é sempre possível — o que não pode existir é
// a queda que não termina, porque ela não devolve o jogo a ninguém.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFallGuardRescuesFromEndlessFallTest,
	"BattleSquare.World.FallGuard.RescuesFromEndlessFall",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFallGuardRescuesFromEndlessFallTest::RunTest(const FString& Parameters)
{
	FScopedWorld Cena;

	AWorldExplorerCharacter* Explorador = Cena.World->SpawnActor<AWorldExplorerCharacter>();
	const FVector Inicio(100.0f, 200.0f, 300.0f);
	Explorador->SetActorLocation(Inicio);

	// Marca de terra firme: onde ele estava antes de cair.
	Explorador->RememberSafeGround();

	// Cai muito abaixo do limite.
	Explorador->SetActorLocation(FVector(100.0f, 200.0f, -50000.0f));
	Explorador->CheckFallGuard();

	const FVector Depois = Explorador->GetActorLocation();
	TestTrue(TEXT("Foi resgatado para cima do limite"), Depois.Z > -1000.0f);
	TestTrue(TEXT("Voltou para perto de onde estava em terra firme"),
		FVector::Dist2D(Depois, Inicio) < 100.0f);

	return true;
}

// Acima do limite, o guarda NÃO age: um resgate que dispara em queda normal
// impediria pular de qualquer degrau.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFallGuardLeavesNormalHeightsAloneTest,
	"BattleSquare.World.FallGuard.LeavesNormalHeightsAlone",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFallGuardLeavesNormalHeightsAloneTest::RunTest(const FString& Parameters)
{
	FScopedWorld Cena;

	AWorldExplorerCharacter* Explorador = Cena.World->SpawnActor<AWorldExplorerCharacter>();
	Explorador->SetActorLocation(FVector(0.0f, 0.0f, 500.0f));
	Explorador->RememberSafeGround();

	const FVector Descendo(0.0f, 0.0f, 100.0f);
	Explorador->SetActorLocation(Descendo);
	Explorador->CheckFallGuard();

	TestEqual(TEXT("Descer um degrau não é resgatado"),
		Explorador->GetActorLocation().Z, Descendo.Z);

	return true;
}

// O JOGADOR precisa ter corpo, pelo mesmo motivo dos inimigos.
//
// ACharacter traz uma malha esqueletal sem asset atribuído: em terceira pessoa
// isso é dirigir um corpo invisível. Terceira ocorrência do mesmo padrão neste
// projeto (APetView, inimigos do mundo, e agora o explorador) — e nenhum teste
// de lógica jamais o acusou.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWorldExplorerHasAVisibleBodyTest,
	"BattleSquare.World.Explorer.HasAVisibleBody",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldExplorerHasAVisibleBodyTest::RunTest(const FString& Parameters)
{
	const AWorldExplorerCharacter* Padrao = GetDefault<AWorldExplorerCharacter>();

	TestTrue(TEXT("Tem componente de corpo"), Padrao->BodyMesh != nullptr);
	TestTrue(TEXT("E a malha está ATRIBUÍDA — sem isto ele é invisível"),
		Padrao->BodyMesh->GetStaticMesh() != nullptr);

	// A esfera é simétrica: sem marca à frente não dá para saber para onde ele
	// está virado, e direção decide tudo num mundo onde encostar inicia luta.
	TestTrue(TEXT("Tem marca de direção"), Padrao->FacingMarker != nullptr);
	TestTrue(TEXT("A marca fica À FRENTE do corpo"),
		Padrao->FacingMarker->GetRelativeLocation().X > 1.0f);

	return true;
}

// Correr acelera e devolve a velocidade ORIGINAL ao soltar.
//
// O erro fácil aqui é multiplicar a velocidade atual: cada toque aceleraria de
// novo, e em três toques o explorador atravessaria o mundo. Guardar a
// velocidade de antes é o que impede isso.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWorldExplorerSprintRestoresSpeedTest,
	"BattleSquare.World.Explorer.SprintRestoresSpeed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldExplorerSprintRestoresSpeedTest::RunTest(const FString& Parameters)
{
	FScopedWorld Cena;

	AWorldExplorerCharacter* Explorador = Cena.World->SpawnActor<AWorldExplorerCharacter>();
	UCharacterMovementComponent* Movimento = Explorador->GetCharacterMovement();

	const float Andando = Movimento->MaxWalkSpeed;

	Explorador->StartSprinting();
	const float Correndo = Movimento->MaxWalkSpeed;
	TestTrue(TEXT("Correr é mais rápido que andar"), Correndo > Andando);

	// Segundo toque sem soltar NÃO acelera de novo.
	Explorador->StartSprinting();
	TestEqual(TEXT("Correr duas vezes não acumula"), Movimento->MaxWalkSpeed, Correndo);

	Explorador->StopSprinting();
	TestEqual(TEXT("Soltar devolve a velocidade original"), Movimento->MaxWalkSpeed, Andando);

	// Soltar duas vezes também não pode estragar nada.
	Explorador->StopSprinting();
	TestEqual(TEXT("Soltar de novo não muda nada"), Movimento->MaxWalkSpeed, Andando);

	return true;
}

// A câmera cicla e VOLTA ao início: um ciclo que não fecha deixaria o jogador
// preso no último modo sem saber como voltar.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWorldExplorerCameraCyclesBackTest,
	"BattleSquare.World.Explorer.CameraCyclesBack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldExplorerCameraCyclesBackTest::RunTest(const FString& Parameters)
{
	FScopedWorld Cena;

	AWorldExplorerCharacter* Explorador = Cena.World->SpawnActor<AWorldExplorerCharacter>();
	const float BracoInicial = Explorador->GetCameraBoom()->TargetArmLength;

	TSet<float> Distancias;
	for (int32 Passo = 0; Passo < 3; ++Passo)
	{
		Explorador->CycleCameraMode();
		Distancias.Add(Explorador->GetCameraBoom()->TargetArmLength);
	}

	TestTrue(TEXT("Os modos são de fato diferentes"), Distancias.Num() > 1);
	TestEqual(TEXT("Três passos voltam ao começo"),
		Explorador->GetCameraBoom()->TargetArmLength, BracoInicial);

	return true;
}

// Copyright 2026 Anderson. All Rights Reserved.

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "World/EncounterRoamingComponent.h"
#include "World/WorldEncounterActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

namespace
{
	struct FScopedRoamer
	{
		UWorld* World = nullptr;
		AWorldEncounterActor* Encontro = nullptr;
		UEncounterRoamingComponent* Passeio = nullptr;

		explicit FScopedRoamer(int32 Seed)
		{
			World = UWorld::CreateWorld(EWorldType::Game, false);
			FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
			Context.SetCurrentWorld(World);
			World->InitializeActorsForPlay(FURL());

			Encontro = World->SpawnActor<AWorldEncounterActor>();
			Encontro->SetActorLocation(FVector::ZeroVector);

			Passeio = NewObject<UEncounterRoamingComponent>(Encontro);
			Passeio->RegisterComponent();
			Passeio->ConfigureRoaming(FVector::ZeroVector, Seed);
		}

		~FScopedRoamer()
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
		}

		void Andar(int32 Quadros, float PassoSegundos = 0.1f)
		{
			for (int32 Quadro = 0; Quadro < Quadros; ++Quadro)
			{
				Passeio->TickComponent(PassoSegundos, LEVELTICK_All, nullptr);
			}
		}
	};
}

// O inimigo ANDA: sem isso ele espera parado, e o mundo vira um museu.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEncounterRoamingActuallyMovesTest,
	"BattleSquare.World.EncounterRoaming.ActuallyMoves",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEncounterRoamingActuallyMovesTest::RunTest(const FString& Parameters)
{
	FScopedRoamer CenaDoPasseio(/*Seed=*/7);
	CenaDoPasseio.Andar(20);

	TestTrue(TEXT("Saiu do lugar"),
		CenaDoPasseio.Encontro->GetActorLocation().Size2D() > 1.0f);
	return true;
}

// Ele não some pelo mapa: o passeio é ao redor de CASA. Um inimigo que anda
// para sempre numa direção acaba longe de qualquer caminho do jogador, e o
// encontro que ele deveria oferecer nunca acontece.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEncounterRoamingStaysNearHomeTest,
	"BattleSquare.World.EncounterRoaming.StaysNearHome",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEncounterRoamingStaysNearHomeTest::RunTest(const FString& Parameters)
{
	FScopedRoamer CenaDoPasseio(/*Seed=*/11);

	// Bastante tempo: muitas pernas de passeio, várias pausas.
	for (int32 Trecho = 0; Trecho < 40; ++Trecho)
	{
		CenaDoPasseio.Andar(50);

		const float Longe = CenaDoPasseio.Encontro->GetActorLocation().Size2D();
		if (Longe > CenaDoPasseio.Passeio->RoamRadiusUnits + 1.0f)
		{
			AddError(FString::Printf(TEXT("Afastou-se %.0f de casa, além do raio %.0f"),
				Longe, CenaDoPasseio.Passeio->RoamRadiusUnits));
			return false;
		}
	}

	return true;
}

// Mesma semente, mesmo passeio: é o que permite um roteiro de verificação
// repetir a cena, e o que impede dois inimigos criados no mesmo quadro de
// andarem colados um no outro.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEncounterRoamingIsSeededTest,
	"BattleSquare.World.EncounterRoaming.IsSeeded",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEncounterRoamingIsSeededTest::RunTest(const FString& Parameters)
{
	FVector Primeiro;
	{
		FScopedRoamer CenaDoPasseio(/*Seed=*/42);
		CenaDoPasseio.Andar(30);
		Primeiro = CenaDoPasseio.Encontro->GetActorLocation();
	}

	{
		FScopedRoamer CenaDoPasseio(/*Seed=*/42);
		CenaDoPasseio.Andar(30);
		TestTrue(TEXT("Mesma semente leva ao mesmo lugar"),
			FVector::Dist(CenaDoPasseio.Encontro->GetActorLocation(), Primeiro) < 1.0f);
	}

	{
		FScopedRoamer CenaDoPasseio(/*Seed=*/43);
		CenaDoPasseio.Andar(30);
		TestTrue(TEXT("Semente diferente leva a outro lugar"),
			FVector::Dist(CenaDoPasseio.Encontro->GetActorLocation(), Primeiro) > 1.0f);
	}

	return true;
}

// O inimigo do mundo precisa TER CORPO.
//
// Ele nasceu sem malha atribuída: existia, andava, disparava batalha — e era
// invisível. É o mesmo defeito de APetView, e ele passa em qualquer teste de
// lógica justamente porque nenhum deles olha a tela. Este olha.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWorldEncounterHasAVisibleBodyTest,
	"BattleSquare.World.EncounterActor.HasAVisibleBody",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldEncounterHasAVisibleBodyTest::RunTest(const FString& Parameters)
{
	const AWorldEncounterActor* Padrao = GetDefault<AWorldEncounterActor>();

	TestTrue(TEXT("Tem componente de malha"), Padrao->EncounterMesh != nullptr);
	TestTrue(TEXT("E a malha está ATRIBUÍDA — sem isto ele é invisível"),
		Padrao->EncounterMesh->GetStaticMesh() != nullptr);

	// Levantado pelo raio: origem no centro faria ele nascer meio enterrado,
	// que já aconteceu na arena.
	TestTrue(TEXT("Levantado do chão"),
		Padrao->EncounterMesh->GetRelativeLocation().Z > 1.0f);

	return true;
}

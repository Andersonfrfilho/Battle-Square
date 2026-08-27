// Copyright 2026 Anderson. All Rights Reserved.

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "World/EncounterRoamingComponent.h"
#include "World/WorldEncounterActor.h"

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
	FScopedRoamer Cena(/*Seed=*/7);
	Cena.Andar(20);

	TestTrue(TEXT("Saiu do lugar"),
		Cena.Encontro->GetActorLocation().Size2D() > 1.0f);
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
	FScopedRoamer Cena(/*Seed=*/11);

	// Bastante tempo: muitas pernas de passeio, várias pausas.
	for (int32 Trecho = 0; Trecho < 40; ++Trecho)
	{
		Cena.Andar(50);

		const float Longe = Cena.Encontro->GetActorLocation().Size2D();
		if (Longe > Cena.Passeio->RoamRadiusUnits + 1.0f)
		{
			AddError(FString::Printf(TEXT("Afastou-se %.0f de casa, além do raio %.0f"),
				Longe, Cena.Passeio->RoamRadiusUnits));
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
		FScopedRoamer Cena(/*Seed=*/42);
		Cena.Andar(30);
		Primeiro = Cena.Encontro->GetActorLocation();
	}

	{
		FScopedRoamer Cena(/*Seed=*/42);
		Cena.Andar(30);
		TestTrue(TEXT("Mesma semente leva ao mesmo lugar"),
			FVector::Dist(Cena.Encontro->GetActorLocation(), Primeiro) < 1.0f);
	}

	{
		FScopedRoamer Cena(/*Seed=*/43);
		Cena.Andar(30);
		TestTrue(TEXT("Semente diferente leva a outro lugar"),
			FVector::Dist(Cena.Encontro->GetActorLocation(), Primeiro) > 1.0f);
	}

	return true;
}

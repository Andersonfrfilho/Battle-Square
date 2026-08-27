// Copyright 2026 Anderson. All Rights Reserved.

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Misc/AutomationTest.h"
#include "Net/BattleSquareGameMode.h"
#include "World/WorldEncounterActor.h"

namespace
{
	int32 ContarEncontros(UWorld* World, bool bApenasNaoResolvidos)
	{
		int32 Total = 0;
		for (TActorIterator<AWorldEncounterActor> It(World); It; ++It)
		{
			if (!bApenasNaoResolvidos || !It->bIsResolved)
			{
				++Total;
			}
		}
		return Total;
	}

	struct FScopedPopulationWorld
	{
		UWorld* World = nullptr;

		FScopedPopulationWorld()
		{
			World = UWorld::CreateWorld(EWorldType::Game, false);
			FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
			Context.SetCurrentWorld(World);
			World->InitializeActorsForPlay(FURL());
		}

		~FScopedPopulationWorld()
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
		}
	};
}

// O mundo não pode ACABAR. Com um número fixo de encontros, seis batalhas
// esvaziavam o mapa e sobrava caminhar por um lugar sem nada acontecendo.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEncounterPopulationRefillsTest,
	"BattleSquare.World.EncounterPopulation.Refills",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEncounterPopulationRefillsTest::RunTest(const FString& Parameters)
{
	FScopedPopulationWorld Cena;

	ABattleSquareGameMode* GameMode = Cena.World->SpawnActor<ABattleSquareGameMode>();
	GameMode->WorldEncounterCount = 3;
	GameMode->WorldEncounterCatalogIds = { TEXT("teste") };

	GameMode->SpawnRoamingEncounters();
	TestEqual(TEXT("O mundo nasceu povoado"), ContarEncontros(Cena.World, true), 3);

	// Uma batalha acontece: o encontro é marcado resolvido.
	TActorIterator<AWorldEncounterActor> Primeiro(Cena.World);
	Primeiro->MarkResolved();
	TestEqual(TEXT("Sobraram dois vivos"), ContarEncontros(Cena.World, true), 2);

	GameMode->MaintainEncounterPopulation();

	// O derrotado sai de cena — continuar andando como fantasma seria pior que
	// sumir — e um novo nasce no lugar dele.
	TestEqual(TEXT("A população voltou ao alvo"), ContarEncontros(Cena.World, true), 3);
	TestEqual(TEXT("Nenhum resolvido ficou vagando"),
		ContarEncontros(Cena.World, false), 3);

	return true;
}

// Com a população já cheia, manter não cria nada: um mundo que acumula
// inimigos a cada verificação vira uma multidão.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEncounterPopulationDoesNotOverfillTest,
	"BattleSquare.World.EncounterPopulation.DoesNotOverfill",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEncounterPopulationDoesNotOverfillTest::RunTest(const FString& Parameters)
{
	FScopedPopulationWorld Cena;

	ABattleSquareGameMode* GameMode = Cena.World->SpawnActor<ABattleSquareGameMode>();
	GameMode->WorldEncounterCount = 4;
	GameMode->WorldEncounterCatalogIds = { TEXT("teste") };

	GameMode->SpawnRoamingEncounters();

	GameMode->MaintainEncounterPopulation();
	GameMode->MaintainEncounterPopulation();
	GameMode->MaintainEncounterPopulation();

	TestEqual(TEXT("Continua com quatro, e não doze"),
		ContarEncontros(Cena.World, true), 4);

	return true;
}

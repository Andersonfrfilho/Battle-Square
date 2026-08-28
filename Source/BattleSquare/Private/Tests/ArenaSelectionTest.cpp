// Copyright 2026 Anderson. All Rights Reserved.

#include "Balance/ArenaLayoutCatalog.h"
#include "Battle/BattleArena.h"
#include "Battle/BattleTypes.h"
#include "Data/BattleDataTranslator.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"

namespace
{
	struct FScopedArenaWorld
	{
		UWorld* World = nullptr;

		FScopedArenaWorld()
		{
			World = UWorld::CreateWorld(EWorldType::Game, false);
			FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
			Context.SetCurrentWorld(World);
			World->InitializeActorsForPlay(FURL());
		}

		~FScopedArenaWorld()
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
		}
	};

	FBattleState MakeSeededState(uint64 Seed)
	{
		FBattleState State;
		State.Random.State = Seed;

		FPetState Meu;
		Meu.PetId = 1; Meu.Side = 0; Meu.Column = 1; Meu.Row = 1;
		Meu.Health = 100; Meu.MaxHealth = 100;
		FPetState Dele = Meu;
		Dele.PetId = 2; Dele.Side = 1; Dele.Column = 2;

		State.Pets.Add(Meu);
		State.Pets.Add(Dele);
		return State;
	}

	TArray<FPetPresentationInfo> MakePresentations()
	{
		TArray<FPetPresentationInfo> Apresentacoes;
		FPetPresentationInfo A; A.PetId = 1; A.Name = TEXT("Meu"); A.Type = TEXT("Agua");
		FPetPresentationInfo B; B.PetId = 2; B.Name = TEXT("Dele"); B.Type = TEXT("Fogo");
		Apresentacoes.Add(A); Apresentacoes.Add(B);
		return Apresentacoes;
	}
}

// A arena precisa VARIAR. O catálogo de layouts existia, era testado, e ninguém
// o carregava em produção: CellLayout nascia todo neutro e nunca era
// preenchido, então casa bloqueada, de dano, de bônus e de água jamais
// apareciam numa batalha real. A feature inteira estava inalcançável.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArenaLayoutIsActuallyAppliedTest,
	"BattleSquare.Battle.Arena.LayoutIsActuallyApplied",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArenaLayoutIsActuallyAppliedTest::RunTest(const FString& Parameters)
{
	FScopedArenaWorld Cena;

	// Sementes diferentes até aparecer uma casa não-neutra: o catálogo tem
	// "Campo Aberto", que é legitimamente todo neutro.
	bool bAlgumaArenaTemTerreno = false;
	for (uint64 Semente = 1; Semente <= 30 && !bAlgumaArenaTemTerreno; ++Semente)
	{
		ABattleArena* Arena = Cena.World->SpawnActor<ABattleArena>();
		Arena->BeginBattle(MakeSeededState(Semente), MakePresentations());

		for (uint8 Propriedade : Arena->GetCurrentState().CellLayout)
		{
			if (Propriedade != static_cast<uint8>(ECellProperty::None))
			{
				bAlgumaArenaTemTerreno = true;
				break;
			}
		}
	}

	TestTrue(TEXT("Alguma batalha abre numa arena com terreno"), bAlgumaArenaTemTerreno);
	return true;
}

// Mesma semente, mesma arena: sem isso um replay abriria noutro mapa, e o
// determinismo do núcleo não valeria de nada acima dele.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArenaChoiceIsDeterministicTest,
	"BattleSquare.Battle.Arena.ChoiceIsDeterministic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArenaChoiceIsDeterministicTest::RunTest(const FString& Parameters)
{
	FScopedArenaWorld Cena;

	ABattleArena* Primeira = Cena.World->SpawnActor<ABattleArena>();
	Primeira->BeginBattle(MakeSeededState(4242), MakePresentations());
	const TArray<uint8> LayoutUm = Primeira->GetCurrentState().CellLayout;

	ABattleArena* Segunda = Cena.World->SpawnActor<ABattleArena>();
	Segunda->BeginBattle(MakeSeededState(4242), MakePresentations());

	TestTrue(TEXT("Mesma semente, mesma arena"),
		LayoutUm == Segunda->GetCurrentState().CellLayout);
	return true;
}

// Nenhuma arena escolhida pode bloquear a casa inicial de um pet: a montagem
// seria rejeitada e a batalha simplesmente não abriria.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArenaNeverBlocksStartingCellsTest,
	"BattleSquare.Battle.Arena.NeverBlocksStartingCells",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArenaNeverBlocksStartingCellsTest::RunTest(const FString& Parameters)
{
	FScopedArenaWorld Cena;

	for (uint64 Semente = 1; Semente <= 40; ++Semente)
	{
		ABattleArena* Arena = Cena.World->SpawnActor<ABattleArena>();

		TestTrue(TEXT("A batalha abre, sempre"),
			Arena->BeginBattle(MakeSeededState(Semente), MakePresentations()));

		const TArray<uint8>& Layout = Arena->GetCurrentState().CellLayout;
		for (const FPetState& Pet : Arena->GetCurrentState().Pets)
		{
			TestNotEqual(TEXT("Casa inicial nunca é bloqueada"),
				static_cast<int32>(Layout[CellLayoutIndex(Pet.Column, Pet.Row)]),
				static_cast<int32>(ECellProperty::Blocked));
		}
	}

	return true;
}

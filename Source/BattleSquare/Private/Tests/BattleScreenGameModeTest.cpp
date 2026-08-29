// Copyright 2026 Anderson. All Rights Reserved.

#include "UI/BattleScreenGameMode.h"
#include "Net/BattleSquarePlayerController.h"
#include "World/WorldExplorerCharacter.h"
#include "GameFramework/SpectatorPawn.h"
#include "Battle/BattleArena.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

namespace
{
	ABattleScreenGameMode* SpawnConfiguredScreenGameMode(UWorld* World)
	{
		ABattleScreenGameMode* GameMode = World->SpawnActor<ABattleScreenGameMode>();
		GameMode->WorldEncounterMirrorPath =
			FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("PetMirrorFixture"), TEXT("pets-mirror.sqlite"));
		GameMode->WorldEncounterMirrorKeyHex =
			TEXT("ad2f50e7781fcca5148299d249825f117ff78cc0b1758de30ce9832f01918e39");
		GameMode->WorldEncounterMirrorPublicKeyPem =
			TEXT("-----BEGIN PUBLIC KEY-----\n")
			TEXT("MCowBQYDK2VwAyEASmCzzcPySYHgKJgbH2uuAjtP4gXGRl2jP4ynBxOF2K0=\n")
			TEXT("-----END PUBLIC KEY-----\n");
		// Slot dedicado: nunca poluir o slot de produção em teste.
		GameMode->PetCollectionSlotName = TEXT("BattleScreenTestCollection");
		// O teste controla a própria entrada: sem isto ele herdaria a classe de
		// widget do DefaultGame.ini e passaria a medir a configuração do
		// projeto em vez do comportamento do GameMode.
		GameMode->ActionSelectorWidgetClassPath.Reset();
		return GameMode;
	}

	UWorld* CreateScreenTestWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
		FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
		WorldContext.SetCurrentWorld(World);
		World->InitializeActorsForPlay(FURL());
		return World;
	}

	void DestroyScreenTestWorld(UWorld* World)
	{
		if (!World)
		{
			return;
		}
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBattleScreenAssemblesWithoutWorldTest,
	"BattleSquare.UI.BattleScreenGameMode.AssemblesArenaWithoutOpenWorld",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBattleScreenAssemblesWithoutWorldTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateScreenTestWorld();
	ABattleScreenGameMode* GameMode = SpawnConfiguredScreenGameMode(World);

	// Sem classe de widget: a batalha ainda tem de MONTAR. A ausência de
	// interface degrada, não impede (DP-ui-04).
	const FString Problem = GameMode->StartScreenBattle();

	TestNotNull(TEXT("a arena foi montada sem nenhum mundo aberto envolvido"), GameMode->ScreenArena.Get());
	TestEqual(TEXT("a arena recebeu os dois pets"),
		GameMode->ScreenArena ? GameMode->ScreenArena->GetCurrentState().Pets.Num() : 0, 2);
	TestTrue(TEXT("o motivo devolvido é só sobre a interface ausente"),
		Problem.Contains(TEXT("ActionSelectorWidgetClassPath")));

	DestroyScreenTestWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBattleScreenPicksDistinctOpponentTest,
	"BattleSquare.UI.BattleScreenGameMode.DefaultOpponentDiffersFromPlayer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBattleScreenPicksDistinctOpponentTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateScreenTestWorld();
	ABattleScreenGameMode* GameMode = SpawnConfiguredScreenGameMode(World);
	GameMode->StartScreenBattle();

	if (!GameMode->ScreenArena)
	{
		AddError(TEXT("arena não montou"));
		DestroyScreenTestWorld(World);
		return false;
	}

	const TArray<FPetState>& Pets = GameMode->ScreenArena->GetCurrentState().Pets;
	TestEqual(TEXT("dois pets"), Pets.Num(), 2);
	// Sem ids configurados, o padrão não pode escalar o mesmo pet dos dois lados.
	TestNotEqual(TEXT("os dois lados são pets diferentes"),
		static_cast<int32>(Pets[0].PetId), static_cast<int32>(Pets[1].PetId));
	TestEqual(TEXT("lado 0 é o jogador"), static_cast<int32>(Pets[0].Side), 0);
	TestEqual(TEXT("lado 1 é o oponente"), static_cast<int32>(Pets[1].Side), 1);

	DestroyScreenTestWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBattleScreenMissingMirrorIsAReasonTest,
	"BattleSquare.UI.BattleScreenGameMode.MissingMirrorIsAReasonNotACrash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBattleScreenMissingMirrorIsAReasonTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateScreenTestWorld();
	ABattleScreenGameMode* GameMode = SpawnConfiguredScreenGameMode(World);
	GameMode->WorldEncounterMirrorPath.Reset();

	const FString Problem = GameMode->StartScreenBattle();

	TestFalse(TEXT("devolve motivo legível"), Problem.IsEmpty());
	TestNull(TEXT("e não deixa arena pela metade"), GameMode->ScreenArena.Get());

	DestroyScreenTestWorld(World);
	return true;
}

// As teclas de depuração (F8 controla os dois lados, F9 copia o painel, F10
// limpa) vivem em ABattleSquarePlayerController. O GameMode do MUNDO o
// declarava; o da tela de batalha não — então elas nunca chegavam ao jogador
// justamente na tela onde ele passa o tempo todo.
//
// Achado pelo usuário apertando F9 e nada acontecer, depois de eu ter LEVANTADO
// essa suspeita e não ter ido verificar.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleScreenGameModeUsesDebugCapableControllerTest,
	"BattleSquare.UI.BattleScreenGameMode.UsesDebugCapableController",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleScreenGameModeUsesDebugCapableControllerTest::RunTest(const FString& Parameters)
{
	const ABattleScreenGameMode* Padrao = GetDefault<ABattleScreenGameMode>();

	TestTrue(TEXT("A tela de batalha usa o controlador com as teclas de depuração"),
		Padrao->PlayerControllerClass == ABattleSquarePlayerController::StaticClass());

	return true;
}

// O pawn de exploração do mundo aberto NASCIA na tela de batalha: o
// construtor de ABattleScreenGameMode dizia em comentário que "nenhum pawn de
// exploração precisa nascer aqui", mas nada no código impedia — o pai resolve
// DefaultPawnClass a partir de WorldExplorerPawnClassPath em InitGame, e a
// classe filha herda a configuração inteira.
//
// Na tela isso aparecia como uma cúpula clara na casa do meio, com as patas do
// pet atravessando: era o corpo esférico do explorador, meio enterrado na
// laje. Encontrado olhando a tela; o censo de atores no mapa vivo
// (obj list class=WorldExplorerCharacter) nomeou o culpado.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleScreenGameModeSpawnsNoExplorerPawnTest,
	"BattleSquare.UI.BattleScreenGameMode.SpawnsNoExplorerPawn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBattleScreenGameModeSpawnsNoExplorerPawnTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateScreenTestWorld();
	ABattleScreenGameMode* GameMode = SpawnConfiguredScreenGameMode(World);

	// Exatamente o estado que o defeito produzia: a configuração herdada do
	// mundo já apontando para o explorador. A tela tem de recusar assim mesmo
	// — senão o teste mediria a ausência de configuração, não a regra.
	GameMode->DefaultPawnClass = AWorldExplorerCharacter::StaticClass();

	UClass* PawnClass = GameMode->GetDefaultPawnClassForController(nullptr);

	TestNotNull(TEXT("a tela de batalha declara alguma classe de pawn"), PawnClass);
	if (PawnClass)
	{
		TestFalse(TEXT("o pawn de exploração do mundo não nasce na tela de batalha"),
			PawnClass->IsChildOf(AWorldExplorerCharacter::StaticClass()));
		TestTrue(TEXT("o que nasce é um pawn sem corpo visível"),
			PawnClass->IsChildOf(ASpectatorPawn::StaticClass()));
	}

	DestroyScreenTestWorld(World);
	return true;
}

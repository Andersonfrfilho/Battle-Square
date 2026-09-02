// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattleState.h"
#include "Battle/FluidRegistry.h"
#include "Misc/AutomationTest.h"
#include "World/ArenaFromWorld.h"

/**
 * A ARENA PÕE FLUIDO NAS CASAS — o eixo da substância chegando à partida.
 *
 * Até aqui, `grep SetFluidAt` fora dos testes vinha VAZIO: toda casa de
 * batalha caía no padrão da fundura, e numa partida de verdade nunca havia
 * lava, ninguém se queimava, nada conduzia. Cento e onze provas verdes sobre um
 * eixo que o jogador não alcançava.
 *
 * A montagem continua PURA: a amostra traz o fluido colhido pela metade suja, e
 * quem decide o tabuleiro se verifica sem levantar `UWorld`.
 */

namespace ProvaDoFluidoDaArena
{
	FArenaFromWorldParams CampoDe(int32 Colunas, int32 Linhas)
	{
		FArenaFromWorldParams Params;
		Params.EncounterLocation = FVector::ZeroVector;
		Params.CellSize = 200.0f;
		Params.Columns = Colunas;
		Params.Rows = Linhas;
		Params.HumidityPercent = 0;   // sem lama: a margem vira poça, não barro
		return Params;
	}

	/** A posição de mundo do centro da casa dada, na mesma conta da montagem. */
	FVector NoCentroDaCasa(const FArenaFromWorldParams& Params, int32 Coluna, int32 Linha)
	{
		const float CantoX = Params.EncounterLocation.X
			- (Params.Columns * Params.CellSize) * 0.5f;
		const float CantoY = Params.EncounterLocation.Y
			- (Params.Rows * Params.CellSize) * 0.5f;

		return FVector(
			CantoX + (Coluna + 0.5f) * Params.CellSize,
			CantoY + (Linha + 0.5f) * Params.CellSize,
			0.0f);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArenaFluidTheCellKeepsTheSampledFluidTest,
	"BattleSquare.ArenaFluid.TheCellKeepsTheSampledFluid",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArenaFluidTheCellKeepsTheSampledFluidTest::RunTest(const FString& Parameters)
{
	FArenaFromWorldParams Params = ProvaDoFluidoDaArena::CampoDe(3, 3);

	// Duas águas de substâncias diferentes, em casas diferentes: é o caso que
	// só existe porque a arena é montada casa a casa, e foi ele que decidiu o
	// fluido ser campo por casa em vez de um por arena.
	Params.Features.Add({ ProvaDoFluidoDaArena::NoCentroDaCasa(Params, 0, 1),
		EWorldFeatureKind::DeepWater,
		static_cast<uint8>(EFluidKind::AguaTermal) });
	Params.Features.Add({ ProvaDoFluidoDaArena::NoCentroDaCasa(Params, 2, 1),
		EWorldFeatureKind::DeepWater,
		static_cast<uint8>(EFluidKind::AguaDoce) });

	TArray<uint8> Fluidos;
	const TArray<uint8> Layout = FArenaFromWorld::Build(Params, &Fluidos);

	if (Layout.Num() != 9 || Fluidos.Num() != 9)
	{
		AddError(TEXT("a montagem nao devolveu tabuleiro e fluidos do mesmo tamanho"));
		return false;
	}

	TestEqual(TEXT("a casa da esquerda ficou TERMAL"),
		static_cast<int32>(Fluidos[CellLayoutIndex(0, 1, 3)]),
		static_cast<int32>(EFluidKind::AguaTermal));
	TestEqual(TEXT("a casa da direita ficou DOCE"),
		static_cast<int32>(Fluidos[CellLayoutIndex(2, 1, 3)]),
		static_cast<int32>(EFluidKind::AguaDoce));

	// O CONTRAPESO: casa seca continua sem fluido. Sem esta linha, uma regra
	// que carimbasse a arena inteira passaria nas duas acima, e o tabuleiro
	// todo viraria água.
	TestEqual(TEXT("a casa seca continua sem fluido"),
		static_cast<int32>(Fluidos[CellLayoutIndex(1, 0, 3)]),
		static_cast<int32>(EFluidKind::Nenhum));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArenaFluidComesFromTheSameTieBreakTest,
	"BattleSquare.ArenaFluid.ComesFromTheSameTieBreak",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArenaFluidComesFromTheSameTieBreakTest::RunTest(const FString& Parameters)
{
	FArenaFromWorldParams Params = ProvaDoFluidoDaArena::CampoDe(3, 3);

	// DUAS amostras na MESMA casa, e a de maior peso vence. O fluido tem de
	// vir da AMOSTRA VENCEDORA — se ele saísse de uma segunda passada, a casa
	// teria a fundura de uma e a substância da outra, sem nada acusar.
	//
	// Margem (poça) perde para água funda; então a substância que fica é a da
	// água funda, e não a da margem.
	const FVector MesmaCasa = ProvaDoFluidoDaArena::NoCentroDaCasa(Params, 1, 1);

	Params.Features.Add({ MesmaCasa, EWorldFeatureKind::Shore,
		static_cast<uint8>(EFluidKind::AguaDePantano) });
	Params.Features.Add({ MesmaCasa, EWorldFeatureKind::DeepWater,
		static_cast<uint8>(EFluidKind::AguaSalgada) });

	TArray<uint8> Fluidos;
	const TArray<uint8> Layout = FArenaFromWorld::Build(Params, &Fluidos);

	const int32 Indice = CellLayoutIndex(1, 1, 3);

	TestEqual(TEXT("a fundura e a da amostra vencedora"),
		static_cast<int32>(Layout[Indice]),
		static_cast<int32>(ECellProperty::Water));
	TestEqual(TEXT("e a substancia e da MESMA amostra"),
		static_cast<int32>(Fluidos[Indice]),
		static_cast<int32>(EFluidKind::AguaSalgada));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArenaFluidIsOptionalForTheCallerTest,
	"BattleSquare.ArenaFluid.IsOptionalForTheCaller",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArenaFluidIsOptionalForTheCallerTest::RunTest(const FString& Parameters)
{
	FArenaFromWorldParams Params = ProvaDoFluidoDaArena::CampoDe(3, 3);
	Params.Features.Add({ ProvaDoFluidoDaArena::NoCentroDaCasa(Params, 1, 1),
		EWorldFeatureKind::DeepWater,
		static_cast<uint8>(EFluidKind::AguaSalgada) });

	// Quem não quer o fluido chama como sempre chamou, e o tabuleiro sai
	// IDÊNTICO. Sem isto, acrescentar a substância teria mudado o terreno de
	// toda arena montada antes dela — e nenhum teste de layout notaria a causa.
	const TArray<uint8> SemPedir = FArenaFromWorld::Build(Params);

	TArray<uint8> Fluidos;
	const TArray<uint8> Pedindo = FArenaFromWorld::Build(Params, &Fluidos);

	TestEqual(TEXT("o tabuleiro nao muda por pedir o fluido"),
		SemPedir, Pedindo);
	TestTrue(TEXT("e o fluido veio"), Fluidos.Num() == Pedindo.Num());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArenaFluidReachesTheBattleStateTest,
	"BattleSquare.ArenaFluid.ReachesTheBattleState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArenaFluidReachesTheBattleStateTest::RunTest(const FString& Parameters)
{
	// A PONTA FINAL: o que a arena monta tem de virar a resposta que o combate
	// faz. Uma lista de fluidos que ninguém copia para o estado seria o mesmo
	// defeito, um degrau adiante.
	FArenaFromWorldParams Params = ProvaDoFluidoDaArena::CampoDe(3, 3);
	Params.Features.Add({ ProvaDoFluidoDaArena::NoCentroDaCasa(Params, 1, 1),
		EWorldFeatureKind::DeepWater,
		static_cast<uint8>(EFluidKind::Lava) });

	TArray<uint8> Fluidos;
	const TArray<uint8> Layout = FArenaFromWorld::Build(Params, &Fluidos);

	FBattleState Estado;
	Estado.GridColumns = 3;
	Estado.GridRows = 3;
	Estado.CellLayout = Layout;
	Estado.CellFluid = Fluidos;
	Estado.ApplyDefaultTerrainRequirements();

	TestEqual(TEXT("a casa e de lava no estado"),
		static_cast<int32>(Estado.FluidAt(1, 1)),
		static_cast<int32>(EFluidKind::Lava));

	// E a regra que depende disso passa a valer: submergir numa casa de água
	// FUNDA que é lava deixa de ser permitido.
	TestFalse(TEXT("submergir nao vale numa casa de lava"),
		Estado.CellAllowsSkill(EActionType::Submergir, 1, 1));

	return true;
}

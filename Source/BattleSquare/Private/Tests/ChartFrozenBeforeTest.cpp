// Copyright 2026 Anderson. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "World/IslandBakedPlan.h"
#include "World/LandUseLayout.h"
#include "World/TrailLayout.h"

// ---------------------------------------------------------------------------
// M1 — O GABARITO DE HOJE, CONGELADO EM 02/09/2026.
//
// SÓ SABE O QUE MUDOU QUEM ANOTOU O QUE ERA. A feature `a-carta-muda-uma-vez`
// vai dar degrau à rocha, e a rocha é a camada mais baixa do mundo (rocha →
// água → região → relevo → trilhas → solo): mexer nela move a costa, as vilas e
// as trilhas de tabela.
//
// Este teste existe para REPROVAR quando isso acontecer — e é reprovando que ele
// diz QUANTO o degrau moveu. Sem ele, o degrau mudaria doze contagens e o único
// sinal seria o `ChartConformanceTest` falhando com "esperava 9, era 8", sem
// dizer se aquilo era o degrau ou um defeito.
//
// ⚠️ ELE NÃO DUPLICA `ChartConformanceTest`, e a diferença é de PROPÓSITO:
// aquele afirma os números como gabarito de PRODUTO (o mundo tem de bater com a
// carta); este os afirma como ESTADO CONGELADO ANTES DA MUDANÇA. Dois testes
// com os mesmos números e propósitos diferentes se justificam por uma feature;
// três seriam cópia.
//
// ⚠️ **ELE MORRE NA M10**, quando o gabarito novo for escrito. Teste temporário
// que sobrevive ao temporário vira um segundo gabarito — e dois gabaritos
// concordam até a primeira edição.
// ---------------------------------------------------------------------------

namespace GabaritoCongeladoDe0209
{
	int32 ManchasDe(const UIslandBakedPlan& Assado, EGroundUse Uso)
	{
		int32 Total = 0;
		for (const FBakedGroundUse& Mancha : Assado.GroundUses)
		{
			if (Mancha.Use == Uso)
			{
				++Total;
			}
		}
		return Total;
	}

	int32 TravessiasDe(const UIslandBakedPlan& Assado, TrailLayout::ECrossingKind Tipo)
	{
		int32 Total = 0;
		for (const FBakedCrossing& Onde : Assado.Crossings)
		{
			if (Onde.Kind == static_cast<uint8>(Tipo))
			{
				++Total;
			}
		}
		return Total;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChartFrozenBeforeTheStepTest,
	"BattleSquare.ChartFrozen.BeforeTheRockStep",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChartFrozenBeforeTheStepTest::RunTest(const FString& Parameters)
{
	const UIslandBakedPlan* Assado = IslandBakedPlan::Load();
	if (!Assado)
	{
		AddError(TEXT("o assado nao existe — rode ./Tools/bake_island.sh"));
		return false;
	}

	// OS DOZE DO SOLO. Estes são os que o degrau na rocha pode mover sem que
	// ninguém tenha pedido: a rocha decide onde há terra plana, e é a terra
	// plana que decide onde cabe fazenda, templo ou cemitério.
	const TPair<EGroundUse, int32> DoSolo[] = {
		{ EGroundUse::Bosque,       9 },
		{ EGroundUse::Clareira,     6 },
		{ EGroundUse::Fazenda,      8 },
		{ EGroundUse::Criadouro,    4 },
		{ EGroundUse::Loja,         4 },
		{ EGroundUse::Acampamento,  6 },
		{ EGroundUse::Pomar,        3 },
		{ EGroundUse::Deck,         7 },
		{ EGroundUse::Templo,       5 },
		{ EGroundUse::Ruina,        4 },
		{ EGroundUse::Cemiterio,    7 },
	};

	for (const TPair<EGroundUse, int32>& Qual : DoSolo)
	{
		TestEqual(*FString::Printf(TEXT("congelado em 02/09: %d manchas do uso %d"),
			Qual.Value, static_cast<int32>(Qual.Key)),
			GabaritoCongeladoDe0209::ManchasDe(*Assado, Qual.Key), Qual.Value);
	}

	// AS TRAVESSIAS, que são o que esta feature MEXE de propósito. Congelá-las
	// não é contradição: elas vão mudar, e é o tamanho da mudança que interessa.
	TestEqual(TEXT("congelado em 02/09: 30 vaus"),
		GabaritoCongeladoDe0209::TravessiasDe(*Assado, TrailLayout::ECrossingKind::Vau), 30);
	TestEqual(TEXT("congelado em 02/09: 25 balsas"),
		GabaritoCongeladoDe0209::TravessiasDe(*Assado, TrailLayout::ECrossingKind::Balsa), 25);
	TestEqual(TEXT("congelado em 02/09: 0 pontes"),
		GabaritoCongeladoDe0209::TravessiasDe(*Assado, TrailLayout::ECrossingKind::Ponte), 0);
	TestEqual(TEXT("congelado em 02/09: 56 travessias ao todo"),
		Assado->Crossings.Num(), 56);

	// E O QUE A ILHA É, por baixo de tudo. Estes três são os que dizem se a
	// rocha se mexeu: se o raio ou a contagem de assentamentos mudar, o degrau
	// mexeu em coisa que ninguém pediu que ele mexesse.
	TestEqual(TEXT("congelado em 02/09: 7 assentamentos"),
		Assado->Settlements.Num(), 7);
	TestEqual(TEXT("congelado em 02/09: 16 cavernas"),
		Assado->Caves.Num(), 16);

	AddInfo(FString::Printf(
		TEXT("CONGELADO em 02/09/2026 — %d manchas de solo, %d travessias, "
			 "%d assentamentos, %d cavernas"),
		Assado->GroundUses.Num(), Assado->Crossings.Num(),
		Assado->Settlements.Num(), Assado->Caves.Num()));

	return true;
}

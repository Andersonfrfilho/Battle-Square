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
	// Struct local em vez de `TPair`: o par exigiria um cabeçalho a mais por um
	// ganho nenhum, e um nome de campo diz mais que `.Key` e `.Value`.
	struct FCongelado
	{
		EGroundUse Uso;
		int32 Quantas;
		const TCHAR* Nome;
	};

	const FCongelado DoSolo[] = {
		{ EGroundUse::Bosque,          9, TEXT("bosques") },
		{ EGroundUse::ClareiraFechada, 6, TEXT("clareiras") },
		{ EGroundUse::Fazenda,         8, TEXT("fazendas") },
		{ EGroundUse::Criadouro,       4, TEXT("criadouros") },
		{ EGroundUse::Loja,            4, TEXT("lojas") },
		{ EGroundUse::Acampamento,     6, TEXT("acampamentos") },
		{ EGroundUse::Pomar,           3, TEXT("pomares") },
		{ EGroundUse::Deck,            7, TEXT("decks") },
		{ EGroundUse::Templo,          5, TEXT("templos") },
		{ EGroundUse::Ruina,           4, TEXT("ruinas") },
		{ EGroundUse::Cemiterio,       7, TEXT("cemiterios") },
	};

	for (const FCongelado& Qual : DoSolo)
	{
		// O NOME na mensagem, e não o número do enum: quando este teste
		// reprovar — e ele existe para reprovar — quem lê precisa saber que
		// foram os cemitérios que mudaram, não "o uso 13".
		TestEqual(*FString::Printf(TEXT("congelado em 02/09: %d %s"),
			Qual.Quantas, Qual.Nome),
			GabaritoCongeladoDe0209::ManchasDe(*Assado, Qual.Uso), Qual.Quantas);
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

	// E O QUE A ILHA É POR BAIXO. As cavernas e o total de manchas dizem se a
	// rocha se mexeu em coisa que ninguém pediu que ela mexesse.
	//
	// ⚠️ Os SETE ASSENTAMENTOS ficam de fora, e é medição, não esquecimento: o
	// assado não os guarda (`UIslandBakedPlan` tem rios, córregos, fontes,
	// trilhas, travessias, usos do solo, cavernas, ligações submersas e
	// aquedutos — e nenhum campo de assentamento). O número 7 que eu media vem
	// do JSON do bake, não do que o jogo carrega. Afirmar aqui exigiria uma
	// segunda fonte, que é justamente o que esta feature combate.
	TestEqual(TEXT("congelado em 02/09: 16 cavernas"),
		Assado->Caves.Num(), 16);
	TestEqual(TEXT("congelado em 02/09: 79 manchas de solo ao todo"),
		Assado->GroundUses.Num(), 79);

	AddInfo(FString::Printf(
		TEXT("CONGELADO em 02/09/2026 — %d manchas de solo, %d travessias, "
			 "%d cavernas, %d aquedutos"),
		Assado->GroundUses.Num(), Assado->Crossings.Num(),
		Assado->Caves.Num(), Assado->Aqueducts.Num()));

	return true;
}

// Copyright 2026 Anderson. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Engine/World.h"
#include "World/CrossingMesh.h"
#include "World/GroundUseActor.h"
#include "World/IslandBakedPlan.h"
#include "World/IslandChart.h"
#include "World/TrailLayout.h"

/**
 * O MUNDO CONTRA A CARTA — `docs/mundo/carta-ilha-de-mata.html`.
 *
 * A carta é o gabarito de aceite desta feature, e até aqui conferir contra ela
 * era DISCIPLINA: alguém abria o mapa, contava, e comparava. Disciplina falha
 * na terceira edição — é o mesmo argumento que fez o mundo e a carta lerem o
 * mesmo arquivo em vez de cada um o seu.
 *
 * Aqui os números da carta viram AFIRMAÇÃO. Eles estão escritos por extenso de
 * propósito: são o aceite combinado com quem desenhou o mundo, e um traçado que
 * passasse a produzir outra quantidade tem de reprovar AQUI, alto, em vez de
 * deixar carta e mundo divergirem em silêncio.
 *
 * Quando um número mudar de verdade, muda-se a carta e muda-se esta lista — nas
 * duas, de propósito, porque a divergência é a informação.
 *
 * ## O GABARITO FOI REESCRITO EM 03/09/2026, DE UMA VEZ SÓ
 *
 * Quatro decisões mexiam no mesmo gabarito — degrau na rocha, fundura por
 * ponto, pontes e mercado-negro escondido. Fazer uma de cada vez o reescreveria
 * quatro vezes, e cada reescrita é uma chance de mundo e carta divergirem sem
 * ninguém ver, que é exatamente o que ele existe para impedir.
 *
 * Por isso **cada número que mudou traz o motivo ao lado**, e **cada número que
 * NÃO mudou continua afirmado**: um gabarito que só lista o que mudou deixa de
 * proteger o que ficou.
 */

namespace ProvaDaCarta
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

	int32 PocosQueDaoAgua(const UIslandBakedPlan& Assado)
	{
		int32 Total = 0;
		for (const FBakedGroundUse& Mancha : Assado.GroundUses)
		{
			if (Mancha.Use == EGroundUse::Poco && Mancha.bYieldsWater)
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChartConformanceGroundUseMatchesTest,
	"BattleSquare.ChartConformance.GroundUseMatches",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChartConformanceGroundUseMatchesTest::RunTest(const FString& Parameters)
{
	const UIslandBakedPlan* Assado = IslandBakedPlan::Load();
	if (!Assado)
	{
		AddError(TEXT("o assado nao existe — rode ./Tools/bake_island.sh"));
		return false;
	}

	// OS QUE NÃO MUDARAM, e continuam afirmados. O degrau na rocha moveu o
	// terreno debaixo deles e eles ficaram onde estavam — isso é informação, e
	// some se o gabarito passar a listar só o que mudou.
	TestEqual(TEXT("carta: 9 bosques"),
		ProvaDaCarta::ManchasDe(*Assado, EGroundUse::Bosque), 9);
	TestEqual(TEXT("carta: 6 clareiras"),
		ProvaDaCarta::ManchasDe(*Assado, EGroundUse::ClareiraFechada), 6);
	TestEqual(TEXT("carta: 8 fazendas"),
		ProvaDaCarta::ManchasDe(*Assado, EGroundUse::Fazenda), 8);
	TestEqual(TEXT("carta: 4 criadouros"),
		ProvaDaCarta::ManchasDe(*Assado, EGroundUse::Criadouro), 4);
	TestEqual(TEXT("carta: 4 lojas"),
		ProvaDaCarta::ManchasDe(*Assado, EGroundUse::Loja), 4);
	TestEqual(TEXT("carta: 3 pomares"),
		ProvaDaCarta::ManchasDe(*Assado, EGroundUse::Pomar), 3);
	TestEqual(TEXT("carta: 7 decks"),
		ProvaDaCarta::ManchasDe(*Assado, EGroundUse::Deck), 7);
	TestEqual(TEXT("carta: 5 templos"),
		ProvaDaCarta::ManchasDe(*Assado, EGroundUse::Templo), 5);
	TestEqual(TEXT("carta: 4 ruinas"),
		ProvaDaCarta::ManchasDe(*Assado, EGroundUse::Ruina), 4);
	TestEqual(TEXT("carta: 7 cemiterios"),
		ProvaDaCarta::ManchasDe(*Assado, EGroundUse::Cemiterio), 7);

	// ACAMPAMENTOS: 6 -> 5. O acampamento nasce PERTO da trilha e LONGE da
	// vila; com o degrau na rocha as trilhas mudaram de rota (23 -> 18), e um
	// dos seis deixou de ter as duas coisas ao mesmo tempo.
	TestEqual(TEXT("carta: 5 acampamentos"),
		ProvaDaCarta::ManchasDe(*Assado, EGroundUse::Acampamento), 5);

	// POÇOS QUE DÃO ÁGUA: 2 -> 4. A chance sai de quão fundo está o lençol, e
	// o lençol acompanha a altura do chão — a escarpa baixou o terreno de dois
	// poços que antes davam secos. Contar todos daria doze, e o mapa prometeria
	// água em oito lugares secos.
	TestEqual(TEXT("carta: 4 pocos que dao agua"),
		ProvaDaCarta::PocosQueDaoAgua(*Assado), 4);

	// O MERCADO-NEGRO, e é aqui que a carta estreia a terceira contagem.
	//
	// Ele é LUGAR (K3) e é ESCONDIDO (J4): o gabarito o CONTA e não o aponta.
	// Sem as três colunas ele precisaria de exceção — e exceção no gabarito é
	// o começo do gabarito não valer.
	const IslandChart::FUseCount Mercados =
		IslandChart::CountOf(Assado->GroundUses, EGroundUse::MercadoNegro);
	TestEqual(TEXT("carta: 0 mercados-negros MOSTRADOS"), Mercados.Shown, 0);
	TestEqual(TEXT("carta: 3 mercados-negros ESCONDIDOS"), Mercados.Hidden, 3);
	TestEqual(TEXT("carta: 3 ao todo"), Mercados.Total(), 3);

	// E ninguém mais se escondeu junto: as manchas de sempre continuam
	// inteiramente mostradas. Sem esta linha, esconder uma loja passaria.
	const IslandChart::FUseCount Lojas =
		IslandChart::CountOf(Assado->GroundUses, EGroundUse::Loja);
	TestEqual(TEXT("carta: nenhuma loja escondida"), Lojas.Hidden, 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChartConformanceCrossingsMatchTest,
	"BattleSquare.ChartConformance.CrossingsMatch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChartConformanceCrossingsMatchTest::RunTest(const FString& Parameters)
{
	const UIslandBakedPlan* Assado = IslandBakedPlan::Load();
	if (!Assado)
	{
		AddError(TEXT("o assado nao existe — rode ./Tools/bake_island.sh"));
		return false;
	}

	// VAUS: 30 -> 22, e BALSAS: 25 -> 11. As duas caíram pelo mesmo motivo, e
	// não porque o mundo tenha menos água: as réguas foram consertadas. A
	// fundura deixou de ser estimada pela largura e passou a ser lida do
	// assado, e a cintura virou 40% da altura de quem pisa. Travessia que a
	// estimativa classificava por engano deixou de existir.
	TestEqual(TEXT("carta: 22 vaus"),
		ProvaDaCarta::TravessiasDe(*Assado, TrailLayout::ECrossingKind::Vau), 22);
	TestEqual(TEXT("carta: 11 balsas"),
		ProvaDaCarta::TravessiasDe(*Assado, TrailLayout::ECrossingKind::Balsa), 11);

	// BARRANCOS: 1 -> 0. O barranco existe onde uma margem é um degrau e a
	// outra não; com o traçado lendo a água como o pé a lê, aquele único ponto
	// passou a comportar ponte. O zero é afirmado como qualquer outro número.
	TestEqual(TEXT("carta: 0 barrancos"),
		ProvaDaCarta::TravessiasDe(*Assado, TrailLayout::ECrossingKind::Barranco), 0);

	// PONTES: 0 -> 4, e este é o dia que o comentário do zero previu.
	//
	// Ele dizia que "o zero é uma medição, não uma ausência de medição", e que
	// sem a linha o dia em que o traçado produzisse pontes passaria
	// despercebido. Passou a produzir, e a linha acusou.
	//
	// E não foi por alguém escrever pontes: as réguas consertadas revelaram
	// água que o traçado não enxergava, e o mundo pediu ponte sozinho.
	TestEqual(TEXT("carta: 4 pontes"),
		ProvaDaCarta::TravessiasDe(*Assado, TrailLayout::ECrossingKind::Ponte), 4);

	// E o total fecha: 22 + 11 + 4 + 0 barrancos = 37.
	TestEqual(TEXT("carta: 37 travessias ao todo"), Assado->Crossings.Num(), 37);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChartConformanceWaterAndTrailsMatchTest,
	"BattleSquare.ChartConformance.WaterAndTrailsMatch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChartConformanceWaterAndTrailsMatchTest::RunTest(const FString& Parameters)
{
	const UIslandBakedPlan* Assado = IslandBakedPlan::Load();
	if (!Assado)
	{
		AddError(TEXT("o assado nao existe — rode ./Tools/bake_island.sh"));
		return false;
	}

	// Os números que a spec desta feature levantou como o que o traçado calcula
	// e ninguém construía. Agora todos são construídos, e são estes.

	// CURSOS: 137 -> 111, e TRILHAS: 23 -> 18. A escarpa é a causa das duas: a
	// água escolhe onde o leito despenca, e com degraus diferentes ela escolhe
	// outros caminhos; a trilha nasce do custo de caminhada, e terreno mais
	// acidentado muda o custo.
	TestEqual(TEXT("111 cursos d'agua"), Assado->Rivers.Num(), 111);
	TestEqual(TEXT("18 trilhas"), Assado->Trails.Num(), 18);

	// CÓRREGOS e FONTES não mudaram, e continuam afirmados.
	TestEqual(TEXT("5 corregos"), Assado->Brooks.Num(), 5);
	TestEqual(TEXT("5 fontes"), Assado->Springs.Num(), 5);

	// GALERIAS: 158 -> 149. Parte é a rede acompanhar a bacia, que encolheu; e
	// parte é a M7 — as emendas entre grutas deixaram de ser sobra da distância
	// e passaram a ser a proporção pedida pelo bioma.
	TestEqual(TEXT("149 galerias"), Assado->UnderwaterLinks.Num(), 149);

	// AQUEDUTOS: 2 -> 3. O aqueduto exige captação MAIS ALTA que a vila, senão
	// é cano cheio de água parada. A escarpa deu essa altura a uma terceira.
	TestEqual(TEXT("3 aquedutos"), Assado->Aqueducts.Num(), 3);

	// CAVERNAS: 16 -> 11. A gruta nasce encostada numa cachoeira, e só onde
	// sobra terra entre o lago e a praia; com as quedas noutros lugares, cinco
	// dos vãos deixaram de caber. "Onde não houver lugar, não há gruta" já era
	// a regra — este é o número que ela deu.
	TestEqual(TEXT("11 cavernas"), Assado->Caves.Num(), 11);

	return true;
}

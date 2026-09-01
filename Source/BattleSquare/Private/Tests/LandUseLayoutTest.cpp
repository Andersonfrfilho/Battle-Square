// Copyright 2026 Anderson. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "World/LandUseLayout.h"
#include "World/RegionLayout.h"
#include "World/VillageLayout.h"
#include "World/TrailLayout.h"
#include "Environment/FreshWater.h"
#include "Environment/IslandGeography.h"

namespace UsoDoSolo
{
	int32 Contar(EGroundUse Uso)
	{
		int32 Quantos = 0;
		for (const FGroundUsePatch& Mancha : LandUseLayout::Plan())
		{
			if (Mancha.Use == Uso)
			{
				++Quantos;
			}
		}
		return Quantos;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLandUseTodaVilaTemFazendaTest,
	"BattleSquare.LandUse.TodaVilaTemFazenda",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLandUseTodaVilaTemFazendaTest::RunTest(const FString& Parameters)
{
	// Quem chega numa vila vê roçado antes de telhado. Vila sem lavoura é vila
	// que não come.
	for (const FSettlementPlacement& Assentamento : RegionLayout::Plan())
	{
		if (Assentamento.Kind == ESettlementKind::PostoDeFronteira)
		{
			continue;
		}

		bool bTemFazenda = false;
		const float Perto = VillageLayout::ClearingHalfExtentUnitsFor(Assentamento.Kind) * 4.0f;

		for (const FGroundUsePatch& Mancha : LandUseLayout::Plan())
		{
			if (Mancha.Use == EGroundUse::Fazenda
				&& FVector2D::Distance(Mancha.CenterUnits, Assentamento.CenterUnits) <= Perto)
			{
				bTemFazenda = true;
			}
		}

		TestTrue(TEXT("o assentamento tem fazenda ao lado"), bTemFazenda);
	}

	// O cais NÃO planta: ele é uma porta, e dar-lhe lavoura o transformaria em
	// destino — o que a spec da região proíbe.
	for (const FSettlementPlacement& Assentamento : RegionLayout::Plan())
	{
		if (Assentamento.Kind != ESettlementKind::PostoDeFronteira)
		{
			continue;
		}

		TestNotEqual(TEXT("o cais não tem lavoura"),
			static_cast<int32>(LandUseLayout::UseAt(Assentamento.CenterUnits)),
			static_cast<int32>(EGroundUse::Fazenda));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLandUseAFazendaOlhaParaAAguaTest,
	"BattleSquare.LandUse.AFazendaOlhaParaAAgua",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLandUseAFazendaOlhaParaAAguaTest::RunTest(const FString& Parameters)
{
	// A direção não é sorteada: é a da água mais perto. Roçado longe do rio é
	// roçado que ninguém rega, e a fazenda estaria ali só porque o gerador
	// precisava pôr uma em algum lugar.
	for (const FSettlementPlacement& Assentamento : RegionLayout::Plan())
	{
		if (Assentamento.Kind == ESettlementKind::PostoDeFronteira)
		{
			continue;
		}

		// A conta olha FONTE E CÓRREGO, que é a mesma água que o traçado
		// consulta. Antes ela olhava só as fontes, e quebrou no dia em que os
		// rios mudaram de curva: a fazenda estava virada para um córrego, e o
		// teste media contra uma fonte do outro lado. Meia fonte de verdade
		// afirma meia coisa.
		auto AteAAgua = [](const FVector2D& Daqui)
		{
			float Menor = TNumericLimits<float>::Max();

			for (const FreshWater::FSpring& Fonte : FreshWater::PlanSprings())
			{
				Menor = FMath::Min(Menor, FVector2D::Distance(Daqui, Fonte.CenterUnits));
			}

			for (const FreshWater::FBrook& Corrego : FreshWater::PlanBrooks())
			{
				for (const FVector2D& Ponto : Corrego.PointsUnits)
				{
					Menor = FMath::Min(Menor, FVector2D::Distance(Daqui, Ponto));
				}
			}

			return Menor;
		};

		const float MaisPertoDaVila = AteAAgua(Assentamento.CenterUnits);
		float MaisPertoDaFazenda = TNumericLimits<float>::Max();

		bool bAchouFazenda = false;
		const float Perto = VillageLayout::ClearingHalfExtentUnitsFor(Assentamento.Kind) * 4.0f;

		for (const FGroundUsePatch& Mancha : LandUseLayout::Plan())
		{
			if (Mancha.Use != EGroundUse::Fazenda
				|| FVector2D::Distance(Mancha.CenterUnits, Assentamento.CenterUnits) > Perto)
			{
				continue;
			}

			bAchouFazenda = true;
			MaisPertoDaFazenda = FMath::Min(MaisPertoDaFazenda, AteAAgua(Mancha.CenterUnits));
		}

		if (bAchouFazenda)
		{
			TestTrue(TEXT("a fazenda está mais perto da água que a vila"),
				MaisPertoDaFazenda <= MaisPertoDaVila);
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLandUseAClareiraEhFechadaTest,
	"BattleSquare.LandUse.AClareiraEhFechada",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLandUseAClareiraEhFechadaTest::RunTest(const FString& Parameters)
{
	// "Fechada" é a única coisa que ela é. Clareira que a trilha corta é um
	// alargamento do caminho; o que faz dela um lugar é ninguém passar por ali
	// sem querer.
	TestTrue(TEXT("existe clareira fechada"),
		UsoDoSolo::Contar(EGroundUse::ClareiraFechada) > 0);

	for (const FGroundUsePatch& Mancha : LandUseLayout::Plan())
	{
		if (Mancha.Use != EGroundUse::ClareiraFechada)
		{
			continue;
		}

		TestFalse(TEXT("nenhuma trilha passa pela clareira"),
			TrailLayout::IsOnTrail(Mancha.CenterUnits));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLandUseOBosqueEhMaisCerradoTest,
	"BattleSquare.LandUse.OBosqueEhMaisCerrado",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLandUseOBosqueEhMaisCerradoTest::RunTest(const FString& Parameters)
{
	// Sem isto, a densidade poderia devolver sempre um e a mata continuaria
	// uniforme — a mesma armadilha do relevo que poderia ser uma constante.
	TestTrue(TEXT("existe bosque"), UsoDoSolo::Contar(EGroundUse::Bosque) > 0);

	for (const FGroundUsePatch& Mancha : LandUseLayout::Plan())
	{
		if (Mancha.Use != EGroundUse::Bosque)
		{
			continue;
		}

		TestTrue(TEXT("no bosque nasce mais planta que no chão comum"),
			LandUseLayout::PlantingDensityAt(Mancha.CenterUnits) > 1.0f);
		TestFalse(TEXT("e o bosque não impede plantar"),
			LandUseLayout::BlocksPlanting(Mancha.CenterUnits));
	}

	// Fazenda e clareira são VAZIOS, por motivos opostos.
	for (const FGroundUsePatch& Mancha : LandUseLayout::Plan())
	{
		if (Mancha.Use == EGroundUse::Fazenda || Mancha.Use == EGroundUse::ClareiraFechada)
		{
			TestTrue(TEXT("ali não se planta"),
				LandUseLayout::BlocksPlanting(Mancha.CenterUnits));
			TestEqual(TEXT("e a densidade é zero"),
				LandUseLayout::PlantingDensityAt(Mancha.CenterUnits), 0.0f);
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLandUseAsFontesMolhamOMioloTest,
	"BattleSquare.LandUse.AsFontesMolhamOMiolo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLandUseAsFontesMolhamOMioloTest::RunTest(const FString& Parameters)
{
	// A ilha só tinha água que descia de montanha, e isso deixava o miolo
	// inteiro seco — que é justamente onde todo mundo mora.
	const TArray<FreshWater::FSpring> Fontes = FreshWater::PlanSprings();
	TestTrue(TEXT("há fontes"), Fontes.Num() > 0);

	const float SaiaDoMonte = FreshWater::Plan().Num() > 0
		? FreshWater::Plan()[0].SourceRadiusUnits
		: IslandGeography::LandRadiusUnits();

	for (const FreshWater::FSpring& Fonte : Fontes)
	{
		TestTrue(TEXT("a fonte brota no miolo, onde nenhum rio chega"),
			Fonte.CenterUnits.Size() < SaiaDoMonte);
		TestTrue(TEXT("e em terra"), IslandGeography::IsOnLand(Fonte.CenterUnits));
	}

	// Água que corre lado a lado sem nunca se encontrar não é bacia, é listras.
	const TArray<FreshWater::FBrook> Corregos = FreshWater::PlanBrooks();
	// UM por fonte, e mais nenhum.
	//
	// Havia também um córrego ligando o curso i ao i+1, e a regra nasceu quando
	// a ilha tinha três rios paralelos que precisavam se encontrar. Com
	// centenas de cursos ela cavava centenas de canais retos atravessando a
	// ilha — mais água inventada que todos os rios juntos.
	//
	// A bacia dendrítica já é conexa por construção; quem liga bacia a bacia é
	// a rede subterrânea.
	TestEqual(TEXT("um córrego por fonte, e mais nenhum"), Corregos.Num(), Fontes.Num());

	for (const FreshWater::FBrook& Corrego : Corregos)
	{
		TestTrue(TEXT("o córrego tem curso"), Corrego.PointsUnits.Num() >= 2);
		TestTrue(TEXT("e é mais estreito que o rio — atravessa-se a pé"),
			Corrego.HalfWidthUnits < FreshWater::RiverHalfWidthUnits());
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLandUseDaParaIrDeBarcoATodoLugarTest,
	"BattleSquare.LandUse.DaParaIrDeBarcoATodoLugar",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLandUseDaParaIrDeBarcoATodoLugarTest::RunTest(const FString& Parameters)
{
	// "Dá para ir de barco a todo lugar" vira UM grafo, e ele é conexo ou não
	// é. Sem esta pergunta em código, a resposta seria olhar o mapa e achar
	// que sim — e achar é o que este projeto vem trocando por medir.
	TestTrue(TEXT("toda a água doce está ligada, por cima ou por baixo"),
		FreshWater::IsWaterNetworkConnected());

	// E as ligações de baixo existem de fato: se `PlanUnderwaterLinks` voltasse
	// vazio, a conexão viria só dos córregos e o teste acima continuaria verde
	// sem que a rede subterrânea existisse.
	const TArray<FreshWater::FUnderwaterLink> Passagens = FreshWater::PlanUnderwaterLinks();
	TestTrue(TEXT("há passagem subterrânea"), Passagens.Num() > 0);

	for (const FreshWater::FUnderwaterLink& Passagem : Passagens)
	{
		// Passagem de pedra é apertada, sempre. Se ela coubesse barco grande, o
		// atalho de baixo tornaria o rio de cima decorativo.
		TestEqual(TEXT("por baixo só passa barco pequeno"),
			static_cast<int32>(Passagem.Navigability),
			static_cast<int32>(FreshWater::ENavigability::BarcoPequeno));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLandUseNemTodoTrechoAceitaBarcoGrandeTest,
	"BattleSquare.LandUse.NemTodoTrechoAceitaBarcoGrande",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLandUseNemTodoTrechoAceitaBarcoGrandeTest::RunTest(const FString& Parameters)
{
	// A largura decide, e é a MESMA largura que desenha o rio. Uma segunda
	// tabela dizendo onde o barco cabe concordaria com o desenho até a
	// primeira vez que alguém alargasse um rio.
	const TArray<FreshWater::FRiverCourse> Rios = FreshWater::Plan();
	if (!TestTrue(TEXT("há rio"), Rios.Num() > 0))
	{
		return false;
	}

	// No lago passa barco grande; na cabeceira, não. Se os dois dessem o mesmo,
	// a navegação seria uniforme e a largura não significaria nada.
	TestEqual(TEXT("o lago aceita barco grande"),
		static_cast<int32>(FreshWater::NavigabilityForHalfWidth(
			FreshWater::HalfWidthAtProgress(Rios[0], Rios[0].LakeAtProgress))),
		static_cast<int32>(FreshWater::ENavigability::BarcoGrande));

	// E o córrego é fio de água: atravessa-se a pé, e é por isso que ele não
	// pede ponte.
	const TArray<FreshWater::FBrook> Corregos = FreshWater::PlanBrooks();
	if (Corregos.Num() > 0)
	{
		TestNotEqual(TEXT("no córrego não passa barco grande"),
			static_cast<int32>(FreshWater::NavigabilityForHalfWidth(Corregos[0].HalfWidthUnits)),
			static_cast<int32>(FreshWater::ENavigability::BarcoGrande));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLandUseABaciaEhUmaArvoreTest,
	"BattleSquare.LandUse.ABaciaEhUmaArvore",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLandUseABaciaEhUmaArvoreTest::RunTest(const FString& Parameters)
{
	// "Parece raiz" vira contagem: precisa haver TRÊS ordens, e mais fiapos
	// que galhos e mais galhos que troncos. Um tronco com dois galhos entrando
	// no mesmo ponto desenha um Y, e Y não é raiz.
	int32 PorOrdem[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

	for (const FreshWater::FRiverCourse& Curso : FreshWater::Plan())
	{
		if (Curso.Order >= 1 && Curso.Order <= 7)
		{
			++PorOrdem[Curso.Order];
		}
	}

	TestTrue(TEXT("há fiapo de cabeceira"), PorOrdem[1] > 0);
	TestTrue(TEXT("há galho"), PorOrdem[2] > 0);

	// A ordem MÁXIMA não é 3 fixo: a bacia cresceu e hoje chega a 5. Amarrar a
	// afirmação ao número 3 era afirmar o tamanho de ontem.
	int32 Maior = 1;
	for (const FreshWater::FRiverCourse& Curso : FreshWater::Plan())
	{
		Maior = FMath::Max(Maior, Curso.Order);
	}

	TestTrue(TEXT("a bacia tem pelo menos três ordens"), Maior >= 3);
	TestTrue(TEXT("há mais fiapos que galhos"), PorOrdem[1] > PorOrdem[2]);

	int32 DoMaior = 0;
	for (const FreshWater::FRiverCourse& Curso : FreshWater::Plan())
	{
		DoMaior += (Curso.Order == Maior) ? 1 : 0;
	}

	TestTrue(TEXT("e mais fiapos que troncos"), PorOrdem[1] > DoMaior);

	// As junções em ALTURAS diferentes: dois galhos entrando no mesmo raio
	// desenham uma flecha, não uma bacia.
	TSet<int32> Encontros;
	for (const FreshWater::FRiverCourse& Curso : FreshWater::Plan())
	{
		if (!Curso.FlowsToTheSea())
		{
			Encontros.Add(FMath::RoundToInt(Curso.JoinRadiusUnits / 100.0f));
		}
	}

	// As junções em raios DIFERENTES: duas no mesmo raio desenham uma flecha.
	//
	// A comparação era com a contagem de cursos de ordem 3, e isso amarrava a
	// afirmação a uma ordem específica — que mudou quando a bacia cresceu, e
	// muda de novo quando a costa muda de forma.
	//
	// O que se quer dizer é que os encontros se ESPALHAM, e isso é uma conta
	// sobre eles mesmos.
	TestTrue(TEXT("as junções ficam em raios diferentes"), Encontros.Num() > 2);

	// E a calha cresce com a ordem: fiapo é fio, tronco é rio.
	float DoFiapo = 0.0f;
	float DoTronco = 0.0f;
	for (const FreshWater::FRiverCourse& Curso : FreshWater::Plan())
	{
		// Fora do lago: com a barriga do lago encostando na nascente, um fiapo
		// mede mais que um tronco e a comparação afirma o contrário do que quer.
		if (Curso.HasLake() && Curso.LakeAtProgress < 0.25f)
		{
			continue;
		}

		const float NaNascente = FreshWater::HalfWidthAtProgress(Curso, 0.0f);
		if (Curso.Order == 1) { DoFiapo = FMath::Max(DoFiapo, NaNascente); }
		if (Curso.Order >= 3) { DoTronco = FMath::Max(DoTronco, NaNascente); }
	}

	TestTrue(TEXT("o tronco nasce mais largo que o fiapo"), DoTronco > DoFiapo);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLandUseACorredeiraSaiDoRelevoTest,
	"BattleSquare.LandUse.ACorredeiraSaiDoRelevo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLandUseACorredeiraSaiDoRelevoTest::RunTest(const FString& Parameters)
{
	// A corredeira não é uma faixa escolhida a dedo: ela é derivada do relevo.
	// Onde o chão desce depressa, a água quebra. Sem este teste, `IsRapidsAt`
	// poderia devolver sempre falso e nada acusaria.
	int32 Corredeiras = 0;
	int32 Mansos = 0;

	for (const FreshWater::FRiverCourse& Curso : FreshWater::PlanTrunks())
	{
		for (int32 Passo = 0; Passo <= 120; ++Passo)
		{
			const float Onde = static_cast<float>(Passo) / 120.0f;
			if (FreshWater::IsRapidsAtProgress(Curso, Onde)) { ++Corredeiras; } else { ++Mansos; }
		}
	}

	TestTrue(TEXT("há corredeira em algum trecho"), Corredeiras > 0);
	TestTrue(TEXT("e trecho manso também — senão o rio inteiro seria corredeira"), Mansos > 0);

	// Dentro do lago não há corredeira, por mais inclinada que esteja a encosta
	// em volta: lago é água PARADA.
	for (const FreshWater::FRiverCourse& Curso : FreshWater::Plan())
	{
		if (!Curso.HasLake()) { continue; }
		TestFalse(TEXT("não há corredeira dentro do lago"),
			FreshWater::IsRapidsAtProgress(Curso, Curso.LakeAtProgress));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLandUseOPocoEhFuroNaoBaciaTest,
	"BattleSquare.LandUse.OPocoEhFuroNaoBacia",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLandUseOPocoEhFuroNaoBaciaTest::RunTest(const FString& Parameters)
{
	// A morfologia de poço de queda diz que a profundidade sai da ALTURA da
	// queda: queda alta cava poço fundo.
	//
	// A primeira versão deste teste afirmava "mais fundo que largo", e estava
	// errada: eu li o "dez vezes" da literatura como forma quando ele é uma
	// razão de VELOCIDADE de erosão — a incisão vertical supera a lateral. Poço
	// é mais largo que fundo, como qualquer poço.
	//
	// O teste reprovou o código, e quem estava errado era o teste. Vale
	// registrar: asserção mal lida vira defeito que ninguém acha, porque ela
	// está verde no dia em que é escrita.
	int32 Pocos = 0;

	for (const FreshWater::FRiverCourse& Curso : FreshWater::Plan())
	{
		const float Fundo = FreshWater::PlungePoolDepthUnits(Curso);
		const float Meia = FreshWater::PlungePoolHalfWidthUnits(Curso);

		if (Fundo <= 0.0f)
		{
			continue;
		}

		++Pocos;
		TestTrue(TEXT("o poço é mais largo que fundo"), Meia > Fundo);
		TestTrue(TEXT("e mais largo que a calha que o alimenta"),
			Meia > FreshWater::RiverHalfWidthUnits());

		// Poço não é lago: o que os separa é o tamanho, e sem esta linha os
		// dois seriam a mesma coisa com nomes diferentes.
		TestTrue(TEXT("e bem menor que o lago"),
			Meia < FreshWater::LakeHalfWidthUnits() * 0.5f);
	}

	TestTrue(TEXT("toda queda tem poço"), Pocos > 0);

	// POÇO existe onde existe QUEDA, e mais em lugar nenhum.
	//
	// A afirmação era "galho não tem poço", e ela valia quando a cachoeira
	// morava no tronco. Ela deixou de morar lá: a queda nasce onde o leito
	// despenca, e isso acontece rio acima — em galho, quase sempre. Amarrar o
	// poço ao tronco passou a afirmar o modelo antigo.
	for (const FreshWater::FRiverCourse& Curso : FreshWater::Plan())
	{
		if (!Curso.HasFall())
		{
			TestEqual(TEXT("curso sem queda não tem poço"),
				FreshWater::PlungePoolDepthUnits(Curso), 0.0f);
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLandUseACachoeiraSeSobeTest,
	"BattleSquare.LandUse.ACachoeiraSeSobe",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLandUseACachoeiraSeSobeTest::RunTest(const FString& Parameters)
{
	// Um paredão de vinte metros não se sobe; uma escada de quatro degraus de
	// cinco, sim. É a escada de poços que a morfologia descreve, e é ela que
	// transforma a cachoeira de parede em lugar.
	int32 ComEscada = 0;

	// TODOS os cursos: a cachoeira deixou de morar no tronco quando passou a
	// ser escolhida pelo leito.
	for (const FreshWater::FRiverCourse& Curso : FreshWater::Plan())
	{
		const TArray<FreshWater::FFallStep> Degraus = FreshWater::PlanFallSteps(Curso);
		if (Degraus.Num() == 0)
		{
			continue;
		}

		++ComEscada;

		// Os patamares SOBEM: se dois tivessem a mesma altura não seria escada,
		// seria uma laje partida.
		for (int32 Qual = 1; Qual < Degraus.Num(); ++Qual)
		{
			TestTrue(TEXT("cada patamar é mais alto que o anterior"),
				Degraus[Qual].HeightUnits > Degraus[Qual - 1].HeightUnits);
		}

		// E a subida existe, com um ponto por patamar no mínimo.
		const TArray<FVector2D> Subida = FreshWater::PlanFallClimb(Curso);
		TestTrue(TEXT("há caminho subindo a queda"), Subida.Num() >= Degraus.Num());

		// A subida vai pela MARGEM, não pela lâmina: quem sobe pela água não
		// sobe, cai.
		// A lâmina é medida NO PONTO onde o caminho passa, não no da queda: a
		// calha muda ao longo do curso, e comparar com a largura de outro
		// trecho afirma a coisa errada nos dois sentidos.
		for (const FVector2D& NoCaminho : Subida)
		{
			float Onde = 0.0f;
			const float Ate = FreshWater::NearestOn(Curso, NoCaminho, Onde);

			TestTrue(TEXT("a subida vai pela margem, fora da lâmina"),
				Ate > FreshWater::HalfWidthAtProgress(Curso, Onde));
		}

		// AS PEDRAS, e as que viraram degrau.
		const TArray<FreshWater::FFallStone> Pedras = FreshWater::PlanFallStones(Curso);
		TestTrue(TEXT("a queda tem pedras"), Pedras.Num() > 0);

		int32 Degrausinhos = 0;
		for (const FreshWater::FFallStone& Pedra : Pedras)
		{
			TestTrue(TEXT("a pedra tem tamanho"), Pedra.RadiusUnits > 0.0f);
			Degrausinhos += Pedra.bIsStep ? 1 : 0;
		}

		// Pedra no meio do poço é obstáculo; pedra na beira do caminho é apoio.
		// Sem alguma das segundas, "escada improvisada" seria só "pedras
		// espalhadas", que é outra coisa.
		TestTrue(TEXT("e algumas delas servem de degrau"), Degrausinhos > 0);
	}

	TestTrue(TEXT("há cachoeira com escada"), ComEscada > 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLandUseOTemploFicaOndeSeuDeusMandaTest,
	"BattleSquare.LandUse.OTemploFicaOndeSeuDeusManda",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLandUseOTemploFicaOndeSeuDeusMandaTest::RunTest(const FString& Parameters)
{
	// A posição do templo é a IDENTIDADE do deus, e não decoração: quem vê um
	// templo sabe de quem ele é pelo lugar onde está. É assim que um panteão se
	// ensina sem uma linha de texto — e é a única coisa que este teste protege.
	TMap<int32, FVector2D> Onde;

	for (const FGroundUsePatch& Mancha : LandUseLayout::Plan())
	{
		if (Mancha.Use == EGroundUse::Templo)
		{
			Onde.Add(static_cast<int32>(Mancha.Deity), Mancha.CenterUnits);
		}
	}

	TestTrue(TEXT("há templos"), Onde.Num() > 0);

	// BRASEIRO na beira da rocha queimada — e do lado de FORA dela. Dentro
	// seria templo que ninguém alcança: o chão ali queima, e a trilha não
	// atravessa. Um templo é um lugar de ir.
	if (const FVector2D* DoFogo = Onde.Find(static_cast<int32>(EDeity::Braseiro)))
	{
		const float AteAsBrasas =
			FVector2D::Distance(*DoFogo, IslandGeography::VolcanoCenterUnits());

		TestTrue(TEXT("o templo do fogo fica fora da rocha queimada"),
			AteAsBrasas > IslandGeography::VolcanoScorchedRadiusUnits());
		TestTrue(TEXT("mas encostado nela"),
			AteAsBrasas < IslandGeography::VolcanoScorchedRadiusUnits() * 2.0f);
	}

	// CORRENTE encostada numa cachoeira, e fora da lâmina: templo dentro do rio
	// é templo submerso.
	if (const FVector2D* DaAgua = Onde.Find(static_cast<int32>(EDeity::Corrente)))
	{
		float MaisPerto = TNumericLimits<float>::Max();
		for (const FreshWater::FRiverCourse& Curso : FreshWater::Plan())
		{
			if (!Curso.HasFall())
			{
				continue;
			}

			MaisPerto = FMath::Min(MaisPerto, static_cast<float>(FVector2D::Distance(
				*DaAgua, FreshWater::PointAtProgress(Curso, Curso.FallAtProgress))));
		}

		TestTrue(TEXT("o templo da água fica junto de uma cachoeira"),
			MaisPerto < FreshWater::LakeHalfWidthUnits() * 3.0f);

		float Aonde = 0.0f;
		float ForaDaAgua = TNumericLimits<float>::Max();
		for (const FreshWater::FRiverCourse& Curso : FreshWater::Plan())
		{
			ForaDaAgua = FMath::Min(ForaDaAgua,
				FreshWater::NearestOn(Curso, *DaAgua, Aonde)
					- FreshWater::HalfWidthAtProgress(Curso, Aonde));
		}

		TestTrue(TEXT("e fora da lâmina"), ForaDaAgua > 0.0f);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLandUseARuinaEstaEsquecidaTest,
	"BattleSquare.LandUse.ARuinaEstaEsquecida",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLandUseARuinaEstaEsquecidaTest::RunTest(const FString& Parameters)
{
	// "Esquecida" é a única coisa que a ruína é, e por isso é o que se afirma.
	//
	// Ela é o oposto do templo em tudo o que importa: perto de trilha ou de
	// vila ela deixa de ser esquecida, e vira mais um prédio. E é dela que vem
	// a única coisa que nenhuma outra peça deste mundo dá — passado: um lugar
	// que já foi importante e deixou de ser conta uma história que ninguém
	// precisou escrever.
	int32 Quantas = 0;

	for (const FGroundUsePatch& Mancha : LandUseLayout::Plan())
	{
		if (Mancha.Use != EGroundUse::Ruina)
		{
			continue;
		}

		++Quantas;

		TestFalse(TEXT("nenhuma trilha passa pela ruína"),
			TrailLayout::IsOnTrail(Mancha.CenterUnits));

		for (const FSettlementPlacement& Assentamento : RegionLayout::Plan())
		{
			TestTrue(TEXT("e ela fica longe de qualquer vila"),
				FVector2D::Distance(Mancha.CenterUnits, Assentamento.CenterUnits)
					> VillageLayout::ClearingHalfExtentUnitsFor(Assentamento.Kind) * 2.5f);
		}
	}

	TestTrue(TEXT("há ruína"), Quantas > 0);

	return true;
}

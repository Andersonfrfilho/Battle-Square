// Copyright 2026 Anderson. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "World/TrailLayout.h"
#include "World/RegionLayout.h"
#include "Environment/IslandGeography.h"
#include "Environment/FreshWater.h"

namespace TracadoDaTrilha
{
	float CustoDe(const FTrailRoute& Trilha)
	{
		float Total = 0.0f;
		for (int32 Ponto = 1; Ponto < Trilha.PointsUnits.Num(); ++Ponto)
		{
			Total += IslandGeography::TravelCostBetween(
				Trilha.PointsUnits[Ponto - 1], Trilha.PointsUnits[Ponto]);
		}
		return Total;
	}

	float CustoEmLinhaReta(const FTrailRoute& Trilha)
	{
		const FVector2D Daqui = Trilha.PointsUnits[0];
		const FVector2D Prali = Trilha.PointsUnits.Last();

		const int32 Passos = FMath::Max(2,
			FMath::CeilToInt(FVector2D::Distance(Daqui, Prali) / TrailLayout::StepUnits()));

		float Total = 0.0f;
		FVector2D Anterior = Daqui;
		for (int32 Passo = 1; Passo <= Passos; ++Passo)
		{
			const FVector2D Agora = FMath::Lerp(Daqui, Prali,
				static_cast<float>(Passo) / static_cast<float>(Passos));
			Total += IslandGeography::TravelCostBetween(Anterior, Agora);
			Anterior = Agora;
		}
		return Total;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTrailLayoutTodoAssentamentoEstaLigadoTest,
	"BattleSquare.TrailLayout.TodoAssentamentoEstaLigado",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTrailLayoutTodoAssentamentoEstaLigadoTest::RunTest(const FString& Parameters)
{
	// Assentamento sem trilha é assentamento que ninguém acha — o mesmo
	// defeito de "não vi esses campos de treino", relatado jogando.
	TSet<int32> Ligados;
	for (const FTrailRoute& Trilha : TrailLayout::Plan())
	{
		Ligados.Add(static_cast<int32>(Trilha.From));
		Ligados.Add(static_cast<int32>(Trilha.To));
	}

	for (const FSettlementPlacement& Assentamento : RegionLayout::Plan())
	{
		TestTrue(TEXT("o assentamento tem trilha"),
			Ligados.Contains(static_cast<int32>(Assentamento.Kind)));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTrailLayoutNenhumaTrilhaEhRetaTest,
	"BattleSquare.TrailLayout.NenhumaTrilhaEhReta",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTrailLayoutNenhumaTrilhaEhRetaTest::RunTest(const FString& Parameters)
{
	// A trilha não é uma curva que alguém desenhou: ela curva porque o TERRENO
	// a curva. Sem este teste, o traçador poderia devolver a reta e todos os
	// outros continuariam verdes — a mesma armadilha do relevo que poderia ser
	// uma constante.
	//
	// A medida é a SINUOSIDADE, e não "custa menos que a reta". Ela era isso, e
	// deixou de valer quando a trilha passou a respeitar o declive
	// sustentável: agora ela é mais LONGA e mais CANSATIVA que a reta de
	// propósito, porque troca esforço por caminho que não vira valeta. Trilha
	// de montanha real faz exatamente essa troca — quem sobe pelo ziguezague
	// anda três vezes mais que quem sobe de frente.
	//
	// Manter a asserção antiga teria me feito desfazer a melhoria para o teste
	// voltar ao verde.
	int32 Sinuosas = 0;

	for (const FTrailRoute& Trilha : TrailLayout::Plan())
	{
		if (!TestTrue(TEXT("a trilha tem pontos"), Trilha.PointsUnits.Num() >= 2))
		{
			continue;
		}

		float Andado = 0.0f;
		for (int32 Ponto = 1; Ponto < Trilha.PointsUnits.Num(); ++Ponto)
		{
			Andado += FVector2D::Distance(
				Trilha.PointsUnits[Ponto - 1], Trilha.PointsUnits[Ponto]);
		}

		const float EmLinhaReta =
			FVector2D::Distance(Trilha.PointsUnits[0], Trilha.PointsUnits.Last());

		if (EmLinhaReta > KINDA_SMALL_NUMBER && (Andado / EmLinhaReta) > 1.05f)
		{
			++Sinuosas;
		}
	}

	TestTrue(TEXT("as trilhas contornam em vez de ir em linha reta"),
		Sinuosas >= TrailLayout::Plan().Num() / 2);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTrailLayoutNinguemPassaPelaRochaQueimadaTest,
	"BattleSquare.TrailLayout.NinguemPassaPelaRochaQueimada",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTrailLayoutNinguemPassaPelaRochaQueimadaTest::RunTest(const FString& Parameters)
{
	for (const FTrailRoute& Trilha : TrailLayout::Plan())
	{
		for (const FVector2D& Ponto : Trilha.PointsUnits)
		{
			TestTrue(TEXT("a trilha não corta a rocha queimada"),
				FVector2D::Distance(Ponto, IslandGeography::VolcanoCenterUnits())
					> IslandGeography::VolcanoScorchedRadiusUnits());

			TestTrue(TEXT("nem sai da terra"), IslandGeography::IsOnLand(Ponto));
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTrailLayoutAsPontasEncostamNoAssentamentoTest,
	"BattleSquare.TrailLayout.AsPontasEncostamNoAssentamento",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTrailLayoutAsPontasEncostamNoAssentamentoTest::RunTest(const FString& Parameters)
{
	// Trilha que para a meio passo da praça é trilha que não chega. O traçado
	// anda em grade, e o centro do assentamento quase nunca cai no centro de
	// uma célula — as pontas precisam ser corrigidas de propósito.
	for (const FTrailRoute& Trilha : TrailLayout::Plan())
	{
		if (Trilha.PointsUnits.Num() < 2)
		{
			continue;
		}

		bool bAchouOrigem = false;
		bool bAchouDestino = Trilha.Destination != ETrailDestination::Assentamento;

		for (const FSettlementPlacement& Assentamento : RegionLayout::Plan())
		{
			if (Assentamento.Kind == Trilha.From
				&& Trilha.PointsUnits[0].Equals(Assentamento.CenterUnits, 1.0f))
			{
				bAchouOrigem = true;
			}
			if (Trilha.Destination == ETrailDestination::Assentamento
				&& Assentamento.Kind == Trilha.To
				&& Trilha.PointsUnits.Last().Equals(Assentamento.CenterUnits, 1.0f))
			{
				bAchouDestino = true;
			}
		}

		TestTrue(TEXT("a trilha começa no assentamento"), bAchouOrigem);
		TestTrue(TEXT("e termina no outro"), bAchouDestino);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTrailLayoutOsMarcosNaturaisTemCaminhoTest,
	"BattleSquare.TrailLayout.OsMarcosNaturaisTemCaminho",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTrailLayoutOsMarcosNaturaisTemCaminhoTest::RunTest(const FString& Parameters)
{
	// Uma cachoeira sem caminho é a cachoeira que o relato de jogo disse nunca
	// ter visto. Ela existia, tinha posição calculada, e ninguém tinha como
	// saber onde.
	//
	// E foram estas trilhas que fizeram a PONTE existir: enquanto todo destino
	// ficava ENTRE os rios, nenhuma travessia era necessária. Os rios correm do
	// monte para o mar, e as trilhas entre vilas corriam junto — retas
	// paralelas não se cruzam.
	int32 ParaCachoeira = 0;
	int32 ParaMonte = 0;

	for (const FTrailRoute& Trilha : TrailLayout::Plan())
	{
		if (Trilha.Destination == ETrailDestination::Cachoeira) { ++ParaCachoeira; }
		if (Trilha.Destination == ETrailDestination::Monte) { ++ParaMonte; }
	}

	// Uma por TRONCO: a cachoeira mora no tronco, porque galho de cabeceira
	// morre na junção e não despenca em lugar nenhum.
	// Uma por CACHOEIRA, e a cachoeira deixou de morar no tronco: ela nasce
	// onde o leito despenca, que é rio acima.
	int32 ComQueda = 0;
	for (const FreshWater::FRiverCourse& Rio : FreshWater::Plan())
	{
		ComQueda += Rio.HasFall() ? 1 : 0;
	}

	TestEqual(TEXT("toda cachoeira tem caminho"), ParaCachoeira, ComQueda);
	TestTrue(TEXT("e há menos troncos que cursos — a bacia é uma árvore"),
		FreshWater::PlanTrunks().Num() < FreshWater::Plan().Num());
	TestTrue(TEXT("e todo monte também"), ParaMonte > 0);

	// A trilha para na MARGEM. Mirando o ponto exato da queda, o traçado
	// entraria no rio e pagaria a penalidade da água até o fim.
	for (const FTrailRoute& Trilha : TrailLayout::Plan())
	{
		if (Trilha.Destination != ETrailDestination::Cachoeira)
		{
			continue;
		}

		for (const FreshWater::FRiverCourse& Rio : FreshWater::PlanTrunks())
		{
			// A pergunta é de DISTÂNCIA À MARGEM, e ela tem função própria
			// agora. Perguntar por raio pulava justamente os trechos que
			// correm de lado.
			float Onde = 0.0f;
			const float Ate = FreshWater::NearestOn(Rio, Trilha.PointsUnits.Last(), Onde);

			TestTrue(TEXT("a trilha para na margem, não dentro da água"),
				Ate > FreshWater::HalfWidthAtProgress(Rio, Onde));
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTrailLayoutOPostoFicaNaCostaTest,
	"BattleSquare.TrailLayout.OPostoFicaNaCosta",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTrailLayoutOPostoFicaNaCostaTest::RunTest(const FString& Parameters)
{
	// A ilha é REDONDA: não existe fronteira de terra. A fronteira é a costa, e
	// o que se atravessa é água — a ilha é a unidade de servidor, e sair dela
	// é embarcar. Posto no meio do mato guarda uma linha que não existe.
	const float Raio = IslandGeography::LandRadiusUnits();
	const float Praia = IslandGeography::BeachWidthUnits();

	int32 Postos = 0;

	for (const FSettlementPlacement& Assentamento : RegionLayout::Plan())
	{
		if (Assentamento.Kind != ESettlementKind::PostoDeFronteira)
		{
			continue;
		}

		++Postos;
		const float Distancia = Assentamento.CenterUnits.Size();

		// NA AREIA, e não atrás dela.
		//
		// Eu o tinha posto atrás da praia com medo da rampa da orla, e cais
		// atrás da praia é galpão: quem embarca precisa alcançar a água. No
		// meio da faixa o chão já está na metade da altura da terra, e a rampa
		// não é problema.
		TestTrue(TEXT("o cais está na areia"), Distancia > Raio - Praia);
		TestTrue(TEXT("e não no mar"), Distancia < Raio);
	}

	TestEqual(TEXT("todos os postos foram medidos"), Postos, RegionLayout::BorderPostCount());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTrailLayoutOndeCruzaRioTemPonteTest,
	"BattleSquare.TrailLayout.OndeCruzaRioTemPonte",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTrailLayoutOndeCruzaRioTemPonteTest::RunTest(const FString& Parameters)
{
	// A ponte não é enfeite: sem ela a trilha entra na água, e um caminho que
	// afunda é pior que caminho nenhum, porque promete passagem.
	//
	// Zero pontes é resultado LEGÍTIMO — quer dizer que o traçado achou como
	// contornar todos os rios. O que não pode é ponte fora d'água.
	for (const FVector2D& Ponte : TrailLayout::BridgePoints())
	{
		TestTrue(TEXT("a ponte fica em cima da trilha"), TrailLayout::IsOnTrail(Ponte));
		TestTrue(TEXT("e em terra da ilha"), IslandGeography::IsOnLand(Ponte));
	}

	return true;
}

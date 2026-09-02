// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattleArenaConstants.h"
#include "Battle/BattleEvent.h"
#include "Battle/BattlePhases.h"
#include "Battle/BattleState.h"
#include "Misc/AutomationTest.h"

/**
 * SUBIR A CORRENTEZA CUSTA — é o que faz o sentido virar decisão de rota.
 *
 * Sem isto, os dois lados do rio custam igual e a corrente é enfeite: ela
 * empurra, mas atravessar continua sendo a mesma escolha nos dois sentidos.
 */

namespace ProvaDaSubida
{
	constexpr int32 Forte = BattleArenaConstants::CurrentCarriesAbovePerMille + 100;
	constexpr int32 Manso = BattleArenaConstants::CurrentCarriesAbovePerMille - 10;

	/**
	 * Rio correndo para a DIREITA na linha do meio, com CINCO colunas.
	 *
	 * Cinco, e não três, e o número foi corrigido por medição: num tabuleiro
	 * de três o pet começa no meio, e descer bate na borda enquanto subir é
	 * barrado e depois carregado — os dois terminam na MESMA casa, e a prova
	 * de que descer rende mais falha sem a regra ter nada de errado.
	 *
	 * O aparelho de medição precisa ser maior que a coisa que ele mede. Este
	 * projeto já se enganou quatro vezes pelo lado oposto — grade grossa
	 * demais para enxergar o que era fino.
	 */
	FBattleState CampoComRio(int32 ForcaPorMil)
	{
		FBattleState Estado;
		Estado.GridColumns = 5;
		Estado.GridRows = 3;
		Estado.CellLayout.SetNumZeroed(15);
		Estado.Random.State = 5;
		Estado.ApplyDefaultTerrainRequirements();

		for (int32 Coluna = 0; Coluna < 5; ++Coluna)
		{
			Estado.CellLayout[Estado.CellIndex(Coluna, 1)] =
				static_cast<uint8>(ECellProperty::Water);
			Estado.SetFlowAt(Coluna, 1, EBattleDirection::Direita, ForcaPorMil);
		}

		FPetState Nadador;
		Nadador.PetId = 1; Nadador.Side = 0; Nadador.Column = 1; Nadador.Row = 1;
		Nadador.Health = 100; Nadador.MaxHealth = 100; Nadador.Speed = 30;
		Estado.Pets.Add(Nadador);

		return Estado;
	}

	FBattleAction Andar(EBattleDirection Rumo)
	{
		FBattleAction Acao;
		Acao.Type = EActionType::Mover;
		Acao.Direction = Rumo;
		return Acao;
	}

	/** Anda e devolve onde parou, contando a carga da corrente. */
	FIntPoint AndouPara(FBattleState& Estado, EBattleDirection Rumo)
	{
		const FBattleAction Nada;
		TArray<FBattleEvent> Traco;
		BattlePhases::ApplyMovement(Estado, Andar(Rumo), Nada, 0, Traco);
		return FIntPoint(Estado.Pets[0].Column, Estado.Pets[0].Row);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUpstreamCostGoingAgainstFailsTest,
	"BattleSim.UpstreamCost.GoingAgainstFails",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUpstreamCostGoingAgainstFailsTest::RunTest(const FString& Parameters)
{
	// O ACEITE: o MESMO trecho, nos dois sentidos, com resultados diferentes.
	//
	// Contra a corrente forte, o passo não acontece — e a carga o devolve para
	// onde ele estava, então ele fica no lugar.
	FBattleState Subindo = ProvaDaSubida::CampoComRio(ProvaDaSubida::Forte);
	const FIntPoint DepoisDeSubir =
		ProvaDaSubida::AndouPara(Subindo, EBattleDirection::Esquerda);

	TestTrue(TEXT("subir a correnteza nao leva ninguem rio acima"),
		DepoisDeSubir.X >= 1);

	// A FAVOR, ele anda E é carregado: duas casas por uma ação. O ganho não foi
	// programado — ele CAI FORA da carga, e é o que torna descer barato sem
	// precisar de uma regra própria dizendo isso.
	FBattleState Descendo = ProvaDaSubida::CampoComRio(ProvaDaSubida::Forte);
	const FIntPoint DepoisDeDescer =
		ProvaDaSubida::AndouPara(Descendo, EBattleDirection::Direita);

	TestTrue(TEXT("descer rende mais que subir"), DepoisDeDescer.X > DepoisDeSubir.X);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUpstreamCostSidewaysPaysNothingTest,
	"BattleSim.UpstreamCost.SidewaysPaysNothing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUpstreamCostSidewaysPaysNothingTest::RunTest(const FString& Parameters)
{
	// O CONTRAPESO da task: atravessar de LADO não paga nem ganha. Sem ele,
	// uma regra que barrasse qualquer passo dentro d'água passaria no aceite —
	// e cortar o rio, que é o que uma travessia É, ficaria impossível.
	FBattleState DeLado = ProvaDaSubida::CampoComRio(ProvaDaSubida::Forte);
	const FIntPoint Depois =
		ProvaDaSubida::AndouPara(DeLado, EBattleDirection::Cima);

	TestEqual(TEXT("atravessar de lado sai da linha do rio"), Depois.Y, 0);

	// E fora da água a corrente não o alcança mais: ele não é carregado.
	TestEqual(TEXT("e fora da agua ele nao e levado"), Depois.X, 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUpstreamCostGentleWaterLetsYouThroughTest,
	"BattleSim.UpstreamCost.GentleWaterLetsYouThrough",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUpstreamCostGentleWaterLetsYouThroughTest::RunTest(const FString& Parameters)
{
	// O SEGUNDO CONTRAPESO: água mansa deixa subir. O limiar é o mesmo da
	// carga, e é o que faz "forte" querer dizer uma coisa só no jogo inteiro —
	// se um fio de água barrasse o passo, andar num riacho seria impossível.
	FBattleState Manso = ProvaDaSubida::CampoComRio(ProvaDaSubida::Manso);
	const FIntPoint Depois =
		ProvaDaSubida::AndouPara(Manso, EBattleDirection::Esquerda);

	TestEqual(TEXT("em agua mansa da para subir"), Depois.X, 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUpstreamCostFlyingIgnoresTheCurrentTest,
	"BattleSim.UpstreamCost.FlyingIgnoresTheCurrent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUpstreamCostFlyingIgnoresTheCurrentTest::RunTest(const FString& Parameters)
{
	// QUEM VOA SOBE. Ele não é carregado, então também não é barrado — as duas
	// regras têm de concordar sobre quem está na água, senão haveria um pet
	// que a corrente empurra e não segura, ou o contrário.
	FBattleState Voando = ProvaDaSubida::CampoComRio(ProvaDaSubida::Forte);
	Voando.Pets[0].PostureFlags |= static_cast<uint16>(EBattlePostureFlags::Flying);

	const FIntPoint Depois =
		ProvaDaSubida::AndouPara(Voando, EBattleDirection::Esquerda);

	TestEqual(TEXT("quem voa sobe a correnteza"), Depois.X, 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUpstreamCostIsNarratedTest,
	"BattleSim.UpstreamCost.IsNarrated",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUpstreamCostIsNarratedTest::RunTest(const FString& Parameters)
{
	// AÇÃO QUE SOME SEM FRASE PARECE DEFEITO — é o que o próprio escorregão
	// documenta, e este projeto já gastou rodadas com regra funcionando e
	// parecendo quebrada.
	FBattleState Estado = ProvaDaSubida::CampoComRio(ProvaDaSubida::Forte);

	const FBattleAction Nada;
	TArray<FBattleEvent> Traco;
	BattlePhases::ApplyMovement(Estado,
		ProvaDaSubida::Andar(EBattleDirection::Esquerda), Nada, 0, Traco);

	bool bNarrou = false;
	for (const FBattleEvent& Evento : Traco)
	{
		if (Evento.Type == EBattleEventType::Escorregou && Evento.ActorId == 1)
		{
			bNarrou = true;

			// E diz QUAL terreno segurou: sem o detalhe, o feed não distingue
			// o gelo da correnteza, e as duas param o passo do mesmo jeito.
			TestEqual(TEXT("o detalhe diz que foi a agua"),
				static_cast<int32>(Evento.Detail),
				static_cast<int32>(ECellProperty::Water));
		}
	}

	TestTrue(TEXT("o passo barrado pela corrente e narrado"), bNarrou);

	return true;
}

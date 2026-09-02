// Copyright 2026 Anderson. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "World/GroundUseActor.h"
#include "World/IslandBakedPlan.h"
#include "World/RegionLayout.h"

/**
 * CEMITÉRIO e CEMITÉRIO ESQUECIDO — o serviço e o vestígio.
 *
 * "O cemitério da vila e este não são o mesmo lugar em tamanhos diferentes: um
 * é serviço, o outro é vestígio." Desenhados iguais, o mapa perde a única
 * diferença que eles têm — e nenhuma contagem muda.
 *
 * ## Por que estes testes são EXISTENCIAIS
 *
 * A regra "o cemitério fica junto da vila" tenta um teste que RECONSTRÓI a
 * decisão, e isso já reprovou desenho certo duas vezes seguidas neste projeto.
 * A segunda tentativa elegia a vila DONA pela proximidade — e o cemitério de um
 * posto de fronteira cai mais perto da vila vizinha, então todas as contas
 * passaram a ser sobre a vila errada.
 *
 * Sem dono eleito não há desempate para errar: afirma-se que EXISTE vila para a
 * qual a propriedade vale.
 */

namespace ProvaDoCemiterio
{
	UWorld* MundoDeTeste()
	{
		return UWorld::CreateWorld(EWorldType::Game, false);
	}

	int32 Quantos(const UIslandBakedPlan& Assado, EGroundUse Uso)
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

	/** A distância até o assentamento mais próximo, seja ele qual for. */
	float AteAVilaMaisPerto(const FVector2D& Onde)
	{
		float Menor = TNumericLimits<float>::Max();
		for (const FSettlementPlacement& Vila : RegionLayout::Plan())
		{
			Menor = FMath::Min(Menor, FVector2D::Distance(Vila.CenterUnits, Onde));
		}
		return Menor;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGroundUseGraveyardMatchesTheChartTest,
	"BattleSquare.GroundUseGraveyard.MatchesTheChart",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGroundUseGraveyardMatchesTheChartTest::RunTest(const FString& Parameters)
{
	const UIslandBakedPlan* Assado = IslandBakedPlan::Load();
	if (!Assado)
	{
		AddError(TEXT("o assado nao existe — rode ./Tools/bake_island.sh"));
		return false;
	}

	// A CARTA diz sete cemitérios. Por extenso, como no sagrado: é o aceite
	// combinado, e um traçado que passasse a produzir outra quantidade tem de
	// reprovar aqui.
	TestEqual(TEXT("os 7 cemiterios da carta"),
		ProvaDoCemiterio::Quantos(*Assado, EGroundUse::Cemiterio), 7);

	// E existe ao menos um ESQUECIDO: zero deles faria toda a distinção abaixo
	// não ter sobre o que falar.
	TestTrue(TEXT("a ilha tem cemiterio esquecido"),
		ProvaDoCemiterio::Quantos(*Assado, EGroundUse::CemiterioEsquecido) > 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGroundUseGraveyardTheForgottenOneLooksDifferentTest,
	"BattleSquare.GroundUseGraveyard.TheForgottenOneLooksDifferent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGroundUseGraveyardTheForgottenOneLooksDifferentTest::RunTest(const FString& Parameters)
{
	UWorld* Mundo = ProvaDoCemiterio::MundoDeTeste();

	// Um é SERVIÇO, o outro é VESTÍGIO. Se saíssem iguais, o mapa mostraria
	// oito cemitérios e nenhuma história — e as contagens continuariam certas.
	FGroundUsePatch DaVila;
	DaVila.Use = EGroundUse::Cemiterio;
	DaVila.HalfExtentUnits = 500.0f;

	FGroundUsePatch Esquecido = DaVila;
	Esquecido.Use = EGroundUse::CemiterioEsquecido;

	AGroundUseActor* Servico = Mundo->SpawnActor<AGroundUseActor>();
	AGroundUseActor* Vestigio = Mundo->SpawnActor<AGroundUseActor>();
	Servico->ConfigureFor(DaVila);
	Vestigio->ConfigureFor(Esquecido);

	const bool bCorDiferente =
		Servico->GetBody()->GetMaterial(0) != Vestigio->GetBody()->GetMaterial(0);
	const bool bAlturaDiferente = !FMath::IsNearlyEqual(
		static_cast<float>(Servico->GetBody()->GetComponentScale().Z),
		static_cast<float>(Vestigio->GetBody()->GetComponentScale().Z));

	if (!bCorDiferente && !bAlturaDiferente)
	{
		AddError(TEXT("o cemiterio da vila e o esquecido saem IDENTICOS — o mapa ")
			TEXT("mostra oito cemiterios e nenhuma historia"));
		Mundo->DestroyWorld(false);
		return false;
	}

	Mundo->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGroundUseGraveyardTheForgottenOneHasNoVillageTest,
	"BattleSquare.GroundUseGraveyard.TheForgottenOneHasNoVillage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGroundUseGraveyardTheForgottenOneHasNoVillageTest::RunTest(const FString& Parameters)
{
	const UIslandBakedPlan* Assado = IslandBakedPlan::Load();
	if (!Assado)
	{
		AddError(TEXT("o assado nao existe — rode ./Tools/bake_island.sh"));
		return false;
	}

	// O ESQUECIDO é o que ficou para trás, SEM vila nenhuma em volta. É a única
	// coisa que o distingue de fundo — a forma é escolha de arte, isto é o que
	// ele É.
	//
	// A comparação é RELATIVA e sem dono eleito: o esquecido está mais longe da
	// vila mais próxima do que o cemitério de vila típico. Nenhuma distância
	// absoluta é escrita aqui — ela mudaria com o tamanho da ilha, que é a
	// armadilha que este projeto já mediu em sete lugares.
	TArray<float> DeVila;
	TArray<float> Esquecidos;

	for (const FBakedGroundUse& Mancha : Assado->GroundUses)
	{
		if (Mancha.Use == EGroundUse::Cemiterio)
		{
			DeVila.Add(ProvaDoCemiterio::AteAVilaMaisPerto(Mancha.CenterUnits));
		}
		else if (Mancha.Use == EGroundUse::CemiterioEsquecido)
		{
			Esquecidos.Add(ProvaDoCemiterio::AteAVilaMaisPerto(Mancha.CenterUnits));
		}
	}

	if (DeVila.Num() == 0 || Esquecidos.Num() == 0)
	{
		AddError(TEXT("faltam cemiterios de um dos dois tipos para comparar"));
		return false;
	}

	DeVila.Sort();
	const float MaisLongeQueUmDeVila = DeVila.Last();

	for (const float Distancia : Esquecidos)
	{
		if (Distancia <= MaisLongeQueUmDeVila)
		{
			AddError(FString::Printf(
				TEXT("um cemiterio esquecido esta a %.0f da vila mais perto, e o ")
				TEXT("cemiterio DE VILA mais afastado esta a %.0f — o esquecido ")
				TEXT("nao ficou para tras de ninguem"),
				Distancia, MaisLongeQueUmDeVila));
			return false;
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGroundUseGraveyardEveryVillageOneServesAVillageTest,
	"BattleSquare.GroundUseGraveyard.EveryVillageOneServesAVillage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGroundUseGraveyardEveryVillageOneServesAVillageTest::RunTest(const FString& Parameters)
{
	const UIslandBakedPlan* Assado = IslandBakedPlan::Load();
	if (!Assado)
	{
		AddError(TEXT("o assado nao existe — rode ./Tools/bake_island.sh"));
		return false;
	}

	// NA FORMA EXISTENCIAL: para cada cemitério de vila, EXISTE uma vila perto
	// o bastante. Não se elege qual — eleger a mais próxima foi o que fez este
	// teste reprovar desenho certo duas vezes, porque o cemitério de um posto
	// de fronteira cai mais perto da vila vizinha.
	//
	// "Perto o bastante" sai da própria distribuição: nenhum cemitério de vila
	// pode estar mais longe de TODA vila do que o cemitério esquecido está.
	float MaisPertoDoEsquecido = TNumericLimits<float>::Max();
	for (const FBakedGroundUse& Mancha : Assado->GroundUses)
	{
		if (Mancha.Use == EGroundUse::CemiterioEsquecido)
		{
			MaisPertoDoEsquecido = FMath::Min(MaisPertoDoEsquecido,
				ProvaDoCemiterio::AteAVilaMaisPerto(Mancha.CenterUnits));
		}
	}

	if (MaisPertoDoEsquecido == TNumericLimits<float>::Max())
	{
		AddError(TEXT("nao ha cemiterio esquecido para servir de regua"));
		return false;
	}

	int32 Conferidos = 0;
	for (const FBakedGroundUse& Mancha : Assado->GroundUses)
	{
		if (Mancha.Use != EGroundUse::Cemiterio)
		{
			continue;
		}

		++Conferidos;

		const float Ate = ProvaDoCemiterio::AteAVilaMaisPerto(Mancha.CenterUnits);
		if (Ate >= MaisPertoDoEsquecido)
		{
			AddError(FString::Printf(
				TEXT("um cemiterio DE VILA esta a %.0f de qualquer vila, mais longe ")
				TEXT("que o esquecido (%.0f) — ele nao serve vila nenhuma"),
				Ate, MaisPertoDoEsquecido));
			return false;
		}
	}

	TestTrue(TEXT("houve cemiterio de vila para conferir"), Conferidos > 0);

	return true;
}

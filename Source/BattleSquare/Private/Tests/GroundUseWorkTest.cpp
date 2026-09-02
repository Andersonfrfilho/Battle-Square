// Copyright 2026 Anderson. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "World/GroundUseActor.h"
#include "World/IslandBakedPlan.h"
#include "World/WaterFooting.h"

/**
 * O USO DO SOLO DE TRABALHO — fazenda, criadouro, pomar, loja, acampamento,
 * deck e poço: o que dá MOTIVO para andar até um lugar.
 *
 * O que se cobra aqui não é que eles apareçam (isso já tem prova), é que cada
 * um continue sendo o que é. Um deck longe da água e um poço seco desenhado
 * como cheio passam em contagem, em malha e em material — e o mapa promete uma
 * coisa que o mundo não tem.
 */

namespace ProvaDoTrabalho
{
	UWorld* MundoDeTeste()
	{
		return UWorld::CreateWorld(EWorldType::Game, false);
	}

	AGroundUseActor* Vestido(UWorld* Mundo, EGroundUse Uso, bool bDaAgua)
	{
		FGroundUsePatch Mancha;
		Mancha.Use = Uso;
		Mancha.HalfExtentUnits = 500.0f;
		Mancha.bYieldsWater = bDaAgua;

		AGroundUseActor* Ator = Mundo->SpawnActor<AGroundUseActor>();
		Ator->ConfigureFor(Mancha);
		return Ator;
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
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGroundUseWorkTheDryWellIsNotTheFullOneTest,
	"BattleSquare.GroundUseWork.TheDryWellIsNotTheFullOne",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGroundUseWorkTheDryWellIsNotTheFullOneTest::RunTest(const FString& Parameters)
{
	const UIslandBakedPlan* Assado = IslandBakedPlan::Load();
	if (!Assado)
	{
		AddError(TEXT("o assado nao existe — rode ./Tools/bake_island.sh"));
		return false;
	}

	UWorld* Mundo = ProvaDoTrabalho::MundoDeTeste();

	// O traçado guarda `bYieldsWater` e diz por quê: um poço seco desenhado
	// igual a um cheio é uma promessa que a carta não cumpre. Esta ilha tem
	// dez secos e dois cheios — todos iguais, o jogador atravessa a ilha atrás
	// de água que não existe, e nenhuma contagem erra um número.
	AGroundUseActor* Cheio = ProvaDoTrabalho::Vestido(Mundo, EGroundUse::Poco, true);
	AGroundUseActor* Seco = ProvaDoTrabalho::Vestido(Mundo, EGroundUse::Poco, false);

	TestTrue(TEXT("o poco cheio sabe que da agua"), Cheio->YieldsWater());
	TestFalse(TEXT("o poco seco sabe que nao da"), Seco->YieldsWater());

	if (Cheio->GetBody()->GetMaterial(0) == Seco->GetBody()->GetMaterial(0))
	{
		AddError(TEXT("o poco seco e o cheio tem a MESMA cor — a carta promete ")
			TEXT("agua onde nao ha"));
		Mundo->DestroyWorld(false);
		return false;
	}

	// E a ilha tem dos dois: só secos, ou só cheios, faria a distinção acima
	// não valer nada no mundo de verdade.
	int32 Secos = 0;
	int32 Cheios = 0;
	for (const FBakedGroundUse& Mancha : Assado->GroundUses)
	{
		if (Mancha.Use == EGroundUse::Poco)
		{
			Mancha.bYieldsWater ? ++Cheios : ++Secos;
		}
	}

	TestTrue(TEXT("a ilha tem poco cheio"), Cheios > 0);
	TestTrue(TEXT("a ilha tem poco seco"), Secos > 0);

	Mundo->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGroundUseWorkEveryWorkUseIsOnTheIslandTest,
	"BattleSquare.GroundUseWork.EveryWorkUseIsOnTheIsland",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGroundUseWorkEveryWorkUseIsOnTheIslandTest::RunTest(const FString& Parameters)
{
	const UIslandBakedPlan* Assado = IslandBakedPlan::Load();
	if (!Assado)
	{
		AddError(TEXT("o assado nao existe — rode ./Tools/bake_island.sh"));
		return false;
	}

	// Os sete usos de trabalho existem no mundo, cada um com pelo menos um
	// lugar. Um uso que o traçado deixou de produzir some do mapa em silêncio,
	// e a contagem total continua parecendo saudável.
	const EGroundUse DeTrabalho[] = {
		EGroundUse::Fazenda, EGroundUse::Criadouro, EGroundUse::Pomar,
		EGroundUse::PomarSelvagem, EGroundUse::Loja, EGroundUse::Acampamento,
		EGroundUse::Deck, EGroundUse::Poco
	};

	for (const EGroundUse Uso : DeTrabalho)
	{
		const int32 Quantos = ProvaDoTrabalho::Quantos(*Assado, Uso);
		if (Quantos == 0)
		{
			AddError(FString::Printf(
				TEXT("a ilha nao tem nenhum '%s' — o uso sumiu do mapa"),
				AGroundUseActor::UseDebugName(Uso)));
			return false;
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGroundUseWorkTheDeckMeetsTheWaterTest,
	"BattleSquare.GroundUseWork.TheDeckMeetsTheWater",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGroundUseWorkTheDeckMeetsTheWaterTest::RunTest(const FString& Parameters)
{
	const UIslandBakedPlan* Assado = IslandBakedPlan::Load();
	if (!Assado)
	{
		AddError(TEXT("o assado nao existe — rode ./Tools/bake_island.sh"));
		return false;
	}

	// UM DECK LONGE DA ÁGUA É UM TABLADO NO MATO. Ele passa em contagem, em
	// malha e em material, e o mapa mostra um ancoradouro onde não há o que
	// ancorar.
	//
	// Na forma EXISTENCIAL, sobre a MAIORIA: exigir de todo deck elegeria uma
	// distância exata como regra, e a distância certa é decisão do traçado.
	int32 Decks = 0;
	int32 NaAgua = 0;

	for (const FBakedGroundUse& Mancha : Assado->GroundUses)
	{
		if (Mancha.Use != EGroundUse::Deck)
		{
			continue;
		}

		++Decks;

		// Molhado no centro, ou a uma extensão dele: o deck fica NA beira, e a
		// beira é onde o seco encosta no molhado.
		if (WaterFooting::At(*Assado, Mancha.CenterUnits) != EWaterFooting::Seco)
		{
			++NaAgua;
			continue;
		}

		for (int32 Rumo = 0; Rumo < 8; ++Rumo)
		{
			const float Angulo = (2.0f * PI * Rumo) / 8.0f;
			const FVector2D AoLado = Mancha.CenterUnits
				+ FVector2D(FMath::Cos(Angulo), FMath::Sin(Angulo))
					* Mancha.HalfExtentUnits * 2.0f;

			if (WaterFooting::At(*Assado, AoLado) != EWaterFooting::Seco)
			{
				++NaAgua;
				break;
			}
		}
	}

	TestTrue(TEXT("a ilha tem decks"), Decks > 0);

	if (NaAgua * 2 <= Decks)
	{
		AddError(FString::Printf(
			TEXT("so %d de %d decks encostam na agua — os outros sao tablado no mato"),
			NaAgua, Decks));
		return false;
	}

	return true;
}

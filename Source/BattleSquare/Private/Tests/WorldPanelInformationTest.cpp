// Copyright 2026 Anderson. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "World/IslandBakedPlan.h"
#include "World/LandUseLayout.h"
#include "World/TrailLayout.h"
#include "World/WaterFooting.h"

/**
 * O QUE O PAINEL DO MUNDO TEM PARA DIZER.
 *
 * O painel em si não se testa — ele é desenho. O que se testa é a REGRA que
 * ele imprime, e é onde ela mora: no assado, nunca na tela (DP-ui-01).
 *
 * As três perguntas desta feature são invisíveis por natureza: fundura não tem
 * silhueta, ponte destruída de longe é uma ponte, e um segredo que ninguém
 * percebe ter achado é indistinguível de não existir.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorldPanelDepthIsAbsentOnDryLandTest,
	"BattleSquare.World.Painel.FunduraSomeEmTerraSeca",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldPanelDepthIsAbsentOnDryLandTest::RunTest(const FString& Parameters)
{
	const UIslandBakedPlan* Assado = IslandBakedPlan::Load();
	if (!Assado)
	{
		AddError(TEXT("o assado nao existe — rode ./Tools/bake_island.sh"));
		return false;
	}

	// AUSÊNCIA NÃO OCUPA LINHA. É o zero que autoriza o painel a não imprimir
	// nada — uma linha fixa dizendo "fundura: 0" em toda a ilha ensinaria a
	// ignorá-la justo onde ela passa a importar.
	FVector2D Seco = FVector2D::ZeroVector;
	bool bAchouSeco = false;

	for (const FBakedGroundUse& Mancha : Assado->GroundUses)
	{
		if (WaterFooting::At(*Assado, Mancha.CenterUnits) == EWaterFooting::Seco)
		{
			Seco = Mancha.CenterUnits;
			bAchouSeco = true;
			break;
		}
	}

	TestTrue(TEXT("ha terra seca para medir"), bAchouSeco);
	if (!bAchouSeco)
	{
		return true;
	}

	TestEqual(TEXT("em terra seca a fundura e zero, e o painel nao imprime"),
		WaterFooting::DepthUnitsAt(*Assado, Seco), 0.0f);

	// E ONDE HÁ ÁGUA FUNDA, HÁ NÚMERO. Sem esta metade, uma função que
	// devolvesse zero sempre passaria no teste de cima.
	FVector2D Molhado = FVector2D::ZeroVector;
	bool bAchouMolhado = false;

	for (const FBakedRiver& Curso : Assado->Rivers)
	{
		for (const FVector2D& Ponto : Curso.PointsUnits)
		{
			if (WaterFooting::DepthUnitsAt(*Assado, Ponto) > 0.0f)
			{
				Molhado = Ponto;
				bAchouMolhado = true;
				break;
			}
		}

		if (bAchouMolhado)
		{
			break;
		}
	}

	TestTrue(TEXT("ha agua com fundura conhecida"), bAchouMolhado);
	if (!bAchouMolhado)
	{
		return true;
	}

	// E A FUNDURA CONCORDA COM O PÉ, porque as duas saem da mesma medição.
	// Se discordassem, o painel diria uma fundura em que ninguém afunda.
	const float Fundura = WaterFooting::DepthUnitsAt(*Assado, Molhado);
	const float Cintura =
		WaterFooting::WaistDepthUnitsFor(WaterFooting::DefaultHeightUnits());
	const EWaterFooting Chao = WaterFooting::At(*Assado, Molhado);

	TestEqual(TEXT("passou da cintura e nado; abaixo dela, anda"),
		static_cast<int32>(Chao),
		static_cast<int32>(Fundura <= Cintura ? EWaterFooting::Vau : EWaterFooting::Fundo));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorldPanelNamesTheCrossingTest,
	"BattleSquare.World.Painel.ATravessiaSeAnuncia",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldPanelNamesTheCrossingTest::RunTest(const FString& Parameters)
{
	const UIslandBakedPlan* Assado = IslandBakedPlan::Load();
	if (!Assado)
	{
		AddError(TEXT("o assado nao existe — rode ./Tools/bake_island.sh"));
		return false;
	}

	TestTrue(TEXT("ha travessia para achar"), Assado->Crossings.Num() > 0);
	if (Assado->Crossings.Num() == 0)
	{
		return true;
	}

	// EM CIMA DELA, ELA APARECE.
	for (int32 Qual = 0; Qual < Assado->Crossings.Num(); ++Qual)
	{
		TestTrue(TEXT("quem esta na travessia acha alguma travessia"),
			Assado->CrossingAt(Assado->Crossings[Qual].CenterUnits) != INDEX_NONE);
	}

	// E LONGE DELA, NÃO. Sem esta metade, uma função que devolvesse sempre a
	// primeira travessia passaria — e o painel diria "ponte" na ilha inteira.
	TestEqual(TEXT("longe de tudo nao ha travessia"),
		Assado->CrossingAt(FVector2D(0.0f, 0.0f)), INDEX_NONE);

	// E O MATERIAL TEM NOME, inclusive o da destruída — que é a que precisa
	// dizer o que é antes de alguém tentar passar.
	TestEqual(TEXT("o bloco se chama bloco"),
		FString(TrailLayout::MaterialDebugName(TrailLayout::EBridgeMaterial::Bloco)),
		FString(TEXT("bloco")));
	TestEqual(TEXT("a madeira se chama madeira"),
		FString(TrailLayout::MaterialDebugName(TrailLayout::EBridgeMaterial::Madeira)),
		FString(TEXT("madeira")));
	TestEqual(TEXT("e a destruida se chama destruida"),
		FString(TrailLayout::MaterialDebugName(TrailLayout::EBridgeMaterial::Destruida)),
		FString(TEXT("destruida")));

	// E A DESTRUÍDA NÃO PASSA — afirmado como PROPRIEDADE, sobre uma travessia
	// montada à mão. Procurar uma no mundo mediria o sorteio em vez da regra:
	// com 30% e três candidatas, nenhuma ruína acontece em 34% das vezes.
	FBakedCrossing Quebrada;
	Quebrada.Kind = static_cast<uint8>(TrailLayout::ECrossingKind::Ponte);
	Quebrada.BridgeMaterial = static_cast<uint8>(TrailLayout::EBridgeMaterial::Destruida);
	TestFalse(TEXT("a ponte destruida nao deixa passar"), Quebrada.CanBeCrossed());

	FBakedCrossing Inteira;
	Inteira.Kind = static_cast<uint8>(TrailLayout::ECrossingKind::Ponte);
	Inteira.BridgeMaterial = static_cast<uint8>(TrailLayout::EBridgeMaterial::Madeira);
	TestTrue(TEXT("a de madeira deixa"), Inteira.CanBeCrossed());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorldPanelRevealsTheHiddenByWalkingTest,
	"BattleSquare.World.Painel.OEscondidoSeRevelaAndando",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldPanelRevealsTheHiddenByWalkingTest::RunTest(const FString& Parameters)
{
	const UIslandBakedPlan* Assado = IslandBakedPlan::Load();
	if (!Assado)
	{
		AddError(TEXT("o assado nao existe — rode ./Tools/bake_island.sh"));
		return false;
	}

	// A CARTA CONTA E NÃO APONTA; O MUNDO REVELA ANDANDO. Este teste é o outro
	// lado daquele trato: chegar até a mancha tem de dizer alguma coisa, senão
	// achar um segredo é indistinguível de não achar nada.
	int32 Escondidas = 0;

	for (int32 Qual = 0; Qual < Assado->GroundUses.Num(); ++Qual)
	{
		const FBakedGroundUse& Mancha = Assado->GroundUses[Qual];

		if (!Mancha.bHidden)
		{
			// E A MANCHA MOSTRADA NÃO É "ACHADO". Sem esta metade, o painel
			// anunciaria uma descoberta em cima de cada fazenda.
			TestEqual(TEXT("mancha mostrada nao e achado"),
				Assado->HiddenAt(Mancha.CenterUnits), INDEX_NONE);
			continue;
		}

		++Escondidas;
		TestTrue(TEXT("quem chega na mancha escondida acha alguma"),
			Assado->HiddenAt(Mancha.CenterUnits) != INDEX_NONE);
	}

	TestTrue(TEXT("ha mancha escondida para achar"), Escondidas > 0);

	return true;
}

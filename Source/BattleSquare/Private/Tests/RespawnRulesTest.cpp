// Copyright 2026 Anderson. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "World/RespawnRules.h"
#include "World/VillageLayout.h"

/**
 * ONDE SE ACORDA — decisão 60, e a fresta que ela fecha.
 *
 * Se acordar levasse à vila natal, morrer seria viagem rápida de graça — o que
 * a decisão 17 proíbe, pela porta dos fundos. Por isso acorda-se no MAIS PERTO
 * de onde se caiu: renascer nunca poupa caminhada.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRespawnWakesAtARecoveryCenterTest,
	"BattleSquare.World.Renascer.AcordaNumCentroDeRecuperacao",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRespawnWakesAtARecoveryCenterTest::RunTest(const FString&)
{
	// A PROPRIEDADE, na forma existencial (regra 12 da geração procedural):
	// a vila eleita TEM o prédio, e nenhuma vila COM o prédio está mais perto.
	// Não se reconstrói o desempate — afirma-se o que "mais perto" garante.
	const FVector2D CaiuAqui(35000.0f, -20000.0f);

	RespawnRules::FWakeSpot Onde;
	TestTrue(TEXT("ha onde acordar nesta regiao"),
		RespawnRules::NearestRecoveryCenter(CaiuAqui, Onde));

	TestTrue(TEXT("a vila eleita TEM Centro de Recuperacao"),
		VillageLayout::HasBuilding(VillageLayout::PlanFor(Onde.Kind),
			EVillageBuilding::CentroDeRecuperacao));

	const float Eleita = FVector2D::Distance(CaiuAqui, Onde.VillageCenterUnits);
	for (const FSettlementPlacement& Vila : RegionLayout::Plan())
	{
		if (!VillageLayout::HasBuilding(VillageLayout::PlanFor(Vila.Kind),
			EVillageBuilding::CentroDeRecuperacao))
		{
			continue;
		}

		TestTrue(TEXT("nenhuma vila com hospital esta mais perto que a eleita"),
			Eleita <= FVector2D::Distance(CaiuAqui, Vila.CenterUnits) + 1.0f);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRespawnWakesAtTheDoorNotInsideTest,
	"BattleSquare.World.Renascer.NaPortaENuncaDentro",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRespawnWakesAtTheDoorNotInsideTest::RunTest(const FString&)
{
	// ACORDAR DENTRO DO PRÉDIO É NASCER EMPAREDADO: o prédio bloqueia, e é o
	// que faz a vila ser um lugar. O ponto de acordar fica FORA da pegada da
	// parede — e dentro da calçada da porta, para o painel anunciar sozinho
	// onde se está.
	RespawnRules::FWakeSpot Onde;
	if (!RespawnRules::NearestRecoveryCenter(FVector2D::ZeroVector, Onde))
	{
		AddError(TEXT("regiao sem hospital nenhum — o mundo mudou de historia"));
		return true;
	}

	const FVillagePlacement* Centro =
		VillageLayout::PlanFor(Onde.Kind).FindByPredicate(
			[](const FVillagePlacement& Peca)
			{
				return Peca.Building == EVillageBuilding::CentroDeRecuperacao;
			});
	TestNotNull(TEXT("o Centro existe no tracado da vila eleita"), Centro);
	if (!Centro)
	{
		return true;
	}

	const FVector2D NoPredio =
		Onde.WakeUnits - (Onde.VillageCenterUnits + Centro->OffsetUnits);

	TestTrue(TEXT("acorda FORA da pegada da parede"),
		FMath::Abs(NoPredio.X) > Centro->HalfExtentUnits.X
			|| FMath::Abs(NoPredio.Y) > Centro->HalfExtentUnits.Y);

	// E perto o bastante para estar na CALÇADA (a porta da CI2 tem 160 de
	// faixa): é a porta quem anuncia "entrou: Centro de Recuperacao" ao
	// acordar, sem texto novo.
	TestTrue(TEXT("e dentro da faixa da porta"),
		FMath::Abs(NoPredio.X) <= Centro->HalfExtentUnits.X + 160.0f
			&& FMath::Abs(NoPredio.Y) <= Centro->HalfExtentUnits.Y + 160.0f);

	return true;
}

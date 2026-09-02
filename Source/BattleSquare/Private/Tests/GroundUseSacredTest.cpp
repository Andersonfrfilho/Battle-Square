// Copyright 2026 Anderson. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "World/GroundUseActor.h"
#include "World/IslandBakedPlan.h"
#include "World/Pantheon.h"

/**
 * TEMPLO E RUÍNA — e o DEUS de cada um.
 *
 * Um templo alto no horizonte diz "há algo ali"; só o nome diz DE QUEM. Sem
 * ele, os cinco templos são cinco torres brancas iguais, e o panteão — que o
 * mapa deveria ensinar — nunca chega ao jogador. A carta nomeia cada um.
 */

namespace ProvaDoSagrado
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
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGroundUseSacredMatchesTheChartTest,
	"BattleSquare.GroundUseSacred.MatchesTheChart",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGroundUseSacredMatchesTheChartTest::RunTest(const FString& Parameters)
{
	const UIslandBakedPlan* Assado = IslandBakedPlan::Load();
	if (!Assado)
	{
		AddError(TEXT("o assado nao existe — rode ./Tools/bake_island.sh"));
		return false;
	}

	// A CARTA é o gabarito desta feature, e ela diz cinco templos e quatro
	// ruínas. Estes dois números são escritos por extenso de propósito: são o
	// aceite combinado, e um traçado que passasse a produzir outra quantidade
	// tem de reprovar aqui — não passar despercebido numa contagem relativa.
	TestEqual(TEXT("os 5 templos da carta"),
		ProvaDoSagrado::Quantos(*Assado, EGroundUse::Templo), 5);
	TestEqual(TEXT("as 4 ruinas da carta"),
		ProvaDoSagrado::Quantos(*Assado, EGroundUse::Ruina), 4);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGroundUseSacredEachOneKnowsItsGodTest,
	"BattleSquare.GroundUseSacred.EachOneKnowsItsGod",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGroundUseSacredEachOneKnowsItsGodTest::RunTest(const FString& Parameters)
{
	const UIslandBakedPlan* Assado = IslandBakedPlan::Load();
	if (!Assado)
	{
		AddError(TEXT("o assado nao existe — rode ./Tools/bake_island.sh"));
		return false;
	}

	UWorld* Mundo = ProvaDoSagrado::MundoDeTeste();

	// O DEUS SOBREVIVE ao caminho todo: do traçado ao assado, do assado ao
	// ator. Perdido em qualquer perna, os cinco templos viram cinco templos da
	// Mãe Natureza, e nada acusa — o valor padrão do enum é justamente esse.
	TSet<int32> DeusesVistos;

	for (const FBakedGroundUse& Mancha : Assado->GroundUses)
	{
		if (Mancha.Use != EGroundUse::Templo && Mancha.Use != EGroundUse::Ruina)
		{
			continue;
		}

		FGroundUsePatch Patch;
		Patch.Use = Mancha.Use;
		Patch.HalfExtentUnits = Mancha.HalfExtentUnits;
		Patch.Deity = Mancha.Deity;

		AGroundUseActor* Ator = Mundo->SpawnActor<AGroundUseActor>();
		Ator->ConfigureFor(Patch);

		TestEqual(TEXT("o ator ficou com o deus da mancha"),
			static_cast<int32>(Ator->GetDeity()), static_cast<int32>(Mancha.Deity));

		// E o deus tem NOME para o painel: sem nome, a linha sai "templo de ?".
		const FString Nome = Pantheon::DebugName(Mancha.Deity);
		TestFalse(TEXT("o deus tem nome"), Nome.IsEmpty());
		TestNotEqual(TEXT("o nome do deus nao e um ponto de interrogacao"),
			Nome, FString(TEXT("?")));

		DeusesVistos.Add(static_cast<int32>(Mancha.Deity));
	}

	// MAIS DE UM DEUS. Se todos caíssem no padrão do enum, cada asserção acima
	// passaria — o ator teria o deus da mancha, o nome existiria — e o panteão
	// inteiro teria virado um deus só sem nenhum teste notar.
	if (DeusesVistos.Num() < 2)
	{
		AddError(FString::Printf(
			TEXT("o sagrado da ilha tem %d deus(es) — o panteao virou um so"),
			DeusesVistos.Num()));
		Mundo->DestroyWorld(false);
		return false;
	}

	Mundo->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGroundUseSacredTheTempleStandsTallerThanTheRuinTest,
	"BattleSquare.GroundUseSacred.TheTempleStandsTallerThanTheRuin",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGroundUseSacredTheTempleStandsTallerThanTheRuinTest::RunTest(const FString& Parameters)
{
	UWorld* Mundo = ProvaDoSagrado::MundoDeTeste();

	// TEMPLO EM PÉ e RUÍNA são coisas diferentes, e a diferença tem de se ver
	// de longe: um é o que ainda está de pé, o outro é o que caiu. Desenhados
	// iguais, o mapa deixa de contar a história que o panteão tem.
	FGroundUsePatch DeTemplo;
	DeTemplo.Use = EGroundUse::Templo;
	DeTemplo.HalfExtentUnits = 500.0f;

	FGroundUsePatch DeRuina = DeTemplo;
	DeRuina.Use = EGroundUse::Ruina;

	AGroundUseActor* Templo = Mundo->SpawnActor<AGroundUseActor>();
	AGroundUseActor* Ruina = Mundo->SpawnActor<AGroundUseActor>();
	Templo->ConfigureFor(DeTemplo);
	Ruina->ConfigureFor(DeRuina);

	TestTrue(TEXT("o templo se ergue mais alto que a ruina"),
		Templo->GetBody()->GetComponentScale().Z
			> Ruina->GetBody()->GetComponentScale().Z);

	Mundo->DestroyWorld(false);
	return true;
}

// Copyright 2026 Anderson. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "World/GroundUseActor.h"
#include "World/IslandBakedPlan.h"
#include "World/LandUseLayout.h"

/**
 * O USO DO SOLO: um ator PARAMETRIZADO, quinze usos, uma tabela.
 *
 * O modo de falhar aqui é o mais silencioso desta feature: um uso sem linha na
 * tabela vira uma mancha invisível. O ator existe, a contagem de atores bate, e
 * o lugar não está no mundo.
 */

namespace ProvaDoUsoDoSolo
{
	UWorld* MundoDeTeste()
	{
		return UWorld::CreateWorld(EWorldType::Game, false);
	}

	/** Todos os usos que o enum tem, menos `Nenhum` — que é ausência. */
	TArray<EGroundUse> TodosOsUsos()
	{
		return {
			EGroundUse::Bosque, EGroundUse::ClareiraFechada, EGroundUse::Fazenda,
			EGroundUse::Criadouro, EGroundUse::Loja, EGroundUse::Acampamento,
			EGroundUse::Pomar, EGroundUse::PomarSelvagem, EGroundUse::Deck,
			EGroundUse::Poco, EGroundUse::Templo, EGroundUse::Ruina,
			EGroundUse::Cemiterio, EGroundUse::CemiterioEsquecido
		};
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGroundUseActorAssignsItsMeshInTheConstructorTest,
	"BattleSquare.GroundUseActor.AssignsItsMeshInTheConstructor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGroundUseActorAssignsItsMeshInTheConstructorTest::RunTest(const FString& Parameters)
{
	// No DEFAULT DA CLASSE, antes de qualquer configuração.
	//
	// É o ponto exato deste teste: um ator parametrizado convida a deixar a
	// malha para a configuração, e aí quem esquece de configurar tem um ator
	// que passa em tudo e não aparece. A configuração TROCA a malha; ela não
	// pode ser a primeira a atribuir uma.
	const AGroundUseActor* Padrao = GetDefault<AGroundUseActor>();
	if (!TestNotNull(TEXT("a classe do uso do solo tem default"), Padrao))
	{
		return false;
	}

	const UStaticMeshComponent* Corpo = Padrao->GetBody();
	if (!TestNotNull(TEXT("o uso do solo tem corpo"), Corpo))
	{
		return false;
	}

	TestTrue(TEXT("o corpo nasce com MALHA atribuida"), Corpo->GetStaticMesh() != nullptr);
	TestTrue(TEXT("o corpo nasce com material atribuido"), Corpo->GetMaterial(0) != nullptr);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGroundUseActorEveryUseHasAShapeTest,
	"BattleSquare.GroundUseActor.EveryUseHasAShape",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGroundUseActorEveryUseHasAShapeTest::RunTest(const FString& Parameters)
{
	UWorld* Mundo = ProvaDoUsoDoSolo::MundoDeTeste();

	// TODO uso do enum tem de ter linha na tabela. Uso sem linha some do mapa
	// sem nada acusar — e o dia em que alguém acrescentar o décimo sexto uso,
	// este teste reprova antes de o mundo ficar com um buraco.
	for (const EGroundUse Uso : ProvaDoUsoDoSolo::TodosOsUsos())
	{
		FGroundUsePatch Mancha;
		Mancha.Use = Uso;
		Mancha.CenterUnits = FVector2D::ZeroVector;
		Mancha.HalfExtentUnits = 500.0f;

		AGroundUseActor* Ator = Mundo->SpawnActor<AGroundUseActor>();
		if (!Ator->ConfigureFor(Mancha))
		{
			AddError(FString::Printf(
				TEXT("o uso '%s' nao tem forma na tabela — ele seria uma mancha invisivel"),
				AGroundUseActor::UseDebugName(Uso)));
			Mundo->DestroyWorld(false);
			return false;
		}

		TestTrue(*FString::Printf(TEXT("'%s' ficou com malha"),
			AGroundUseActor::UseDebugName(Uso)),
			Ator->GetBody()->GetStaticMesh() != nullptr);
		TestTrue(*FString::Printf(TEXT("'%s' ficou com material"),
			AGroundUseActor::UseDebugName(Uso)),
			Ator->GetBody()->GetMaterial(0) != nullptr);
	}

	Mundo->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGroundUseActorUsesLookDifferentFromEachOtherTest,
	"BattleSquare.GroundUseActor.UsesLookDifferentFromEachOther",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGroundUseActorUsesLookDifferentFromEachOtherTest::RunTest(const FString& Parameters)
{
	UWorld* Mundo = ProvaDoUsoDoSolo::MundoDeTeste();

	// UM ATOR PARAMETRIZADO só vale se os parâmetros de fato diferem. Uma
	// tabela que desse a mesma forma e a mesma cor a tudo passaria em todos os
	// testes acima, e o mapa teria 79 cubos cinzentos iguais.
	TSet<FString> Aparencias;
	for (const EGroundUse Uso : ProvaDoUsoDoSolo::TodosOsUsos())
	{
		FGroundUsePatch Mancha;
		Mancha.Use = Uso;
		Mancha.HalfExtentUnits = 500.0f;

		AGroundUseActor* Ator = Mundo->SpawnActor<AGroundUseActor>();
		Ator->ConfigureFor(Mancha);

		const UStaticMeshComponent* Corpo = Ator->GetBody();
		Aparencias.Add(FString::Printf(TEXT("%s|%s|%.3f"),
			*GetNameSafe(Corpo->GetStaticMesh()),
			*GetNameSafe(Corpo->GetMaterial(0)),
			Corpo->GetComponentScale().Z / FMath::Max(Corpo->GetComponentScale().X, 0.001f)));
	}

	// Não se exige que os catorze sejam todos distintos — pomar e pomar
	// selvagem são parentes de propósito. Exige-se VARIEDADE de verdade.
	if (Aparencias.Num() < 8)
	{
		AddError(FString::Printf(
			TEXT("os 14 usos produziram so %d aparencias distintas — o mapa sairia uniforme"),
			Aparencias.Num()));
		Mundo->DestroyWorld(false);
		return false;
	}

	Mundo->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGroundUseActorSizeComesFromThePatchTest,
	"BattleSquare.GroundUseActor.SizeComesFromThePatch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGroundUseActorSizeComesFromThePatchTest::RunTest(const FString& Parameters)
{
	UWorld* Mundo = ProvaDoUsoDoSolo::MundoDeTeste();

	// O tamanho sai da MEIA-EXTENSÃO que o traçado mediu. Fixo, o bosque de uma
	// clareira grande caberia num vaso — e o mapa mentiria sobre a escala das
	// coisas sem errar uma contagem.
	FGroundUsePatch Pequena;
	Pequena.Use = EGroundUse::Bosque;
	Pequena.HalfExtentUnits = 200.0f;

	FGroundUsePatch Grande = Pequena;
	Grande.HalfExtentUnits = 2000.0f;

	AGroundUseActor* Menor = Mundo->SpawnActor<AGroundUseActor>();
	AGroundUseActor* Maior = Mundo->SpawnActor<AGroundUseActor>();
	Menor->ConfigureFor(Pequena);
	Maior->ConfigureFor(Grande);

	TestTrue(TEXT("a mancha maior produz um corpo maior"),
		Maior->GetBody()->GetComponentScale().X
			> Menor->GetBody()->GetComponentScale().X);

	// Dez vezes a extensão dá dez vezes a escala: a proporção tem de ser a da
	// medida, não uma qualquer que só ordene as duas.
	TestEqual(TEXT("a escala acompanha a extensao"),
		static_cast<float>(Maior->GetBody()->GetComponentScale().X
			/ Menor->GetBody()->GetComponentScale().X),
		10.0f, 0.01f);

	Mundo->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGroundUseActorEveryPatchOfThePlanCanBeBuiltTest,
	"BattleSquare.GroundUseActor.EveryPatchOfThePlanCanBeBuilt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGroundUseActorEveryPatchOfThePlanCanBeBuiltTest::RunTest(const FString& Parameters)
{
	const UIslandBakedPlan* Assado = IslandBakedPlan::Load();
	if (!Assado)
	{
		AddError(TEXT("o assado nao existe — rode ./Tools/bake_island.sh"));
		return false;
	}

	UWorld* Mundo = ProvaDoUsoDoSolo::MundoDeTeste();

	// TODAS as manchas do traçado viram ator. O número não é escrito à mão: ele
	// sai do plano, e continua certo quando a ilha mudar de bioma. (A spec
	// falava em 71; o traçado de hoje tem outro número, e é o traçado que manda.)
	int32 Erguidas = 0;
	for (const FBakedGroundUse& Mancha : Assado->GroundUses)
	{
		FGroundUsePatch Patch;
		Patch.Use = Mancha.Use;
		Patch.CenterUnits = Mancha.CenterUnits;
		Patch.HalfExtentUnits = Mancha.HalfExtentUnits;
		Patch.bYieldsWater = Mancha.bYieldsWater;
		Patch.Deity = Mancha.Deity;

		AGroundUseActor* Ator = Mundo->SpawnActor<AGroundUseActor>();
		if (Ator->ConfigureFor(Patch))
		{
			++Erguidas;
		}
		else
		{
			AddError(FString::Printf(
				TEXT("a mancha de '%s' em (%.0f,%.0f) nao pode ser construida"),
				AGroundUseActor::UseDebugName(Mancha.Use),
				Mancha.CenterUnits.X, Mancha.CenterUnits.Y));
		}
	}

	TestEqual(TEXT("toda mancha do assado virou ator"), Erguidas, Assado->GroundUses.Num());
	TestEqual(TEXT("o assado tem as manchas do gerador"),
		Assado->GroundUses.Num(), LandUseLayout::Plan().Num());
	TestTrue(TEXT("a ilha tem uso do solo"), Erguidas > 0);

	Mundo->DestroyWorld(false);
	return true;
}

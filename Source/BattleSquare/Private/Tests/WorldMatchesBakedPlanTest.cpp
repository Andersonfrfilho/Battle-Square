// Copyright 2026 Anderson. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Engine/World.h"
#include "World/AqueductMesh.h"
#include "World/CrossingMesh.h"
#include "World/GroundUseActor.h"
#include "World/IslandBakedPlan.h"
#include "World/RiverMesh.h"
#include "World/TerrainMesh.h"
#include "World/TrailMesh.h"

/**
 * O MUNDO CONSTRUÍDO CONTRA O ASSADO, elemento a elemento.
 *
 * Este é o teste que impede a construção de perder peças EM SILÊNCIO — que é o
 * modo de falhar desta feature inteira. Cada prova das fases anteriores olha
 * uma coisa: os rios, as trilhas, as travessias. Nenhuma delas nota quando um
 * ator inteiro deixa de ser construído, porque cada uma só sabe do seu.
 *
 * ## Ele diz TUDO o que faltou, não a primeira coisa
 *
 * Reprovar no primeiro item faria a pessoa consertar, rodar de novo, bater no
 * segundo, e voltar — uma rodada por peça perdida. A lista sai inteira, com
 * o número esperado e o encontrado ao lado: só o nome não distingue "faltou
 * um" de "faltaram todos", e essa diferença é a que diz onde procurar.
 */

namespace ConferenciaDoMundo
{
	UWorld* MundoDeTeste()
	{
		return UWorld::CreateWorld(EWorldType::Game, false);
	}

	/** Uma divergência entre o que o assado tem e o que o mundo ergueu. */
	struct FFalta
	{
		FString OQue;
		int32 NoAssado = 0;
		int32 NoMundo = 0;
	};

	void Conferir(TArray<FFalta>& Faltas, const TCHAR* OQue, int32 NoAssado, int32 NoMundo)
	{
		if (NoAssado != NoMundo)
		{
			Faltas.Add({ OQue, NoAssado, NoMundo });
		}
	}

	/**
	 * A lista INTEIRA, de uma vez, com esperado e encontrado ao lado.
	 *
	 * Sai daqui, e não do corpo do teste, para que a prova da MENSAGEM possa
	 * chamá-la. Um caminho de falha que nunca falha em teste nenhum é código
	 * sem prova, e este é justamente o caminho que a feature inteira depende
	 * que funcione no dia em que uma peça sumir.
	 */
	FString Descrever(const TArray<FFalta>& Faltas)
	{
		FString Recado = TEXT("o mundo construido nao bate com o assado:");
		for (const FFalta& Falta : Faltas)
		{
			Recado += FString::Printf(TEXT("\n  %s: o assado tem %d, o mundo ergueu %d"),
				*Falta.OQue, Falta.NoAssado, Falta.NoMundo);
		}
		return Recado;
	}

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
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorldMatchesBakedPlanTest,
	"BattleSquare.WorldMatchesBakedPlan.NothingIsMissing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldMatchesBakedPlanTest::RunTest(const FString& Parameters)
{
	const UIslandBakedPlan* Assado = IslandBakedPlan::Load();
	if (!Assado)
	{
		AddError(TEXT("o assado nao existe — rode ./Tools/bake_island.sh"));
		return false;
	}

	UWorld* Mundo = ConferenciaDoMundo::MundoDeTeste();
	TArray<ConferenciaDoMundo::FFalta> Faltas;

	// O RELEVO: um vértice por casa da grade. Chão faltando é buraco no mundo.
	ATerrainMesh* Relevo = Mundo->SpawnActor<ATerrainMesh>();
	const int32 Vertices = Relevo->BuildFrom(*Assado);
	ConferenciaDoMundo::Conferir(Faltas, TEXT("vertices do relevo"),
		Assado->HeightGridSide * Assado->HeightGridSide, Vertices);

	// A ÁGUA: cursos, lagos, poços de queda, córregos e fontes — cada um
	// contado à parte, porque cada um some à parte.
	ARiverMesh* Agua = Mundo->SpawnActor<ARiverMesh>();
	Agua->BuildFrom(*Assado);

	int32 LagosNoAssado = 0;
	int32 PocosNoAssado = 0;
	for (const FBakedRiver& Curso : Assado->Rivers)
	{
		if (Curso.bHasLake)
		{
			++LagosNoAssado;
		}
		if (Curso.bHasFall && Curso.PlungePoolHalfWidthUnits > 0.0f)
		{
			++PocosNoAssado;
		}
	}

	ConferenciaDoMundo::Conferir(Faltas, TEXT("cursos d'agua"),
		Assado->Rivers.Num(), Agua->GetBuiltCourseCount());
	ConferenciaDoMundo::Conferir(Faltas, TEXT("lagos"),
		LagosNoAssado, Agua->GetBuiltLakeCount());
	ConferenciaDoMundo::Conferir(Faltas, TEXT("pocos de queda"),
		PocosNoAssado, Agua->GetBuiltPlungePoolCount());
	ConferenciaDoMundo::Conferir(Faltas, TEXT("corregos"),
		Assado->Brooks.Num(), Agua->GetBuiltBrookCount());
	ConferenciaDoMundo::Conferir(Faltas, TEXT("fontes"),
		Assado->Springs.Num(), Agua->GetBuiltSpringCount());

	// AS TRILHAS.
	ATrailMesh* Trilhas = Mundo->SpawnActor<ATrailMesh>();
	ConferenciaDoMundo::Conferir(Faltas, TEXT("trilhas"),
		Assado->Trails.Num(), Trilhas->BuildFrom(*Assado));

	// AS TRAVESSIAS: conferidas pelas VISTAS, não pelas construídas. O vau não
	// constrói nada de propósito, e contar só as obras esconderia uma travessia
	// perdida atrás dele.
	ACrossingMesh* Travessias = Mundo->SpawnActor<ACrossingMesh>();
	Travessias->BuildFrom(*Assado);
	ConferenciaDoMundo::Conferir(Faltas, TEXT("travessias"),
		Assado->Crossings.Num(), Travessias->GetSeenTotal());

	// OS AQUEDUTOS.
	AAqueductMesh* Aquedutos = Mundo->SpawnActor<AAqueductMesh>();
	ConferenciaDoMundo::Conferir(Faltas, TEXT("aquedutos"),
		Assado->Aqueducts.Num(), Aquedutos->BuildFrom(*Assado));

	// O USO DO SOLO, uso POR uso. Um total certo com a distribuição errada —
	// dez fazendas a mais e dez templos a menos — passaria numa conta só, e o
	// mapa perderia o panteão inteiro sem mudar um número.
	const EGroundUse TodosOsUsos[] = {
		EGroundUse::Bosque, EGroundUse::ClareiraFechada, EGroundUse::Fazenda,
		EGroundUse::Criadouro, EGroundUse::Loja, EGroundUse::Acampamento,
		EGroundUse::Pomar, EGroundUse::PomarSelvagem, EGroundUse::Deck,
		EGroundUse::Poco, EGroundUse::Templo, EGroundUse::Ruina,
		EGroundUse::Cemiterio, EGroundUse::CemiterioEsquecido
	};

	for (const EGroundUse Uso : TodosOsUsos)
	{
		const int32 NoAssado = ConferenciaDoMundo::ManchasDe(*Assado, Uso);

		int32 NoMundo = 0;
		for (const FBakedGroundUse& Mancha : Assado->GroundUses)
		{
			if (Mancha.Use != Uso)
			{
				continue;
			}

			FGroundUsePatch Patch;
			Patch.Use = Mancha.Use;
			Patch.CenterUnits = Mancha.CenterUnits;
			Patch.HalfExtentUnits = Mancha.HalfExtentUnits;
			Patch.bYieldsWater = Mancha.bYieldsWater;
			Patch.Deity = Mancha.Deity;

			AGroundUseActor* Ator = Mundo->SpawnActor<AGroundUseActor>();
			if (Ator->ConfigureFor(Patch))
			{
				++NoMundo;
			}
		}

		ConferenciaDoMundo::Conferir(Faltas,
			AGroundUseActor::UseDebugName(Uso), NoAssado, NoMundo);
	}

	if (Faltas.Num() > 0)
	{
		AddError(ConferenciaDoMundo::Descrever(Faltas));
		Mundo->DestroyWorld(false);
		return false;
	}

	// A CONFERÊNCIA TEM DE TER CONFERIDO ALGO. Um assado vazio bateria com um
	// mundo vazio, e este teste passaria com a ilha inteira ausente — que é
	// exatamente o silêncio que ele existe para quebrar.
	TestTrue(TEXT("o assado tem relevo"), Assado->GroundHeightUnits.Num() > 0);
	TestTrue(TEXT("o assado tem cursos d'agua"), Assado->Rivers.Num() > 0);
	TestTrue(TEXT("o assado tem trilhas"), Assado->Trails.Num() > 0);
	TestTrue(TEXT("o assado tem travessias"), Assado->Crossings.Num() > 0);
	TestTrue(TEXT("o assado tem aquedutos"), Assado->Aqueducts.Num() > 0);
	TestTrue(TEXT("o assado tem uso do solo"), Assado->GroundUses.Num() > 0);

	Mundo->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorldMatchesBakedPlanNamesWhatIsMissingTest,
	"BattleSquare.WorldMatchesBakedPlan.NamesWhatIsMissing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldMatchesBakedPlanNamesWhatIsMissingTest::RunTest(const FString& Parameters)
{
	// A CONFERÊNCIA ACIMA PASSA — e é isso que torna esta prova necessária.
	//
	// Enquanto o mundo estiver inteiro, o caminho de falha nunca roda: a
	// mensagem que deveria nomear as peças perdidas é código sem prova, e ela é
	// justamente a coisa de que a feature depende no dia em que uma sumir.
	//
	// Aqui se alimenta a comparação com um mundo deliberadamente incompleto.
	TArray<ConferenciaDoMundo::FFalta> Faltas;
	ConferenciaDoMundo::Conferir(Faltas, TEXT("cursos d'agua"), 137, 130);
	ConferenciaDoMundo::Conferir(Faltas, TEXT("templo"), 5, 0);
	ConferenciaDoMundo::Conferir(Faltas, TEXT("trilhas"), 23, 23);

	// Só as que de fato divergem entram: o que bate não vira ruído na lista.
	TestEqual(TEXT("duas pecas divergiram"), Faltas.Num(), 2);

	const FString Recado = ConferenciaDoMundo::Descrever(Faltas);

	// NOMEIA AS DUAS, não a primeira. Nomear só a primeira custaria uma rodada
	// por peça perdida: consertar, rodar, bater na seguinte, voltar.
	TestTrue(TEXT("nomeia os cursos d'agua"), Recado.Contains(TEXT("cursos d'agua")));
	TestTrue(TEXT("nomeia o templo"), Recado.Contains(TEXT("templo")));

	// E diz os DOIS números. Só o nome não distingue "faltou um" de "faltaram
	// todos", e essa diferença é a que diz onde procurar.
	TestTrue(TEXT("diz quantos o assado tem"), Recado.Contains(TEXT("137")));
	TestTrue(TEXT("diz quantos o mundo ergueu"), Recado.Contains(TEXT("130")));
	TestTrue(TEXT("diz o zero do templo"), Recado.Contains(TEXT("tem 5")));

	// O que BATEU não aparece: uma lista que repetisse tudo esconderia as
	// divergências no meio do inventário.
	TestFalse(TEXT("nao cita as trilhas, que bateram"), Recado.Contains(TEXT("trilhas")));

	return true;
}

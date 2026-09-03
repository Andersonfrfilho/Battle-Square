// Copyright 2026 Anderson. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Environment/FreshWater.h"
#include "World/IslandBakedPlan.h"

// ---------------------------------------------------------------------------
// M3 — A FUNDURA EXISTE POR PONTO, E QUEM A DECIDE É O GERADOR.
//
// O aceite não é "existe um número": é que ele VARIA ao longo do curso. Fundura
// constante seria a largura com outro nome, e a largura é justamente o que a
// estimativa antiga usava.
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRiverDepthVariesAlongTheCourseTest,
	"BattleSquare.RiverDepth.VariesAlongTheCourse",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRiverDepthVariesAlongTheCourseTest::RunTest(const FString& Parameters)
{
	const UIslandBakedPlan* Assado = IslandBakedPlan::Load();
	if (!Assado)
	{
		AddError(TEXT("o assado nao existe — rode ./Tools/bake_island.sh"));
		return false;
	}

	int32 CursosComFundura = 0;
	int32 CursosQueVariam = 0;
	float MenorDeTodas = TNumericLimits<float>::Max();
	float MaiorDeTodas = 0.0f;

	for (const FBakedRiver& Curso : Assado->Rivers)
	{
		if (Curso.DepthUnits.IsEmpty())
		{
			continue;
		}

		++CursosComFundura;

		// A fundura tem de existir PARA CADA PONTO. Uma lista mais curta que a
		// dos pontos faria quem lê casar fundura de um ponto com a posição de
		// outro — e o erro seria mudo, porque os dois são números plausíveis.
		TestEqual(TEXT("ha uma fundura por ponto do curso"),
			Curso.DepthUnits.Num(), Curso.PointsUnits.Num());

		float Menor = TNumericLimits<float>::Max();
		float Maior = 0.0f;
		for (const float Fundo : Curso.DepthUnits)
		{
			Menor = FMath::Min(Menor, Fundo);
			Maior = FMath::Max(Maior, Fundo);
		}

		MenorDeTodas = FMath::Min(MenorDeTodas, Menor);
		MaiorDeTodas = FMath::Max(MaiorDeTodas, Maior);

		// VARIA quando o fundo do trecho mais fundo é ao menos um quinto maior
		// que o do mais raso. Menos que isso é ruído de amostragem.
		if (Maior > Menor * 1.2f)
		{
			++CursosQueVariam;
		}
	}

	AddInfo(FString::Printf(
		TEXT("FUNDURA: %d cursos, %d variam ao longo do percurso; "
			 "menor %.1f, maior %.1f"),
		CursosComFundura, CursosQueVariam, MenorDeTodas, MaiorDeTodas));

	TestTrue(TEXT("os cursos tem fundura assada"), CursosComFundura > 0);

	// O ACEITE: a maioria dos cursos varia. Um ou outro pode correr com declive
	// parelho do começo ao fim, e isso é um rio manso, não um defeito.
	TestTrue(TEXT("a fundura VARIA na maioria dos cursos"),
		CursosQueVariam > CursosComFundura / 2);

	return true;
}

// A FUNDURA NÃO É A LARGURA COM OUTRO NOME.
//
// É a afirmação que separa esta feature da estimativa que ela substitui. Se as
// duas andassem juntas ponto a ponto, o `largura × 0,065` teria sido suficiente
// e esta task não precisaria existir.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRiverDepthIsNotWidthTest,
	"BattleSquare.RiverDepth.IsNotWidthRenamed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRiverDepthIsNotWidthTest::RunTest(const FString& Parameters)
{
	const UIslandBakedPlan* Assado = IslandBakedPlan::Load();
	if (!Assado)
	{
		AddError(TEXT("o assado nao existe"));
		return false;
	}

	// Procura DOIS pontos com a MESMA largura e funduras diferentes. Basta um
	// par para provar que a fundura carrega informação que a largura não tem.
	//
	// A busca é por par, e não por correlação: correlação alta seria esperada
	// (rio grande é largo E fundo), e não contradiz nada. O que a estimativa
	// não conseguia era dar respostas DIFERENTES para larguras iguais.
	float MaiorDiferenca = 0.0f;
	float LarguraDoPar = 0.0f;

	for (const FBakedRiver& Curso : Assado->Rivers)
	{
		if (Curso.DepthUnits.Num() != Curso.HalfWidthUnits.Num())
		{
			continue;
		}

		for (int32 Um = 0; Um < Curso.DepthUnits.Num(); ++Um)
		{
			for (int32 Outro = Um + 1; Outro < Curso.DepthUnits.Num(); ++Outro)
			{
				const float DeltaLargura =
					FMath::Abs(Curso.HalfWidthUnits[Um] - Curso.HalfWidthUnits[Outro]);

				// Larguras praticamente iguais.
				if (DeltaLargura > FMath::Max(1.0f, Curso.HalfWidthUnits[Um] * 0.02f))
				{
					continue;
				}

				const float DeltaFundura =
					FMath::Abs(Curso.DepthUnits[Um] - Curso.DepthUnits[Outro]);

				if (DeltaFundura > MaiorDiferenca)
				{
					MaiorDiferenca = DeltaFundura;
					LarguraDoPar = Curso.HalfWidthUnits[Um];
				}
			}
		}
	}

	AddInfo(FString::Printf(
		TEXT("mesma largura (%.0f), funduras diferindo em ate %.1f"),
		LarguraDoPar, MaiorDiferenca));

	TestTrue(TEXT("ha pontos de MESMA largura com funduras DIFERENTES"),
		MaiorDiferenca > 1.0f);

	return true;
}

// A CORREDEIRA É RASA, E O REMANSO É FUNDO — a regra que faz a fundura variar.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRiverDepthRapidsAreShallowTest,
	"BattleSquare.RiverDepth.RapidsAreShallow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRiverDepthRapidsAreShallowTest::RunTest(const FString& Parameters)
{
	int32 Comparados = 0;
	int32 RasosNaCorredeira = 0;

	for (const FreshWater::FRiverCourse& Curso : FreshWater::Plan())
	{
		// A CORREDEIRA CONTRA O REMANSO **MAIS PERTO DELA**, e não contra o
		// primeiro remanso do curso.
		//
		// A primeira versão pegava o primeiro de cada, e reprovou em 1 de 35:
		// quando o remanso está na cabeceira e a corredeira lá adiante, a
		// corredeira sai mais funda mesmo com o declive raseando — porque a
		// BASE cresce com o percurso, e eu estava comparando dois pontos em
		// progressos diferentes.
		//
		// Colados no progresso, a base é praticamente a mesma e só o declive
		// difere — que é a única coisa que este teste afirma.
		constexpr int32 Passos = 40;

		int32 OndeACorredeira = INDEX_NONE;
		for (int32 Passo = 0; Passo <= Passos; ++Passo)
		{
			if (FreshWater::IsRapidsAtProgress(Curso, static_cast<float>(Passo) / Passos))
			{
				OndeACorredeira = Passo;
				break;
			}
		}

		if (OndeACorredeira == INDEX_NONE)
		{
			continue;
		}

		// O remanso mais próximo, andando para os dois lados.
		int32 OndeOManso = INDEX_NONE;
		for (int32 Longe = 1; Longe <= Passos && OndeOManso == INDEX_NONE; ++Longe)
		{
			for (const int32 Lado : { -1, 1 })
			{
				const int32 Candidato = OndeACorredeira + Lado * Longe;
				if (Candidato < 0 || Candidato > Passos)
				{
					continue;
				}

				if (!FreshWater::IsRapidsAtProgress(Curso,
					static_cast<float>(Candidato) / Passos))
				{
					OndeOManso = Candidato;
					break;
				}
			}
		}

		if (OndeOManso != INDEX_NONE)
		{
			++Comparados;

			const float NaCorredeira = FreshWater::DepthAtProgress(
				Curso, static_cast<float>(OndeACorredeira) / Passos);
			const float NoManso = FreshWater::DepthAtProgress(
				Curso, static_cast<float>(OndeOManso) / Passos);

			if (NaCorredeira < NoManso) { ++RasosNaCorredeira; }
		}
	}

	AddInfo(FString::Printf(
		TEXT("cursos com corredeira E remanso: %d; a corredeira e mais rasa em %d"),
		Comparados, RasosNaCorredeira));

	if (Comparados == 0)
	{
		AddError(TEXT("nenhum curso tem corredeira e remanso — nada foi comparado"));
		return false;
	}

	TestEqual(TEXT("a corredeira e MAIS RASA que o remanso, em todos"),
		RasosNaCorredeira, Comparados);

	return true;
}

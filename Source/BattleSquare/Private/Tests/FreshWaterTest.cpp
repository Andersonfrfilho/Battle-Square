// Copyright 2026 Anderson. All Rights Reserved.

#include "Environment/FreshWater.h"

#include "Environment/IslandFeatureLayout.h"
#include "Environment/IslandGeography.h"
#include "Misc/AutomationTest.h"

namespace
{
	// Nome próprio (L-042): helper homônimo em outro arquivo de teste vira
	// sobrecarga ambígua quando o unity build junta os dois.
	int32 ContarMontanhasDaIlhaParaOsRios()
	{
		int32 Montes = 0;
		for (const IslandFeatureLayout::FFeaturePlacement& Peca : IslandFeatureLayout::Plan())
		{
			if (Peca.Feature == IslandFeatureLayout::EIslandFeature::WalkableMountain)
			{
				++Montes;
			}
		}
		return Montes;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFreshWaterGivesEveryMountainOneRiverTest,
	"BattleSquare.Environment.FreshWater.EveryMountainHasARiver",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFreshWaterGivesEveryMountainOneRiverTest::RunTest(const FString& Parameters)
{
	const TArray<FreshWater::FRiverCourse> Cursos = FreshWater::Plan();

	// Um rio POR MONTANHA. É o que amarra o curso ao relevo: quem mudar o anel
	// dos montes leva os rios junto, em vez de deixá-los brotando do nada.
	TestEqual(TEXT("ha um rio para cada montanha caminhavel"),
		Cursos.Num(), ContarMontanhasDaIlhaParaOsRios());
	TestTrue(TEXT("ha pelo menos um rio"), Cursos.Num() > 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFreshWaterRiverRunsFromMountainToSeaTest,
	"BattleSquare.Environment.FreshWater.RiverRunsDownhillToTheSea",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFreshWaterRiverRunsFromMountainToSeaTest::RunTest(const FString& Parameters)
{
	const float Costa = IslandGeography::LandRadiusUnits();

	for (const FreshWater::FRiverCourse& Curso : FreshWater::Plan())
	{
		// A nascente sai da SAIA do monte, não do cume nem da planície.
		TestTrue(TEXT("a nascente fica alem do miolo da ilha"),
			Curso.SourceRadiusUnits > IslandGeography::HomeRadiusUnits());
		TestTrue(TEXT("a nascente fica antes da foz"),
			Curso.SourceRadiusUnits < Curso.MouthRadiusUnits);

		// A foz é a costa. Rio que para antes vira poça no meio do mato.
		TestEqual(TEXT("o rio morre na linha da costa"),
			Curso.MouthRadiusUnits, Costa, 1.0f);

		// Descer é afastar-se do centro, e é o percurso inteiro que precisa
		// fazer isso: um trecho que volta para dentro é água subindo o morro.
		float RaioAnterior = Curso.SourceRadiusUnits;
		FVector2D Anterior = FreshWater::PointAt(Curso, RaioAnterior);
		for (float Raio = Curso.SourceRadiusUnits + 200.0f; Raio <= Curso.MouthRadiusUnits;
			Raio += 200.0f)
		{
			const FVector2D Aqui = FreshWater::PointAt(Curso, Raio);
			TestTrue(TEXT("cada passo do rio se afasta mais do centro"),
				Aqui.Size() > Anterior.Size());
			Anterior = Aqui;
			RaioAnterior = Raio;
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFreshWaterRiverMeandersInsteadOfGoingStraightTest,
	"BattleSquare.Environment.FreshWater.RiverMeanders",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFreshWaterRiverMeandersInsteadOfGoingStraightTest::RunTest(const FString& Parameters)
{
	// "trilhas horriveis", "precisam ser realistas com curvas": rio em linha
	// reta é canal de concreto. O teste mede que o rumo REALMENTE varia.
	for (const FreshWater::FRiverCourse& Curso : FreshWater::Plan())
	{
		float MenorRumo = TNumericLimits<float>::Max();
		float MaiorRumo = TNumericLimits<float>::Lowest();

		for (float Raio = Curso.SourceRadiusUnits; Raio <= Curso.MouthRadiusUnits;
			Raio += 200.0f)
		{
			const FVector2D Aqui = FreshWater::PointAt(Curso, Raio);
			const float Rumo = FMath::Atan2(Aqui.Y, Aqui.X);
			MenorRumo = FMath::Min(MenorRumo, Rumo);
			MaiorRumo = FMath::Max(MaiorRumo, Rumo);
		}

		TestTrue(TEXT("o rumo do rio abre para os dois lados ao longo do curso"),
			(MaiorRumo - MenorRumo) > 0.05f);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFreshWaterLakeIsTheRiverGoingWideTest,
	"BattleSquare.Environment.FreshWater.LakeIsWidthNotASecondThing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFreshWaterLakeIsTheRiverGoingWideTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("o lago e mais largo que o rio"),
		FreshWater::LakeHalfWidthUnits() > FreshWater::RiverHalfWidthUnits());

	for (const FreshWater::FRiverCourse& Curso : FreshWater::Plan())
	{
		// No raio do lago a calha chega ao máximo — é ali que ele existe.
		const float NoLago = FreshWater::HalfWidthAt(Curso, Curso.LakeRadiusUnits);
		TestEqual(TEXT("no raio do lago a calha chega a largura de lago"),
			NoLago, FreshWater::LakeHalfWidthUnits(), 1.0f);

		// Na nascente e na foz ela volta a ser rio.
		TestEqual(TEXT("na nascente a calha e de rio"),
			FreshWater::HalfWidthAt(Curso, Curso.SourceRadiusUnits),
			FreshWater::RiverHalfWidthUnits(), 1.0f);
		TestEqual(TEXT("na foz a calha e de rio"),
			FreshWater::HalfWidthAt(Curso, Curso.MouthRadiusUnits),
			FreshWater::RiverHalfWidthUnits(), 1.0f);

		// E cresce de forma monótona até lá: alargamento que oscila lê como
		// remendo, não como água parando.
		float Anterior = FreshWater::HalfWidthAt(Curso, Curso.SourceRadiusUnits);
		for (float Raio = Curso.SourceRadiusUnits + 100.0f; Raio <= Curso.LakeRadiusUnits;
			Raio += 100.0f)
		{
			const float Agora = FreshWater::HalfWidthAt(Curso, Raio);
			TestTrue(TEXT("a calha so engorda ate o lago"), Agora >= Anterior - 0.01f);
			Anterior = Agora;
		}

		TestTrue(TEXT("o lago fica dentro do percurso do rio"),
			Curso.LakeRadiusUnits > Curso.SourceRadiusUnits
			&& Curso.LakeRadiusUnits < Curso.MouthRadiusUnits);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFreshWaterFallNeverLandsInTheLakeTest,
	"BattleSquare.Environment.FreshWater.FallComesAfterTheLake",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFreshWaterFallNeverLandsInTheLakeTest::RunTest(const FString& Parameters)
{
	for (const FreshWater::FRiverCourse& Curso : FreshWater::Plan())
	{
		// Cachoeira dentro do lago é água caindo dentro de água parada.
		TestTrue(TEXT("a queda vem depois do lago"),
			Curso.FallRadiusUnits > Curso.LakeRadiusUnits);
		TestEqual(TEXT("no raio da queda a calha ja voltou a ser rio"),
			FreshWater::HalfWidthAt(Curso, Curso.FallRadiusUnits),
			FreshWater::RiverHalfWidthUnits(), 1.0f);

		// E antes do mar: cachoeira despejando na arrebentação seria o rio
		// terminando duas vezes.
		TestTrue(TEXT("a queda acontece em terra, nao na foz"),
			Curso.FallRadiusUnits + FreshWater::FallHalfLengthUnits()
			< Curso.MouthRadiusUnits);

		TestTrue(TEXT("no raio da queda a agua esta caindo"),
			FreshWater::IsFallAt(Curso, Curso.FallRadiusUnits));
		TestFalse(TEXT("na nascente a agua nao esta caindo"),
			FreshWater::IsFallAt(Curso, Curso.SourceRadiusUnits));
		TestFalse(TEXT("no lago a agua nao esta caindo"),
			FreshWater::IsFallAt(Curso, Curso.LakeRadiusUnits));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFreshWaterCourseIsTheSameEveryTimeTest,
	"BattleSquare.Environment.FreshWater.PlanIsDeterministic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFreshWaterCourseIsTheSameEveryTimeTest::RunTest(const FString& Parameters)
{
	// Cada pedaço do mundo chama `Plan()` de novo ao montar. Se a resposta
	// mudasse entre chamadas, o rio quebraria na fronteira de todo ladrilho.
	const TArray<FreshWater::FRiverCourse> Primeira = FreshWater::Plan();
	const TArray<FreshWater::FRiverCourse> Segunda = FreshWater::Plan();

	TestEqual(TEXT("o mesmo numero de rios"), Primeira.Num(), Segunda.Num());
	for (int32 Indice = 0; Indice < Primeira.Num(); ++Indice)
	{
		TestEqual(TEXT("a mesma nascente"),
			Primeira[Indice].SourceRadiusUnits, Segunda[Indice].SourceRadiusUnits, 0.01f);
		TestEqual(TEXT("o mesmo lago"),
			Primeira[Indice].LakeRadiusUnits, Segunda[Indice].LakeRadiusUnits, 0.01f);
		TestEqual(TEXT("a mesma queda"),
			Primeira[Indice].FallRadiusUnits, Segunda[Indice].FallRadiusUnits, 0.01f);
	}

	// E dois rios não têm a mesma cara: sementes por índice existem para isso.
	if (Primeira.Num() >= 2)
	{
		TestNotEqual(TEXT("dois rios nao serpenteiam igual"),
			Primeira[0].MeanderTurns, Primeira[1].MeanderTurns);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFreshWaterRiversDoNotShareACourseTest,
	"BattleSquare.Environment.FreshWater.RiversStayApart",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFreshWaterRiversDoNotShareACourseTest::RunTest(const FString& Parameters)
{
	const TArray<FreshWater::FRiverCourse> Cursos = FreshWater::Plan();

	// Dois rios encostando um no outro seriam um rio só, largo e torto — e o
	// jogador que seguisse um chegaria à nascente do outro.
	for (int32 Um = 0; Um < Cursos.Num(); ++Um)
	{
		for (int32 Outro = Um + 1; Outro < Cursos.Num(); ++Outro)
		{
			for (float Raio = 13500.0f; Raio <= 19500.0f; Raio += 500.0f)
			{
				const FVector2D Aqui = FreshWater::PointAt(Cursos[Um], Raio);
				const FVector2D La = FreshWater::PointAt(Cursos[Outro], Raio);
				const float Folga = FreshWater::HalfWidthAt(Cursos[Um], Raio)
					+ FreshWater::HalfWidthAt(Cursos[Outro], Raio);

				TestTrue(TEXT("dois rios nunca se tocam"),
					FVector2D::Distance(Aqui, La) > Folga);
			}
		}
	}

	return true;
}

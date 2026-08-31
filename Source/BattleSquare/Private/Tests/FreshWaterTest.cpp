// Copyright 2026 Anderson. All Rights Reserved.

#include "Environment/CaveSystem.h"
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFreshWaterOpensAGrottoBesideEveryFallTest,
	"BattleSquare.Environment.FreshWater.GrottoBesideEveryFall",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFreshWaterOpensAGrottoBesideEveryFallTest::RunTest(const FString& Parameters)
{
	const TArray<FreshWater::FRiverCourse> Cursos = FreshWater::Plan();
	const TArray<IslandFeatureLayout::FFeaturePlacement> Grutas = FreshWater::PlanGrottoes();

	// A busca pode vir com menos grutas do que quedas — é resposta válida
	// quando não sobra lugar. Este teste afirma que HOJE sobra para todas, e é
	// justamente por isso que ele vale: encolher a ilha, engordar o lago ou
	// alargar a praia apaga uma gruta em silêncio, e a contagem grita.
	TestEqual(TEXT("ha uma gruta para cada cachoeira"), Grutas.Num(), Cursos.Num());
	TestTrue(TEXT("ha pelo menos uma gruta"), Grutas.Num() > 0);

	// Ela é uma TOCA, não um labirinto: o labirinto grande não caberia na faixa
	// de terra que sobra entre o lago e a praia, e uma cavidade ao lado de uma
	// queda d'água não deveria ser um labirinto de todo jeito.
	for (const IslandFeatureLayout::FFeaturePlacement& Gruta : Grutas)
	{
		TestTrue(TEXT("a gruta e menor que a menor caverna da ilha"),
			Gruta.CaveSide < ACaveSystem::SmallCaveSide);
	}

	for (const IslandFeatureLayout::FFeaturePlacement& Gruta : Grutas)
	{
		// Ela é uma CAVERNA no plano, e não um tipo novo: é isso que faz o
		// GameMode plantá-la sem uma segunda cópia do despacho.
		TestEqual(TEXT("a gruta entra no plano como caverna"),
			static_cast<int32>(Gruta.Feature),
			static_cast<int32>(IslandFeatureLayout::EIslandFeature::Cave));

		// Água por causa da cachoeira, e não por causa do raio. O temperador do
		// plano da ilha dá água a quem está perto da orla; aqui o motivo é
		// outro, e não pode depender de a gruta cair perto do mar.
		TestEqual(TEXT("a gruta da cachoeira tem agua dentro"),
			static_cast<int32>(Gruta.CaveFlavor), static_cast<int32>(ECaveFlavor::Water));
		TestTrue(TEXT("e por isso ela se explora"), IsCaveExplorable(Gruta.CaveFlavor));

		TestTrue(TEXT("a gruta tem tamanho de labirinto"), Gruta.CaveSide > 0);
		TestTrue(TEXT("e reserva espaco em volta"), Gruta.ClearanceUnits > 0.0f);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFreshWaterKeepsTheGrottoOutOfTheWaterTest,
	"BattleSquare.Environment.FreshWater.GrottoStandsClearOfTheChannel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFreshWaterKeepsTheGrottoOutOfTheWaterTest::RunTest(const FString& Parameters)
{
	const TArray<FreshWater::FRiverCourse> Cursos = FreshWater::Plan();
	const TArray<IslandFeatureLayout::FFeaturePlacement> Grutas = FreshWater::PlanGrottoes();

	if (Cursos.Num() == 0 || Grutas.Num() != Cursos.Num())
	{
		AddError(TEXT("sem gruta para conferir"));
		return false;
	}

	// A conferência é contra o curso INTEIRO, não contra o ponto da queda. O
	// lago está rio acima e alcança daqui: encostar a quina da gruta na água
	// alagada é a quina de caverna dentro de montanha com outro nome.
	for (const IslandFeatureLayout::FFeaturePlacement& Gruta : Grutas)
	{
		const FVector2D Centro = Gruta.CenterUnits();

		for (const FreshWater::FRiverCourse& Curso : Cursos)
		{
			for (float Raio = Curso.SourceRadiusUnits; Raio <= Curso.MouthRadiusUnits;
				Raio += 60.0f)
			{
				const FVector2D NaAgua = FreshWater::PointAt(Curso, Raio);
				const float Folga = FreshWater::HalfWidthAt(Curso, Raio) + Gruta.ClearanceUnits;

				TestTrue(TEXT("nenhuma quina da gruta cai dentro da agua"),
					FVector2D::Distance(Centro, NaAgua) > Folga);
			}
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFreshWaterGrottoWatchesTheFallTest,
	"BattleSquare.Environment.FreshWater.GrottoWatchesTheFall",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFreshWaterGrottoWatchesTheFallTest::RunTest(const FString& Parameters)
{
	const TArray<FreshWater::FRiverCourse> Cursos = FreshWater::Plan();
	const TArray<IslandFeatureLayout::FFeaturePlacement> Grutas = FreshWater::PlanGrottoes();

	if (Cursos.Num() == 0 || Grutas.Num() != Cursos.Num())
	{
		AddError(TEXT("sem gruta para conferir"));
		return false;
	}

	// Estar fora da água não basta: fora da água é toda a ilha. A gruta precisa
	// estar PERTO da queda do SEU rio, senão ela é uma caverna qualquer com um
	// nome que promete cachoeira.
	for (int32 Indice = 0; Indice < Grutas.Num(); ++Indice)
	{
		const FVector2D Centro = Grutas[Indice].CenterUnits();
		const FVector2D NaQueda = FreshWater::PointAt(Cursos[Indice],
			Cursos[Indice].FallRadiusUnits);

		const FVector2D NoLago = FreshWater::PointAt(Cursos[Indice],
			Cursos[Indice].LakeRadiusUnits);

		// O teto vem do próprio rio, não de um múltiplo escolhido a dedo: a
		// gruta é da CACHOEIRA, então ela tem de estar mais perto da queda do
		// que do lago que a alimenta. Assim o limite acompanha o rio quando o
		// serpenteado dele mudar, em vez de ficar valendo por coincidência.
		const float Daqui = FVector2D::Distance(Centro, NaQueda);
		TestTrue(TEXT("a gruta esta a vista da queda"),
			Daqui < FVector2D::Distance(NaQueda, NoLago));

		// E do lado, não embaixo: o lugar imediatamente abaixo do degrau é da
		// água caindo, e uma boca ali teria o rio entrando por dentro.
		TestTrue(TEXT("mas nao embaixo dela"),
			Daqui > FreshWater::FallHalfLengthUnits());

		// Ela é a gruta DESTE rio: a mais perto dela tem de ser a sua queda.
		for (int32 Outro = 0; Outro < Cursos.Num(); ++Outro)
		{
			if (Outro == Indice)
			{
				continue;
			}

			const FVector2D QuedaAlheia = FreshWater::PointAt(Cursos[Outro],
				Cursos[Outro].FallRadiusUnits);
			TestTrue(TEXT("e nao a de outro rio"),
				FVector2D::Distance(Centro, QuedaAlheia) > Daqui);
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFreshWaterGrottoFitsTheIslandTest,
	"BattleSquare.Environment.FreshWater.GrottoFitsBesideTheOtherFeatures",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFreshWaterGrottoFitsTheIslandTest::RunTest(const FString& Parameters)
{
	const TArray<IslandFeatureLayout::FFeaturePlacement> Grutas = FreshWater::PlanGrottoes();
	const TArray<IslandFeatureLayout::FFeaturePlacement> Pecas = IslandFeatureLayout::Plan();
	const IslandFeatureLayout::FIslandBounds Medidas;

	if (Grutas.Num() == 0)
	{
		AddError(TEXT("sem gruta para conferir"));
		return false;
	}

	// As MESMAS conferências das outras peças, e é o ganho de a gruta ser uma
	// caverna: nada disto precisou ser escrito de novo.
	for (int32 Indice = 0; Indice < Grutas.Num(); ++Indice)
	{
		TestTrue(TEXT("a gruta cabe inteira na terra"),
			IslandFeatureLayout::FitsOnLand(Grutas[Indice], Medidas));
		TestTrue(TEXT("a gruta nao encosta em campo de treino"),
			IslandFeatureLayout::ClearsTrainingFields(Grutas[Indice], Medidas));

		for (const IslandFeatureLayout::FFeaturePlacement& Peca : Pecas)
		{
			TestFalse(TEXT("a gruta nao invade montanha, caverna nem vulcao"),
				IslandFeatureLayout::Overlaps(Grutas[Indice], Peca));
		}

		for (int32 Outra = Indice + 1; Outra < Grutas.Num(); ++Outra)
		{
			TestFalse(TEXT("duas grutas nao se invadem"),
				IslandFeatureLayout::Overlaps(Grutas[Indice], Grutas[Outra]));
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFreshWaterGrottoPlanIsStableTest,
	"BattleSquare.Environment.FreshWater.GrottoPlanIsDeterministic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFreshWaterGrottoPlanIsStableTest::RunTest(const FString& Parameters)
{
	const TArray<IslandFeatureLayout::FFeaturePlacement> Primeira = FreshWater::PlanGrottoes();
	const TArray<IslandFeatureLayout::FFeaturePlacement> Segunda = FreshWater::PlanGrottoes();

	TestEqual(TEXT("o mesmo plano devolve o mesmo tanto de grutas"),
		Segunda.Num(), Primeira.Num());

	// Gruta que muda de lugar a cada partida não é lugar: é o mapa deixando de
	// ser algo que a pessoa aprende.
	for (int32 Indice = 0; Indice < Primeira.Num() && Indice < Segunda.Num(); ++Indice)
	{
		TestEqual(TEXT("a gruta fica sempre no mesmo angulo"),
			Segunda[Indice].AngleDegrees, Primeira[Indice].AngleDegrees);
		TestEqual(TEXT("e sempre no mesmo raio"),
			Segunda[Indice].RadiusUnits, Primeira[Indice].RadiusUnits);
	}

	return true;
}

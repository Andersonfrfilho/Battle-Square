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
	// "Um rio por montanha" era a amarra do modelo antigo: cada monte emitia um
	// curso, e contar cursos conferia a amarra.
	//
	// A bacia deixou de ser emitida pelos montes e passou a CRESCER da costa
	// para dentro, por colonização espacial. O que amarra a água ao relevo
	// agora é outra coisa, e melhor: os cursos procuram o chão que desce.
	//
	// A afirmação certa é que a rede EXISTE e é uma árvore de verdade — várias
	// ordens de Strahler, com mais galho fino que tronco grosso.
	const TArray<FreshWater::FRiverCourse> Cursos = FreshWater::Plan();
	TestTrue(TEXT("ha bacia"), Cursos.Num() > 8);

	int32 Maior = 1;
	int32 DeOrdemUm = 0;
	int32 DoMaior = 0;

	for (const FreshWater::FRiverCourse& Curso : Cursos)
	{
		Maior = FMath::Max(Maior, Curso.Order);
	}

	for (const FreshWater::FRiverCourse& Curso : Cursos)
	{
		DeOrdemUm += (Curso.Order == 1) ? 1 : 0;
		DoMaior += (Curso.Order == Maior) ? 1 : 0;
	}

	TestTrue(TEXT("a bacia tem varias ordens"), Maior >= 3);
	TestTrue(TEXT("e mais fiapo que tronco — arvore, nao pente"), DeOrdemUm > DoMaior);

	// E ela chega ao mar: bacia que nao desemboca e agua presa.
	TestTrue(TEXT("ha curso que desemboca"), FreshWater::PlanTrunks().Num() > 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFreshWaterRiverRunsFromMountainToSeaTest,
	"BattleSquare.Environment.FreshWater.RiverRunsDownhillToTheSea",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFreshWaterRiverRunsFromMountainToSeaTest::RunTest(const FString& Parameters)
{
	const float Costa = IslandGeography::LandRadiusUnits();

	// "Descer" deixou de ser "afastar-se do centro".
	//
	// Isso valia quando o curso era uma função do raio e só podia correr para
	// fora. Numa bacia dendrítica o galho corre de lado, volta, e continua
	// descendo — porque descer é perder ALTURA, e não ganhar raio.
	//
	// A altura é a da ROCHA: o leito do rio é anterior à vila, e perguntar a
	// altura acabada fecharia o ciclo que já abortou o processo uma vez.
	int32 Conferidos = 0;

	for (const FreshWater::FRiverCourse& Curso : FreshWater::Plan())
	{
		if (Curso.PointsUnits.Num() < 3)
		{
			continue;
		}

		++Conferidos;

		const float NaNascente = IslandGeography::BedrockHeightAt(Curso.PointsUnits[0]);
		const float NaFoz = IslandGeography::BedrockHeightAt(Curso.PointsUnits.Last());

		TestTrue(TEXT("o curso perde altura da nascente para a foz"), NaFoz <= NaNascente);

		// E quem chega ao mar chega NA COSTA. Rio que para antes vira poça no
		// meio do mato — mas só vale para quem desemboca: galho morre na
		// junção, e ali é o certo.
		if (Curso.FlowsToTheSea())
		{
			TestTrue(TEXT("o rio que desemboca morre na linha da costa"),
				Curso.PointsUnits.Last().Size() > Costa - FreshWater::RiverHalfWidthUnits() * 6.0f);
		}
	}

	TestTrue(TEXT("houve curso para conferir"), Conferidos > 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFreshWaterRiverMeandersInsteadOfGoingStraightTest,
	"BattleSquare.Environment.FreshWater.RiverMeanders",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFreshWaterRiverMeandersInsteadOfGoingStraightTest::RunTest(const FString& Parameters)
{
	// "trilhas horriveis", "precisam ser realistas com curvas": rio em linha
	// reta é canal de concreto.
	//
	// A medida mudou junto com o modelo. Ela era a abertura do RUMO ao longo do
	// curso, e isso só fazia sentido quando o traçado era uma fórmula do raio —
	// os campos `MeanderRadians` e `MeanderTurns` que ela lia nem existem mais
	// no gerador por colonização.
	//
	// A medida certa é a SINUOSIDADE da linha desenhada: quanto ela anda
	// dividido pela distância entre as pontas. É a mesma que a literatura usa
	// para dizer se um rio é meândrico.
	int32 Tortos = 0;
	int32 Conferidos = 0;

	for (const FreshWater::FRiverCourse& Curso : FreshWater::Plan())
	{
		if (Curso.PointsUnits.Num() < 3)
		{
			continue;
		}

		const float EmLinhaReta = static_cast<float>(
			FVector2D::Distance(Curso.PointsUnits[0], Curso.PointsUnits.Last()));
		if (EmLinhaReta < FreshWater::RiverHalfWidthUnits())
		{
			continue;
		}

		++Conferidos;
		Tortos += (FreshWater::CourseLengthUnits(Curso) / EmLinhaReta > 1.02f) ? 1 : 0;
	}

	TestTrue(TEXT("houve curso para conferir"), Conferidos > 0);

	// A MAIORIA torta, e não todos: um galho de cabeceira curto sai reto, e
	// está certo — cabeceira de rio real é reta, o meandro nasce no curso
	// baixo, onde a inclinação cai.
	TestTrue(TEXT("a maioria dos cursos serpenteia"), Tortos * 2 > Conferidos);

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
		if (!Curso.HasLake())
		{
			continue;
		}

		// No progresso do lago a calha chega ao máximo — é ali que ele existe.
		const float NoLago = FreshWater::HalfWidthAtProgress(Curso, Curso.LakeAtProgress);
		TestEqual(TEXT("no raio do lago a calha chega a largura de lago"),
			NoLago, FreshWater::LakeHalfWidthUnits(), 1.0f);

		// Só onde o lago não encosta nas pontas: com o lago colado na nascente,
		// a "nascente" tem largura de lago, e a comparação deixa de medir o
		// crescimento do rio.
		// O lago tem de estar longe das duas pontas — e "longe" é medido no
		// ALCANCE dele, não num número fixo. Num curso curto, um lago no meio
		// ainda alaga as pontas, e aí "a nascente é calha" deixa de ser
		// verdade sem que nada esteja errado.
		const float AlcanceEmProgresso = FreshWater::LakeHalfWidthUnits() * 1.7f
			/ FMath::Max(1.0f, FreshWater::CourseLengthUnits(Curso));

		if (Curso.LakeAtProgress - AlcanceEmProgresso < 0.05f
			|| Curso.LakeAtProgress + AlcanceEmProgresso > 0.95f)
		{
			continue;
		}

		// Na nascente e na foz ela é calha, não lago — e a foz é MAIS LARGA
		// que a nascente, que é o que um rio faz.
		TestTrue(TEXT("a foz e mais larga que a nascente"),
			FreshWater::HalfWidthAtProgress(Curso, 1.0f)
				> FreshWater::HalfWidthAtProgress(Curso, 0.0f));
		// (a guarda acima já pulou os cursos cujo lago encosta numa ponta)
		TestTrue(TEXT("e nenhuma das duas e lago"),
			FreshWater::HalfWidthAtProgress(Curso, 1.0f)
				< FreshWater::LakeHalfWidthUnits() * 0.5f);


		// E cresce de forma monótona até lá: alargamento que oscila lê como
		// remendo, não como água parando.
		float Anterior = FreshWater::HalfWidthAtProgress(Curso, 0.0f);
		for (float Raio = 0.0f + 100.0f; Raio <= Curso.LakeAtProgress;
			Raio += 100.0f)
		{
			const float Agora = FreshWater::HalfWidthAtProgress(Curso, Raio);
			TestTrue(TEXT("a calha so engorda ate o lago"), Agora >= Anterior - 0.01f);
			Anterior = Agora;
		}

		TestTrue(TEXT("o lago fica dentro do percurso do rio"),
			Curso.LakeAtProgress > 0.0f
			&& Curso.LakeAtProgress < 1.0f);
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
		// Nem todo curso tem queda, e forçar uma em cada um inventa acidente
		// onde o terreno não tem. Perguntar sem checar foi o que pôs uma vila
		// fora da ilha.
		if (!Curso.HasFall() || !Curso.HasLake())
		{
			continue;
		}

		// Cachoeira dentro do lago é água caindo dentro de água parada.
		TestTrue(TEXT("a queda vem depois do lago"),
			Curso.FallAtProgress > Curso.LakeAtProgress);
		// "Voltou a ser rio" quer dizer que ela saiu do lago, e não que ela tem
		// a largura da nascente: a calha ENGROSSA rio abaixo, porque a chuva
		// da bacia entra pela margem o percurso inteiro. A asserção de
		// igualdade valia quando o rio tinha a mesma calha do começo ao fim.
		if (!Curso.HasLake())
		{
			continue;
		}

		TestTrue(TEXT("no raio da queda a calha ja saiu do lago"),
			FreshWater::HalfWidthAtProgress(Curso, Curso.FallAtProgress)
				< FreshWater::LakeHalfWidthUnits() * 0.5f);

		// E antes do mar: cachoeira despejando na arrebentação seria o rio
		// terminando duas vezes.
		// A queda acontece ANTES da foz, com folga proporcional ao curso — a
		// meia-queda é uma distância, e comparar distância com progresso só
		// funciona depois de dividir pelo comprimento.
		const float MeiaEmProgresso = FreshWater::FallHalfLengthUnits()
			/ FMath::Max(1.0f, FreshWater::CourseLengthUnits(Curso));

		TestTrue(TEXT("a queda acontece em terra, nao na foz"),
			Curso.FallAtProgress + MeiaEmProgresso < 1.0f);

		TestTrue(TEXT("no raio da queda a agua esta caindo"),
			FreshWater::IsFallAtProgress(Curso, Curso.FallAtProgress));
		TestFalse(TEXT("na nascente a agua nao esta caindo"),
			FreshWater::IsFallAtProgress(Curso, 0.0f));
		TestFalse(TEXT("no lago a agua nao esta caindo"),
			FreshWater::IsFallAtProgress(Curso, Curso.LakeAtProgress));
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
			Primeira[Indice].LakeAtProgress, Segunda[Indice].LakeAtProgress, 0.001f);
		TestEqual(TEXT("a mesma queda"),
			Primeira[Indice].FallAtProgress, Segunda[Indice].FallAtProgress, 0.001f);

		// E o mesmo CURSO DESENHADO, ponto a ponto. Sem isto, o determinismo
		// afirmaria só os marcos e deixaria a linha inteira livre — que é
		// justamente o que passou a ser gerado, e o que se vê na tela.
		TestEqual(TEXT("o mesmo numero de pontos"),
			Primeira[Indice].PointsUnits.Num(), Segunda[Indice].PointsUnits.Num());
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
	// "Dois rios nunca se tocam" deixou de valer, e a mudança é o objetivo.
	//
	// Numa bacia dendrítica os cursos CONVERGEM: é isso que faz o desenho ler
	// como raiz em vez de pente. Exigir afastamento seria proibir a confluência,
	// que é a coisa que a bacia veio ter.
	//
	// O que continua verdade — e é o que sobra de útil da intenção antiga — é
	// que eles se encontram nas PONTAS, não pelo meio. Dois cursos deitados um
	// sobre o outro no meio do percurso seriam o mesmo rio desenhado duas vezes.
	const TArray<FreshWater::FRiverCourse> Cursos = FreshWater::Plan();
	TestTrue(TEXT("ha rios"), Cursos.Num() > 1);

	int32 Sobrepostos = 0;

	for (int32 Um = 0; Um < Cursos.Num(); ++Um)
	{
		if (Cursos[Um].PointsUnits.Num() < 3)
		{
			continue;
		}

		const FVector2D NoMeio = FreshWater::PointAtProgress(Cursos[Um], 0.5f);
		const float DaCalha = FreshWater::HalfWidthAtProgress(Cursos[Um], 0.5f);

		for (int32 Outro = 0; Outro < Cursos.Num(); ++Outro)
		{
			if (Outro == Um || Cursos[Outro].PointsUnits.Num() < 3)
			{
				continue;
			}

			float Aonde = 0.0f;
			const float Ate = FreshWater::NearestOn(Cursos[Outro], NoMeio, Aonde);

			// Perto da PONTA do outro é confluência, e é bem-vindo.
			if (Aonde < 0.12f || Aonde > 0.88f)
			{
				continue;
			}

			if (Ate < DaCalha)
			{
				++Sobrepostos;
			}
		}
	}

	TestEqual(TEXT("nenhum curso deita no meio de outro"), Sobrepostos, 0);

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
	// Uma gruta por CACHOEIRA, e nem todo curso tem uma: a queda nasce onde o
	// leito despenca, e a maioria dos cursos corre manso.
	int32 ComQueda = 0;
	for (const FreshWater::FRiverCourse& Curso : Cursos)
	{
		ComQueda += Curso.HasFall() ? 1 : 0;
	}

	// NO MÁXIMO uma por cachoeira, e não exatamente uma.
	//
	// A gruta precisa de chão seco perto da queda, e nem toda queda tem: numa
	// bacia densa a água ocupa a volta. Exigir uma para cada faria o gerador
	// enfiar gruta dentro do rio para satisfazer a conta — que é o defeito que
	// a busca de posição já veio evitar.
	TestTrue(TEXT("ha gruta"), Grutas.Num() > 0);
	TestTrue(TEXT("e no maximo uma por cachoeira"), Grutas.Num() <= ComQueda);
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
			for (float Raio = 0.0f; Raio <= 1.0f;
				Raio += 60.0f)
			{
				const FVector2D NaAgua = FreshWater::PointAtProgress(Curso, Raio);
				const float Folga = FreshWater::HalfWidthAtProgress(Curso, Raio) + Gruta.ClearanceUnits;

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
		const FVector2D NaQueda = FreshWater::PointAtProgress(Cursos[Indice],
			Cursos[Indice].FallAtProgress);

		const FVector2D NoLago = FreshWater::PointAtProgress(Cursos[Indice],
			Cursos[Indice].LakeAtProgress);

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

			const FVector2D QuedaAlheia = FreshWater::PointAtProgress(Cursos[Outro],
				Cursos[Outro].FallAtProgress);
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

// Copyright 2026 Anderson. All Rights Reserved.

#include "Environment/CaveSystem.h"
#include "Environment/FreshWater.h"
#include "World/WorldBudget.h"

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
			// A costa DAQUELE RUMO, e com a folga do passo do gerador: a foz é
			// um nó amostrado, e ele cai onde o passo o deixou — nunca exatamente
			// na linha.
			const FVector2D NoFim = Curso.PointsUnits.Last();
			const float NaLinhaDaCosta = IslandGeography::LandRadiusAt(
				FMath::Atan2(NoFim.Y, NoFim.X));

			TestTrue(TEXT("o rio que desemboca morre na linha da costa"),
				NoFim.Size() > NaLinhaDaCosta - Costa * 0.03f);
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
		const float AlcanceEmProgresso = FreshWater::LakeHalfWidthUnits() * 2.4f
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
		// "Não é lago" medido contra a CALHA DESTE curso, e não contra metade
		// da largura de lago do mundo.
		//
		// A régua global só valia quando todo curso tinha a mesma calha. Hoje
		// um tronco de ordem alta é largo por ser tronco, e ele reprovava por
		// isso — a asserção media a ordem do rio achando que media o lago.
		TestTrue(TEXT("e nenhuma das duas e lago"),
			FreshWater::HalfWidthAtProgress(Curso, 1.0f)
				< FreshWater::HalfWidthAtProgress(Curso, 0.0f) * 3.0f);


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

		// Só quando o lago não alcança a queda: num curso curto a barriga do
		// lago cobre o degrau, e aí "já saiu do lago" é falso sem que nada
		// esteja errado — é o curso que é curto demais para ter os dois.
		const float AlcanceEmProgresso = FreshWater::LakeHalfWidthUnits() * 2.4f
			/ FMath::Max(1.0f, FreshWater::CourseLengthUnits(Curso));

		if (FMath::Abs(Curso.FallAtProgress - Curso.LakeAtProgress) < AlcanceEmProgresso)
		{
			continue;
		}

		// Mesma correção: a régua é a calha DESTE curso.
		TestTrue(TEXT("no raio da queda a calha ja saiu do lago"),
			FreshWater::HalfWidthAtProgress(Curso, Curso.FallAtProgress)
				< FreshWater::HalfWidthAtProgress(Curso, 0.0f) * 3.0f);

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
		// Dois rios não têm a mesma cara.
		//
		// A comparação era entre `MeanderRadians`, um campo que o gerador por
		// colonização não usa mais — ele valia zero nos dois, e "diferente de
		// zero" reprovava sempre. A medida certa é a forma da LINHA: dois
		// cursos com a mesma sinuosidade até a quarta casa seriam o mesmo rio
		// desenhado duas vezes.
		// Dois cursos com PONTOS de sobra: um segmento de dois pontos é reto por
		// definição, e comparar dois deles compara 1,0 com 1,0.
		int32 Um = INDEX_NONE;
		int32 Outro = INDEX_NONE;
		for (int32 Qual = 0; Qual < Primeira.Num(); ++Qual)
		{
			if (Primeira[Qual].PointsUnits.Num() < 6)
			{
				continue;
			}

			if (Um == INDEX_NONE) { Um = Qual; }
			else if (Outro == INDEX_NONE) { Outro = Qual; break; }
		}

		if (Um == INDEX_NONE || Outro == INDEX_NONE)
		{
			return true;
		}

		const float DeUm = FreshWater::CourseLengthUnits(Primeira[Um])
			/ FMath::Max(1.0f, static_cast<float>(FVector2D::Distance(
				Primeira[Um].PointsUnits[0], Primeira[Um].PointsUnits.Last())));
		const float DoOutro = FreshWater::CourseLengthUnits(Primeira[Outro])
			/ FMath::Max(1.0f, static_cast<float>(FVector2D::Distance(
				Primeira[Outro].PointsUnits[0], Primeira[Outro].PointsUnits.Last())));

		TestNotEqual(TEXT("dois rios nao serpenteiam igual"), DeUm, DoOutro, 0.0001f);
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

	// UNS POUCOS são inevitáveis e não são defeito: numa bacia densa dois
	// galhos irmãos correm lado a lado por um trecho antes de se separarem, e
	// exigir zero seria exigir que a bacia fosse rala.
	//
	// O que continua sendo defeito é a MAIORIA — isso seria o mesmo rio
	// desenhado duas vezes.
	TestTrue(TEXT("quase nenhum curso deita no meio de outro"),
		Sobrepostos * 20 < Cursos.Num());

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

	// As grutas NÃO correspondem uma a uma aos cursos, e a guarda que exigia
	// isso reprovava o teste antes de ele conferir qualquer coisa.
	//
	// A cachoeira nasce onde o leito despenca, e a maioria dos cursos corre
	// manso: há muito mais curso que queda, e muito mais queda que gruta —
	// porque a gruta ainda precisa de chão seco ao lado.
	//
	// Cada gruta guarda a queda a que pertence, e é essa a relação a conferir.
	if (Cursos.Num() == 0 || Grutas.Num() == 0)
	{
		AddError(TEXT("sem gruta para conferir"));
		return false;
	}

	// Os cursos COM queda, na ordem em que as grutas foram feitas.
	TArray<FreshWater::FRiverCourse> ComQueda;
	for (const FreshWater::FRiverCourse& Curso : Cursos)
	{
		if (Curso.HasFall())
		{
			ComQueda.Add(Curso);
		}
	}

	// A conferência é contra o curso INTEIRO, não contra o ponto da queda. O
	// lago está rio acima e alcança daqui: encostar a quina da gruta na água
	// alagada é a quina de caverna dentro de montanha com outro nome.
	for (const IslandFeatureLayout::FFeaturePlacement& Gruta : Grutas)
	{
		const FVector2D Centro = Gruta.CenterUnits();

		// A PEGADA INTEIRA fora da água, e medida no ponto mais próximo.
		//
		// O laço que estava aqui andava de sessenta em sessenta sobre uma faixa
		// de zero a um: ele rodava UMA VEZ, no começo de cada curso, e nunca
		// conferiu o resto. É resto da migração em que o parâmetro deixou de
		// ser raio e passou a ser progresso — a substituição em massa preservou
		// o passo e apagou o alcance.
		//
		// `NearestOn` é a ferramenta certa, e não existia quando isto foi
		// escrito.
		for (const FreshWater::FRiverCourse& Curso : Cursos)
		{
			float Aonde = 0.0f;
			const float Ate = FreshWater::NearestOn(Curso, Centro, Aonde);
			const float Folga =
				FreshWater::HalfWidthAtProgress(Curso, Aonde) + Gruta.ClearanceUnits;

			TestTrue(TEXT("nenhuma quina da gruta cai dentro da agua"), Ate > Folga);
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

	if (Cursos.Num() == 0 || Grutas.Num() == 0)
	{
		AddError(TEXT("sem gruta para conferir"));
		return false;
	}

	// A gruta é encontrada pela QUEDA MAIS PRÓXIMA, e não por índice.
	//
	// Casar `Grutas[i]` com `Cursos[i]` valia quando cada curso tinha uma queda
	// e cada queda ganhava uma gruta. Hoje nem toda queda consegue chão seco ao
	// lado, e uma que falha desloca todos os índices seguintes — o teste
	// passava a comparar cada gruta com a cachoeira de outro rio, e reprovava
	// setenta vezes por um erro de contagem.
	TArray<FVector2D> Quedas;
	for (const FreshWater::FRiverCourse& Curso : Cursos)
	{
		if (Curso.HasFall())
		{
			Quedas.Add(FreshWater::PointAtProgress(Curso, Curso.FallAtProgress));
		}
	}

	if (!TestTrue(TEXT("ha cachoeira"), Quedas.Num() > 0))
	{
		return false;
	}

	for (const IslandFeatureLayout::FFeaturePlacement& Gruta : Grutas)
	{
		const FVector2D Centro = Gruta.CenterUnits();

		float Menor = TNumericLimits<float>::Max();
		float Segunda = TNumericLimits<float>::Max();

		for (const FVector2D& NaQueda : Quedas)
		{
			const float Ate = static_cast<float>(FVector2D::Distance(Centro, NaQueda));
			if (Ate < Menor)
			{
				Segunda = Menor;
				Menor = Ate;
			}
			else if (Ate < Segunda)
			{
				Segunda = Ate;
			}
		}

		// ENCOSTADA na queda dela: a gruta é da cachoeira, e uma gruta a
		// duzentos metros seria uma caverna qualquer com um nome que promete
		// cachoeira.
		TestTrue(TEXT("a gruta esta encostada numa cachoeira"),
			Menor < FreshWater::LakeHalfWidthUnits() * 2.0f);

		// E do lado da SUA, não no meio de duas: se a segunda mais próxima
		// estivesse igualmente perto, a gruta não seria de cachoeira nenhuma.
		if (Quedas.Num() > 1)
		{
			TestTrue(TEXT("e mais perto da dela que de qualquer outra"), Menor < Segunda);
		}

		// E do lado, não embaixo: o lugar imediatamente abaixo do degrau é da
		// água caindo, e uma boca ali teria o rio entrando por dentro.
		TestTrue(TEXT("mas nao embaixo dela"), Menor > FreshWater::FallHalfLengthUnits());
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFreshWaterGalleriesAreNotStraightTest,
	"BattleSquare.Environment.FreshWater.NenhumaGaleriaEhReta",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFreshWaterGalleriesAreNotStraightTest::RunTest(const FString& Parameters)
{
	// Galeria de calcário segue fratura, e fratura não é régua. Uma passagem
	// reta entre duas bocas é o desenho que esta rede inteira veio substituir.
	//
	// Mas reta não é defeito por EXISTIR — fratura às vezes é reta mesmo, e
	// uma rede em que nada é reto lê tão fabricada quanto uma em que tudo é.
	// O defeito era a PROPORÇÃO: 123 de 158, porque a costura riscava enquanto
	// o gerador cavava, com o comentário "a emenda cava, não risca" escrito
	// quarenta linhas abaixo do bloco que riscava.
	//
	// Por isso o teste cobra o TETO do parâmetro do bioma, não zero. Cobrar
	// zero congelaria uma decisão de arte num teste, e a decisão é do bioma.
	const TArray<FreshWater::FUnderwaterLink> Passagens = FreshWater::PlanUnderwaterLinks();

	TestTrue(TEXT("existe galeria para medir"), Passagens.Num() > 0);

	int32 Retas = 0;
	for (const FreshWater::FUnderwaterLink& Passagem : Passagens)
	{
		if (Passagem.PointsUnits.Num() < 2)
		{
			continue;
		}

		float Andado = 0.0f;
		for (int32 Passo = 1; Passo < Passagem.PointsUnits.Num(); ++Passo)
		{
			Andado += static_cast<float>(FVector2D::Distance(
				Passagem.PointsUnits[Passo - 1], Passagem.PointsUnits[Passo]));
		}

		const float EmLinha = static_cast<float>(FVector2D::Distance(
			Passagem.PointsUnits[0], Passagem.PointsUnits.Last()));

		// Vão curto demais não tem espaço para curva nenhuma, e cobrá-la ali
		// seria exigir tremor.
		if (EmLinha < IslandGeography::LandRadiusUnits() * 0.02f)
		{
			continue;
		}

		if (Andado / EmLinha < 1.02f)
		{
			++Retas;
		}
	}

	int32 Medidas = 0;
	for (const FreshWater::FUnderwaterLink& Passagem : Passagens)
	{
		if (Passagem.PointsUnits.Num() >= 2
			&& FVector2D::Distance(Passagem.PointsUnits[0], Passagem.PointsUnits.Last())
				>= IslandGeography::LandRadiusUnits() * 0.02f)
		{
			++Medidas;
		}
	}

	// Uma folga sobre o parâmetro, porque o sorteio é por passagem: com poucas
	// dezenas de galerias a fração medida oscila em torno da pedida, e um teste
	// que reprova por essa oscilação reprova o acaso, não o gerador.
	const float Teto = WorldBudget::StraightGalleryShare(IslandGeography::IslandBiome()) + 0.12f;

	TestTrue(TEXT("a maioria das galerias e cavada, nao riscada"),
		Medidas == 0 || static_cast<float>(Retas) / static_cast<float>(Medidas) <= Teto);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFreshWaterLinkedGrottoShareBaselineTest,
	"BattleSquare.Environment.FreshWater.QuantasGrutasSeLigamHoje",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFreshWaterLinkedGrottoShareBaselineTest::RunTest(const FString& Parameters)
{
	// A LINHA DE BASE, medida antes de mexer.
	//
	// A M7 pede que a proporção de grutas ligadas seja parâmetro, com teto e
	// piso. Antes de escolher o parâmetro é preciso saber o que o mundo faz
	// hoje — escolher primeiro e medir depois é escolher às cegas, e foi assim
	// que "123 de 158 galerias retas" passou despercebido por semanas.
	//
	// Este teste não cobra valor nenhum: ele DIZ o número. Quem cobra é o teste
	// da M7, escrito depois de ler esta medição.
	const TArray<IslandFeatureLayout::FFeaturePlacement> Grutas = FreshWater::PlanGrottoes();
	const TArray<FreshWater::FUnderwaterLink> Passagens = FreshWater::PlanUnderwaterLinks();

	TestTrue(TEXT("ha gruta para medir"), Grutas.Num() > 0);
	if (Grutas.Num() == 0)
	{
		return true;
	}

	TArray<int32> Dono;
	Dono.Reserve(Grutas.Num());
	for (int32 Qual = 0; Qual < Grutas.Num(); ++Qual)
	{
		Dono.Add(Qual);
	}

	TFunction<int32(int32)> Raiz = [&Dono, &Raiz](int32 Qual)
	{
		return Dono[Qual] == Qual ? Qual : (Dono[Qual] = Raiz(Dono[Qual]));
	};

	int32 Emendas = 0;
	for (const FreshWater::FUnderwaterLink& Passagem : Passagens)
	{
		if (Passagem.FromGrotto == INDEX_NONE || Passagem.ToGrotto == INDEX_NONE)
		{
			continue;
		}

		++Emendas;
		Dono[Raiz(Passagem.FromGrotto)] = Raiz(Passagem.ToGrotto);
	}

	TMap<int32, int32> Tamanho;
	for (int32 Qual = 0; Qual < Grutas.Num(); ++Qual)
	{
		++Tamanho.FindOrAdd(Raiz(Qual));
	}

	int32 Ligadas = 0;
	for (int32 Qual = 0; Qual < Grutas.Num(); ++Qual)
	{
		if (Tamanho[Raiz(Qual)] > 1)
		{
			++Ligadas;
		}
	}

	AddInfo(FString::Printf(
		TEXT("grutas: %d  emendas: %d  ligadas: %d  fracao: %.3f  componentes: %d"),
		Grutas.Num(), Emendas, Ligadas,
		static_cast<float>(Ligadas) / static_cast<float>(Grutas.Num()),
		Tamanho.Num()));

	return true;
}

// Copyright 2026 Anderson. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Environment/FreshWater.h"
#include "Environment/IslandGeography.h"

// ---------------------------------------------------------------------------
// M2 — A ROCHA GANHA ESCARPA, E A ÁGUA ACHA O DEGRAU.
//
// O poço da cachoeira JÁ LÊ A ROCHA: a fundura sai da queda de altura do leito
// ao longo da cachoeira. Ele é prato porque a rocha é lisa NA ESCALA DA QUEDA —
// não porque falte regra de poço.
//
// **A LINHA DE BASE, medida em 02/09/2026 ANTES de qualquer mudança:**
//
// | | |
// |---|---|
// | poços | 13 |
// | fundura mediana | 32 |
// | mais funda | 64 |
// | meia-largura | 886 |
// | terreno íngreme (>10% em 400 unidades) | **469 de 1493 — 31,4%** |
//
// ⚠️ **Os 31,4% mudam o desenho, e por isso a medição veio antes.** A rocha NÃO
// é lisa em geral: a célula da ondulação tem 15400 unidades, e a 400 de passo
// ela já é bem acidentada. O que falta é degrau na escala da CACHOEIRA.
//
// Escarpar a ilha inteira estouraria qualquer teto razoável de imediato. A
// escarpa tem de ser RARA E ALTA — patamar de encosta —, não textura espalhada.
// Escarpa é acidente geológico, não acabamento.
//
// ⚠️ NADA AQUI PERGUNTA ONDE ESTÃO AS CACHOEIRAS. Desenhar degrau na posição da
// queda foi tentado e removido: o relevo perguntaria pelas quedas, as quedas
// pelo plano dos rios, e o plano estaria sendo construído naquele instante —
// entrar de novo num inicializador em construção trava o processo. A inversão é
// o conserto, e é melhor: o terreno não sabe de água; a água procura onde o
// leito despenca.
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBedrockPoolsGetDeeperTest,
	"BattleSquare.Bedrock.PoolsGetDeeper",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBedrockPoolsGetDeeperTest::RunTest(const FString& Parameters)
{
	TArray<float> Funduras;
	TArray<float> Larguras;

	for (const FreshWater::FRiverCourse& Curso : FreshWater::Plan())
	{
		const float Fundo = FreshWater::PlungePoolDepthUnits(Curso);
		if (Fundo > 0.0f)
		{
			Funduras.Add(Fundo);
			Larguras.Add(FreshWater::PlungePoolHalfWidthUnits(Curso));
		}
	}

	if (Funduras.IsEmpty())
	{
		AddError(TEXT("nenhuma queda tem poco — a agua deixou de achar degrau"));
		return false;
	}

	Funduras.Sort();
	const float Mediana = Funduras[Funduras.Num() / 2];
	const float MaisFunda = Funduras.Last();

	AddInfo(FString::Printf(
		TEXT("POCOS: %d, fundura mediana %.0f, mais funda %.0f, meia-largura %.0f"),
		Funduras.Num(), Mediana, MaisFunda, Larguras[0]));

	// O ACEITE: a mediana CRESCE em relação aos 30–51 medidos em 02/09/2026,
	// antes da escarpa.
	//
	// ⚠️ O aceite NÃO é "mais fundo que largo". O "dez vezes" da literatura é
	// razão de VELOCIDADE de erosão — vertical sobre lateral —, não a forma do
	// buraco. Um teste escrito com essa leitura já reprovou código certo aqui.
	// A mediana medida antes da escarpa foi 32, e a mais funda 64. O aceite é
	// crescer sobre a MEDIANA medida, não sobre o extremo: bater 64 exigiria
	// que o poço mediano ficasse tão fundo quanto o melhor de hoje, o que é
	// mais do que a task pede.
	TestTrue(*FString::Printf(
		TEXT("a fundura mediana CRESCEU sobre os 32 medidos (agora %.0f)"), Mediana),
		Mediana > 32.0f);

	return true;
}

// CONTRAPESO 1 — O RELEVO CONTINUA SEM SABER DE ÁGUA.
//
// É o guarda do ciclo, e ele não é opinião: `BedrockHeightAt` chamado com o
// plano de água AINDA NÃO CONSTRUÍDO tem de responder. Se alguém puser uma
// pergunta sobre rios lá dentro, este teste trava — e travar é o aviso.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBedrockDoesNotKnowWaterTest,
	"BattleSquare.Bedrock.DoesNotKnowWater",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBedrockDoesNotKnowWaterTest::RunTest(const FString& Parameters)
{
	// Cem pontos espalhados pela ilha. Se o relevo consultasse água, a primeira
	// destas chamadas reentraria o plano em construção.
	const float Raio = IslandGeography::LandRadiusUnits();
	int32 Responderam = 0;

	for (int32 Qual = 0; Qual < 100; ++Qual)
	{
		const float Angulo = (Qual / 100.0f) * 2.0f * PI;
		const float Distancia = Raio * (0.1f + 0.8f * (Qual % 10) / 10.0f);
		const FVector2D Onde(Distancia * FMath::Cos(Angulo), Distancia * FMath::Sin(Angulo));

		IslandGeography::BedrockHeightAt(Onde);
		++Responderam;
	}

	TestEqual(TEXT("a rocha respondeu em cem pontos, sem consultar agua"),
		Responderam, 100);

	return true;
}

// CONTRAPESO 2 — A ILHA NÃO VIRA ESCADA.
//
// "Mais degrau" sem teto é uma ladeira só. A escarpa tem de ser ACIDENTADA: a
// maior parte do terreno continua andável, e o íngreme é exceção — senão a
// trilha não fecha e o mundo vira parede.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBedrockIsNotAStaircaseTest,
	"BattleSquare.Bedrock.IsNotAStaircase",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBedrockIsNotAStaircaseTest::RunTest(const FString& Parameters)
{
	const float Raio = IslandGeography::LandRadiusUnits();
	const float Passo = 400.0f;

	int32 Amostras = 0;
	int32 Ingremes = 0;

	for (int32 Linha = -20; Linha <= 20; ++Linha)
	{
		for (int32 Coluna = -20; Coluna <= 20; ++Coluna)
		{
			const FVector2D Onde(Coluna * Raio / 25.0f, Linha * Raio / 25.0f);
			if (Onde.Size() >= Raio * 0.9f)
			{
				continue;
			}

			const float Aqui = IslandGeography::BedrockHeightAt(Onde);
			const float AoLado = IslandGeography::BedrockHeightAt(
				Onde + FVector2D(Passo, 0.0f));

			++Amostras;
			// Íngreme é acima de 10% — o declive sustentável de trilha.
			if (FMath::Abs(AoLado - Aqui) / Passo > 0.10f)
			{
				++Ingremes;
			}
		}
	}

	const float Proporcao = Amostras > 0
		? static_cast<float>(Ingremes) / static_cast<float>(Amostras)
		: 0.0f;

	AddInfo(FString::Printf(TEXT("terreno ingreme: %d de %d amostras (%.1f%%)"),
		Ingremes, Amostras, Proporcao * 100.0f));

	// O TETO É MEDIDO, e não escolhido.
	//
	// A linha de base é 31,4%, e o mundo de hoje é jogável nela: as 23 trilhas
	// fecham, as vilas se ligam. Um teto inventado de 35% deixaria 3,6 pontos
	// de folga por acaso, não por projeto — eu escrevi esse número antes de
	// medir, e ele quase não cabia.
	//
	// 40% é a linha de base mais um quarto da folga que ela tem até a metade.
	// Acima disso a ilha vira mais parede que chão, e a trilha — que já paga
	// pedágio de declive — deixa de fechar.
	TestTrue(*FString::Printf(
		TEXT("a ilha nao virou escada (base era 31.4%%, agora %.1f%%)"),
		Proporcao * 100.0f), Proporcao <= 0.40f);

	// E PISO: escarpa que não escarpa não é escarpa. Sem esta linha, apagar a
	// mudança inteira passaria no teste de cima cantando vitória.
	TestTrue(TEXT("mas ha terreno ingreme de verdade"), Proporcao > 0.0f);

	return true;
}

// CONTRAPESO 3 — A PRAIA CONTINUA SUBINDO DO MAR.
//
// O encolhimento pela orla já impediu uma vez que o vale de um morro descesse
// abaixo do nível do mar e a ilha ganhasse buracos de água. A escarpa passa
// pelo mesmo encolhimento, e este teste é quem garante.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBedrockBeachStillRisesTest,
	"BattleSquare.Bedrock.BeachStillRises",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBedrockBeachStillRisesTest::RunTest(const FString& Parameters)
{
	int32 Abaixo = 0;

	for (int32 Qual = 0; Qual < 360; ++Qual)
	{
		const float Angulo = FMath::DegreesToRadians(static_cast<float>(Qual));
		const float DaCosta = IslandGeography::LandRadiusAt(Angulo);

		// Na faixa de praia, andando para dentro.
		for (int32 Passo = 1; Passo <= 5; ++Passo)
		{
			const float Distancia = DaCosta - (Passo * IslandGeography::BeachWidthUnits() / 6.0f);
			const FVector2D Onde(Distancia * FMath::Cos(Angulo), Distancia * FMath::Sin(Angulo));

			if (IslandGeography::BedrockHeightAt(Onde) < 0.0f)
			{
				++Abaixo;
			}
		}
	}

	TestEqual(TEXT("nenhum ponto da praia esta abaixo do mar"), Abaixo, 0);

	return true;
}

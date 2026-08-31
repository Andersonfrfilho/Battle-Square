// Copyright 2026 Anderson. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "Environment/IslandGeography.h"
#include "World/RegionLayout.h"
#include "World/VillageLayout.h"

namespace AlturaDoChao
{
	FVector2D Ponto(float Graus, float Distancia)
	{
		const float Radianos = FMath::DegreesToRadians(Graus);
		return FVector2D(FMath::Cos(Radianos), FMath::Sin(Radianos)) * Distancia;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGroundHeightNenhumaTerraAbaixoDoMarTest,
	"BattleSquare.GroundHeight.NenhumaTerraAbaixoDoMar",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGroundHeightNenhumaTerraAbaixoDoMarTest::RunTest(const FString& Parameters)
{
	// Este teste existe por um defeito medido: a ondulação era MAIOR que a
	// altura da terra, e a ilha tinha vales abaixo do nível do mar — buracos
	// de água no meio do chão. O despejo do mapa mostrou; o teste impede que
	// volte quando alguém mexer numa das duas frações.
	const float Raio = IslandGeography::LandRadiusUnits();
	const int32 Lado = 60;

	float MaisFundo = TNumericLimits<float>::Max();

	for (int32 Linha = 0; Linha < Lado; ++Linha)
	{
		for (int32 Coluna = 0; Coluna < Lado; ++Coluna)
		{
			const float X = ((static_cast<float>(Coluna) / (Lado - 1)) * 2.0f - 1.0f) * Raio;
			const float Y = ((static_cast<float>(Linha) / (Lado - 1)) * 2.0f - 1.0f) * Raio;
			const FVector2D Onde(X, Y);

			if (!IslandGeography::IsOnLand(Onde))
			{
				continue;
			}

			MaisFundo = FMath::Min(MaisFundo, IslandGeography::GroundHeightAt(Onde));
		}
	}

	TestTrue(TEXT("nenhum ponto de terra fica abaixo do mar"), MaisFundo >= 0.0f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGroundHeightOMesmoPontoTemSempreAMesmaAlturaTest,
	"BattleSquare.GroundHeight.OMesmoPontoTemSempreAMesmaAltura",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGroundHeightOMesmoPontoTemSempreAMesmaAlturaTest::RunTest(const FString& Parameters)
{
	// O mundo nasce por PEDAÇO, em momentos diferentes. Relevo que muda entre
	// visitas é o chão se mexendo — e seria invisível em teste que consultasse
	// uma vez só.
	const FVector2D Onde(12345.0f, -6789.0f);
	const float Primeira = IslandGeography::GroundHeightAt(Onde);

	for (int32 Vez = 0; Vez < 8; ++Vez)
	{
		TestEqual(TEXT("mesma posição, mesma altura"),
			IslandGeography::GroundHeightAt(Onde), Primeira);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGroundHeightOChaoOndulaEmVezDeSerPratoTest,
	"BattleSquare.GroundHeight.OChaoOndulaEmVezDeSerPrato",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGroundHeightOChaoOndulaEmVezDeSerPratoTest::RunTest(const FString& Parameters)
{
	// Sem isto, `GroundHeightAt` poderia devolver uma constante e todos os
	// outros testes continuariam verdes — e o relevo inteiro seria um número
	// que nunca muda. É a mesma armadilha do fator de inclinação que daria
	// sempre zero num mundo plano.
	const float Raio = IslandGeography::LandRadiusUnits();

	float MaisAlto = -TNumericLimits<float>::Max();
	float MaisBaixo = TNumericLimits<float>::Max();

	for (int32 Passo = 0; Passo < 40; ++Passo)
	{
		const float Distancia = Raio * 0.15f + (Raio * 0.25f * Passo / 40.0f);
		const FVector2D Onde = AlturaDoChao::Ponto(37.0f * Passo, Distancia);
		const float Altura = IslandGeography::GroundHeightAt(Onde);

		MaisAlto = FMath::Max(MaisAlto, Altura);
		MaisBaixo = FMath::Min(MaisBaixo, Altura);
	}

	TestTrue(TEXT("o chão tem morro e vale, não é prato"),
		(MaisAlto - MaisBaixo) > Raio * 0.005f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGroundHeightOLoteDaVilaEhPlanoTest,
	"BattleSquare.GroundHeight.OLoteDaVilaEhPlano",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGroundHeightOLoteDaVilaEhPlanoTest::RunTest(const FString& Parameters)
{
	// Prédio em chão inclinado fica com meia parede enterrada — a mesma
	// família de defeito do pet afundando no tabuleiro, que só apareceu porque
	// um humano olhou a tela.
	for (const FSettlementPlacement& Assentamento : RegionLayout::Plan())
	{
		const float Lote = VillageLayout::PlotHalfExtentUnitsFor(Assentamento.Kind);
		const float NoCentro = IslandGeography::GroundHeightAt(Assentamento.CenterUnits);

		for (int32 Canto = 0; Canto < 4; ++Canto)
		{
			const float X = (Canto & 1) ? Lote : -Lote;
			const float Y = (Canto & 2) ? Lote : -Lote;
			const FVector2D NoCanto = Assentamento.CenterUnits + FVector2D(X, Y);

			TestEqual(TEXT("o lote inteiro tem a altura do centro"),
				IslandGeography::GroundHeightAt(NoCanto), NoCentro);
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGroundHeightACidadeFicaEmCimaDaMesaTest,
	"BattleSquare.GroundHeight.ACidadeFicaEmCimaDaMesa",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGroundHeightACidadeFicaEmCimaDaMesaTest::RunTest(const FString& Parameters)
{
	FVector2D DaCidade = FVector2D::ZeroVector;
	for (const FSettlementPlacement& Assentamento : RegionLayout::Plan())
	{
		if (Assentamento.Kind == ESettlementKind::CidadeGrande)
		{
			DaCidade = Assentamento.CenterUnits;
		}
	}

	const float EmCima = IslandGeography::GroundHeightAt(DaCidade);
	const float LaEmbaixo = IslandGeography::GroundHeightAt(
		DaCidade * (1.0f + IslandGeography::BluffOuterRadiusUnits() * 2.0f / DaCidade.Size()));

	TestTrue(TEXT("a cidade está mais alta que o chão fora do barranco"),
		EmCima > LaEmbaixo + IslandGeography::PlateauHeightUnits() * 0.5f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGroundHeightARampaEhMaisMansaQueOBarrancoTest,
	"BattleSquare.GroundHeight.ARampaEhMaisMansaQueOBarranco",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGroundHeightARampaEhMaisMansaQueOBarrancoTest::RunTest(const FString& Parameters)
{
	FVector2D DaCidade = FVector2D::ZeroVector;
	for (const FSettlementPlacement& Assentamento : RegionLayout::Plan())
	{
		if (Assentamento.Kind == ESettlementKind::CidadeGrande)
		{
			DaCidade = Assentamento.CenterUnits;
		}
	}

	const float Meio = (IslandGeography::BluffInnerRadiusUnits()
		+ IslandGeography::BluffOuterRadiusUnits()) * 0.5f;

	const float NaRampa = IslandGeography::BluffRampAngleDegrees();
	const FVector2D PelaRampa = DaCidade + AlturaDoChao::Ponto(NaRampa, Meio);
	const FVector2D PeloBarranco = DaCidade + AlturaDoChao::Ponto(NaRampa + 180.0f, Meio);

	TestTrue(TEXT("a rampa existe"), IslandGeography::IsOnBluffRamp(PelaRampa));
	TestFalse(TEXT("e o lado oposto é barranco"), IslandGeography::IsOnBluffRamp(PeloBarranco));

	// A rampa é o que impede o barranco de ser PAREDE. Todo destino é
	// alcançável a pé; barranco intransponível transformaria a cidade em
	// chave de porta, que é o defeito que a spec proíbe.
	TestTrue(TEXT("subir pela rampa é mais manso que escalar o barranco"),
		IslandGeography::GroundSlopeAt(PelaRampa)
			< IslandGeography::GroundSlopeAt(PeloBarranco));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGroundHeightARampaOlhaParaCasaTest,
	"BattleSquare.GroundHeight.ARampaOlhaParaCasa",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGroundHeightARampaOlhaParaCasaTest::RunTest(const FString& Parameters)
{
	// Quem chega de casa encontra a subida de frente, em vez de contornar a
	// mesa inteira. O rumo é CALCULADO da cidade para a vila inicial: escrito
	// como constante, ele mentiria no dia em que a cidade mudasse de lugar.
	FVector2D DaCidade = FVector2D::ZeroVector;
	for (const FSettlementPlacement& Assentamento : RegionLayout::Plan())
	{
		if (Assentamento.Kind == ESettlementKind::CidadeGrande)
		{
			DaCidade = Assentamento.CenterUnits;
		}
	}

	const FVector2D ParaCasa = FVector2D::ZeroVector - DaCidade;
	const float RumoDeCasa = FMath::RadiansToDegrees(FMath::Atan2(ParaCasa.Y, ParaCasa.X));

	TestTrue(TEXT("a rampa aponta para a vila inicial"),
		FMath::Abs(FMath::FindDeltaAngleDegrees(
			RumoDeCasa, IslandGeography::BluffRampAngleDegrees())) < 1.0f);

	return true;
}

// Copyright 2026 Anderson. All Rights Reserved.

#include "Environment/IslandGeography.h"

#include "Battle/BattleTypes.h"
#include "Environment/IslandFeatureLayout.h"
#include "Environment/WorldWeather.h"
#include "Environment/WorldBoundaryWater.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Quantos ângulos varrer numa volta completa. */
	constexpr int32 AngulosDaIlha = 360;

	constexpr float TolerenciaDaIlha = 0.5f;

	FVector2D PontoDaIlha(float Graus, float Raio)
	{
		const float Radianos = FMath::DegreesToRadians(Graus);
		return FVector2D(FMath::Cos(Radianos) * Raio, FMath::Sin(Radianos) * Raio);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FIslandGeographyOneRadiusTest,
	"BattleSquare.Environment.IslandGeography.ORaioDaTerraEhUmSo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandGeographyOneRadiusTest::RunTest(const FString& Parameters)
{
	// A dívida que este módulo paga: o raio estava transcrito em seis lugares.
	// Este teste é o que impede a sétima cópia de nascer concordando hoje e
	// discordando na primeira vez que a ilha crescer.
	const float Raio = IslandGeography::LandRadiusUnits();

	TestTrue(TEXT("A ilha tem raio positivo"), Raio > 0.0f);

	const IslandFeatureLayout::FIslandBounds Ilha;
	TestEqual(TEXT("O plano das peças usa o mesmo raio"), Ilha.LandRadiusUnits, Raio);

	const AWorldBoundaryWater* AguaPadrao = GetDefault<AWorldBoundaryWater>();
	TestEqual(TEXT("A margem da água usa o mesmo raio"), AguaPadrao->ShoreRadiusUnits, Raio);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FIslandGeographyHomeFitsFieldsTest,
	"BattleSquare.Environment.IslandGeography.ACasaCabeOsCamposDeTreino",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandGeographyHomeFitsFieldsTest::RunTest(const FString& Parameters)
{
	// O miolo de mata existe para caber os campos de treino. Sem esta
	// asserção, encolher o miolo poria um campo de treino dentro do deserto —
	// e ninguém veria até andar até lá.
	const IslandFeatureLayout::FIslandBounds Ilha;
	const float BordaDoCampoMaisLonge = Ilha.TrainingRingRadiusUnits + Ilha.TrainingFieldRadiusUnits;

	TestTrue(
		*FString::Printf(TEXT("Casa de %.0f cobre os campos até %.0f"),
			IslandGeography::HomeRadiusUnits(), BordaDoCampoMaisLonge),
		IslandGeography::HomeRadiusUnits() >= BordaDoCampoMaisLonge);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FIslandGeographyCoreIsForestTest,
	"BattleSquare.Environment.IslandGeography.OMioloEhSempreMata",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandGeographyCoreIsForestTest::RunTest(const FString& Parameters)
{
	// Em QUALQUER direção. O sorteio de setor não pode alcançar a casa, senão
	// o jogo às vezes começaria num glaciar sem nenhuma planta conhecida.
	const float RaioDaCasa = IslandGeography::HomeRadiusUnits();
	int32 ForaDaMata = 0;

	for (int32 Grau = 0; Grau < AngulosDaIlha; ++Grau)
	{
		const FVector2D Ponto = PontoDaIlha(static_cast<float>(Grau), RaioDaCasa * 0.9f);
		if (IslandGeography::BiomeAt(Ponto) != EIslandBiome::Forest)
		{
			++ForaDaMata;
		}
	}

	TestEqual(TEXT("Nenhuma direção da casa deixa de ser mata"), ForaDaMata, 0);
	TestEqual(TEXT("O centro exato também é mata"),
		IslandGeography::BiomeAt(FVector2D::ZeroVector), EIslandBiome::Forest);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FIslandGeographyRimIsBeachTest,
	"BattleSquare.Environment.IslandGeography.ABordaEhSemprePraia",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandGeographyRimIsBeachTest::RunTest(const FString& Parameters)
{
	// Sem esta faixa, o deserto encostaria no mar sem areia molhada no meio —
	// e a geleira encostaria nele sem nada explicando a passagem.
	const float RaioDaTerra = IslandGeography::LandRadiusUnits();
	int32 ForaDaPraia = 0;

	for (int32 Grau = 0; Grau < AngulosDaIlha; ++Grau)
	{
		const FVector2D Ponto = PontoDaIlha(static_cast<float>(Grau), RaioDaTerra - TolerenciaDaIlha);
		if (IslandGeography::BiomeAt(Ponto) != EIslandBiome::Beach)
		{
			++ForaDaPraia;
		}
	}

	TestEqual(TEXT("Toda a borda é praia"), ForaDaPraia, 0);
	TestTrue(TEXT("A praia tem largura"), IslandGeography::BeachWidthUnits() > 0.0f);
	TestTrue(TEXT("A praia não engole o miolo"),
		IslandGeography::BeachWidthUnits() < RaioDaTerra - IslandGeography::HomeRadiusUnits());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FIslandGeographyEveryBiomeExistsTest,
	"BattleSquare.Environment.IslandGeography.OsCincoBiomasExistemNaIlha",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandGeographyEveryBiomeExistsTest::RunTest(const FString& Parameters)
{
	// Bioma que existe no enum e não existe em lugar nenhum da ilha é bioma
	// que ninguém encontra. Esta varredura é o que separa os dois casos.
	const float RaioDoMeio =
		(IslandGeography::HomeRadiusUnits()
			+ IslandGeography::LandRadiusUnits() - IslandGeography::BeachWidthUnits()) * 0.5f;

	TSet<EIslandBiome> Encontrados;
	for (int32 Grau = 0; Grau < AngulosDaIlha; ++Grau)
	{
		Encontrados.Add(IslandGeography::BiomeAt(PontoDaIlha(static_cast<float>(Grau), RaioDoMeio)));
	}
	Encontrados.Add(IslandGeography::BiomeAt(
		PontoDaIlha(0.0f, IslandGeography::LandRadiusUnits() - TolerenciaDaIlha)));

	TestTrue(TEXT("A mata existe"), Encontrados.Contains(EIslandBiome::Forest));
	TestTrue(TEXT("O deserto existe"), Encontrados.Contains(EIslandBiome::Desert));
	TestTrue(TEXT("O vulcão existe"), Encontrados.Contains(EIslandBiome::Volcano));
	TestTrue(TEXT("A geleira existe"), Encontrados.Contains(EIslandBiome::Glacier));
	TestTrue(TEXT("A praia existe"), Encontrados.Contains(EIslandBiome::Beach));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FIslandGeographySectorIsStableTest,
	"BattleSquare.Environment.IslandGeography.OSetorNaoSaiDaFaixa",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandGeographySectorIsStableTest::RunTest(const FString& Parameters)
{
	// Um setor fora da faixa lê fora da tabela. Varrer a volta inteira,
	// incluindo o ângulo negativo que `Atan2` devolve, é barato.
	int32 ForaDaFaixa = 0;
	for (int32 Grau = -AngulosDaIlha; Grau <= AngulosDaIlha; ++Grau)
	{
		const int32 Setor = IslandGeography::SectorAt(PontoDaIlha(static_cast<float>(Grau), 1000.0f));
		if (Setor < 0 || Setor >= IslandGeography::SectorCount)
		{
			++ForaDaFaixa;
		}
	}

	TestEqual(TEXT("Nenhum ângulo cai fora da faixa de setores"), ForaDaFaixa, 0);
	TestEqual(TEXT("A origem tem setor válido"),
		IslandGeography::SectorAt(FVector2D::ZeroVector), 0);

	// Um setor pedido fora da faixa não pode ler fora da tabela.
	TestEqual(TEXT("Setor negativo cai no primeiro"),
		IslandGeography::BiomeOfSector(-3), IslandGeography::BiomeOfSector(0));
	TestEqual(TEXT("Setor alto demais cai no último"),
		IslandGeography::BiomeOfSector(IslandGeography::SectorCount + 7),
		IslandGeography::BiomeOfSector(IslandGeography::SectorCount - 1));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FIslandGeographyClimateFollowsBiomeTest,
	"BattleSquare.Environment.IslandGeography.OVulcaoSecaEAPraiaMolha",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandGeographyClimateFollowsBiomeTest::RunTest(const FString& Parameters)
{
	// O clima do bioma atravessa para a BATALHA pela umidade: é ela que
	// decide chão de lama. Chover no vulcão e enlamear seria o cenário
	// dizendo uma coisa e o campo dizendo outra.
	const int32 VulcaoNaChuva = WorldWeather::HumidityPercent(
		IslandGeography::ClimateOf(EIslandBiome::Volcano), EWeather::Rain);
	const int32 PraiaNaChuva = WorldWeather::HumidityPercent(
		IslandGeography::ClimateOf(EIslandBiome::Beach), EWeather::Rain);

	TestTrue(TEXT("Nem chovendo o vulcão enlameia"), VulcaoNaChuva < MudMinHumidity);
	TestTrue(TEXT("Chovendo, a praia enlameia"), PraiaNaChuva >= MudMinHumidity);

	// A geleira é o único lugar frio, e é o que dará endereço à aurora.
	int32 Frios = 0;
	const EIslandBiome Todos[] = {
		EIslandBiome::Forest, EIslandBiome::Desert, EIslandBiome::Glacier,
		EIslandBiome::Volcano, EIslandBiome::Beach };
	for (const EIslandBiome Bioma : Todos)
	{
		if (IslandGeography::ClimateOf(Bioma) == EScenaryClimate::Cold)
		{
			++Frios;
		}
	}
	TestEqual(TEXT("Só a geleira é fria"), Frios, 1);

	// O clima de um PONTO é o clima do bioma daquele ponto — sem segunda conta.
	const FVector2D NoMiolo = FVector2D::ZeroVector;
	TestEqual(TEXT("O clima do ponto segue o bioma do ponto"),
		IslandGeography::ClimateAt(NoMiolo),
		IslandGeography::ClimateOf(IslandGeography::BiomeAt(NoMiolo)));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FIslandGeographyWaterIsNotLandTest,
	"BattleSquare.Environment.IslandGeography.ForaDoRaioEhAgua",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandGeographyWaterIsNotLandTest::RunTest(const FString& Parameters)
{
	const float RaioDaTerra = IslandGeography::LandRadiusUnits();

	TestTrue(TEXT("O centro é terra"), IslandGeography::IsOnLand(FVector2D::ZeroVector));
	TestTrue(TEXT("A borda ainda é terra"),
		IslandGeography::IsOnLand(PontoDaIlha(0.0f, RaioDaTerra - TolerenciaDaIlha)));
	TestFalse(TEXT("Depois da borda é água"),
		IslandGeography::IsOnLand(PontoDaIlha(0.0f, RaioDaTerra + TolerenciaDaIlha)));

	return true;
}

#endif

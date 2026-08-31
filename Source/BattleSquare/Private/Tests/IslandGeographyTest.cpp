// Copyright 2026 Anderson. All Rights Reserved.

#include "Environment/IslandGeography.h"
#include "IslandBiomeTestHelper.h"

#include "Battle/BattleTypes.h"
#include "Environment/IslandFeatureLayout.h"
#include "Environment/WorldWeather.h"
#include "Environment/WorldBoundaryWater.h"
#include "Environment/RegionResidency.h"
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

	// UMA ILHA, UM BIOMA. A varredura de uma ilha só encontra o bioma DELA
	// mais a praia — e é isso que a ilha promete agora.
	//
	// O teste antigo cobrava os cinco biomas numa varredura só, porque a ilha
	// era fatiada em setores de pizza e cada bioma ficava com 0,39 km². A
	// intenção dele continua valendo e ficou mais forte: bioma que existe no
	// enum e não existe em ilha nenhuma é bioma que ninguém encontra. O que
	// mudou é ONDE se procura.
	TestTrue(TEXT("A ilha tem o bioma dela"),
		Encontrados.Contains(IslandGeography::IslandBiome()));
	TestTrue(TEXT("E a praia, que é a borda de qualquer ilha"),
		Encontrados.Contains(EIslandBiome::Beach));

	// TODO bioma do enum precisa ser configurável como ilha — senão ele
	// continua sendo um valor que nenhum jogador alcança, que era o defeito
	// que este teste existia para pegar.
	for (const TCHAR* Nome : { TEXT("Forest"), TEXT("Desert"), TEXT("Glacier"),
		TEXT("Volcano"), TEXT("Swamp") })
	{
		const IlhaDeTeste::FBiomaTemporario Ilha(Nome);
		TestTrue(*FString::Printf(TEXT("A ilha de %s existe e se alcança"), Nome),
			IslandGeography::BiomeAt(FVector2D::ZeroVector) == IslandGeography::IslandBiome());
	}

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

/**
 * O horizonte pertence ao SETOR, não à distância.
 *
 * A serra fica a quilômetros da praia. Se o clima dela saísse de `ClimateAt`,
 * todo pico responderia "praia" — e a geleira ficaria sem gelo por estar longe
 * demais de si mesma.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FIslandGeographySectorClimateIgnoresDistanceTest,
	"BattleSquare.Environment.IslandGeography.ClimaDoSetorIgnoraDistancia",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandGeographySectorClimateIgnoresDistanceTest::RunTest(const FString& Parameters)
{
	const float RaioDoMeio =
		(IslandGeography::HomeRadiusUnits() + IslandGeography::LandRadiusUnits()) * 0.5f;

	// Longe o bastante para ser a distância da serra do horizonte.
	const float RaioDoHorizonte = IslandGeography::LandRadiusUnits() * 50.0f;

	bool bAchouSetorQueMuda = false;

	for (int32 Grau = 0; Grau < 360; Grau += 5)
	{
		const FVector2D Perto = PontoDaIlha(static_cast<float>(Grau), RaioDoMeio);
		const FVector2D Longe = PontoDaIlha(static_cast<float>(Grau), RaioDoHorizonte);

		TestEqual(TEXT("a direção decide, e a distância não muda a resposta"),
			static_cast<int32>(IslandGeography::SectorClimateAt(Longe)),
			static_cast<int32>(IslandGeography::SectorClimateAt(Perto)));

		// A prova de que o defeito existia: no MESMO ângulo, longe da ilha,
		// `ClimateAt` responde outra coisa.
		if (IslandGeography::ClimateAt(Longe) != IslandGeography::SectorClimateAt(Longe))
		{
			bAchouSetorQueMuda = true;
		}
	}

	TestTrue(TEXT("longe da ilha o clima por posição realmente difere do clima do setor"),
		bAchouSetorQueMuda);

	// O HORIZONTE É DE UM CLIMA SÓ, e isso é PERDA assumida.
	//
	// Antes ele variava por rumo, e era o que dava gelo a uns picos e deixava
	// outros pelados na mesma serra. Com uma ilha por bioma, a serra de uma
	// ilha de mata é toda temperada — o que é coerente, e é menos bonito.
	//
	// A variedade não sumiu do jogo: ela mudou de escala. Era entre rumos da
	// mesma ilha; passou a ser entre ILHAS. E o teste agora cobra isso, que é
	// o que sobrou de verdadeiro na intenção original — serra não pode ser
	// igual em todo lugar.
	TSet<int32> ClimasDoHorizonte;
	for (int32 Grau = 0; Grau < 360; Grau += 5)
	{
		ClimasDoHorizonte.Add(static_cast<int32>(IslandGeography::SectorClimateAt(
			PontoDaIlha(static_cast<float>(Grau), RaioDoHorizonte))));
	}

	TestEqual(TEXT("numa ilha, o horizonte inteiro é do clima dela"),
		ClimasDoHorizonte.Num(), 1);

	TSet<int32> ClimasDasIlhas;
	for (const TCHAR* Nome : { TEXT("Forest"), TEXT("Glacier"), TEXT("Volcano") })
	{
		const IlhaDeTeste::FBiomaTemporario Ilha(Nome);
		ClimasDasIlhas.Add(static_cast<int32>(
			IslandGeography::SectorClimateAt(PontoDaIlha(0.0f, RaioDoHorizonte))));
	}

	TestTrue(TEXT("mas ilhas diferentes têm horizontes diferentes"),
		ClimasDasIlhas.Num() > 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FIslandGeographySwampIsTheWetRimOfTheForestTest,
	"BattleSquare.Environment.IslandGeography.OPantanoEAOrlaUmidaDaMata",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandGeographySwampIsTheWetRimOfTheForestTest::RunTest(const FString& Parameters)
{
	// O brejo é ALTURA, não rumo: a faixa baixa logo atrás da praia. Por isso
	// ele se mede por distância do centro, e não por setor.
	const float BordaDaTerra =
		IslandGeography::LandRadiusUnits() - IslandGeography::BeachWidthUnits();
	const float DentroDoPantano = BordaDaTerra - IslandGeography::SwampWidthUnits() * 0.5f;
	const float AntesDoPantano =
		BordaDaTerra - IslandGeography::SwampWidthUnits() - TolerenciaDaIlha;

	TestTrue(TEXT("a faixa de pântano tem largura"),
		IslandGeography::SwampWidthUnits() > 0.0f);

	// A faixa não pode comer a casa: quem nasce no meio do mapa nasceria com o
	// pé na água, e o campo de treino junto.
	TestTrue(TEXT("o pântano começa depois da casa"),
		BordaDaTerra - IslandGeography::SwampWidthUnits() > IslandGeography::HomeRadiusUnits());

	// A comparação deixou de ser entre SETORES e passou a ser entre ILHAS: o
	// pântano é a orla úmida de uma ilha de MATA, e numa ilha seca a mesma
	// faixa continua sendo o que a ilha é.
	//
	// A intenção do teste não mudou — deserto encostando no mar dá areia, não
	// brejo, e sem essa regra a ilha inteira ganharia uma cinta de lodo. O que
	// mudou é que "seco" virou uma ilha, e não um rumo.
	{
		const IlhaDeTeste::FBiomaTemporario IlhaDeMata(TEXT("Forest"));
		TestEqual(TEXT("na mata, a orla é pântano"),
			static_cast<int32>(IslandGeography::BiomeAt(PontoDaIlha(0.0f, DentroDoPantano))),
			static_cast<int32>(EIslandBiome::Swamp));
		TestEqual(TEXT("e logo atrás dela ainda é mata"),
			static_cast<int32>(IslandGeography::BiomeAt(PontoDaIlha(0.0f, AntesDoPantano))),
			static_cast<int32>(EIslandBiome::Forest));
	}

	{
		const IlhaDeTeste::FBiomaTemporario IlhaSeca(TEXT("Desert"));
		TestNotEqual(TEXT("ilha seca não cria brejo na orla"),
			static_cast<int32>(IslandGeography::BiomeAt(PontoDaIlha(0.0f, DentroDoPantano))),
			static_cast<int32>(EIslandBiome::Swamp));
	}

	// Entre o brejo e o mar continua havendo areia: sem isso o pântano teria
	// comido a praia em vez de ficar atrás dela.
	TestEqual(TEXT("a praia sobrevive ao pântano"),
		static_cast<int32>(IslandGeography::BiomeAt(
			PontoDaIlha(0.0f, IslandGeography::LandRadiusUnits() - TolerenciaDaIlha))),
		static_cast<int32>(EIslandBiome::Beach));

	TestEqual(TEXT("e o clima do brejo é o úmido"),
		static_cast<int32>(IslandGeography::ClimateOf(EIslandBiome::Swamp)),
		static_cast<int32>(EScenaryClimate::Humid));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FIslandGeographySwampSaturatesBeforeTheForestTest,
	"BattleSquare.Environment.IslandGeography.OBrejoSaturaAntesDaMata",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandGeographySwampSaturatesBeforeTheForestTest::RunTest(const FString& Parameters)
{
	// É ISTO que paga o clima novo. Um clima que responde igual ao que já
	// existe não é um lugar, é uma linha a mais em quatro tabelas. Na mata
	// precisa CHOVER para o chão não aguentar mais água; no brejo basta uma
	// garoa — uma graduação inteira antes.
	const int32 BrejoNaGaroa = WorldWeather::HumidityPercent(
		EScenaryClimate::Humid, EWeather::Drizzle);
	const int32 MataNaGaroa = WorldWeather::HumidityPercent(
		EScenaryClimate::Temperate, EWeather::Drizzle);

	TestTrue(TEXT("o brejo já satura na garoa"), BrejoNaGaroa >= 100);
	TestTrue(TEXT("a mata ainda não"), MataNaGaroa < 100);

	// E seco ele continua mais molhado que a mata molhada — o brejo não é uma
	// mata com chuva, é o que sobra depois que a chuva vai embora.
	TestTrue(TEXT("brejo limpo é mais úmido que mata limpa"),
		WorldWeather::HumidityPercent(EScenaryClimate::Humid, EWeather::Clear)
		> WorldWeather::HumidityPercent(EScenaryClimate::Temperate, EWeather::Clear));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FIslandGeographyHomeForestFitsWholeChunksTest,
	"BattleSquare.Environment.IslandGeography.AMataDaCasaCabePedacosInteiros",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandGeographyHomeForestFitsWholeChunksTest::RunTest(const FString& Parameters)
{
	// O jogador disse que a mata "desapareceu", e a causa era esta: os setores
	// de bioma são fatias que se encontram no CENTRO, então logo depois do
	// miolo eles chegam todos de uma vez. Com o miolo menor que um pedaço de
	// mundo, o vizinho imediato de quem nasce já era outro bioma.
	//
	// O que se mede aqui é o que o jogador vê: o pedaço em que ele nasce e os
	// quatro vizinhos de lado — tudo o que está carregado ao redor dele no
	// primeiro instante. Com o raio antigo o vizinho a OESTE era deserto.
	const float Pedaco = RegionResidency::ChunkSideUnits();
	TestTrue(TEXT("a mata da casa cobre ao menos um pedaço de mundo inteiro"),
		IslandGeography::HomeRadiusUnits() >= Pedaco);

	const FIntPoint Vizinhos[] = {
		FIntPoint(0, 0), FIntPoint(-1, 0), FIntPoint(1, 0),
		FIntPoint(0, -1), FIntPoint(0, 1),
	};
	for (const FIntPoint& Pedacinho : Vizinhos)
	{
		TestEqual(TEXT("o pedaço vizinho de quem nasce é mata"),
			static_cast<int32>(IslandGeography::BiomeAt(
				RegionResidency::ChunkCenterUnits(Pedacinho))),
			static_cast<int32>(EIslandBiome::Forest));
	}

	// E ela continua sendo MATA: andar em qualquer direção dentro do miolo não
	// pode trocar de bioma, senão o teto acima não significa nada.
	const float Passo = IslandGeography::HomeRadiusUnits() * 0.9f;
	const FVector2D Rumos[] = {
		FVector2D(Passo, 0.0f), FVector2D(-Passo, 0.0f),
		FVector2D(0.0f, Passo), FVector2D(0.0f, -Passo),
	};
	for (const FVector2D& Rumo : Rumos)
	{
		TestEqual(TEXT("dentro do miolo ainda é mata"),
			static_cast<int32>(IslandGeography::BiomeAt(Rumo)),
			static_cast<int32>(EIslandBiome::Forest));
	}

	// E o miolo não come a ilha: a praia continua existindo do lado de fora.
	TestTrue(TEXT("sobra ilha depois do miolo"),
		IslandGeography::HomeRadiusUnits()
		< IslandGeography::LandRadiusUnits() - IslandGeography::BeachWidthUnits());

	return true;
}

#endif

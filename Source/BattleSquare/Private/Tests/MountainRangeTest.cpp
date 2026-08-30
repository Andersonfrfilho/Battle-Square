// Copyright 2026 Anderson. All Rights Reserved.

#include "Environment/MountainRange.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Environment/ScenaryClimate.h"
#include "Misc/AutomationTest.h"

namespace
{
	// Nome próprio (L-042): helper homônimo em outro arquivo de teste vira
	// sobrecarga ambígua quando o unity build junta os dois.
	UWorld* CreateMountainTestWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
		FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
		WorldContext.SetCurrentWorld(World);
		World->InitializeActorsForPlay(FURL());
		return World;
	}

	void DestroyMountainTestWorld(UWorld* World)
	{
		if (!World)
		{
			return;
		}
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
	}

	constexpr uint32 SementeDaSerra = 20260830u;
}

/**
 * O padrão que já custou TRÊS defeitos a este projeto: componente criado,
 * asset nunca atribuído. Ele passa em toda verificação de lógica e não existe
 * na tela. Por isso o que se afirma aqui é a ATRIBUIÇÃO, não a criação.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMountainRangeAssignsMeshesTest,
	"BattleSquare.MountainRange.AssignsMeshes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMountainRangeAssignsMeshesTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateMountainTestWorld();
	AMountainRange* Serra = World->SpawnActor<AMountainRange>();

	TestNotNull(TEXT("a serra nasce"), Serra);
	TestNotNull(TEXT("o agrupamento de rocha existe"), Serra->GetRockPeaks());
	TestNotNull(TEXT("o agrupamento de gelo existe"), Serra->GetSnowCaps());
	TestNotNull(TEXT("a rocha tem malha atribuída"),
		Serra->GetRockPeaks()->GetStaticMesh().Get());
	TestNotNull(TEXT("o gelo tem malha atribuída"),
		Serra->GetSnowCaps()->GetStaticMesh().Get());

	DestroyMountainTestWorld(World);
	return true;
}

/** Serra que não ergue corpo nenhum é horizonte vazio — a queixa original. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMountainRangeRaisesBodiesTest,
	"BattleSquare.MountainRange.RaisesBodies",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMountainRangeRaisesBodiesTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateMountainTestWorld();
	AMountainRange* Serra = World->SpawnActor<AMountainRange>();
	Serra->BuildRange(EScenaryClimate::Temperate, SementeDaSerra);

	TestTrue(TEXT("a serra ergue dezenas de corpos"), Serra->GetPeakCount() >= 40);
	TestEqual(TEXT("cada corpo é uma instância de rocha"),
		Serra->GetRockPeaks()->GetInstanceCount(), Serra->GetPeakCount());

	DestroyMountainTestWorld(World);
	return true;
}

/**
 * Montar duas vezes tem que dar a MESMA serra, e montar de novo não pode
 * empilhar por cima da anterior — foi assim que a mata duplicou uma vez.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMountainRangeIsDeterministicTest,
	"BattleSquare.MountainRange.IsDeterministic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMountainRangeIsDeterministicTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateMountainTestWorld();
	AMountainRange* Serra = World->SpawnActor<AMountainRange>();

	Serra->BuildRange(EScenaryClimate::Temperate, SementeDaSerra);
	const TArray<float> Primeira = Serra->GetPeakHeightsMeters();

	Serra->BuildRange(EScenaryClimate::Temperate, SementeDaSerra);
	const TArray<float> Segunda = Serra->GetPeakHeightsMeters();

	TestEqual(TEXT("montar de novo não empilha"), Segunda.Num(), Primeira.Num());
	TestTrue(TEXT("a mesma semente dá a mesma serra"), Primeira == Segunda);

	Serra->BuildRange(EScenaryClimate::Temperate, SementeDaSerra + 1u);
	TestFalse(TEXT("semente diferente dá serra diferente"),
		Serra->GetPeakHeightsMeters() == Primeira);

	DestroyMountainTestWorld(World);
	return true;
}

/**
 * "Montanha DEPENDENDO DO TAMANHO forma gelo": no bioma da mata, parte dos
 * corpos congela e parte não. Se todos congelassem, o tamanho não estaria
 * decidindo nada — e se nenhum congelasse, a regra não existiria.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMountainRangeIcesOnlyTallPeaksTest,
	"BattleSquare.MountainRange.IcesOnlyTallPeaks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMountainRangeIcesOnlyTallPeaksTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateMountainTestWorld();
	AMountainRange* Serra = World->SpawnActor<AMountainRange>();
	Serra->BuildRange(EScenaryClimate::Temperate, SementeDaSerra);

	const int32 ComGelo = Serra->GetSnowCapCount();

	TestTrue(TEXT("alguma montanha alcança o congelamento"), ComGelo > 0);
	TestTrue(TEXT("nem toda montanha alcança"), ComGelo < Serra->GetPeakCount());

	// E o gelo cai exatamente sobre quem passou da linha da neve.
	int32 AcimaDaLinha = 0;
	for (const float Altura : Serra->GetPeakHeightsMeters())
	{
		if (Altura > ScenaryClimate::SnowLineMeters(EScenaryClimate::Temperate))
		{
			++AcimaDaLinha;
		}
	}
	TestEqual(TEXT("o gelo cai sobre quem passou da linha da neve"),
		ComGelo, AcimaDaLinha);

	DestroyMountainTestWorld(World);
	return true;
}

/**
 * A exceção pedida, na letra: deserto e clima bom não formam gelo em altura
 * nenhuma desta serra. Ela não é um `if` — sai da temperatura do pé da
 * montanha contra os 6,5 °C que se perdem a cada quilômetro de subida.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMountainRangeSkipsIceInWarmClimatesTest,
	"BattleSquare.MountainRange.SkipsIceInWarmClimates",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMountainRangeSkipsIceInWarmClimatesTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateMountainTestWorld();
	AMountainRange* Serra = World->SpawnActor<AMountainRange>();

	Serra->BuildRange(EScenaryClimate::Desert, SementeDaSerra);
	TestEqual(TEXT("no deserto, nenhum cume congela"), Serra->GetSnowCapCount(), 0);
	TestTrue(TEXT("mas a serra continua lá"), Serra->GetPeakCount() > 0);

	Serra->BuildRange(EScenaryClimate::Mild, SementeDaSerra);
	TestEqual(TEXT("em clima bom, nenhum cume congela"), Serra->GetSnowCapCount(), 0);

	Serra->BuildRange(EScenaryClimate::Cold, SementeDaSerra);
	TestTrue(TEXT("na serra fria, quase tudo congela"),
		Serra->GetSnowCapCount() > Serra->GetPeakCount() / 2);

	DestroyMountainTestWorld(World);
	return true;
}

/** A regra do gelo, sem mundo e sem malha: aritmética pura, verificável só. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FScenaryClimateSnowLineFollowsTemperatureTest,
	"BattleSquare.ScenaryClimate.SnowLineFollowsTemperature",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FScenaryClimateSnowLineFollowsTemperatureTest::RunTest(const FString& Parameters)
{
	// Quanto mais quente o pé, mais alta a linha da neve.
	TestTrue(TEXT("a serra fria congela antes da mata"),
		ScenaryClimate::SnowLineMeters(EScenaryClimate::Cold)
			< ScenaryClimate::SnowLineMeters(EScenaryClimate::Temperate));
	TestTrue(TEXT("a mata congela antes do clima bom"),
		ScenaryClimate::SnowLineMeters(EScenaryClimate::Temperate)
			< ScenaryClimate::SnowLineMeters(EScenaryClimate::Mild));
	TestTrue(TEXT("o clima bom congela antes do deserto"),
		ScenaryClimate::SnowLineMeters(EScenaryClimate::Mild)
			< ScenaryClimate::SnowLineMeters(EScenaryClimate::Desert));

	// A fatia de gelo cresce com o tamanho, e é ZERO abaixo da linha.
	const float Baixa = 1200.0f;
	const float Alta = 3300.0f;
	TestEqual(TEXT("montanha baixa na mata não tem gelo"),
		ScenaryClimate::SnowCapFraction(EScenaryClimate::Temperate, Baixa), 0.0f);
	TestTrue(TEXT("montanha alta na mata tem gelo"),
		ScenaryClimate::SnowCapFraction(EScenaryClimate::Temperate, Alta) > 0.0f);
	TestTrue(TEXT("quanto mais alta, mais gelo"),
		ScenaryClimate::SnowCapFraction(EScenaryClimate::Temperate, Alta)
			> ScenaryClimate::SnowCapFraction(EScenaryClimate::Temperate, 2800.0f));

	// A exceção, na fonte: nem a mais alta da faixa congela nesses dois.
	TestEqual(TEXT("deserto não congela nem no ponto mais alto"),
		ScenaryClimate::SnowCapFraction(EScenaryClimate::Desert, Alta), 0.0f);
	TestEqual(TEXT("clima bom não congela nem no ponto mais alto"),
		ScenaryClimate::SnowCapFraction(EScenaryClimate::Mild, Alta), 0.0f);

	// E a temperatura cai com a altitude, que é de onde tudo isto vem.
	TestTrue(TEXT("o cume é mais frio que o pé"),
		ScenaryClimate::TemperatureAtMeters(EScenaryClimate::Temperate, Alta)
			< ScenaryClimate::TemperatureAtMeters(EScenaryClimate::Temperate, 0.0f));

	return true;
}

/** O clima vem do `.ini` pelo nome escrito, e erro de digitação não apaga a serra. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FScenaryClimateReadsNameTest,
	"BattleSquare.ScenaryClimate.ReadsName",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FScenaryClimateReadsNameTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Desert"),
		ScenaryClimate::ClimateFromName(FName(TEXT("Desert"))) == EScenaryClimate::Desert);
	TestTrue(TEXT("Mild"),
		ScenaryClimate::ClimateFromName(FName(TEXT("Mild"))) == EScenaryClimate::Mild);
	TestTrue(TEXT("Cold"),
		ScenaryClimate::ClimateFromName(FName(TEXT("Cold"))) == EScenaryClimate::Cold);
	TestTrue(TEXT("nome desconhecido cai na mata, não em vazio"),
		ScenaryClimate::ClimateFromName(FName(TEXT("Jupiter"))) == EScenaryClimate::Temperate);

	return true;
}

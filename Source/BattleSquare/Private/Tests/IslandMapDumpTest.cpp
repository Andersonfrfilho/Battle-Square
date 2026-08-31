// Copyright 2026 Anderson. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Environment/IslandGeography.h"
#include "Environment/IslandFeatureLayout.h"
#include "World/RegionLayout.h"
#include "World/VillageLayout.h"

/**
 * Despeja o traçado da ilha em `Saved/IslandMap.json`.
 *
 * Não é teste de regra: é o INSTRUMENTO que permite desenhar o mapa a partir
 * dos números do código em vez de à mão. Mapa desenhado à parte seria mais uma
 * cópia da verdade, e cópias concordam até a primeira edição (L-032).
 *
 * Vive como teste porque é assim que este projeto roda coisa headless.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandMapDumpTest,
	"BattleSquare.IslandMap.Dump",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

namespace DespejoDoMapa
{
	const TCHAR* NomeDoTipo(ESettlementKind Tipo)
	{
		switch (Tipo)
		{
		case ESettlementKind::VilaInicial:      return TEXT("vila-inicial");
		case ESettlementKind::VilaDaAcademia:   return TEXT("vila-academia");
		case ESettlementKind::VilaDoMercado:    return TEXT("vila-mercado");
		case ESettlementKind::CidadeGrande:     return TEXT("cidade-grande");
		case ESettlementKind::PostoDeFronteira: return TEXT("posto-fronteira");
		}
		return TEXT("?");
	}

	const TCHAR* NomeDaPeca(IslandFeatureLayout::EIslandFeature Peca)
	{
		switch (Peca)
		{
		case IslandFeatureLayout::EIslandFeature::Volcano:         return TEXT("vulcao");
		case IslandFeatureLayout::EIslandFeature::WalkableMountain: return TEXT("montanha");
		default: break;
		}
		return TEXT("caverna");
	}
}

bool FIslandMapDumpTest::RunTest(const FString& Parameters)
{
	FString Json = TEXT("{\n");
	Json += FString::Printf(TEXT("  \"raio\": %.1f,\n"), IslandGeography::LandRadiusUnits());
	Json += FString::Printf(TEXT("  \"praia\": %.1f,\n"), IslandGeography::BeachWidthUnits());
	Json += FString::Printf(TEXT("  \"barrancoInterno\": %.1f,\n"),
		IslandGeography::BluffInnerRadiusUnits());
	Json += FString::Printf(TEXT("  \"barrancoExterno\": %.1f,\n"),
		IslandGeography::BluffOuterRadiusUnits());
	Json += FString::Printf(TEXT("  \"rampaGraus\": %.1f,\n"),
		IslandGeography::BluffRampAngleDegrees());
	Json += FString::Printf(TEXT("  \"vulcaoX\": %.1f, \"vulcaoY\": %.1f, \"vulcaoQueimado\": %.1f,\n"),
		IslandGeography::VolcanoCenterUnits().X, IslandGeography::VolcanoCenterUnits().Y,
		IslandGeography::VolcanoScorchedRadiusUnits());

	Json += TEXT("  \"assentamentos\": [\n");
	const TArray<FSettlementPlacement> Assentamentos = RegionLayout::Plan();
	for (int32 Indice = 0; Indice < Assentamentos.Num(); ++Indice)
	{
		Json += FString::Printf(
			TEXT("    {\"tipo\":\"%s\",\"x\":%.1f,\"y\":%.1f,\"lote\":%.1f,\"predios\":%d}%s\n"),
			DespejoDoMapa::NomeDoTipo(Assentamentos[Indice].Kind),
			Assentamentos[Indice].CenterUnits.X, Assentamentos[Indice].CenterUnits.Y,
			VillageLayout::PlotHalfExtentUnitsFor(Assentamentos[Indice].Kind),
			VillageLayout::PlanFor(Assentamentos[Indice].Kind).Num(),
			Indice + 1 < Assentamentos.Num() ? TEXT(",") : TEXT(""));
	}
	Json += TEXT("  ],\n");

	Json += TEXT("  \"pecas\": [\n");
	const TArray<IslandFeatureLayout::FFeaturePlacement> Pecas = IslandFeatureLayout::Plan();
	for (int32 Indice = 0; Indice < Pecas.Num(); ++Indice)
	{
		Json += FString::Printf(TEXT("    {\"tipo\":\"%s\",\"x\":%.1f,\"y\":%.1f,\"raio\":%.1f}%s\n"),
			DespejoDoMapa::NomeDaPeca(Pecas[Indice].Feature),
			Pecas[Indice].CenterUnits().X, Pecas[Indice].CenterUnits().Y,
			Pecas[Indice].ClearanceUnits,
			Indice + 1 < Pecas.Num() ? TEXT(",") : TEXT(""));
	}
	Json += TEXT("  ],\n");

	// A malha de alturas: é ela que permite ver o relevo no desenho, e é a
	// única prova de que `GroundHeightAt` produz morro em vez de prato.
	const int32 Lado = 96;
	const float Raio = IslandGeography::LandRadiusUnits();
	Json += FString::Printf(TEXT("  \"malhaLado\": %d,\n"), Lado);
	Json += TEXT("  \"alturas\": [");
	for (int32 Linha = 0; Linha < Lado; ++Linha)
	{
		for (int32 Coluna = 0; Coluna < Lado; ++Coluna)
		{
			const float X = ((static_cast<float>(Coluna) / (Lado - 1)) * 2.0f - 1.0f) * Raio;
			const float Y = ((static_cast<float>(Linha) / (Lado - 1)) * 2.0f - 1.0f) * Raio;
			Json += FString::Printf(TEXT("%s%.0f"),
				(Linha == 0 && Coluna == 0) ? TEXT("") : TEXT(","),
				IslandGeography::GroundHeightAt(FVector2D(X, Y)));
		}
	}
	Json += TEXT("]\n}\n");

	const FString Caminho = FPaths::ProjectSavedDir() / TEXT("IslandMap.json");
	TestTrue(TEXT("o despejo foi gravado"), FFileHelper::SaveStringToFile(Json, *Caminho));

	return true;
}

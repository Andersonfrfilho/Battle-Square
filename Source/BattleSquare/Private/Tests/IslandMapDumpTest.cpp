// Copyright 2026 Anderson. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Environment/IslandGeography.h"
#include "Environment/IslandFeatureLayout.h"
#include "World/RegionLayout.h"
#include "World/VillageLayout.h"
#include "Environment/FreshWater.h"
#include "World/TrailLayout.h"
#include "World/LandUseLayout.h"
#include "Environment/CaveLabyrinth.h"
#include "Net/BattleSquareGameMode.h"

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

	// Os RIOS, com o lago e a cachoeira de cada um. Eles já existiam no
	// mundo e nunca apareceram em mapa nenhum — foi por isso que o relato de
	// jogo disse "a cachoeira nunca foi vista".
	Json += TEXT("  \"rios\": [\n");
	const TArray<FreshWater::FRiverCourse> Rios = FreshWater::Plan();
	for (int32 Indice = 0; Indice < Rios.Num(); ++Indice)
	{
		Json += TEXT("    {\"curso\":[");
		const int32 Passos = 40;
		for (int32 Passo = 0; Passo <= Passos; ++Passo)
		{
			const float Raio = FMath::Lerp(Rios[Indice].SourceRadiusUnits,
				Rios[Indice].MouthRadiusUnits, static_cast<float>(Passo) / Passos);
			const FVector2D Onde = FreshWater::PointAt(Rios[Indice], Raio);
			Json += FString::Printf(TEXT("%s[%.0f,%.0f,%.0f]"),
				Passo == 0 ? TEXT("") : TEXT(","), Onde.X, Onde.Y,
				FreshWater::HalfWidthAt(Rios[Indice], Raio));
		}

		const FVector2D NoLago = FreshWater::PointAt(Rios[Indice], Rios[Indice].LakeRadiusUnits);
		const FVector2D NaQueda = FreshWater::PointAt(Rios[Indice], Rios[Indice].FallRadiusUnits);
		Json += FString::Printf(
			TEXT("],\"lagoX\":%.0f,\"lagoY\":%.0f,\"quedaX\":%.0f,\"quedaY\":%.0f}%s\n"),
			NoLago.X, NoLago.Y, NaQueda.X, NaQueda.Y,
			Indice + 1 < Rios.Num() ? TEXT(",") : TEXT(""));
	}
	Json += TEXT("  ],\n");

	// As TRILHAS, com os pontos que o traçado achou. Elas não são retas, e
	// isso se vê no desenho porque o terreno é que as entortou.
	Json += TEXT("  \"trilhas\": [\n");
	const TArray<FTrailRoute>& Trilhas = TrailLayout::Plan();
	for (int32 Indice = 0; Indice < Trilhas.Num(); ++Indice)
	{
		Json += TEXT("    [");
		for (int32 Ponto = 0; Ponto < Trilhas[Indice].PointsUnits.Num(); ++Ponto)
		{
			Json += FString::Printf(TEXT("%s[%.0f,%.0f]"),
				Ponto == 0 ? TEXT("") : TEXT(","),
				Trilhas[Indice].PointsUnits[Ponto].X, Trilhas[Indice].PointsUnits[Ponto].Y);
		}
		Json += FString::Printf(TEXT("]%s\n"), Indice + 1 < Trilhas.Num() ? TEXT(",") : TEXT(""));
	}
	Json += TEXT("  ],\n");

	Json += TEXT("  \"pontes\": [");
	const TArray<FVector2D> Pontes = TrailLayout::BridgePoints();
	for (int32 Indice = 0; Indice < Pontes.Num(); ++Indice)
	{
		Json += FString::Printf(TEXT("%s[%.0f,%.0f]"),
			Indice == 0 ? TEXT("") : TEXT(","), Pontes[Indice].X, Pontes[Indice].Y);
	}
	Json += TEXT("],\n");

	// FONTES e CÓRREGOS: a água do miolo, que a ilha não tinha.
	Json += TEXT("  \"fontes\": [");
	const TArray<FreshWater::FSpring> Fontes = FreshWater::PlanSprings();
	for (int32 Indice = 0; Indice < Fontes.Num(); ++Indice)
	{
		Json += FString::Printf(TEXT("%s[%.0f,%.0f,%.0f]"), Indice == 0 ? TEXT("") : TEXT(","),
			Fontes[Indice].CenterUnits.X, Fontes[Indice].CenterUnits.Y,
			Fontes[Indice].PoolHalfWidthUnits);
	}
	Json += TEXT("],\n  \"corregos\": [\n");
	const TArray<FreshWater::FBrook> Corregos = FreshWater::PlanBrooks();
	for (int32 Indice = 0; Indice < Corregos.Num(); ++Indice)
	{
		Json += TEXT("    [");
		for (int32 Ponto = 0; Ponto < Corregos[Indice].PointsUnits.Num(); ++Ponto)
		{
			Json += FString::Printf(TEXT("%s[%.0f,%.0f]"), Ponto == 0 ? TEXT("") : TEXT(","),
				Corregos[Indice].PointsUnits[Ponto].X, Corregos[Indice].PointsUnits[Ponto].Y);
		}
		Json += FString::Printf(TEXT("]%s\n"), Indice + 1 < Corregos.Num() ? TEXT(",") : TEXT(""));
	}
	Json += TEXT("  ],\n");

	// USO DO SOLO: bosque, clareira fechada e fazenda.
	Json += TEXT("  \"solo\": [\n");
	const TArray<FGroundUsePatch> Manchas = LandUseLayout::Plan();
	for (int32 Indice = 0; Indice < Manchas.Num(); ++Indice)
	{
		const TCHAR* Nome = TEXT("nenhum");
		switch (Manchas[Indice].Use)
		{
		case EGroundUse::Bosque:          Nome = TEXT("bosque"); break;
		case EGroundUse::ClareiraFechada: Nome = TEXT("clareira"); break;
		case EGroundUse::Fazenda:         Nome = TEXT("fazenda"); break;
		default: break;
		}

		Json += FString::Printf(TEXT("    {\"uso\":\"%s\",\"x\":%.0f,\"y\":%.0f,\"meio\":%.0f}%s\n"),
			Nome, Manchas[Indice].CenterUnits.X, Manchas[Indice].CenterUnits.Y,
			Manchas[Indice].HalfExtentUnits, Indice + 1 < Manchas.Num() ? TEXT(",") : TEXT(""));
	}
	Json += TEXT("  ],\n");

	// OS MAPAS DAS CAVERNAS. A semente sai de `SeedForPlacement`, a mesma que
	// o GameMode usa — é para isso que ela saiu de lá.
	Json += TEXT("  \"cavernas\": [\n");
	{
		const ABattleSquareGameMode* Padrao = GetDefault<ABattleSquareGameMode>();
		const int32 SementeDoMundo = Padrao ? Padrao->GetWorldScenerySeedForMap() : 0;

		TArray<IslandFeatureLayout::FFeaturePlacement> Cavernas;
		for (const IslandFeatureLayout::FFeaturePlacement& Peca : IslandFeatureLayout::Plan())
		{
			if (Peca.Feature == IslandFeatureLayout::EIslandFeature::Cave)
			{
				Cavernas.Add(Peca);
			}
		}
		Cavernas.Append(FreshWater::PlanGrottoes());

		for (int32 Indice = 0; Indice < Cavernas.Num(); ++Indice)
		{
			const int32 Lado = Cavernas[Indice].CaveSide;
			const CaveLabyrinth::FCaveGrid Planta = CaveLabyrinth::Carve(Lado, Lado,
				IslandFeatureLayout::SeedForPlacement(SementeDoMundo, Cavernas[Indice]));

			Json += FString::Printf(
				TEXT("    {\"x\":%.0f,\"y\":%.0f,\"lado\":%d,\"entrada\":%d,\"paredes\":["),
				Cavernas[Indice].CenterUnits().X, Cavernas[Indice].CenterUnits().Y,
				Planta.Columns, Planta.EntranceColumn);

			for (int32 Casa = 0; Casa < Planta.Walls.Num(); ++Casa)
			{
				Json += FString::Printf(TEXT("%s%d"),
					Casa == 0 ? TEXT("") : TEXT(","), static_cast<int32>(Planta.Walls[Casa]));
			}

			Json += FString::Printf(TEXT("]}%s\n"),
				Indice + 1 < Cavernas.Num() ? TEXT(",") : TEXT(""));
		}
	}
	Json += TEXT("  ],\n");

	// As passagens subterrâneas: a ligação que não cabe na superfície.
	Json += TEXT("  \"passagens\": [");
	const TArray<FreshWater::FUnderwaterLink> Passagens = FreshWater::PlanUnderwaterLinks();
	for (int32 Indice = 0; Indice < Passagens.Num(); ++Indice)
	{
		Json += FString::Printf(TEXT("%s[%.0f,%.0f,%.0f,%.0f]"),
			Indice == 0 ? TEXT("") : TEXT(","),
			Passagens[Indice].FromUnits.X, Passagens[Indice].FromUnits.Y,
			Passagens[Indice].ToUnits.X, Passagens[Indice].ToUnits.Y);
	}
	Json += TEXT("],\n");

	// Os campos de treino, no anel que o GameMode usa.
	{
		const ABattleSquareGameMode* Padrao = GetDefault<ABattleSquareGameMode>();
		Json += FString::Printf(TEXT("  \"anelDeTreino\": %.1f,\n"),
			Padrao ? Padrao->GetTrainingFieldRingForMap() : 0.0f);
	}

	// A malha de alturas: é ela que permite ver o relevo no desenho, e é a
	// única prova de que `GroundHeightAt` produz morro em vez de prato.
	const int32 Lado = 180;
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

// Copyright 2026 Anderson. All Rights Reserved.

#include "World/VillageBuildingArt.h"

#include "Misc/ConfigCacheIni.h"

const TCHAR* VillageBuildingArt::BuildingConfigKey(EVillageBuilding Building)
{
	// Chave estavel por predio: renomear o rotulo do predio nao mexe na chave.
	switch (Building)
	{
	case EVillageBuilding::CentroDeRecuperacao: return TEXT("Mesh_Village_CentroDeRecuperacao");
	case EVillageBuilding::Escola:              return TEXT("Mesh_Village_Escola");
	case EVillageBuilding::Arena:               return TEXT("Mesh_Village_Arena");
	case EVillageBuilding::Marco:               return TEXT("Mesh_Village_Marco");
	case EVillageBuilding::Praca:               return TEXT("Mesh_Village_Praca");
	case EVillageBuilding::Casa:                return TEXT("Mesh_Village_Casa");
	case EVillageBuilding::Academia:            return TEXT("Mesh_Village_Academia");
	case EVillageBuilding::Mercado:             return TEXT("Mesh_Village_Mercado");
	case EVillageBuilding::Portao:              return TEXT("Mesh_Village_Portao");
	case EVillageBuilding::Palafita:            return TEXT("Mesh_Village_Palafita");
	case EVillageBuilding::Passarela:           return TEXT("Mesh_Village_Passarela");
	case EVillageBuilding::Chinampa:            return TEXT("Mesh_Village_Chinampa");
	default:                                    return TEXT("");
	}
}

FString VillageBuildingArt::MeshPathFor(EVillageBuilding Building)
{
	const TCHAR* Chave = BuildingConfigKey(Building);
	if (!Chave || !*Chave)
	{
		return FString();
	}

	FString Caminho;
	GConfig->GetString(TEXT("/Script/BattleSquare.Art"), Chave, Caminho, GGameIni);
	return Caminho;
}

FVector VillageBuildingArt::ScaleToFootprint(
	const FVector& MeshBounds, const FVector2D& FootprintUnits, float HeightUnits)
{
	// Eixo sem medida cai em 1, NUNCA em zero: escala zero e malha atribuida e
	// invisivel — o defeito que o teste da vila cobra desde sempre.
	const auto Eixo = [](float Desejado, float Medido) -> float
	{
		return (Medido > KINDA_SMALL_NUMBER) ? (Desejado / Medido) : 1.0f;
	};

	return FVector(
		Eixo(FootprintUnits.X, MeshBounds.X),
		Eixo(FootprintUnits.Y, MeshBounds.Y),
		Eixo(HeightUnits, MeshBounds.Z));
}

// Copyright 2026 Anderson. All Rights Reserved.

#include "Environment/ScenaryClimate.h"

#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"

namespace ClimaDoCenario
{
	// A temperatura ao pé da serra, por bioma. São as únicas quatro decisões
	// de gosto deste arquivo; o resto sai delas por conta.
	constexpr float TemperadoCelsius = 16.0f;
	constexpr float FrioCelsius = 3.0f;
	constexpr float AmenoCelsius = 24.0f;
	constexpr float DesertoCelsius = 34.0f;
}

float ScenaryClimate::BaseTemperatureCelsius(EScenaryClimate Climate)
{
	switch (Climate)
	{
	case EScenaryClimate::Cold:   return ClimaDoCenario::FrioCelsius;
	case EScenaryClimate::Mild:   return ClimaDoCenario::AmenoCelsius;
	case EScenaryClimate::Desert: return ClimaDoCenario::DesertoCelsius;
	case EScenaryClimate::Temperate:
		break;
	}

	return ClimaDoCenario::TemperadoCelsius;
}

float ScenaryClimate::TemperatureAtMeters(EScenaryClimate Climate, float AltitudeMeters)
{
	return BaseTemperatureCelsius(Climate)
		- LapseRateCelsiusPerKilometer * (AltitudeMeters / 1000.0f);
}

float ScenaryClimate::SnowLineMeters(EScenaryClimate Climate)
{
	// Onde a queda por altitude consome toda a temperatura de baixo.
	const float AcimaDoCongelamento = BaseTemperatureCelsius(Climate) - FreezingCelsius;

	// Clima que já nasce congelado tem neve desde o chão. Sem este piso, um
	// clima negativo devolveria altitude negativa e a "fatia com gelo"
	// passaria de 1 — neve abaixo do nível do mar, que ninguém pediu.
	if (AcimaDoCongelamento <= 0.0f)
	{
		return 0.0f;
	}

	return (AcimaDoCongelamento / LapseRateCelsiusPerKilometer) * 1000.0f;
}

float ScenaryClimate::SnowCapFraction(EScenaryClimate Climate, float PeakMeters)
{
	if (PeakMeters <= 0.0f)
	{
		return 0.0f;
	}

	const float LinhaDaNeve = SnowLineMeters(Climate);
	if (PeakMeters <= LinhaDaNeve)
	{
		return 0.0f;
	}

	return FMath::Clamp((PeakMeters - LinhaDaNeve) / PeakMeters, 0.0f, 1.0f);
}

EScenaryClimate ScenaryClimate::ClimateFromName(FName Name)
{
	if (Name == FName(TEXT("Cold")))   { return EScenaryClimate::Cold; }
	if (Name == FName(TEXT("Mild")))   { return EScenaryClimate::Mild; }
	if (Name == FName(TEXT("Desert"))) { return EScenaryClimate::Desert; }
	return EScenaryClimate::Temperate;
}

EScenaryClimate ScenaryClimate::ConfiguredClimate()
{
	FString Escrito;
	if (GConfig && GConfig->GetString(
		TEXT("/Script/BattleSquare.MountainRange"), TEXT("Climate"), Escrito, GGameIni))
	{
		return ClimateFromName(FName(*Escrito));
	}

	return EScenaryClimate::Temperate;
}

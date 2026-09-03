// Copyright 2026 Anderson. All Rights Reserved.

#include "World/EncounterPredominance.h"

namespace
{
	/** "Fisica/Terra" -> "Terra". Sem barra, é o texto inteiro. */
	FString ElementoDoTipo(const FString& Tipo)
	{
		FString Escola;
		FString Elemento;
		return Tipo.Split(TEXT("/"), &Escola, &Elemento) ? Elemento : Tipo;
	}

	/** O comum da mata pesa mais; o deslocado pesa menos — nunca zero. */
	constexpr int32 PesoComum = 160;
	constexpr int32 PesoNeutro = 100;
	constexpr int32 PesoDeslocado = 30;
	constexpr int32 PesoNoAmbienteCerto = 180;
}

int32 EncounterPredominance::PlaceWeightPercent(const FString& PetType,
	const FSpawnSurroundings& EmVolta)
{
	// SÓ A MATA tem predominância desenhada por enquanto — ela é o primeiro
	// bioma (decisão L1). Os outros ficam neutros até alguém desenhar a fauna
	// deles: neutro é a ausência honesta, e pesar por palpite seria decidir a
	// fauna do deserto sem ninguém pedir.
	if (EmVolta.Biome != EIslandBiome::Forest && EmVolta.Biome != EIslandBiome::Beach)
	{
		return PesoNeutro;
	}

	const FString Elemento = ElementoDoTipo(PetType);

	// O AQUÁTICO é "alguns, perto d'água": comum na margem, raro longe dela.
	if (Elemento == TEXT("Agua"))
	{
		return EmVolta.bNearWater ? PesoNoAmbienteCerto : PesoDeslocado;
	}

	// O CAVERNÍCOLA da mata é o fantasma: quem atravessa pedra mora nela.
	if (Elemento == TEXT("Fantasma"))
	{
		return EmVolta.bNearCave ? PesoNoAmbienteCerto : PesoDeslocado;
	}

	// O COMUM da natureza comum: planta e terra são a mata sendo mata.
	if (Elemento == TEXT("Planta") || Elemento == TEXT("Terra"))
	{
		return PesoComum;
	}

	// O deslocado da mata: fogo e gelo não são bicho de floresta — raros,
	// nunca impossíveis.
	if (Elemento == TEXT("Fogo"))
	{
		return PesoDeslocado;
	}

	return PesoNeutro;
}

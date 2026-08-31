// Copyright 2026 Anderson. All Rights Reserved.

#include "World/WorldMapPins.h"

const FWorldMapPin* FWorldMapPins::FindAt(const FVector2D& WorldXY) const
{
	// Distância AO QUADRADO: comparar sem raiz é a mesma resposta, e a raiz
	// aqui seria conta desperdiçada num laço que roda a cada marcação.
	const float LimiteAoQuadrado = SamePlaceUnits * SamePlaceUnits;

	for (const FWorldMapPin& Marcacao : Pins)
	{
		if (FVector2D::DistSquared(Marcacao.WorldXY, WorldXY) <= LimiteAoQuadrado)
		{
			return &Marcacao;
		}
	}

	return nullptr;
}

FWorldMapPins::EResult FWorldMapPins::ToggleAt(const FVector2D& WorldXY, EWorldPinKind Kind)
{
	const float LimiteAoQuadrado = SamePlaceUnits * SamePlaceUnits;

	for (int32 Indice = 0; Indice < Pins.Num(); ++Indice)
	{
		if (FVector2D::DistSquared(Pins[Indice].WorldXY, WorldXY) <= LimiteAoQuadrado)
		{
			// APAGA, mesmo que o tipo pedido seja outro. A alternativa —
			// trocar o tipo — faria o jogador que quer apagar ter de marcar
			// três vezes até dar a volta nos tipos, sem nada explicando.
			Pins.RemoveAt(Indice);
			return EResult::Apagada;
		}
	}

	if (Pins.Num() >= MaxPins)
	{
		return EResult::Cheio;
	}

	FWorldMapPin Nova;
	Nova.WorldXY = WorldXY;
	Nova.Kind = Kind;
	Pins.Add(Nova);
	return EResult::Posta;
}

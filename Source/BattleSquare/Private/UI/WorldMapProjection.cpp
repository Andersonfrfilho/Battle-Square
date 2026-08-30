// Copyright 2026 Anderson. All Rights Reserved.

#include "UI/WorldMapProjection.h"

FVector2D FWorldMapProjection::ToMapSpace(const FVector2D& WorldXY,
	const FWorldMapSnapshot& Snapshot, EMode Mode, float RangeUnits)
{
	if (RangeUnits <= 0.0f)
	{
		return FVector2D::ZeroVector;
	}

	const FVector2D Deslocamento = WorldXY - Snapshot.PlayerXY;

	// NORTE ACIMA: +X do mundo (norte) sobe na tela, +Y (leste) vai à direita.
	// O Y da tela cresce para BAIXO, então o norte leva sinal negativo — e é
	// exatamente aqui que mora o erro de eixo que este projeto já cometeu.
	float MapaX = Deslocamento.Y;
	float MapaY = -Deslocamento.X;

	if (Mode == EMode::SeguindoOOlhar)
	{
		// Gira o MUNDO ao CONTRÁRIO do olhar — daí o sinal negativo, e ele é a
		// linha inteira desta função.
		//
		// A primeira versão girava no mesmo sentido, e o comentário já dizia
		// "ao contrário": o texto estava certo e o código não o obedecia.
		// Olhando para leste, o leste ia parar EMBAIXO — o mapa ficava
		// consistente consigo mesmo e errado contra a tela, que é o modo de
		// falhar que não se enxerga relendo o código.
		const float Radianos = FMath::DegreesToRadians(-Snapshot.PlayerYawDegrees);
		const float Seno = FMath::Sin(Radianos);
		const float Cosseno = FMath::Cos(Radianos);

		const float GiradoX = MapaX * Cosseno - MapaY * Seno;
		const float GiradoY = MapaX * Seno + MapaY * Cosseno;

		MapaX = GiradoX;
		MapaY = GiradoY;
	}

	return FVector2D(MapaX / RangeUnits, MapaY / RangeUnits);
}

float FWorldMapProjection::PlayerArrowAngleDegrees(const FWorldMapSnapshot& Snapshot, EMode Mode)
{
	// Seguindo o olhar, a seta NÃO gira: o que gira é o mundo em volta dela.
	// Uma seta que gira num mapa que também gira soma os dois e mente.
	return Mode == EMode::SeguindoOOlhar ? 0.0f : Snapshot.PlayerYawDegrees;
}

bool FWorldMapProjection::IsMarkerVisible(const FWorldMapMarkerInfo& Marker,
	const FWorldMapSnapshot& Snapshot)
{
	if (Marker.Kind == EWorldMapMarker::Jogador)
	{
		return true;
	}

	if (!Snapshot.bHidesUndiscovered)
	{
		return true;
	}

	return Snapshot.Discovery.IsDiscovered(Marker.WorldXY);
}

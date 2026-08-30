// Copyright 2026 Anderson. All Rights Reserved.

#include "UI/WorldMapProjection.h"

#define LOCTEXT_NAMESPACE "MapaDoMundo"

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

FLinearColor FWorldMapProjection::ColorForTerrain(EWorldMapTerrain Terrain)
{
	switch (Terrain)
	{
		// SURDAS de propósito. O terreno é o fundo sobre o qual os marcadores
		// precisam saltar; um mapa com mata verde-viva e água azul-viva engole
		// o adversário laranja que é a informação urgente. É a mesma regra que
		// a paleta do mundo já segue — criatura é viva, terreno é terra.
		case EWorldMapTerrain::Clareira: return FLinearColor(0.42f, 0.40f, 0.32f);
		case EWorldMapTerrain::Mata:     return FLinearColor(0.20f, 0.30f, 0.20f);
		case EWorldMapTerrain::Margem:   return FLinearColor(0.46f, 0.42f, 0.30f);
		case EWorldMapTerrain::Agua:     return FLinearColor(0.16f, 0.26f, 0.40f);
		case EWorldMapTerrain::Relevo:   return FLinearColor(0.34f, 0.32f, 0.34f);
	}

	return FLinearColor(0.30f, 0.30f, 0.30f);
}

FText FWorldMapProjection::LabelForTerrain(EWorldMapTerrain Terrain)
{
	switch (Terrain)
	{
		case EWorldMapTerrain::Clareira: return LOCTEXT("MapaClareira", "clareira");
		case EWorldMapTerrain::Mata:     return LOCTEXT("MapaMata", "mata");
		case EWorldMapTerrain::Margem:   return LOCTEXT("MapaMargem", "margem");
		case EWorldMapTerrain::Agua:     return LOCTEXT("MapaAgua", "água");
		case EWorldMapTerrain::Relevo:   return LOCTEXT("MapaRelevo", "serra");
	}

	return LOCTEXT("MapaDesconhecido", "desconhecido");
}

#undef LOCTEXT_NAMESPACE

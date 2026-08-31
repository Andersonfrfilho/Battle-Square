// Copyright 2026 Anderson. All Rights Reserved.

#include "UI/WorldMapProjection.h"
#include "World/WorldDiscovery.h"

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

float FWorldMapProjection::TerrainTileSideUnits(float LandRadiusUnits)
{
	const float Alcance = FMath::Max(LandRadiusUnits, 1.0f) * TerrainMarginFactor;
	const float Desejado = (Alcance * 2.0f) / static_cast<float>(TerrainTilesAcross);

	// ENCAIXA num divisor da região de descoberta: o pedaço nunca passa do
	// tamanho da região, e sempre cabe um número inteiro deles dentro dela.
	// É o que garante que um pedaço pertença a UMA região só — e portanto que
	// a fronteira do que foi descoberto seja a fronteira que se vê.
	const float Regiao = FWorldDiscovery::RegionSizeUnits();
	const int32 QuantosCabem = FMath::Max(1,
		FMath::CeilToInt(Regiao / FMath::Max(Desejado, 1.0f)));

	return Regiao / static_cast<float>(QuantosCabem);
}

float FWorldMapProjection::NorthAngleDegrees(const FWorldMapSnapshot& Snapshot, EMode Mode)
{
	if (Mode == EMode::NorteAcima)
	{
		return 0.0f;
	}

	// O mapa gira CONTRA o olhar para pôr a frente em cima; então o norte,
	// visto de dentro do mapa, gira junto — pelo mesmo ângulo e no mesmo
	// sentido em que o mundo girou.
	return FRotator::ClampAxis(-Snapshot.PlayerYawDegrees);
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
		case EWorldMapTerrain::Areia:    return FLinearColor(0.52f, 0.45f, 0.28f);
		case EWorldMapTerrain::Gelo:     return FLinearColor(0.40f, 0.46f, 0.52f);
		case EWorldMapTerrain::Lava:     return FLinearColor(0.30f, 0.12f, 0.10f);
		case EWorldMapTerrain::Pantano:  return FLinearColor(0.20f, 0.22f, 0.12f);
		case EWorldMapTerrain::Count:    break;
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
		case EWorldMapTerrain::Areia:    return LOCTEXT("MapaAreia", "deserto");
		case EWorldMapTerrain::Gelo:     return LOCTEXT("MapaGelo", "glaciar");
		case EWorldMapTerrain::Lava:     return LOCTEXT("MapaLava", "vulcão");
		case EWorldMapTerrain::Pantano:  return LOCTEXT("MapaPantano", "pântano");
		case EWorldMapTerrain::Count:    break;
	}

	return LOCTEXT("MapaDesconhecido", "desconhecido");
}

EWorldMapTerrain FWorldMapProjection::TerrainForBiome(EIslandBiome Biome)
{
	switch (Biome)
	{
		// A praia vira MARGEM, e não um terreno novo: margem já quer dizer "a
		// terra molhada onde ela encontra o mar", que é a praia inteira. Dois
		// nomes para a mesma faixa seriam duas cores para o mesmo lugar.
		case EIslandBiome::Beach:   return EWorldMapTerrain::Margem;
		case EIslandBiome::Desert:  return EWorldMapTerrain::Areia;
		case EIslandBiome::Glacier: return EWorldMapTerrain::Gelo;
		case EIslandBiome::Volcano: return EWorldMapTerrain::Lava;
		case EIslandBiome::Swamp:   return EWorldMapTerrain::Pantano;
		case EIslandBiome::Forest:  break;
	}

	return EWorldMapTerrain::Mata;
}

FLinearColor FWorldMapProjection::ColorForPin(EWorldPinKind Kind)
{
	// VIVAS, ao contrário do terreno: a marcação é o que o jogador procura no
	// mapa, e ela precisa saltar do fundo tanto quanto o adversário.
	switch (Kind)
	{
		case EWorldPinKind::Interesse: return FLinearColor(1.00f, 0.85f, 0.30f);
		case EWorldPinKind::Perigo:    return FLinearColor(0.95f, 0.25f, 0.25f);
		case EWorldPinKind::Destino:   return FLinearColor(0.40f, 0.85f, 1.00f);
	}

	return FLinearColor::White;
}

FText FWorldMapProjection::LabelForPin(EWorldPinKind Kind)
{
	switch (Kind)
	{
		case EWorldPinKind::Interesse: return LOCTEXT("PinoInteresse", "sua marca: interesse");
		case EWorldPinKind::Perigo:    return LOCTEXT("PinoPerigo", "sua marca: perigo");
		case EWorldPinKind::Destino:   return LOCTEXT("PinoDestino", "sua marca: destino");
	}

	return LOCTEXT("PinoDesconhecido", "sua marca");
}

#undef LOCTEXT_NAMESPACE

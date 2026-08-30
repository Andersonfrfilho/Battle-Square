// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/** O que um marcador do mapa é — a forma diz a categoria, a cor diz qual. */
UENUM()
enum class EWorldMapMarker : uint8
{
	Jogador,
	CampoDeTreino,
	Adversario,
	Relevo
};

struct BATTLESQUARE_API FWorldMapMarkerInfo
{
	FVector2D WorldXY = FVector2D::ZeroVector;
	FLinearColor Color = FLinearColor::White;
	EWorldMapMarker Kind = EWorldMapMarker::Relevo;

	/** Raio no MUNDO, em unidades. O mapa converte para pixel. */
	float WorldRadiusUnits = 100.0f;
};

/** O mundo como o mapa o vê, num instante. */
struct BATTLESQUARE_API FWorldMapSnapshot
{
	FVector2D PlayerXY = FVector2D::ZeroVector;

	/** Para onde o jogador OLHA, em graus. Yaw 0 é o +X do mundo. */
	float PlayerYawDegrees = 0.0f;

	/** Onde a terra acaba — o mapa desenha a água a partir daqui. */
	float ShoreRadiusUnits = 6000.0f;

	TArray<FWorldMapMarkerInfo> Markers;
};

/**
 * Converte mundo em mapa.
 *
 * PURA, e separada do desenho de propósito: o erro que este tipo de código
 * comete não é de pintura, é de EIXO. Este projeto já pagou por um —
 * "Baixo" andava para a direita porque coluna virou X e linha virou Y — e
 * custou o usuário jogar e descrever o que viu. Aqui o mapeamento tem teste
 * que fixa os quatro pontos cardeais, e ele roda sem abrir o editor.
 *
 * CONVENÇÃO DO MUNDO: +X é o norte, +Y é o leste (é o mesmo mapeamento que a
 * arena usa para a câmera, e discordar dele faria o mapa mentir sobre o
 * mundo).
 */
class BATTLESQUARE_API FWorldMapProjection
{
public:
	/**
	 * Modo do mapa. Não é preferência: são usos diferentes.
	 *
	 * `SeguindoOOlhar` põe para cima o que está à frente — é o que serve
	 * enquanto se anda, porque casa com o que a tela mostra. `NorteAcima` fixa
	 * a orientação, e é o que serve para planejar: um mapa que gira enquanto
	 * se olha para ele não se memoriza.
	 */
	enum class EMode : uint8
	{
		SeguindoOOlhar,
		NorteAcima
	};

	/**
	 * Posição no mapa, em coordenadas de TELA normalizadas: (0,0) é o centro,
	 * X cresce para a direita, Y cresce para BAIXO — a convenção do Slate.
	 *
	 * `RangeUnits` é o raio do mundo que cabe no mapa. Fora dele o resultado
	 * passa de 1, e quem desenha decide se recorta ou empurra para a borda.
	 */
	static FVector2D ToMapSpace(const FVector2D& WorldXY, const FWorldMapSnapshot& Snapshot,
		EMode Mode, float RangeUnits);

	/** Para onde a seta do jogador aponta na tela, em graus horários. */
	static float PlayerArrowAngleDegrees(const FWorldMapSnapshot& Snapshot, EMode Mode);
};

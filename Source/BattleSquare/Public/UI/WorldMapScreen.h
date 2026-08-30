// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/WorldMapProjection.h"

class UWorld;

/**
 * O minimapa no canto, e o mapa completo por cima.
 *
 * DOIS mapas, e não um redimensionado: o do canto segue o olhar, porque casa
 * com o que a tela mostra enquanto se anda; o completo fica com o norte acima,
 * porque mapa que gira não se memoriza. Usos diferentes, orientações
 * diferentes.
 *
 * Slate por código, sem asset autorado — o caminho que funciona neste projeto.
 * Compilado TAMBÉM em Shipping: mapa é jogo, não ferramenta.
 */
class BATTLESQUARE_API FWorldMapScreen
{
public:
	static void Show(UWorld* World);
	static void Hide();

	/** Atualiza o que os dois mapas desenham. */
	static void Update(const FWorldMapSnapshot& Snapshot);

	/** Abre e fecha o mapa completo. O minimapa não sai. */
	static void ToggleFullMap();

	static bool IsFullMapOpen();
	static bool IsVisible();
};

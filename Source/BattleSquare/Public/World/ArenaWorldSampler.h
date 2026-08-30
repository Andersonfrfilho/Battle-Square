// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "World/ArenaFromWorld.h"

class UWorld;

/**
 * Varre o mundo em volta de um ponto e diz o que há ali.
 *
 * A metade SUJA de "a arena é o lugar": esta parte precisa de `UWorld`, de
 * iteradores de ator e de instâncias de malha, e por isso não se verifica
 * headless. Ela não decide NADA — só coleta. Quem transforma amostra em
 * tabuleiro é `FArenaFromWorld`, que é pura e tem teste.
 *
 * A separação é o ponto: sem ela, a regra de qual pedra vira qual casa só
 * poderia ser verificada abrindo o Editor.
 */
class BATTLESQUARE_API FArenaWorldSampler
{
public:
	static void Collect(const UWorld* World, const FVector& Around,
		TArray<FWorldFeatureSample>& OutFeatures);
};

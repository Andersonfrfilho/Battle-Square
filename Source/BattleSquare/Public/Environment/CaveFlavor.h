// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * O sabor de uma caverna — o que existe dentro dela.
 *
 * Mora em cabeçalho próprio, e não dentro de `ACaveSystem`, porque quem DECIDE
 * o sabor é o planejador da ilha (`IslandFeatureLayout`), que não deve arrastar
 * um ator inteiro só para nomear três valores. A decisão é do mapa: caverna
 * perto do vulcão tem lava, caverna perto da água tem água, o resto é pedra
 * seca. Sortear o sabor faria o mundo mentir sobre onde a pessoa está.
 *
 * Valores novos entram no FIM: a ordem é serializável e aparece em teste.
 */
enum class ECaveFlavor : uint8
{
	/** Pedra e mais nada. A caverna que se explora sem pensar duas vezes. */
	Dry,

	/**
	 * Poças de lava. NÃO se entra nela.
	 *
	 * Isto não é decoração: é a única regra que separa a caverna que se explora
	 * da que só se olha, e ela se explica sozinha. Ninguém precisa de um aviso
	 * para entender por que a boca de uma caverna de lava está fechada.
	 */
	Lava,

	/** Poças de água salgada, das que ficam quando a maré desce. */
	Water,
};

/**
 * Se dá para entrar.
 *
 * Uma função, um lugar. Espalhar `Sabor != Lava` pelo ator, pelo planejador e
 * pelo painel seria três cópias que concordam até a primeira edição — que foi
 * exatamente como nasceram L-032 e L-033.
 */
inline bool IsCaveExplorable(ECaveFlavor Flavor)
{
	return Flavor != ECaveFlavor::Lava;
}

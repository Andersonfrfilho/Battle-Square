// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * O BOSQUE É DO ASSENTAMENTO; A MATA SELVAGEM É REGULADA (mundo-vivo, MV7 —
 * decisões 35/68-d). Corte livre no bosque que o assentamento plantou; na mata
 * selvagem, o Guarda Florestal regula (o mesmo padrão exceção-com-prazo de MV3).
 */
namespace ForestRegulation
{
	/**
	 * Pode cortar aqui livremente?
	 *
	 * No bosque plantado (do assentamento), sim. Na mata selvagem, só se a
	 * regulação permitir — o Guarda Florestal barra o corte livre. O contrapeso:
	 * mata selvagem NUNCA é corte livre por omissão.
	 */
	BATTLESQUARE_API bool MayCutFreely(bool bIsPlantedGrove, bool bRegulationAllows);
}

// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Battle/FluidRegistry.h"

class UIslandBakedPlan;

/**
 * O QUE O PÉ ENCONTRA num ponto da ilha.
 *
 * A água precisa MOLHAR. Um rio desenhado que não muda nada ao ser pisado é
 * enfeite: ele lê como obstáculo e se comporta como chão, e é essa promessa
 * quebrada que faz a travessia — as 56 do traçado — perder o sentido.
 */
enum class EWaterFooting : uint8
{
	/** Terra. */
	Seco,

	/** VAU: raso o bastante para passar andando, molhando o pé. */
	Vau,

	/** FUNDO: aqui não se anda. Atravessa-se nadando, ou por obra. */
	Fundo
};

namespace WaterFooting
{
	/**
	 * O que há sob os pés nesta posição.
	 *
	 * A LARGURA decide, e é a mesma largura que desenha o rio — passa por
	 * `FreshWater::NavigabilityForHalfWidth`, que já é a regra. Uma segunda
	 * tabela de fundura concordaria com a primeira até alguém alargar um rio.
	 */
	BATTLESQUARE_API EWaterFooting At(const UIslandBakedPlan& Baked,
		const FVector2D& PositionUnits);

	/**
	 * Quanto o passo rende aqui, como fração do passo em terra.
	 *
	 * Fração, e não velocidade em unidades: número absoluto escolhido quando só
	 * existia uma velocidade é a armadilha que este projeto já mediu em sete
	 * lugares.
	 */
	BATTLESQUARE_API float SpeedMultiplierFor(EWaterFooting Footing);

	BATTLESQUARE_API const TCHAR* DebugName(EWaterFooting Footing);

	/**
	 * DE QUE FLUIDO é a água que se está pisando.
	 *
	 * O mundo tinha seis fluidos e tratava todos como uma água só. Quem entra
	 * numa fonte termal na saia do vulcão sentia o mesmo que quem atravessa um
	 * córrego de montanha — e o registro, que existe para separá-los, não
	 * chegava até aqui.
	 *
	 * Devolve `Nenhum` em terra firme. Fora da costa é ÁGUA SALGADA: o mar não
	 * está no traçado como curso, ele é o que sobra depois que a terra acaba.
	 */
	BATTLESQUARE_API EFluidKind FluidAt(const UIslandBakedPlan& Baked,
		const FVector2D& PositionUnits);
}

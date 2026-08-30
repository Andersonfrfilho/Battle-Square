// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Onde as montanhas e as cavernas ficam na ilha.
 *
 * O planejamento vive aqui, fora do GameMode, por um motivo que já custou caro
 * neste projeto: um ângulo errado põe a caverna DENTRO da montanha, e isso é
 * exatamente o tipo de defeito que só aparece quando alguém abre o jogo, anda
 * até lá e olha. Sendo uma função pura sobre números, a mesma pergunta vira
 * uma asserção que roda em milissegundos.
 *
 * A ilha é apertada, e é por isso que os números não são redondos. A terra tem
 * seis mil unidades de raio, e o miolo até duas mil e duzentas já é dos campos
 * de treino. Sobra um anel — e a caverna grande, com quase três mil unidades de
 * lado, só cabe nele encaixada num VÃO entre dois campos. Ela é a única peça
 * que depende do ângulo para caber; as outras têm folga radial e podem ir a
 * qualquer lado.
 *
 * A posição é decisão de projeto, não sorteio. Uma montanha que muda de lugar a
 * cada partida não é paisagem: é o mapa deixando de ser um lugar que a pessoa
 * aprende.
 */
namespace IslandFeatureLayout
{
	enum class EIslandFeature : uint8
	{
		WalkableMountain,
		Cave,
	};

	/** Uma peça plantada: o que é, onde fica e quanto espaço ocupa. */
	struct BATTLESQUARE_API FFeaturePlacement
	{
		EIslandFeature Feature = EIslandFeature::WalkableMountain;

		float AngleDegrees = 0.0f;
		float RadiusUnits = 0.0f;

		/**
		 * Raio do círculo que a peça não deixa ninguém invadir.
		 *
		 * Para a caverna é a META-DIAGONAL, não o meio-lado: o quadrado dela
		 * não está girado para o vizinho, e usar o meio-lado deixaria a quina
		 * passar por dentro da montanha sem nenhum teste reclamar.
		 */
		float ClearanceUnits = 0.0f;

		/** Lado do labirinto quando é caverna; zero quando é montanha. */
		int32 CaveSide = 0;

		FVector2D CenterUnits() const;
	};

	/** As medidas da ilha que a decisão de plantar precisa respeitar. */
	struct BATTLESQUARE_API FIslandBounds
	{
		float LandRadiusUnits = 6000.0f;
		float TrainingRingRadiusUnits = 1800.0f;
		float TrainingFieldRadiusUnits = 400.0f;
		int32 TrainingFieldCount = 5;
	};

	BATTLESQUARE_API float MountainClearanceUnits();
	BATTLESQUARE_API float CaveClearanceUnits(int32 Side);

	/** As peças da ilha, na ordem em que são plantadas. */
	BATTLESQUARE_API TArray<FFeaturePlacement> Plan();

	/** A peça inteira cabe na terra — nenhuma quina cai na água. */
	BATTLESQUARE_API bool FitsOnLand(const FFeaturePlacement& Placement, const FIslandBounds& Bounds);

	/** A peça não encosta em nenhum campo de treino. */
	BATTLESQUARE_API bool ClearsTrainingFields(const FFeaturePlacement& Placement, const FIslandBounds& Bounds);

	/** Duas peças se invadem. */
	BATTLESQUARE_API bool Overlaps(const FFeaturePlacement& First, const FFeaturePlacement& Second);
}

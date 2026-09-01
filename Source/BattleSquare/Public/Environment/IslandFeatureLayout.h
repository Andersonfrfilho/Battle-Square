// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Environment/CaveFlavor.h"
#include "Environment/IslandGeography.h"

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

		/**
		 * O vulcão. Entra no fim porque a ordem daqui vira o `if` de quem
		 * planta, e valor novo no meio remendaria despacho já escrito.
		 */
		Volcano,
	};

	/**
	 * A semente de uma peça. A FONTE ÚNICA.
	 *
	 * Ela morava no GameMode, e por isso ninguém de fora conseguia reproduzir
	 * o labirinto de uma caverna — nem o mapa que quer desenhá-lo. Semente que
	 * só existe onde o ator nasce é semente que nada mais pode conferir.
	 *
	 * Muda com o ÂNGULO: com a mesma para todas, as três trilhas de monte
	 * serpenteariam igual e a ilha pareceria a mesma montanha copiada.
	 */
	BATTLESQUARE_API uint32 SeedForPlacement(int32 WorldSeed, const struct FFeaturePlacement& Placement);

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

		/**
		 * O outro lado, quando a caverna não é quadrada.
		 *
		 * Zero quer dizer "igual ao primeiro". Caverna sempre quadrada é
		 * assinatura de gerador: a rocha não faz quadrados, e cinco quadrados
		 * de tamanhos diferentes leem como cinco versões da mesma caverna.
		 */
		int32 CaveOtherSide = 0;

		/**
		 * O que a caverna tem dentro. Ignorado pelas outras peças.
		 *
		 * Mora AQUI porque é o planejador quem sabe onde o vulcão e o mar ficam.
		 * Redecidir isso na hora de plantar seria a segunda cópia da regra, e a
		 * segunda cópia concorda com a primeira até alguém mexer numa delas.
		 */
		ECaveFlavor CaveFlavor = ECaveFlavor::Dry;

		FVector2D CenterUnits() const;
	};

	/** As medidas da ilha que a decisão de plantar precisa respeitar. */
	struct BATTLESQUARE_API FIslandBounds
	{
		float LandRadiusUnits = IslandGeography::LandRadiusUnits();
		float TrainingRingRadiusUnits = 1800.0f;
		float TrainingFieldRadiusUnits = 400.0f;
		int32 TrainingFieldCount = 5;
	};

	BATTLESQUARE_API float MountainClearanceUnits();
	BATTLESQUARE_API float CaveClearanceUnits(int32 Side);
	BATTLESQUARE_API float VolcanoClearanceUnits();

	/** As peças da ilha, na ordem em que são plantadas. */
	BATTLESQUARE_API TArray<FFeaturePlacement> Plan();

	/** A peça inteira cabe na terra — nenhuma quina cai na água. */
	BATTLESQUARE_API bool FitsOnLand(const FFeaturePlacement& Placement, const FIslandBounds& Bounds);

	/** A peça não encosta em nenhum campo de treino. */
	BATTLESQUARE_API bool ClearsTrainingFields(const FFeaturePlacement& Placement, const FIslandBounds& Bounds);

	/** Duas peças se invadem. */
	BATTLESQUARE_API bool Overlaps(const FFeaturePlacement& First, const FFeaturePlacement& Second);
}

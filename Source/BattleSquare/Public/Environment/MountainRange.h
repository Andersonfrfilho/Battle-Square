// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Environment/ScenaryClimate.h"
#include "GameFramework/Actor.h"
#include "Templates/Function.h"
#include "MountainRange.generated.h"

class UHierarchicalInstancedStaticMeshComponent;

/**
 * A serra do horizonte — o que fecha o fundo ATRÁS da mata.
 *
 * A mata veste os primeiros metros e para: além do último tronco havia céu
 * encostado no chão, e um mundo sem relevo distante não tem tamanho. Sem nada
 * ao longe, a clareira de sessenta metros é o universo inteiro.
 *
 * Três decisões que não são de gosto:
 *
 * 1. **A escala é a real.** Os picos têm quilômetros e ficam a quilômetros.
 *    Comprimir a serra para caber perto entregaria um morro grande colado no
 *    quintal, e a atmosfera da engine — que azula o que está longe — não teria
 *    distância nenhuma para trabalhar.
 * 2. **A neve sai do clima, não da vontade.** Quem decide é
 *    `ScenaryClimate::SnowCapFraction`, e ela é aritmética pura, verificável
 *    sem levantar mundo. Aqui só se desenha o que ela mandou.
 * 3. **Nenhum pico é um cone.** Cada montanha é um corpo principal mais dois
 *    contrafortes de altura e largura próprias, com base elíptica e giro
 *    próprios. Um cone sozinho no horizonte lê como primitiva da engine, que
 *    é exatamente a queixa que esta serra existe para responder.
 *
 * Não colide e não projeta sombra: é cenário a quilômetros, e uma sombra de
 * três quilômetros estouraria as cascatas do sol por nada.
 */
UCLASS()
class BATTLESQUARE_API AMountainRange : public AActor
{
	GENERATED_BODY()

public:
	AMountainRange();

	/**
	 * Ergue a serra em volta do ponto onde este ator está.
	 *
	 * O clima entra por parâmetro, e não por campo do ator, porque quem sabe
	 * o bioma é quem monta a cena — a serra só obedece.
	 */
	void BuildRange(EScenaryClimate Climate, uint32 Seed);

	/**
	 * Ergue a serra deixando cada pico perguntar o clima do LUGAR onde ele caiu.
	 *
	 * A roda de montanhas tem quilômetros de raio e atravessa os setores de
	 * bioma da ilha: um clima só para a serra inteira poria neve no pico que
	 * nasceu dentro do deserto e deixaria pelado o que nasceu no glaciar.
	 */
	void BuildRangeAcrossIsland(uint32 Seed);

	/** Quantas montanhas a última montagem ergueu. */
	int32 GetPeakCount() const;

	/** Quantos cumes ficaram com gelo. */
	int32 GetSnowCapCount() const;

	/** Altura, em metros, de cada corpo erguido — o que o teste mede. */
	const TArray<float>& GetPeakHeightsMeters() const { return PeakHeightsMeters; }

	UHierarchicalInstancedStaticMeshComponent* GetRockPeaks() const { return RockPeaks; }
	UHierarchicalInstancedStaticMeshComponent* GetSnowCaps() const { return SnowCaps; }

	/** Unidades de mundo por metro. A engine usa centímetro. */
	static constexpr float UnitsPerMeter = 100.0f;

private:
	/**
	 * Põe um corpo de `AlturaMetros` na direção dada, com o gelo que o clima
	 * mandar.
	 *
	 * Um lugar só para rocha e gelo porque as duas geometrias são a MESMA:
	 * o cume é o próprio corpo reduzido à fatia que passa da linha da neve.
	 * Calculadas em dois lugares, elas se desencontram na primeira edição e o
	 * gelo flutua acima da pedra (L-032/L-033).
	 */
	void RaiseBody(const FVector& BaseLocation, float HeightMeters,
		float BaseRadiusMeters, float FlatteningY, float YawDegrees, EScenaryClimate Climate);

	/**
	 * O corpo da montagem, com o clima entrando por PERGUNTA em vez de valor.
	 *
	 * As duas portas públicas diferem só nisso, e um lugar só as atende porque
	 * duas cópias do sorteio se desencontrariam na primeira edição — e a serra
	 * que já foi vista mudaria de forma sem ninguém pedir (L-032).
	 */
	void BuildRangeWith(uint32 Seed, TFunctionRef<EScenaryClimate(const FVector&)> ClimateAtBase);

	UPROPERTY()
	TObjectPtr<USceneComponent> RangeRoot;

	UPROPERTY()
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> RockPeaks;

	UPROPERTY()
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> SnowCaps;

	/** Altura de cada corpo erguido, na ordem em que entraram. */
	TArray<float> PeakHeightsMeters;
};

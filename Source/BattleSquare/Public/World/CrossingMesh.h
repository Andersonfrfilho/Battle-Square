// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "World/IslandBakedPlan.h"
#include "CrossingMesh.generated.h"

class UProceduralMeshComponent;

/**
 * AS TRAVESSIAS — os 56 lugares onde a trilha encontra a água.
 *
 * Cada tipo é uma coisa DIFERENTE, e essa é a razão de a travessia existir:
 * se todas se atravessassem igual, o traçado não precisaria de quatro.
 *
 * | tipo     | o que é                              | o que se constrói |
 * |----------|--------------------------------------|-------------------|
 * | vau      | raso: passa-se andando               | NADA              |
 * | ponte    | fundo, margens no mesmo nível        | um tabuleiro      |
 * | barranco | uma margem é um degrau               | uma rampa cavada  |
 * | balsa    | largo demais para ponte              | uma plataforma    |
 *
 * O VAU NÃO CONSTRÓI NADA, e isso é decisão, não omissão. Ele é a AUSÊNCIA de
 * obstáculo — quem decide que ali se passa a pé é `WaterFooting`, e pôr uma
 * geometria no vau seria construir uma obra onde o traçado disse que não há.
 */
UCLASS()
class BATTLESQUARE_API ACrossingMesh : public AActor
{
	GENERATED_BODY()

public:
	ACrossingMesh();

	/** Constrói as travessias do assado. Devolve quantas ganharam obra. */
	int32 BuildFrom(const UIslandBakedPlan& Baked);

	UProceduralMeshComponent* GetStructure() const { return Structure; }

	/** Quantas travessias de cada tipo foram VISTAS no traçado. */
	int32 GetSeenCount(uint8 Kind) const;

	/** Quantas ganharam GEOMETRIA. O vau é visto e não construído, de propósito. */
	int32 GetBuiltCount(uint8 Kind) const;

	/** Quantas travessias o traçado tem ao todo. */
	int32 GetSeenTotal() const;

	/** Quanto o tabuleiro de uma ponte fica acima da lâmina d'água. */
	static float DeckClearanceUnits();

	/**
	 * A obra construída para a travessia de índice dado no assado.
	 *
	 * O ator diz de que altura ficou CADA obra, em vez de deixar quem confere
	 * descobrir por proximidade. Atribuir geometria a uma travessia pela
	 * distância elege um dono, e o vizinho ganha a peça quando duas obras
	 * ficam perto — foi assim que a rampa de um barranco passou por balsa.
	 */
	struct FBuiltCrossing
	{
		uint8 Kind = 0;
		float TopZ = 0.0f;
		float BottomZ = 0.0f;
		float WaterZ = 0.0f;
	};

	const TArray<FBuiltCrossing>& GetBuilt() const { return Built; }

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	TObjectPtr<UProceduralMeshComponent> Structure;

	UPROPERTY()
	TArray<int32> SeenByKind;

	UPROPERTY()
	TArray<int32> BuiltByKind;

	/** Não é `UPROPERTY`: é medição da construção, lida pela conferência. */
	TArray<FBuiltCrossing> Built;

public:
	/**
	 * ONDE VAI CADA BALSA — posição, rumo, vão e a altura da lâmina.
	 *
	 * A travessia CALCULA a balsa e não a instancia. Quem põe ator no mundo
	 * neste projeto é o `GameMode`; um ator que monta malha nascendo outros
	 * atores por dentro mistura duas responsabilidades, e foi o que derrubou o
	 * processo com uma asserção de thread ao erguer as 25 de uma vez.
	 *
	 * Separado, cada lado se prova sozinho: aqui, que a balsa foi PLANEJADA no
	 * lugar certo; no ator dela, que ela anda, flutua e esbarra.
	 */
	struct FFerryPlacement
	{
		FVector2D CenterUnits = FVector2D::ZeroVector;
		FVector2D AxisUnits = FVector2D(1.0f, 0.0f);
		float SpanUnits = 0.0f;
		float WaterZ = 0.0f;
	};

	const TArray<FFerryPlacement>& GetFerryPlacements() const { return FerryPlacements; }

private:
	/** Não é `UPROPERTY`: é plano de construção, não estado de jogo. */
	TArray<FFerryPlacement> FerryPlacements;
};

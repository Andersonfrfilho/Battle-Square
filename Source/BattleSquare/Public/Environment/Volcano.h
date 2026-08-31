// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "Volcano.generated.h"

class UHierarchicalInstancedStaticMeshComponent;
class USceneComponent;
class UStaticMeshComponent;
class UPointLightComponent;

/**
 * O vulcão do setor de vulcão: o marco mais alto da ilha.
 *
 * Ele não é uma montanha escura. O que faz um vulcão ser lido como vulcão à
 * distância é a CRATERA — um topo cortado, com um anel de rocha e luz dentro.
 * Um cone que termina em ponta é um morro, por mais preto que seja pintado.
 *
 * Por isso o corpo NÃO é o cone da engine, que é o caminho fácil e o errado:
 * a ponta dele não se corta. O corpo é uma pilha de discos de raio decrescente
 * que PARA antes do topo, e o degrau que sobra em cada disco é o que dá as
 * camadas de derrame que uma encosta vulcânica tem.
 *
 * A cratera é o que se vê de cima: um anel de blocos em volta de um poço, e a
 * lava no fundo dele. A lava fica ABAIXO da borda de propósito — lava no nível
 * do topo lê como uma tampa laranja, não como algo contido.
 *
 * Diferente de `AWalkableMountain`, aqui não há trilha e não se sobe. Uma
 * trilha existe para levar a algum lugar; o topo deste é um poço de lava, e
 * levar alguém até lá seria prometer um destino que o jogo não tem.
 */
UCLASS()
class BATTLESQUARE_API AVolcano : public AActor
{
	GENERATED_BODY()

public:
	AVolcano();

	/** Monta o cone, a cratera e os derrames. A semente só mexe nos derrames. */
	void BuildVolcano(uint32 Seed);

	float GetHeightUnits() const { return HeightUnits; }
	float GetBaseRadiusUnits() const { return BaseRadiusUnits; }

	/**
	 * O raio do POÇO no topo — o vazio, não a rocha em volta.
	 *
	 * É o que separa este ator de um cone: aqui `RadiusAtHeight` no cume
	 * devolve um número POSITIVO, e é ele.
	 */
	float GetCraterRadiusUnits() const;

	/** Onde a lava está parada. Sempre abaixo da borda da cratera. */
	float GetLavaSurfaceUnits() const;

	/** O raio da encosta naquela altura. Nunca chega a zero. */
	float RadiusAtHeight(float ZUnits) const;

	/**
	 * A luz da cratera: o que faz o vulcão ACENDER em vez de só ser laranja.
	 *
	 * Pintar a lava de laranja não é brilhar. À noite o material pintado fica
	 * tão escuro quanto o basalto ao lado, e o vulcão vira uma silhueta de
	 * pedra — que é exatamente o que ele já era. Quem acende o entorno é uma
	 * luz de verdade, e é ela que conta que há lava lá dentro para quem está
	 * longe.
	 */
	UPointLightComponent* GetCraterGlow() const;

	/** A coluna de fumaça, para o vulcão ser achado de longe também de dia. */
	UHierarchicalInstancedStaticMeshComponent* GetPlume() const;

	int32 GetSliceCount() const { return SliceCount; }

	/** Cada bloco do anel da cratera, para conferir o poço sem abrir o jogo. */
	const TArray<FTransform>& GetRimBlocks() const { return RimBlocks; }

	/** Cada pedaço de derrame, na ordem em que desce. */
	const TArray<FTransform>& GetFlowSteps() const { return FlowSteps; }

	int32 GetFlowCount() const { return FlowCount; }

	UHierarchicalInstancedStaticMeshComponent* GetSlopes() const { return Slopes; }
	UHierarchicalInstancedStaticMeshComponent* GetRim() const { return Rim; }
	UHierarchicalInstancedStaticMeshComponent* GetFlows() const { return Flows; }
	UStaticMeshComponent* GetLava() const { return Lava; }

	/**
	 * O raio da base com que um vulcão nasce.
	 *
	 * Aqui em cima, e não só como valor inicial do campo, porque QUEM ESCOLHE
	 * ONDE PLANTAR precisa do tamanho antes de existir um vulcão para medir —
	 * a mesma razão de `ACaveSystem::DefaultCellSizeUnits` (L-032).
	 */
	static constexpr float DefaultBaseRadiusUnits = 2600.0f;

private:
	/** Empilha os discos da encosta, do chão até a boca da cratera. */
	void RaiseSlopes();

	/** Fecha o anel de rocha em volta do poço. */
	void CloseRim();

	/** Derrama a lava pela encosta abaixo. */
	void PourFlows(uint32 Seed);

	/** Levanta a coluna de fumaça acima do cume. */
	void RaisePlume(uint32 Seed);

	UPROPERTY()
	TObjectPtr<USceneComponent> VolcanoRoot;

	UPROPERTY()
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Slopes;

	UPROPERTY()
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Rim;

	UPROPERTY()
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Flows;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Lava;

	UPROPERTY()
	TObjectPtr<UPointLightComponent> CraterGlow;

	/**
	 * A pluma: esferas subindo do cume, cada uma maior e mais alta.
	 *
	 * Sem sombra e sem colisão — fumaça que projeta sombra dura no chão é uma
	 * bola de pedra flutuando, e foi assim que a "montanha pontuda" começou:
	 * primitiva usada como se fosse a coisa, sem pensar em como ela lê.
	 */
	UPROPERTY()
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Plume;

	TArray<FTransform> RimBlocks;
	TArray<FTransform> FlowSteps;

	/** Mais alto que a montanha que se sobe: o vulcão é o marco do setor. */
	UPROPERTY(EditAnywhere, Category = "Vulcao")
	float HeightUnits = 3600.0f;

	UPROPERTY(EditAnywhere, Category = "Vulcao")
	float BaseRadiusUnits = DefaultBaseRadiusUnits;

	/**
	 * O poço, como fração da base.
	 *
	 * Abaixo de um quinto a cratera some de longe e o vulcão volta a ser um
	 * morro; acima de um terço a encosta fica curta demais para as camadas
	 * aparecerem, e o corpo lê como um copo.
	 */
	UPROPERTY(EditAnywhere, Category = "Vulcao")
	float CraterRadiusFraction = 0.3f;

	/** Quanto a boca da cratera afunda até encontrar a lava. */
	UPROPERTY(EditAnywhere, Category = "Vulcao")
	float CraterDepthUnits = 420.0f;

	/** Quantos discos formam a encosta. */
	UPROPERTY(EditAnywhere, Category = "Vulcao")
	int32 SliceCount = 26;

	/** Quantos blocos fecham o anel da cratera. */
	UPROPERTY(EditAnywhere, Category = "Vulcao")
	int32 RimBlockCount = 20;

	/**
	 * Quanto cada bloco do anel passa do vizinho.
	 *
	 * Acima de 1 pela mesma razão do patamar da trilha: dois blocos que apenas
	 * se encostam deixam uma fresta, e uma fresta no anel deixa a lava vazar
	 * pela lateral em vez de ficar contida.
	 */
	UPROPERTY(EditAnywhere, Category = "Vulcao")
	float RimOverlap = 1.35f;

	/** A espessura do disco de lava. Só precisa não ser um plano. */
	UPROPERTY(EditAnywhere, Category = "Vulcao")
	float LavaThicknessUnits = 80.0f;

	/** Quanto do poço a lava enche. Menos que tudo, para a borda aparecer. */
	UPROPERTY(EditAnywhere, Category = "Vulcao")
	float LavaFillFraction = 0.92f;

	/** Quantos derrames descem a encosta. */
	UPROPERTY(EditAnywhere, Category = "Vulcao")
	int32 FlowCount = 5;

	/** Em quantos pedaços cada derrame se divide ao descer. */
	UPROPERTY(EditAnywhere, Category = "Vulcao")
	int32 FlowStepCount = 16;

	UPROPERTY(EditAnywhere, Category = "Vulcao")
	float FlowWidthUnits = 150.0f;

	UPROPERTY(EditAnywhere, Category = "Vulcao")
	float FlowThicknessUnits = 40.0f;

	/** Quantas bolas de fumaça sobem da cratera. */
	UPROPERTY(EditAnywhere, Category = "Vulcao")
	int32 PlumePuffCount = 14;

	/** Quanto a coluna sobe acima do cume. */
	UPROPERTY(EditAnywhere, Category = "Vulcao")
	float PlumeHeightUnits = 2800.0f;

	/**
	 * O alcance da luz da cratera, em múltiplos do raio da base.
	 *
	 * Menor que 1 e a luz morre dentro da própria cratera, iluminando só a
	 * lava que já era laranja — que é o mesmo nada de antes.
	 */
	UPROPERTY(EditAnywhere, Category = "Vulcao")
	float GlowReachInBaseRadii = 2.2f;

	/** A intensidade da brasa. */
	UPROPERTY(EditAnywhere, Category = "Vulcao")
	float GlowIntensity = 90000.0f;
};

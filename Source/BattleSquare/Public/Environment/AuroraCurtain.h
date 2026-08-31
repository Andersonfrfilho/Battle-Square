// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "AuroraCurtain.generated.h"

class UHierarchicalInstancedStaticMeshComponent;
class UPointLightComponent;
class USceneComponent;

/**
 * A aurora boreal do setor de geleira: a única coisa deste mundo que fica no
 * céu e não no chão.
 *
 * Ela existia como REGRA desde o relógio de dia e noite —
 * `WorldNightSky::AuroraStrength` já dizia a força dela, e o painel já escrevia
 * `AURORA BOREAL` — e não existia como nada que se pudesse ver. Regra sem
 * imagem é exatamente o defeito que este projeto pagou três vezes: passa em
 * todo teste e não está na tela.
 *
 * O desenho é uma CORTINA, não um véu chapado no horizonte: fitas verticais
 * penduradas de uma altura grande, encurvadas ao longo de um arco e ondulando
 * de leve. A curva é o que faz a coisa ler como aurora — uma faixa reta no céu
 * lê como uma parede pintada, e foi essa a reclamação que derrubou a primeira
 * geração de montanhas.
 *
 * A cor muda com a altura porque a aurora real muda de gás com a altura; aqui
 * ela muda para a cortina ter TOPO. Verde embaixo (`AuroraVeil`), roxo em cima
 * (`AuroraCrown`).
 *
 * O que ela NÃO faz: seguir o jogador. A aurora mora sobre a geleira e se vê de
 * longe, que é o motivo de atravessar a ilha para chegar lá. Cortina amarrada à
 * câmera é filtro de tela, não lugar.
 */
UCLASS()
class BATTLESQUARE_API AAuroraCurtain : public AActor
{
	GENERATED_BODY()

public:
	AAuroraCurtain();

	/** Pendura as fitas. A semente só mexe na ondulação de cada uma. */
	void BuildCurtain(uint32 Seed);

	/**
	 * Acende a aurora na força pedida, de 0 a 1.
	 *
	 * A força vem de `WorldNightSky::AuroraStrength` e de mais lugar nenhum: uma
	 * segunda conta de "está de noite?" concordaria com a primeira até a
	 * primeira edição, e aí o painel diria AURORA com o céu vazio (L-032).
	 *
	 * Em 0 a cortina fica INVISÍVEL, e não apenas escura. Sem canal de emissão,
	 * verde escuro contra o azul do dia continua sendo uma mancha — e mancha
	 * verde no céu ao meio-dia é pior que aurora nenhuma.
	 */
	void SetStrength(float Strength);

	float GetStrength() const { return Strength; }

	/**
	 * Onde no mundo esta cortina deve ser plantada.
	 *
	 * SAI da geografia em vez de repetir a coordenada da geleira: mover o setor
	 * de gelo e ver a aurora ficar onde estava seria descobrir tarde que havia
	 * duas cópias do mesmo lugar (L-032).
	 */
	static FVector2D SkyCenterUnits();

	UHierarchicalInstancedStaticMeshComponent* GetVeil() const { return Veil; }
	UHierarchicalInstancedStaticMeshComponent* GetCrown() const { return Crown; }

	/**
	 * A luz da aurora: o pouco de verde que cai no gelo.
	 *
	 * Mesma lição da cratera do vulcão — cor pintada não ilumina nada. Sem esta
	 * luz, a geleira embaixo da aurora fica com exatamente o mesmo azul de
	 * noite que a floresta do outro lado da ilha, e a cortina passa a parecer
	 * colada na frente da câmera.
	 */
	UPointLightComponent* GetGlow() const;

	/** Cada fita, na ordem em que foi pendurada. Para conferir sem abrir o jogo. */
	const TArray<FTransform>& GetVeilSegments() const { return VeilSegments; }
	const TArray<FTransform>& GetCrownSegments() const { return CrownSegments; }

	/** A base da cortina, para quem planta saber que ela está acima de tudo. */
	float GetAltitudeUnits() const { return AltitudeUnits; }

	/** O topo do roxo — o ponto mais alto do mundo inteiro. */
	float GetTopUnits() const { return AltitudeUnits + VeilHeightUnits + CrownHeightUnits; }

private:
	/** Pendura uma faixa de fitas entre duas alturas. */
	void HangBand(
		UHierarchicalInstancedStaticMeshComponent* Band,
		TArray<FTransform>& Registro,
		float BottomUnits,
		float HeightUnits,
		uint32 Seed,
		int32 PrimeiroFluxo);

	/** Repinta as duas faixas na força atual, sem trocar a matiz da paleta. */
	void TingeBands();

	UPROPERTY()
	TObjectPtr<USceneComponent> CurtainRoot;

	UPROPERTY()
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Veil;

	UPROPERTY()
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Crown;

	UPROPERTY()
	TObjectPtr<UPointLightComponent> Glow;

	TArray<FTransform> VeilSegments;
	TArray<FTransform> CrownSegments;

	/** Quanto da aurora está acesa agora, de 0 a 1. */
	float Strength = 0.0f;

	/**
	 * A que altura a cortina começa.
	 *
	 * Mais alto que o vulcão (3600) e que qualquer montanha, com folga: aurora
	 * que passa RASANTE por um cume lê como neblina presa no morro.
	 */
	UPROPERTY(EditAnywhere, Category = "Aurora")
	float AltitudeUnits = 9000.0f;

	/** A altura do verde. */
	UPROPERTY(EditAnywhere, Category = "Aurora")
	float VeilHeightUnits = 3200.0f;

	/** A altura do roxo, em cima do verde. Menor, porque é o remate. */
	UPROPERTY(EditAnywhere, Category = "Aurora")
	float CrownHeightUnits = 1700.0f;

	/**
	 * Quantas fitas.
	 *
	 * Uma só não é cortina, é faixa. Acima de umas quatro elas se encobrem e o
	 * conjunto volta a ler como parede — o oposto do que as fitas existem para
	 * evitar.
	 */
	UPROPERTY(EditAnywhere, Category = "Aurora")
	int32 RibbonCount = 3;

	/** Em quantos pedaços cada fita se divide ao longo do arco. */
	UPROPERTY(EditAnywhere, Category = "Aurora")
	int32 SegmentCount = 28;

	/** Quanto do céu a cortina atravessa, em graus vistos de baixo. */
	UPROPERTY(EditAnywhere, Category = "Aurora")
	float ArcSpanDegrees = 70.0f;

	/** O raio da curva do arco. Grande, para a curva ser suave e não um cotovelo. */
	UPROPERTY(EditAnywhere, Category = "Aurora")
	float ArcRadiusUnits = 9000.0f;

	/** A distância entre uma fita e a seguinte, na profundidade. */
	UPROPERTY(EditAnywhere, Category = "Aurora")
	float RibbonSpacingUnits = 1600.0f;

	/** O quanto a fita serpenteia para dentro e para fora do arco. */
	UPROPERTY(EditAnywhere, Category = "Aurora")
	float WaveAmplitudeUnits = 1300.0f;

	/** Quantas ondas completas cabem numa fita. */
	UPROPERTY(EditAnywhere, Category = "Aurora")
	float WaveCycles = 2.2f;

	/** A espessura de cada pedaço. Fina: é luz, não muro. */
	UPROPERTY(EditAnywhere, Category = "Aurora")
	float SegmentThicknessUnits = 45.0f;

	/**
	 * Quanto cada pedaço passa do vizinho.
	 *
	 * Acima de 1 pela mesma razão do anel da cratera: dois pedaços que apenas
	 * se encostam deixam uma fresta, e fresta numa cortina de luz aparece como
	 * uma linha preta atravessando ela.
	 */
	UPROPERTY(EditAnywhere, Category = "Aurora")
	float SegmentOverlap = 1.5f;

	/** O alcance da luz que cai no gelo. */
	UPROPERTY(EditAnywhere, Category = "Aurora")
	float GlowReachUnits = 26000.0f;

	/** A intensidade da luz quando a aurora está em cheio. */
	UPROPERTY(EditAnywhere, Category = "Aurora")
	float GlowIntensity = 40000.0f;
};

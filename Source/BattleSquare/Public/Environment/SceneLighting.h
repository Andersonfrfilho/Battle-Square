// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SceneLighting.generated.h"

class UDirectionalLightComponent;
class USkyLightComponent;
class USkyAtmosphereComponent;
class UPostProcessComponent;

/**
 * O sol da cena, em código.
 *
 * O mapa do jogo (WorldStreamingTest) não tem NENHUM ator de luz: nem sol,
 * nem céu, nem SkyLight. Sem sol, a engine ilumina tudo com o ambiente azul
 * padrão — e foi por isso que a mata verde apareceu azul-clara na tela, não
 * por causa do material das folhas.
 *
 * Vive em código pelo mesmo motivo da barra de botões: capacidade que espera
 * edição de asset é capacidade que não existe hoje.
 */
UCLASS()
class BATTLESQUARE_API ABattleSceneLighting : public AActor
{
	GENERATED_BODY()

public:
	ABattleSceneLighting();

	virtual void Tick(float DeltaSeconds) override;

	/**
	 * Põe o sol na posição, na cor e no brilho desta hora.
	 *
	 * É a ÚNICA porta por onde a hora vira luz. Quem quiser um instante
	 * específico chama isto e pronto; quem quiser o dia inteiro passando
	 * liga o ciclo, que chama isto a cada quadro. Duas portas fariam a
	 * arena e o mundo desenharem meio-dias diferentes (L-032).
	 */
	void ApplyHour(float Hour);

	/**
	 * Faz o dia correr, começando nesta hora.
	 *
	 * Quem liga é o MUNDO, e ele é o único: a hora é uma só, e a batalha que
	 * acontece dentro do mundo simplesmente herda o sol que já está lá — 30
	 * segundos de luta adiantam o relógio meia hora, o que se vê como a luz
	 * esquentando um pouco, e é o que deve acontecer.
	 *
	 * O mapa de arena SOZINHO (sem mundo, para verificar tabuleiro) não liga:
	 * ele chama `ApplyHour` uma vez e para. Não é economia — é que ali não
	 * existe relógio de mundo a seguir, e um sol girando num mapa de teste é
	 * uma variável a mais entre a pergunta e a resposta.
	 */
	void StartDayCycle(float StartHour);

	/** A hora que este ator está desenhando agora. */
	float GetHour() const { return CurrentHour; }

	/** Verdadeiro quando o dia está correndo — falso na arena, por projeto. */
	bool IsDayCycleRunning() const { return bDayCycleRunning; }

	/** Quantos segundos de relógio de parede um dia inteiro leva. */
	void SetSecondsPerDay(float Seconds);

	UDirectionalLightComponent* GetSunLight() const { return SunLight; }
	USkyLightComponent* GetSkyLight() const { return SkyLight; }
	USkyAtmosphereComponent* GetSkyAtmosphere() const { return SkyAtmosphere; }

	/**
	 * A exposição da cena, travada.
	 *
	 * Sol quente não basta: com exposição automática a engine reabre o
	 * diafragma até o quadro médio ficar cinza, e o azul do céu — que é
	 * ambiente, ocupa tudo e não tem sombra — é justamente o que sobrevive a
	 * essa reabertura. Foi por isso que a cena continuou lavada de azul mesmo
	 * depois de o sol existir.
	 */
	UPostProcessComponent* GetSceneExposure() const { return SceneExposure; }

	/** Verdadeiro quando o mundo já tem um sol qualquer — dele ou nosso. */
	static bool WorldAlreadyHasSun(const UWorld* World);

	/**
	 * Verdadeiro quando a cena já está acesa por ESTE ator, em outra instância.
	 *
	 * Pergunta diferente de `WorldAlreadyHasSun`, e é a diferença que importa:
	 * o mapa ter sol próprio dispensa o nosso SOL, mas não dispensa a trava de
	 * exposição, que é regra de cena e não de luz. Enquanto as duas perguntas
	 * eram uma só, o mapa com sol próprio descartava o ator inteiro e a tela
	 * continuava lavada de azul, por mais que o sol fosse ajustado.
	 */
	static bool WorldAlreadyLitByUs(const UWorld* World);

	/**
	 * Apaga a iluminação que veio no mapa e devolve quantos componentes calou.
	 *
	 * Na arena a luz é a nossa: somar o sol do mapa ao nosso dá duas vezes a
	 * mesma cena, e o céu e a névoa que o autor do nível deixou são justamente
	 * o que tinge tudo de azul claro. Não toca em nenhum componente deste ator.
	 */
	static int32 DimLightingAuthoredInMap(UWorld* World);

private:
	/** A hora desenhada. Única fonte: a cor e o giro saem dela por função. */
	float CurrentHour = 12.0f;

	bool bDayCycleRunning = false;

	float SecondsPerDay = 1200.0f;

	UPROPERTY()
	TObjectPtr<USceneComponent> LightingRoot;

	UPROPERTY()
	TObjectPtr<UDirectionalLightComponent> SunLight;

	UPROPERTY()
	TObjectPtr<USkyLightComponent> SkyLight;

	UPROPERTY()
	TObjectPtr<USkyAtmosphereComponent> SkyAtmosphere;

	UPROPERTY()
	TObjectPtr<UPostProcessComponent> SceneExposure;
};

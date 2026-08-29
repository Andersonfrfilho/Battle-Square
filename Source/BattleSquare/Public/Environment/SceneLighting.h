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

	/** Verdadeiro quando o mundo já tem um sol próprio e este ator é dispensável. */
	static bool WorldAlreadyHasSun(const UWorld* World);

private:
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

// Copyright 2026 Anderson. All Rights Reserved.

#include "Environment/SceneLighting.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/World.h"
#include "EngineUtils.h"

namespace LuzDaCena
{
	/** Sol de fim de manhã: quente, e ALTO o bastante para a mata não virar silhueta. */
	const FRotator InclinacaoDoSol(-46.0f, -38.0f, 0.0f);

	/**
	 * Quente de propósito. O defeito que este ator conserta era exatamente a
	 * cena inteira puxada para o azul; um sol neutro deixaria metade dele de pé.
	 */
	const FLinearColor CorDoSol(1.0f, 0.94f, 0.82f);

	constexpr float IntensidadeDoSol = 6.0f;

	/** Preenchimento de sombra. Alto demais e a cena volta a lavar. */
	constexpr float IntensidadeDoCeu = 1.0f;
}

ABattleSceneLighting::ABattleSceneLighting()
{
	using namespace LuzDaCena;

	PrimaryActorTick.bCanEverTick = false;

	LightingRoot = CreateDefaultSubobject<USceneComponent>(TEXT("LightingRoot"));
	SetRootComponent(LightingRoot);

	SunLight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("SunLight"));
	SunLight->SetupAttachment(LightingRoot);
	// Movable: ator criado em tempo de execução não participa do build de luz,
	// e luz estática spawnada em jogo simplesmente não acende.
	SunLight->SetMobility(EComponentMobility::Movable);
	SunLight->SetRelativeRotation(InclinacaoDoSol);
	SunLight->SetIntensity(IntensidadeDoSol);
	SunLight->SetLightColor(CorDoSol);
	// É este sol que o céu usa para tingir a atmosfera — sem isso o
	// SkyAtmosphere procura outro e a cena fica sem hora do dia.
	SunLight->bAtmosphereSunLight = true;

	SkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("SkyLight"));
	SkyLight->SetupAttachment(LightingRoot);
	SkyLight->SetMobility(EComponentMobility::Movable);
	SkyLight->SourceType = ESkyLightSourceType::SLS_CapturedScene;
	SkyLight->bRealTimeCapture = true;
	SkyLight->SetIntensity(IntensidadeDoCeu);

	SkyAtmosphere = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("SkyAtmosphere"));
	SkyAtmosphere->SetupAttachment(LightingRoot);
}

bool ABattleSceneLighting::WorldAlreadyHasSun(const UWorld* World)
{
	if (!World)
	{
		return false;
	}

	// Basta o primeiro: a pergunta é "existe sol?", não "quantos?".
	TActorIterator<ADirectionalLight> Sol(const_cast<UWorld*>(World));
	return static_cast<bool>(Sol);
}

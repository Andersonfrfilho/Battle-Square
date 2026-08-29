// Copyright 2026 Anderson. All Rights Reserved.

#include "Environment/SceneLighting.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/PostProcessComponent.h"
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

	constexpr float IntensidadeDoSol = 10.0f;

	/**
	 * Preenchimento de sombra, e SÓ isso.
	 *
	 * Com captura em tempo real de um SkyAtmosphere, o que o céu devolve é
	 * azul por física, não por engano. Ele precisa então ficar bem abaixo do
	 * sol: em pé de igualdade, a luz que vem de todas as direções vence a que
	 * vem de uma só, e a cena inteira puxa para o azul mesmo com sol quente.
	 */
	constexpr float IntensidadeDoCeu = 0.5f;

	/**
	 * Exposição TRAVADA: mínimo e máximo no mesmo valor.
	 *
	 * Travar é o ponto — não o número. Enquanto a exposição era automática, a
	 * engine compensava qualquer ajuste de luz e a tela voltava para o mesmo
	 * cinza-azulado, o que fazia toda medição de cor mentir.
	 */
	constexpr float ExposicaoFixa = 1.0f;
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

	SceneExposure = CreateDefaultSubobject<UPostProcessComponent>(TEXT("SceneExposure"));
	SceneExposure->SetupAttachment(LightingRoot);
	// Sem limite de volume: o efeito vale onde a câmera estiver, e a câmera da
	// arena fica fora de qualquer caixa que coubesse neste ator.
	SceneExposure->bUnbound = true;

	SceneExposure->Settings.bOverride_AutoExposureMinBrightness = true;
	SceneExposure->Settings.AutoExposureMinBrightness = ExposicaoFixa;
	SceneExposure->Settings.bOverride_AutoExposureMaxBrightness = true;
	SceneExposure->Settings.AutoExposureMaxBrightness = ExposicaoFixa;
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

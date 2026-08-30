// Copyright 2026 Anderson. All Rights Reserved.

#include "Environment/SceneLighting.h"

#include "Environment/WorldTimeOfDay.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/LightComponentBase.h"
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

	/** Nenhum ponto de exposição somado por fora — ver o construtor. */
	constexpr float SemRealceExtra = 0.0f;

	/**
	 * O piso de luz ambiente da noite.
	 *
	 * A exposição está TRAVADA (ver acima), então noite sem piso é noite em
	 * que não se enxerga a própria mão — e quem joga conclui que o jogo
	 * travou, não que anoiteceu. O piso é ambiente, não sol: ele clareia sem
	 * projetar sombra, que é justamente como a noite se parece.
	 */
	constexpr float IntensidadeDoLuar = 0.35f;
}

ABattleSceneLighting::ABattleSceneLighting()
{
	using namespace LuzDaCena;

	// Pode tiquetaquear, mas só começa a correr quando alguém liga o ciclo —
	// e a arena, por projeto, nunca liga.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

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

	// Sem realce escondido em cima da trava.
	//
	// O padrão da engine soma um ponto de exposição ao que a medição decidiu.
	// Travar o piso e o teto e deixar esse ponto de pé é travar no lugar
	// errado — e um ponto é justamente a diferença entre a cor do asset e o
	// tom lavado que apareceu na tela.
	SceneExposure->Settings.bOverride_AutoExposureBias = true;
	SceneExposure->Settings.AutoExposureBias = SemRealceExtra;
}

void ABattleSceneLighting::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bDayCycleRunning || SecondsPerDay <= 0.0f)
	{
		return;
	}

	const float HorasPassadas = DeltaSeconds / SecondsPerDay * WorldTimeOfDay::HoursPerDay;
	ElapsedHours += HorasPassadas;
	ApplyHour(CurrentHour + HorasPassadas);
}

void ABattleSceneLighting::ApplyHour(float Hour)
{
	using namespace LuzDaCena;

	CurrentHour = FMath::Fmod(FMath::Fmod(Hour, WorldTimeOfDay::HoursPerDay) + WorldTimeOfDay::HoursPerDay,
		WorldTimeOfDay::HoursPerDay);

	if (SunLight)
	{
		SunLight->SetRelativeRotation(WorldTimeOfDay::SunRotation(CurrentHour));
		SunLight->SetLightColor(WorldTimeOfDay::SunColor(CurrentHour));
		SunLight->SetIntensity(IntensidadeDoSol * BrilhoAgora());
	}

	if (SkyLight)
	{
		// O céu escurece junto com o sol, mas nunca abaixo do luar.
		SkyLight->SetIntensity(FMath::Max(IntensidadeDoCeu * BrilhoAgora(), IntensidadeDoLuar));
	}
}

float ABattleSceneLighting::BrilhoAgora() const
{
	// A hora manda, a nuvem desconta. Dia encoberto é mais escuro que dia
	// limpo e mais claro que noite limpa — e é assim que a chuva aparece na
	// tela sem uma única partícula.
	return WorldTimeOfDay::SunBrightness(CurrentHour) * WorldWeather::SunDimming(CurrentWeather);
}

void ABattleSceneLighting::SetWeather(EWeather Weather)
{
	if (Weather == CurrentWeather)
	{
		return;
	}

	CurrentWeather = Weather;
	ApplyHour(CurrentHour);
}

void ABattleSceneLighting::StartDayCycle(float StartHour)
{
	bDayCycleRunning = true;
	ElapsedHours = 0.0f;
	PrimaryActorTick.SetTickFunctionEnable(true);
	ApplyHour(StartHour);
}

void ABattleSceneLighting::SetSecondsPerDay(float Seconds)
{
	SecondsPerDay = FMath::Max(Seconds, 1.0f);
}

bool ABattleSceneLighting::WorldAlreadyHasSun(const UWorld* World)
{
	if (!World)
	{
		return false;
	}

	UWorld* Cena = const_cast<UWorld*>(World);

	// Duas formas de a cena já estar acesa, e as duas contam.
	//
	// Procurar só por ADirectionalLight respondia "não" para a luz que ESTE
	// ator acende, porque ele é um AActor com componente de luz dentro, não o
	// ator de luz da engine. O mundo aberto acendia o cenário, a arena
	// perguntava, ouvia não, e acendia um segundo sol em cima do primeiro.
	if (TActorIterator<ABattleSceneLighting>(Cena))
	{
		return true;
	}

	// Basta o primeiro: a pergunta é "existe sol?", não "quantos?".
	TActorIterator<ADirectionalLight> Sol(Cena);
	return static_cast<bool>(Sol);
}

bool ABattleSceneLighting::WorldAlreadyLitByUs(const UWorld* World)
{
	if (!World)
	{
		return false;
	}

	return static_cast<bool>(
		TActorIterator<ABattleSceneLighting>(const_cast<UWorld*>(World)));
}

int32 ABattleSceneLighting::DimLightingAuthoredInMap(UWorld* World)
{
	if (!World)
	{
		return 0;
	}

	int32 Caladas = 0;
	for (TActorIterator<AActor> Ator(World); Ator; ++Ator)
	{
		// A nossa própria luz nunca entra na conta: apagá-la deixaria a cena no
		// escuro exatamente no quadro em que ela acabou de nascer.
		if (Ator->IsA<ABattleSceneLighting>())
		{
			continue;
		}

		TArray<USceneComponent*> Componentes;
		Ator->GetComponents<USceneComponent>(Componentes);
		for (USceneComponent* Componente : Componentes)
		{
			// Luz, céu e névoa: os três pintam o quadro inteiro, e é a soma
			// deles com a nossa luz que devolve o branco-azulado.
			const bool bPintaACena =
				Componente->IsA<ULightComponentBase>()
				|| Componente->IsA<USkyAtmosphereComponent>()
				|| Componente->IsA<UExponentialHeightFogComponent>();

			if (bPintaACena && Componente->IsVisible())
			{
				Componente->SetVisibility(false);
				++Caladas;
			}
		}
	}

	return Caladas;
}

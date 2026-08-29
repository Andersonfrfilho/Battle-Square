// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattleArena.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Environment/SceneLighting.h"
#include "Misc/AutomationTest.h"

namespace CenaIluminada
{
	UWorld* CriarMundoDaIluminacao()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
		FWorldContext& Contexto = GEngine->CreateNewWorldContext(EWorldType::Game);
		Contexto.SetCurrentWorld(World);
		World->InitializeActorsForPlay(FURL());
		return World;
	}

	void DestruirMundoDaIluminacao(UWorld* World)
	{
		if (!World)
		{
			return;
		}
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
	}
}

// Luz é o mesmo defeito de "componente criado não é componente visível", só
// que invisível ao contrário: sem sol nada some da tela, tudo fica LAVADO.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSceneLightingAssignsEveryLightTest,
	"BattleSquare.Environment.SceneLighting.AssignsEveryLightComponent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSceneLightingAssignsEveryLightTest::RunTest(const FString& Parameters)
{
	const ABattleSceneLighting* Padrao = GetDefault<ABattleSceneLighting>();

	TestNotNull(TEXT("Tem sol"), Padrao->GetSunLight());
	TestNotNull(TEXT("Tem luz de céu"), Padrao->GetSkyLight());
	TestNotNull(TEXT("Tem atmosfera"), Padrao->GetSkyAtmosphere());

	// Ator criado em tempo de execução não participa do build de luz: luz
	// estática spawnada em jogo simplesmente não acende.
	if (Padrao->GetSunLight())
	{
		TestEqual(TEXT("O sol é móvel, senão não acende quando spawnado"),
			Padrao->GetSunLight()->Mobility.GetValue(), EComponentMobility::Movable);
	}
	if (Padrao->GetSkyLight())
	{
		TestEqual(TEXT("A luz de céu também"),
			Padrao->GetSkyLight()->Mobility.GetValue(), EComponentMobility::Movable);
	}

	return true;
}

// ESTE é o teste do defeito que o usuário viu na tela: a mata verde chegando
// azul-clara. A causa não era o material das folhas — era não haver sol nenhum,
// e a cena inteira ser iluminada pelo ambiente azul padrão da engine.
//
// Um sol frio reintroduziria o defeito sem quebrar mais nada.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSceneLightingSunIsWarmTest,
	"BattleSquare.Environment.SceneLighting.SunIsWarmNotBlue",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSceneLightingSunIsWarmTest::RunTest(const FString& Parameters)
{
	const ABattleSceneLighting* Padrao = GetDefault<ABattleSceneLighting>();
	if (!TestNotNull(TEXT("Tem sol"), Padrao->GetSunLight()))
	{
		return false;
	}

	const FLinearColor Cor = Padrao->GetSunLight()->GetLightColor();
	TestTrue(TEXT("O sol é QUENTE: mais vermelho que azul"), Cor.R > Cor.B);
	TestTrue(TEXT("E tem intensidade de sol, não de lanterna"),
		Padrao->GetSunLight()->Intensity > 1.0f);

	// O céu tinge a atmosfera a partir DESTE sol. Sem a marca, o SkyAtmosphere
	// procura outro e a cena fica sem hora do dia.
	TestTrue(TEXT("É o sol da atmosfera"), Padrao->GetSunLight()->bAtmosphereSunLight);

	return true;
}

// O mapa do jogo (WorldStreamingTest) não tem ator de luz NENHUM — nem sol,
// nem céu, nem atmosfera. Medido em 2026-08-29. Por isso a arena acende a
// cena por código: esperar que o mapa traga luz foi exatamente o que falhou.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSceneLightingIsSpawnedByArenaTest,
	"BattleSquare.Environment.SceneLighting.IsSpawnedByArenaWhenWorldHasNoSun",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSceneLightingIsSpawnedByArenaTest::RunTest(const FString& Parameters)
{
	UWorld* World = CenaIluminada::CriarMundoDaIluminacao();

	TestFalse(TEXT("O mundo nu não tem sol"), ABattleSceneLighting::WorldAlreadyHasSun(World));

	ABattleArena* Arena = World->SpawnActor<ABattleArena>();
	Arena->DispatchBeginPlay();

	TestNotNull(TEXT("A arena acendeu a cena"), Arena->GetSceneLighting());

	CenaIluminada::DestruirMundoDaIluminacao(World);
	return true;
}

// Dois sóis somam intensidade e devolvem a cena lavada pelo outro caminho.
// BattleScreen.umap TEM sol próprio (DirectionalLight_0), e é ela que manda.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSceneLightingDoesNotDuplicateSunTest,
	"BattleSquare.Environment.SceneLighting.DoesNotDuplicateExistingSun",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSceneLightingDoesNotDuplicateSunTest::RunTest(const FString& Parameters)
{
	UWorld* World = CenaIluminada::CriarMundoDaIluminacao();

	World->SpawnActor<ADirectionalLight>(
		ADirectionalLight::StaticClass(), FVector::ZeroVector, FRotator(-45.0f, 0.0f, 0.0f));

	TestTrue(TEXT("O mundo já tem sol"), ABattleSceneLighting::WorldAlreadyHasSun(World));

	ABattleArena* Arena = World->SpawnActor<ABattleArena>();
	Arena->DispatchBeginPlay();

	TestNull(TEXT("E a arena NÃO acendeu um segundo"), Arena->GetSceneLighting());

	CenaIluminada::DestruirMundoDaIluminacao(World);
	return true;
}

// A exposição automática desfaz qualquer ajuste de luz.
//
// "As cores estão muito azul claro, deveria ser mais verde" — e o sol JÁ era
// quente quando o usuário disse isso. O motivo é que a engine reabria o
// diafragma até o quadro médio virar cinza: escurecer o céu deixava a cena
// igualmente lavada, só que com o azul reamplificado. Enquanto a exposição
// flutua, medir cor na tela é medir a compensação, não a iluminação.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSceneLightingExposureIsFixedTest,
	"BattleSquare.Environment.SceneLighting.ExposureIsFixedNotAutomatic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSceneLightingExposureIsFixedTest::RunTest(const FString& Parameters)
{
	const ABattleSceneLighting* Padrao = GetDefault<ABattleSceneLighting>();

	const UPostProcessComponent* Exposicao = Padrao->GetSceneExposure();
	if (!TestNotNull(TEXT("A cena controla a própria exposição"), Exposicao))
	{
		return false;
	}

	// Sem isto o efeito só vale dentro de uma caixa, e a câmera da arena fica
	// longe de qualquer caixa que coubesse neste ator.
	TestTrue(TEXT("O pós-processamento vale em toda parte"), Exposicao->bUnbound);

	TestTrue(TEXT("O piso da exposição é nosso"),
		static_cast<bool>(Exposicao->Settings.bOverride_AutoExposureMinBrightness));
	TestTrue(TEXT("O teto também"),
		static_cast<bool>(Exposicao->Settings.bOverride_AutoExposureMaxBrightness));

	// Piso == teto é o que TRAVA: com folga entre eles a engine volta a
	// compensar dentro da folga, que é o defeito de novo, só que menor.
	TestEqual(TEXT("Piso e teto no mesmo valor — exposição travada"),
		Exposicao->Settings.AutoExposureMinBrightness,
		Exposicao->Settings.AutoExposureMaxBrightness, 0.001f);

	return true;
}

// Luz que vem de todas as direções vence luz que vem de uma só.
//
// A captura em tempo real de um SkyAtmosphere devolve azul por física. Em pé
// de igualdade com o sol, esse azul pinta cada superfície da mata e a cena
// inteira puxa para o ciano — mesmo com o sol quente que SunIsWarmNotBlue já
// garante. O sol precisa mandar com folga, e o número que garante isso não
// pode ficar a critério de quem editar a próxima vez.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSceneLightingAmbientDoesNotDrownSunTest,
	"BattleSquare.Environment.SceneLighting.AmbientDoesNotDrownTheSun",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSceneLightingAmbientDoesNotDrownSunTest::RunTest(const FString& Parameters)
{
	const ABattleSceneLighting* Padrao = GetDefault<ABattleSceneLighting>();

	const UDirectionalLightComponent* Sol = Padrao->GetSunLight();
	const USkyLightComponent* Ceu = Padrao->GetSkyLight();
	if (!TestNotNull(TEXT("Tem sol"), Sol) || !TestNotNull(TEXT("Tem céu"), Ceu))
	{
		return false;
	}

	TestTrue(TEXT("O céu é preenchimento de sombra, não a luz principal"),
		Ceu->Intensity < Sol->Intensity * 0.2f);

	return true;
}

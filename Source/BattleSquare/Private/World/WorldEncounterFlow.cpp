// Copyright 2026 Anderson. All Rights Reserved.

#include "World/WorldEncounterFlow.h"
#include "World/EncounterDetectionComponent.h"
#include "World/WorldBattleTransitionService.h"
#include "World/WorldEncounterActor.h"
#include "World/ArenaWorldSampler.h"
#include "Environment/SceneLighting.h"
#include "EngineUtils.h"
#include "Battle/BattleArena.h"

void UWorldEncounterFlow::Initialize(AActor* InWorldPawn,
	UEncounterDetectionComponent* InDetection,
	TSubclassOf<ABattleArena> InArenaClass,
	const FEncounterMatchParams& InMatchParams)
{
	WorldPawn = InWorldPawn;
	Detection = InDetection;
	ArenaClass = InArenaClass;
	MatchParams = InMatchParams;

	if (!TransitionService)
	{
		TransitionService = NewObject<UWorldBattleTransitionService>(this);
	}

	if (Detection)
	{
		Detection->OnEncounterTriggered.AddUObject(this, &UWorldEncounterFlow::HandleEncounterTriggeredInternal);
	}
}

ABattleArena* UWorldEncounterFlow::HandleEncounterTriggered(AWorldEncounterActor* Encounter)
{
	if (!Encounter || !WorldPawn || !Detection || !TransitionService)
	{
		return nullptr;
	}

	FEncounterMatchParams Params = MatchParams;
	Params.EncounterCatalogId = Encounter->CatalogId.ToString();

	// A ARENA É O LUGAR: a batalha nasce do pedaço de mapa onde os dois se
	// encontraram. Sem esta coleta, tudo o que a montagem sabe fazer com
	// terreno de mundo fica inalcançável, e a arena volta a ser sorteio.
	Params.EncounterLocation = Encounter->GetActorLocation();
	FArenaWorldSampler::Collect(Encounter->GetWorld(),
		Params.EncounterLocation, Params.WorldFeatures);

	// O CÉU DE AGORA, perguntado a quem já o está mostrando. A cena do mundo é
	// quem o GameMode mantém em dia; ler dela garante que a arena chegue com o
	// mesmo tempo que o jogador acabou de atravessar a pé. Recalcular aqui
	// pediria semente e hora decorrida de volta, e criaria um segundo lugar
	// que discordaria do painel na virada do bloco (L-032).
	TActorIterator<ABattleSceneLighting> Cena(Encounter->GetWorld());
	if (Cena)
	{
		Params.Weather = Cena->GetWeather();
	}

	FBattleState InitialState;
	TArray<FPetPresentationInfo> Presentations;
	if (!FEncounterMatchAssembler::AssembleFromEncounter(Params, InitialState, Presentations))
	{
		// Montagem impossível não vira batalha meia-boca: a detecção volta a
		// ligar para o jogador não ficar preso num encontro que nunca começa.
		Detection->SetDetectionEnabled(true);
		return nullptr;
	}

	ABattleArena* Arena = TransitionService->BeginTransition(WorldPawn, Detection, Encounter, ArenaClass);
	if (!Arena)
	{
		Detection->SetDetectionEnabled(true);
		return nullptr;
	}

	Arena->BeginBattle(InitialState, Presentations);

	OnWorldBattleStarted.Broadcast(Arena);
	return Arena;
}

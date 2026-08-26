// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Net/BattleRoomRegistry.h"
#include "Net/BattleTurnCoordinator.h"
#include "Battle/BattleArena.h"
#include "Data/BattleDataTranslator.h"
#include "BattleSquareGameMode.generated.h"

class ABattleSquarePlayerController;

// T7–T9 (tasks.md, Sala e Pareamento Simples): fiação fina — liga
// UBattleRoomRegistry (lógica pura, T2–T6) a jogadores/atores reais.
// Toda decisão de "pode ou não pode" já mora no registro; este ator só
// reage aos delegates dele e monta o que o Combate Online já construiu
// (ABattleArena, UBattleTurnCoordinator, UBattleNetCommitComponent).
class UWorldEncounterFlow;

// config=Game: as credenciais do espelho de pets e o pet do jogador vêm de
// DefaultGame.ini, não de um Blueprint. O projeto não tem Blueprint algum, e
// sem isto não haveria onde configurar um GameMode escolhido por World Settings.
UCLASS(config = Game)
class BATTLESQUARE_API ABattleSquareGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ABattleSquareGameMode();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void Logout(AController* Exiting) override;

	UPROPERTY()
	TObjectPtr<UBattleRoomRegistry> RoomRegistry;

	// --- Encontros no mundo aberto (M5) ---
	// Ligam o UEncounterDetectionComponent do pawn ao UWorldBattleTransitionService,
	// que é o que faz "andar → encontrar → batalhar → voltar" acontecer em jogo.
	// Sem esta fiação as classes existem e são testadas, mas nada as instancia.

	UPROPERTY(config, EditDefaultsOnly, Category = "Mundo")
	FString WorldEncounterPlayerCatalogId;

	UPROPERTY(config, EditDefaultsOnly, Category = "Mundo")
	FString WorldEncounterMirrorPath;

	UPROPERTY(config, EditDefaultsOnly, Category = "Mundo")
	FString WorldEncounterMirrorKeyHex;

	UPROPERTY(config, EditDefaultsOnly, Category = "Mundo")
	FString WorldEncounterMirrorPublicKeyPem;

	UPROPERTY()
	TObjectPtr<UWorldEncounterFlow> WorldEncounterFlow;

	/** Devolve o motivo quando não conseguiu ligar — nunca falha em silêncio. */
	FString SetUpWorldEncounterFlow();

private:
	// Em nível com World Partition os atores chegam por streaming DEPOIS do
	// BeginPlay do GameMode — tentar uma vez só encontra um mundo vazio.
	// A tentativa se repete no Tick até pegar, e o motivo é logado uma vez.
	bool bHasLoggedWorldEncounterProblem = false;

public:

	// Registrado pelo controller depois de CreateRoom/JoinRoom bem-
	// sucedidos — é como o GameMode sabe qual controller pertence a
	// qual sala/lado quando Logout ou OnRoomReady dispararem.
	void RegisterControllerForRoom(const FString& Code, uint8 Side, ABattleSquarePlayerController* Controller);

	// T8: caminho TESTÁVEL — monta ABattleArena + UBattleTurnCoordinator
	// reais para uma sala, a partir de um FBattleState já pronto. Não
	// toca no espelho de pets — quem chama já decidiu os pets.
	void AssembleMatchForRoom(const FString& Code, const FBattleState& InitialState, const TArray<FPetPresentationInfo>& Presentations);

	// T5 (niveis-experiencia-evolucao): mesmo padrão de
	// ABattleArena::PetCollectionSlotName — configurável para que testes
	// usem um slot dedicado, nunca o de produção.
	UPROPERTY(EditDefaultsOnly, Category = "Coleção")
	FString PetCollectionSlotName = TEXT("PetCollection");

	// T5: extraído de HandleRoomReady para ser testável sem depender do
	// espelho real de pets — se Presentation.CatalogId já está na
	// coleção do slot dado, aplica o bônus do nível correspondente.
	// Estático e público de propósito: não usa nenhum estado de
	// instância além do slot recebido como parâmetro.
	static void ApplyOwnedPetProgressionBonus(const FString& SlotName, FPetState& PetState, const FPetPresentationInfo& Presentation);

	// Config do espelho local de pets — placeholder até existir seleção
	// de time (SALA-08, design.md: "decisão pragmática, registrada
	// honestamente"). Vazio por padrão: propaga o erro já existente de
	// FPetDataLoader (MissingMirrorFailsExplicitly) em vez de escondê-lo.
	UPROPERTY(EditDefaultsOnly, Category = "Pets")
	FString PetMirrorPath;

	UPROPERTY(EditDefaultsOnly, Category = "Pets")
	FString PetMirrorPublicKeyPem;

	UPROPERTY(EditDefaultsOnly, Category = "Pets")
	TArray<uint8> PetMirrorEncryptionKey;

	struct FActiveMatch
	{
		TObjectPtr<ABattleArena> Arena;
		TObjectPtr<UBattleTurnCoordinator> Coordinator;
	};

	const FActiveMatch* GetActiveMatch(const FString& Code) const { return ActiveMatches.Find(Code); }

	// Garante RoomRegistry existente e com os delegates ligados — não
	// depende de BeginPlay ter disparado. Mesmo padrão de
	// ABattleArena::TracePlayer (criação preguiçosa, guardada por
	// `if (!X)`), necessário porque um UWorld montado manualmente para
	// teste (CreateWorld + InitializeActorsForPlay) nem sempre dispara
	// BeginPlay em atores spawnados depois — descoberto por um SIGSEGV
	// real ao chamar RoomRegistry->OnRoomAbandoned.Broadcast() com
	// RoomRegistry ainda nulo. Pública para que testes headless possam
	// garantir o estado explicitamente, sem depender de BeginPlay.
	void EnsureRoomRegistry();

private:
	TMap<FString, FActiveMatch> ActiveMatches;
	TMap<FString, TArray<TWeakObjectPtr<ABattleSquarePlayerController>>> RoomControllers;

	// T8: caminho de PRODUÇÃO — carrega pets reais do espelho e chama
	// AssembleMatchForRoom. Ligado a RoomRegistry->OnRoomReady.
	void HandleRoomReady(const FString& Code);

	// T9: abandono real aciona o coordenador real da sala.
	void HandleRoomAbandoned(const FString& Code, uint8 PresentSide);

	void ConnectControllersToCoordinator(const FString& Code, UBattleTurnCoordinator* Coordinator);
};

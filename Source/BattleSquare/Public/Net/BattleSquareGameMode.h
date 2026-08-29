// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Math/RandomStream.h"
#include "Engine/TimerHandle.h"
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
class UBattleActionSelectorWidget;
class ABattleArena;

// config=Game: as credenciais do espelho de pets e o pet do jogador vêm de
// DefaultGame.ini, não de um Blueprint. O projeto não tem Blueprint algum, e
// sem isto não haveria onde configurar um GameMode escolhido por World Settings.
UCLASS(config = Game)
class BATTLESQUARE_API ABattleSquareGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ABattleSquareGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void Logout(AController* Exiting) override;

	UPROPERTY()
	TObjectPtr<UBattleRoomRegistry> RoomRegistry;

	// --- Encontros no mundo aberto (M5) ---
	// Ligam o UEncounterDetectionComponent do pawn ao UWorldBattleTransitionService,
	// que é o que faz "andar → encontrar → batalhar → voltar" acontecer em jogo.
	// Sem esta fiação as classes existem e são testadas, mas nada as instancia.

	/**
	 * Pawn que o jogador controla no mundo aberto. Ter isto aqui é o que
	 * conserta o defeito de 2026-08-26: o bootstrap dependia de um pawn
	 * COLOCADO no nível, e num mundo com World Partition um ator colocado
	 * longe do PlayerStart simplesmente nunca é carregado. Spawnar o pawn
	 * certo não depende de streaming — e é ele quem PUXA o streaming.
	 * Vazio mantém o DefaultPawn da engine (comportamento de M1–M4 intacto).
	 */
	UPROPERTY(config, EditDefaultsOnly, Category = "Mundo")
	FSoftClassPath WorldExplorerPawnClassPath;

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

	/**
	 * Monta a interface de ações quando a batalha do mundo começa, e a
	 * desmonta quando ela acaba.
	 *
	 * Sem isto o jogador caminhava, encontrava um oponente, a câmera ia para a
	 * arena — e ele ficava olhando uma luta que não podia jogar.
	 */
	void HandleWorldBattleStarted(ABattleArena* Arena);

	/**
	 * Povoa o mundo com encontros que ANDAM.
	 *
	 * Até 2026-08-27 ninguém criava encontro nenhum: eles só existiam se
	 * alguém os tivesse colocado à mão no nível. Caminhar pelo mundo, então,
	 * nunca disparava batalha — o recurso inteiro de encontro ficava
	 * inalcançável sem que nada acusasse.
	 */
	void SpawnRoamingEncounters();

	/**
	 * Dá SOL e CHÃO ao mundo aberto.
	 *
	 * Sem isto o nível não tem ator de luz nenhum, e a engine ilumina tudo
	 * com o ambiente azul padrão — foi exatamente assim que a mata da arena
	 * apareceu azul-clara na tela. É o mesmo defeito, num lugar diferente.
	 */
	void SpawnWorldScenery();

	/**
	 * Escreve o painel do mundo: seu pet, seus atributos, quem está por perto.
	 *
	 * Num TEMPORIZADOR lento, não a cada quadro: nada disso muda em 16ms, e um
	 * painel reescrito 60 vezes por segundo é custo puro. A coleção só é
	 * relida quando pode ter mudado — o disco não tem por que ser tocado
	 * enquanto o jogador só anda.
	 */
	void RefreshWorldStatus();

	void ReloadOwnedPetSnapshot();

	FTimerHandle WorldStatusTimer;
	FOwnedPetInstance CachedOwnedPet;
	bool bHasCachedOwnedPet = false;

	/** De quantos em quantos segundos o painel do mundo é reescrito. */
	UPROPERTY(config, EditDefaultsOnly, Category = "Mundo")
	float WorldStatusRefreshSeconds = 0.5f;

	/**
	 * Repõe a população: tira de cena os derrotados e cria novos até o alvo.
	 *
	 * Sem isto o mundo ACABA — com um número fixo, seis batalhas esvaziavam o
	 * mapa e sobrava caminhar por um lugar onde nada mais acontece. E o pet
	 * derrotado continuava passeando como fantasma, o que é pior que sumir:
	 * dá a entender que a batalha não valeu.
	 */
	void MaintainEncounterPopulation();

private:
	void SpawnOneEncounter(const FVector& Centro, FRandomStream& Sorteio, int32 SementeDoPasseio);

	FTimerHandle EncounterPopulationTimer;
	int32 EncounterRefillCounter = 0;

public:

	/** Quantos encontros povoam o mundo. Zero desliga. */
	UPROPERTY(config, EditDefaultsOnly, Category = "Mundo")
	int32 WorldEncounterCount = 6;

	/** De quantos em quantos segundos a população é conferida. */
	UPROPERTY(config, EditDefaultsOnly, Category = "Mundo")
	float EncounterPopulationCheckSeconds = 5.0f;

	UPROPERTY(config, EditDefaultsOnly, Category = "Mundo")
	float WorldEncounterSpawnRadiusUnits = 4000.0f;

	/** Semente do povoamento: repetir a mesma dá o mesmo mundo. */
	UPROPERTY(config, EditDefaultsOnly, Category = "Mundo")
	int32 WorldEncounterSeed = 20260827;

	/**
	 * Escala da mata do mundo, em unidades por "casa".
	 *
	 * AForestBackdrop mede tudo em casas de tabuleiro porque nasceu para a
	 * arena. O mundo reusa o MESMO ator com uma casa maior, em vez de ganhar
	 * um segundo sistema de cenário: duas matas concordariam até a primeira
	 * edição, e a paleta do jogo passaria a depender de qual delas se está
	 * olhando.
	 *
	 * 200 dá chão de raio 6000 — o bastante para cobrir os 4000 em que os
	 * encontros nascem, com margem para o jogador andar além deles.
	 */
	UPROPERTY(config, EditDefaultsOnly, Category = "Mundo")
	float WorldSceneryCellSizeUnits = 200.0f;

	/** Semente da mata. Separada da do povoamento: mudar onde os inimigos
	 *  nascem não deveria replantar a floresta inteira. */
	UPROPERTY(config, EditDefaultsOnly, Category = "Mundo")
	int32 WorldScenerySeed = 20260829;

	/** Ids do catálogo sorteados para os encontros. */
	UPROPERTY(config, EditDefaultsOnly, Category = "Mundo")
	TArray<FString> WorldEncounterCatalogIds;

	void HandleWorldBattleFinished();

	void TearDownBattleUi();

	/** Mesmo widget da tela de batalha: uma tela de ações, não duas. */
	UPROPERTY(config, EditDefaultsOnly, Category = "Mundo")
	FSoftClassPath BattleActionSelectorWidgetClassPath;

	UPROPERTY(Transient)
	TObjectPtr<UBattleActionSelectorWidget> WorldBattleActionSelector;

	/**
	 * Carrega os pets do espelho configurado em DefaultGame.ini. Extraído para
	 * ser reusado por quem monta uma partida sem passar pelo mundo aberto
	 * (a tela de batalha). Devolve o motivo em caso de falha, nunca vazio.
	 */
	FString LoadConfiguredMirrorPets(TArray<FLoadedPetRecord>& OutPets) const;

private:
	// Em nível com World Partition os atores chegam por streaming DEPOIS do
	// BeginPlay do GameMode — tentar uma vez só encontra um mundo vazio.
	// A tentativa se repete no Tick até pegar, e o motivo é logado uma vez.
	bool bHasLoggedWorldEncounterProblem = false;

	// Só a AUSÊNCIA DE PAWN é transitória (World Partition ainda vai trazê-lo).
	// Espelho ausente ou config errada nunca se conserta sozinha em runtime —
	// insistir a cada Tick só produz enxurrada de erro, como aconteceu.
	bool bWorldEncounterSetupIsTransient = true;

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

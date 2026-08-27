// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Battle/BattleState.h"
#include "Battle/PetView.h"
#include "Battle/BattleActionQueueComponent.h"
#include "Battle/BattleTracePlayer.h"
#include "Net/BattleTurnCoordinator.h"
#include "Data/BattleDataTranslator.h"
#include "BattleArena.generated.h"

class UCameraComponent;
class UStaticMeshComponent;
class UMaterialInterface;

// T9 (tasks.md, PRES-06/07/08): scaffold da cena de combate — câmera fixa
// enquadrando a grade 3x3, spawn de APetView a partir do estado inicial.
// Estética final (tilt-shift, material fosco de verdade) é verificação
// visual manual — ver T14. O que este ator garante por código é: as 9
// casas caem dentro do frustum da câmera, e a grade usa um material
// configurável (nunca cor hardcoded).
DECLARE_MULTICAST_DELEGATE(FBattleFinishedSignature);

/** Categoria própria para o que aconteceu no turno — silenciável sozinha. */
DECLARE_LOG_CATEGORY_EXTERN(LogBattleArena, Display, All);

UCLASS()
class BATTLESQUARE_API ABattleArena : public AActor
{
	GENERATED_BODY()

public:
	ABattleArena();

	virtual void Tick(float DeltaSeconds) override;

	// Tamanho de uma casa da grade, em unidades do mundo — a única fonte
	// de verdade para o espaçamento; nunca um número mágico espalhado.
	UPROPERTY(EditDefaultsOnly, Category = "Grade")
	float CellSize = 150.0f;

	// Material da grade — configurável por Blueprint/instância, nunca uma
	// cor hex fixa no C++.
	UPROPERTY(EditDefaultsOnly, Category = "Grade")
	TSoftObjectPtr<UMaterialInterface> GridMaterial;

	// Centro (em espaço de mundo) da casa (Column, Row), Column/Row em [0,2].
	UFUNCTION(BlueprintPure)
	FVector GetCellWorldLocation(uint8 Column, uint8 Row) const;

	// Cria um APetView por pet do estado inicial, posicionado na casa
	// correspondente. Não exposto ao Blueprint — FBattleState/
	// FPetPresentationInfo não são BlueprintType.
	void SpawnPetViews(const FBattleState& InitialState, const TArray<FPetPresentationInfo>& Presentations);

	const TArray<TObjectPtr<APetView>>& GetPetViews() const { return SpawnedPetViews; }

	// Checagem programática (PRES-06/07): projeta os 9 centros de casa no
	// espaço da câmera e confirma que caem dentro do FOV configurado.
	// Não depende de PlayerController/Viewport — só de FOV+AspectRatio+
	// transform da câmera, para ser testável headless.
	UFUNCTION(BlueprintCallable)
	bool AreAllGridCellsInCameraFrustum() const;

	// T10: fiação de ponta a ponta. Guarda o estado inicial, spawna as
	// views e deixa a fila do jogador (PlayerActionQueue) pronta para
	// receber seleções. Assim que o jogador commitar, a IA gera o commit
	// dela, o resolvedor real roda, e o trace resultante anima as views.
	// Não exposto ao Blueprint pelo mesmo motivo de SpawnPetViews.
	//
	// T7 (arenas-variadas, ARENA-02): retorna false, sem montar nada, se
	// algum pet estivesse posicionado numa casa Blocked — erro de
	// configuração, nunca reposicionado silenciosamente.
	bool BeginBattle(const FBattleState& InitialState, const TArray<FPetPresentationInfo>& Presentations);

	// T4 (encontros-transicao-batalha): a arena não tinha como anunciar o
	// próprio fim — CheckForCapture/GrantExperienceIfOwned varrem o trace em
	// silêncio, e quem está de fora não fica sabendo. Dispara UMA vez, e
	// sempre DEPOIS de captura e XP terem rodado: quem escuta (a transição de
	// volta ao mundo) destrói esta arena, e destruí-la antes perderia os dois.
	FBattleFinishedSignature OnBattleFinished;

	const FBattleState& GetCurrentState() const { return CurrentState; }

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBattleActionQueueComponent> PlayerActionQueue;

	// T8 (tasks.md, Combate Online, NET-09/NET-10): registra que o
	// oponente é um jogador humano real, servido por InCoordinator — a
	// partir daqui, HandlePlayerCommitted delega ao coordenador em vez de
	// chamar FTacticalOpponentAI. Presença de oponente real decide o caminho,
	// nunca uma flag de "modo online/offline" separada. Sem esta chamada,
	// o comportamento continua byte a byte o de antes (Standalone).
	void ConfigureNetworkedOpponent(UBattleTurnCoordinator* InCoordinator);

	// T4/T5 (colecao-e-captura): lado que representa o jogador local —
	// mesma convenção já implícita em HandlePlayerCommitted/
	// ConfigureNetworkedOpponent/UBattleResultWidget::ApplyBattleEndedEvent
	// (Side 0 é sempre "eu"). Nomeia o que já era verdade, não muda nada.
	UPROPERTY(EditDefaultsOnly, Category = "Coleção")
	uint8 LocalPlayerSide = 0;

	// T4 (colecao-e-captura): slot de save da coleção local — constante
	// nomeada por padrão, mas exposta para testes usarem um slot dedicado
	// (design.md, Riscos — nunca poluir o slot de produção em teste).
	UPROPERTY(EditDefaultsOnly, Category = "Coleção")
	FString PetCollectionSlotName = TEXT("PetCollection");

protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> ArenaRoot;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCameraComponent> ArenaCamera;

private:
	UPROPERTY()
	TArray<TObjectPtr<APetView>> SpawnedPetViews;

	UPROPERTY()
	TObjectPtr<UBattleTracePlayer> TracePlayer;

	UPROPERTY()
	FBattleState CurrentState;

	// Presença, não flag: null quando o oponente é FTacticalOpponentAI
	// (Standalone/sem oponente humano); setado por ConfigureNetworkedOpponent
	// quando um jogador real está do outro lado.
	UPROPERTY()
	TObjectPtr<UBattleTurnCoordinator> ServerCoordinator;

	// T4 (colecao-e-captura): retido de BeginBattle — CheckForCapture (T5)
	// precisa saber o CatalogId/Name/Type do pet oponente quando a
	// batalha termina, e essa informação não existe mais em FPetState
	// (fronteira do núcleo já a descartou, de propósito, AD-012).
	UPROPERTY()
	TMap<uint8, FPetPresentationInfo> PresentationsByPetId;

	bool IsPointInCameraFrustum(const FVector& WorldPoint) const;

	UFUNCTION()
	void HandlePlayerCommitted();

	void HandleCoordinatorTurnResolved(const FBattleState& NextState, const TArray<FBattleEvent>& Trace);

	void DispatchEventToPetViews(const FBattleEvent& Event);

	/** Traduz o evento para o feed de produto (DP-leg-02: traduz, não decide). */
	void NarrateEvent(const FBattleEvent& Event);

	/** Cada pet vivo passa a olhar para o adversário vivo. */
	void RefreshGazes();

	uint8 FindPostureFlagsForPet(uint8 PetId) const;

	// T5 (colecao-e-captura) 🧠: varre o trace por BatalhaEncerrada; se o
	// jogador local venceu, captura o pet do lado OPOSTO — nunca o
	// próprio pet do jogador.
	void CheckForCapture(const TArray<FBattleEvent>& Trace);

	// T4 (niveis-experiencia-evolucao): credita XP ao pet do JOGADOR
	// LOCAL (nunca o oponente) se o CatalogId dele já está na coleção —
	// em qualquer resultado (vitória/derrota/empate), quantidades
	// diferentes. Pet ainda não capturado não gera XP fantasma.
	void GrantExperienceIfOwned(const TArray<FBattleEvent>& Trace);

	// Dispara OnBattleFinished se o trace contiver BatalhaEncerrada, no
	// máximo uma vez por arena.
	void AnnounceBattleFinishedIfEnded(const TArray<FBattleEvent>& Trace);

	void LogCommit(const TCHAR* Quem, const FTurnCommit& Commit) const;

	/** Destrava a fila para a próxima rodada, se a batalha ainda não acabou. */
	void OpenNextTurnIfBattleContinues();

	/**
	 * Desenha a grade 3x3 no mundo, com a coordenada de cada casa e quem está
	 * nela. É o que torna posição e direção LEGÍVEIS — teria mostrado na hora
	 * que "Baixo" andava para a direita, e mostra a coabitação sem precisar
	 * de explicação. Só desenha com bs.ShowBattleDebug ligado.
	 */
	void DrawDebugGrid() const;

	// A rodada seguinte só abre quando a reprodução do trace termina: abrir
	// antes deixaria o jogador escolhendo o próximo turno enquanto o anterior
	// ainda está acontecendo na tela.
	bool bWaitingForPlaybackToOpenNextTurn = false;

	bool bHasAnnouncedBattleFinished = false;
};

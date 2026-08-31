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
#include "Meta/PetAttributeProgression.h"
#include "BattleArena.generated.h"

class UCameraComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UMaterialInterface;
class AForestBackdrop;
class AMountainRange;
class ABattleSceneLighting;
class APetOwnerView;
enum class EIslandBiome : uint8;
enum class EScenaryClimate : uint8;

// T9 (tasks.md, PRES-06/07/08): scaffold da cena de combate — câmera fixa
// enquadrando a grade 3x3, spawn de APetView a partir do estado inicial.
// Estética final (tilt-shift, material fosco de verdade) é verificação
// visual manual — ver T14. O que este ator garante por código é: as 9
// casas caem dentro do frustum da câmera, e a grade usa um material
// configurável (nunca cor hardcoded).
DECLARE_MULTICAST_DELEGATE(FBattleFinishedSignature);

/** Categoria própria para o que aconteceu no turno — silenciável sozinha. */
DECLARE_LOG_CATEGORY_EXTERN(LogBattleArena, Display, All);

UCLASS(config = Game)
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

	/**
	 * Dimensões da grade desta arena, editáveis por `DefaultGame.ini`.
	 *
	 * Não precisam ser iguais — 3x2 e 4x6 são campos legítimos. O tabuleiro,
	 * a clareira, a moldura e o enquadramento da câmera saem daqui; nada
	 * mais no ator assume três.
	 *
	 * A grade que VALE numa batalha é a do estado (FBattleState::GridColumns),
	 * porque é ela que entra no hash. Estes valores são o que a arena usa
	 * quando monta a batalha ela mesma.
	 */
	UPROPERTY(EditDefaultsOnly, config, Category = "Grade")
	int32 GridColumns = BattleGridDefaultColumns;

	UPROPERTY(EditDefaultsOnly, config, Category = "Grade")
	int32 GridRows = BattleGridDefaultRows;

	/** Colunas/linhas em uso agora: as do estado durante a batalha. */
	int32 GetActiveGridColumns() const;
	int32 GetActiveGridRows() const;

private:
	/** Lê GridColumns/GridRows do .ini já no construtor — ver o .cpp. */
	void ResolveConfiguredGridSize();

public:

	// Material da grade — configurável por Blueprint/instância, nunca uma
	// cor hex fixa no C++.
	UPROPERTY(EditDefaultsOnly, Category = "Grade")
	TSoftObjectPtr<UMaterialInterface> GridMaterial;

	/**
	 * Materiais da arena — instâncias reais em /Game/Arena/Materials, nunca
	 * cor solta no código. Trocar a paleta é editar o asset, e a laje já sabe
	 * qual delas é a dela.
	 */
	/**
	 * Altura em que o tabuleiro assenta acima do chão do nível.
	 *
	 * O mapa traz um plano de chão em Z=-0.5 (herança do template), e a laje
	 * de andar tem topo em -4: sem erguer, o tabuleiro inteiro fica ENTERRADO
	 * sob esse plano e não existe na tela. Erguer aqui, e não nos três pontos
	 * de spawn, porque três cópias do mesmo número discordam na primeira
	 * edição (L-032/L-033).
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Arena")
	float BoardElevation = 120.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Arena|Materiais")
	TSoftObjectPtr<UMaterialInterface> NeutralTileMaterial;

	UPROPERTY(EditDefaultsOnly, Category = "Arena|Materiais")
	TSoftObjectPtr<UMaterialInterface> WaterTileMaterial;

	UPROPERTY(EditDefaultsOnly, Category = "Arena|Materiais")
	TSoftObjectPtr<UMaterialInterface> DamageTileMaterial;

	UPROPERTY(EditDefaultsOnly, Category = "Arena|Materiais")
	TSoftObjectPtr<UMaterialInterface> BuffTileMaterial;

	UPROPERTY(EditDefaultsOnly, Category = "Arena|Materiais")
	TSoftObjectPtr<UMaterialInterface> BlockedTileMaterial;

	/**
	 * Altura da SUPERFÍCIE da casa, por terreno.
	 *
	 * Fonte única (L-032/L-033): a laje 3D se dimensiona por aqui e a grade
	 * desenhada se ERGUE por aqui. Fossem dois números, água afundaria a laje
	 * e deixaria a linha boiando no ar na primeira vez que alguém editasse um
	 * dos dois.
	 */
	/** Onde a superfície daquele terreno é DESENHADA. */
	static float GetCellSurfaceHeight(uint8 CellProperty);

	/**
	 * Onde o PÉ do pet pousa naquele terreno.
	 *
	 * Só a água separa os dois: a lâmina fica visível acima do solo e o pé
	 * desce por dentro dela — quem está na água está DENTRO dela, não em cima,
	 * e é o que faz submergir parecer submergir.
	 */
	static float GetCellFootingHeight(uint8 CellProperty);

	/** O que a casa É, ou None enquanto não houver layout montado. */
	uint8 GetCellProperty(uint8 Column, uint8 Row) const;

	/**
	 * Altura da superfície DAQUELA casa — a que o pé do pet encosta.
	 *
	 * GetCellSurfaceHeight sozinha só sabe traduzir um terreno em altura;
	 * quem sabe QUAL terreno está na casa é o estado. Sem este par, quem
	 * posiciona um pet tem de repetir a consulta ao layout, e foi assim que
	 * a laje ganhou relevo e o pet continuou no plano zero.
	 */
	float GetCellSurfaceHeightAt(uint8 Column, uint8 Row) const;

	/**
	 * O TOPO do obstáculo daquela casa — onde o pé de quem escalou encosta.
	 *
	 * Mede a malha que está na tela, e não repete a conta que a montou: o
	 * obstáculo é escalado pela altura, mas essa escala ainda é aparada para
	 * ele não transbordar a casa, e quem recalculasse `altura * casa` acharia
	 * um topo que a pedra larga não tem. Assertar contra os próprios limites
	 * do componente é o mesmo padrão do teste que já cuida do obstáculo.
	 *
	 * Casa sem obstáculo montado devolve a superfície bloqueada, que é onde
	 * ele assentaria — nunca uma altura inventada.
	 */
	float GetObstacleTopHeightAt(uint8 Column, uint8 Row) const;

	/**
	 * A linha de painel que carrega a SEMENTE da partida.
	 *
	 * Sem ela o painel conta o turno mas não permite voltar nele: o BattleSim
	 * é determinístico justamente para que semente mais commits recriem a
	 * partida inteira, e essa metade não saía de lugar nenhum.
	 *
	 * Em hexadecimal porque é para ser copiada e colada, não lida.
	 */
	static FString FormatSeedPanelLine(uint64 Seed);

	/**
	 * A linha de painel que diz se o ACASO entrou neste turno.
	 *
	 * O gerador anda um passo por sorteio, e não existe conta barata que
	 * recupere QUANTOS passos ele deu entre dois estados. Mas a pergunta que
	 * custa caro é outra — "esse resultado estranho foi sorteio ou foi
	 * regra?" — e para ela basta o estado ter mudado ou não.
	 */
	static FString FormatRandomPanelLine(uint64 StateBefore, uint64 StateAfter);

	/**
	 * Veste a arena com o ambiente do lugar onde o encontro aconteceu.
	 *
	 * A arena nasce a um milhão de unidades do mundo (DP-enc-03), fora de
	 * qualquer célula de World Partition: luz direcional, céu e névoa são
	 * globais e já a alcançam, mas o CHÃO não — e é o chão que denuncia que
	 * a batalha se passa noutro lugar. Sonda o terreno sob o encontro e
	 * empresta o material dele ao chão da arena.
	 *
	 * As LAJES não adotam nada: elas dizem o que cada casa é (água, dano,
	 * bônus), e essa leitura é regra de jogo, não decoração.
	 *
	 * Devolve false quando não há chão sondável ou ele não tem material —
	 * e aí a paleta autorada continua valendo. Degrada, não quebra.
	 *
	 * O BIOMA do lugar é aprendido aqui também, e não numa segunda chamada:
	 * quem leva o jogador para a luta chama isto UMA vez, e capacidade que
	 * depende de o chamador lembrar de um segundo passo é capacidade que
	 * algum dia não acontece. Ele é aprendido ANTES da sondagem, para o
	 * cenário da geleira valer mesmo quando não há chão nenhum a herdar.
	 */
	bool AdoptAmbienceFromWorldLocation(const FVector& WorldLocation);

	/** Material herdado do mundo, ou nulo quando o chão da mata vale por si. */
	UMaterialInterface* GetAdoptedFloorMaterial() const { return AdoptedFloorMaterial; }

	/**
	 * O bioma de onde a luta começou, ou vazio quando ninguém situou a arena.
	 *
	 * Vazio não é "floresta": é a ausência da pergunta, e é ela que decide se
	 * o clima da serra sai do chão do encontro ou do `.ini`. Guardar
	 * `Forest` no lugar do vazio apagaria essa diferença.
	 */
	const TOptional<EIslandBiome>& GetEncounterBiome() const { return EncounterBiome; }

	/**
	 * O clima que veste a serra: o do chão onde a luta começou, e o do `.ini`
	 * só quando a luta não veio de lugar nenhum.
	 *
	 * É UMA função porque a pergunta é uma. Dois lugares decidindo isto
	 * deixariam a serra nevada ao lado de uma mata de deserto (L-032).
	 */
	EScenaryClimate ResolveScenaryClimate() const;

	/** O bioma que veste a mata: o do encontro, ou floresta quando não houve. */
	EIslandBiome ResolveEncounterBiome() const;

	/**
	 * A mata que veste o entorno, ou nulo antes de BeginPlay.
	 *
	 * A arena não desenha cenário: ela diz onde o tabuleiro está e de que
	 * tamanho é a casa, e a mata se espalha a partir disso. Fossem os raios
	 * calculados aqui, o cenário viraria regra de arena (DP-ui-01).
	 */
	AForestBackdrop* GetForestBackdrop() const { return ForestBackdrop; }

	/** A serra do horizonte, para o teste que exige que ela exista. */
	AMountainRange* GetMountainRange() const { return MountainRange; }

	/** Semente da mata — a mesma arena dá a mesma floresta, sempre. */
	UPROPERTY(EditDefaultsOnly, Category = "Arena")
	int32 ForestSeed = 20260829;

	/** Malhas da arena, para o teste que exige asset atribuído em todas. */
	const TArray<TObjectPtr<UStaticMeshComponent>>& GetCellTileMeshes() const { return CellTileMeshes; }

	/**
	 * O que OCUPA cada casa — visível só onde a casa é bloqueada.
	 *
	 * Um por casa, e não um por bloqueio, porque o layout do campo muda
	 * entre batalhas: componente criado sob demanda no meio da partida é
	 * componente que às vezes não existe quando alguém o procura.
	 */
	const TArray<TObjectPtr<UStaticMeshComponent>>& GetCellObstacleMeshes() const { return CellObstacleMeshes; }

	/**
	 * Põe cada laje — e o que ocupa cada casa — na altura, no material e no
	 * relevo do terreno que aquela casa virou.
	 *
	 * Pública porque o que ela produz é justamente o que só se vê na tela, e
	 * o teste precisa poder mudar o campo e pedir o redesenho sem esperar
	 * uma batalha inteira acontecer.
	 */
	void RefreshTileVisuals();

	/**
	 * O sol da cena, ou nulo quando o mapa já tinha um.
	 *
	 * O mapa do jogo não tem ator de luz nenhum, e sem sol a engine ilumina
	 * tudo com o ambiente azul padrão — foi assim que a mata verde apareceu
	 * azul-clara na tela.
	 */
	ABattleSceneLighting* GetSceneLighting() const { return SceneLighting; }

	/** Os treinadores em campo, um por lado que tem pet. */
	const TArray<TObjectPtr<APetOwnerView>>& GetOwnerViews() const { return SpawnedOwnerViews; }

	// Centro (em espaço de mundo) da casa (Column, Row), dentro da grade ativa.
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

	/**
	 * Skills por tipo de pet. Vazio = arquivo padrão em Config/.
	 *
	 * Ausente ou malformado degrada para "todo pet tem só os seis universais"
	 * (DP-skill-04) — nunca para crash, e nunca para "todas as skills".
	 */
	UPROPERTY(config, EditDefaultsOnly, Category = "Balanceamento")
	FString PetSkillCatalogPath;

	/**
	 * Layouts de arena. Vazio = arquivo padrão em Config/.
	 *
	 * Ausente ou malformado degrada para arena neutra — o comportamento de
	 * antes desta ligação, nunca crash.
	 */
	UPROPERTY(config, EditDefaultsOnly, Category = "Balanceamento")
	FString ArenaLayoutCatalogPath;

	/** Tela de resultado. Vazia = o feed de combate anuncia sozinho. */
	UPROPERTY(config, EditDefaultsOnly, Category = "Interface")
	FSoftClassPath BattleResultWidgetClassPath;

protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> ArenaRoot;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCameraComponent> ArenaCamera;

	UPROPERTY()
	TObjectPtr<AForestBackdrop> ForestBackdrop;

	UPROPERTY()
	TObjectPtr<AMountainRange> MountainRange;

	UPROPERTY()
	TObjectPtr<ABattleSceneLighting> SceneLighting;

	UPROPERTY()
	TArray<TObjectPtr<APetOwnerView>> SpawnedOwnerViews;

	/** Planta a mata em volta, uma vez, quando a arena entra em cena. */
	void SpawnForestBackdrop();

	/**
	 * Ergue a serra que fecha o fundo, ATRÁS da mata.
	 *
	 * A mata veste os primeiros sessenta metros e para; sem relevo ao longe,
	 * o céu encosta no chão logo depois do último tronco e o mundo não tem
	 * tamanho nenhum.
	 */
	void SpawnMountainRange();

	/** Acende sol e céu, a menos que o mapa já traga os seus. */
	void SpawnSceneLighting();

	/** Põe um treinador em campo por lado que tem pet lutando. */
	void SpawnOwnerViews(const FBattleState& InitialState);

	/**
	 * Empurra para a mata o chão que o mundo emprestou.
	 *
	 * Quem tem chão é a MATA, não a arena: enquanto a arena desenhava terra
	 * própria, ela lia na tela como um prato pousado sobre o cenário. O que
	 * o mundo empresta, portanto, veste o chão da mata.
	 */
	void ApplyAdoptedGroundMaterial();

	/**
	 * Refaz mata e serra com o bioma já sabido.
	 *
	 * Existe porque a arena nasce e roda `BeginPlay` ANTES de alguém contar de
	 * onde a luta veio — o mesmo atraso que o chão emprestado já tinha. Como
	 * as duas construções limpam antes de plantar, refazer é idempotente.
	 */
	void RebuildScenaryForBiome();

	/**
	 * Escreve no painel o bioma e a contagem do que a mata plantou.
	 *
	 * Uma função e não duas linhas soltas porque quem planta são DOIS lugares
	 * (o nascimento da arena e o refazer com o bioma sabido), e só o primeiro
	 * escrevia. O painel ficava dizendo "floresta com N elementos" depois de a
	 * luta ter mudado para geleira — que é exatamente a leitura de "a arena nao
	 * pegou o cenario", com o cenário certo por baixo.
	 */
	void ShowScenaryPanelLine();

	/** De onde a luta veio, quando alguém contou. Ver GetEncounterBiome. */
	TOptional<EIslandBiome> EncounterBiome;

	/** Chão emprestado pelo mundo (AdoptAmbienceFromWorldLocation). */
	UPROPERTY()
	TObjectPtr<UMaterialInterface> AdoptedFloorMaterial;

	UPROPERTY(VisibleAnywhere)
	TArray<TObjectPtr<UStaticMeshComponent>> CellTileMeshes;

	UPROPERTY(VisibleAnywhere)
	TArray<TObjectPtr<UStaticMeshComponent>> CellObstacleMeshes;

	/** As duas matérias de que um bloqueio é feito: pedra ou tronco caído. */
	UPROPERTY()
	TObjectPtr<UStaticMesh> RockObstacleMesh;

	UPROPERTY()
	TObjectPtr<UStaticMesh> LogObstacleMesh;

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

	/** Nome do golpe usado no slot deste evento, ou vazio se não houver. */
	FString FindMoveNameForEvent(const FBattleEvent& Event) const;

	/** Cada pet vivo passa a olhar para o adversário vivo. */
	void RefreshGazes();

public:
	/**
	 * Modo de teste: o mesmo jogador escolhe pelos DOIS lados.
	 *
	 * Existe porque verificar trombada, esquiva na trombada ou camuflagem
	 * depende de as duas escolhas serem deliberadas — esperar o sorteio do
	 * oponente cair no caso desejado é medir a sorte, não a regra.
	 *
	 * Console: `bs.ControlOpponent 1`.
	 */
	void SetControllingBothSides(bool bEnabled);

	bool IsControllingBothSides() const { return bControlsBothSides; }

	/**
	 * Troca QUAL jogador você comanda. O outro passa a ser jogado pelo bot.
	 *
	 * É o que permite experimentar ações diferentes de cada lado: joga-se um
	 * turno pelo jogador 1, troca, e joga-se pelo 2 — sem depender de o
	 * sorteio da IA cair no caso que se quer ver.
	 *
	 * Zera as escolhas pendentes: elas foram pensadas para o OUTRO pet, e
	 * aplicá-las ao novo produziria uma jogada que ninguém pediu.
	 */
	void SwapControlledPlayer();

	/** O que o pet daquele lado sabe fazer — universais mais skills do tipo. */
	TArray<EActionType> GetAvailableActionsForSide(uint8 Side) const;

	/**
	 * Ações do jogador 2, escolhidas à mão na barra da tela.
	 *
	 * Enquanto houver alguma aqui, ela SUBSTITUI o commit do bot. O turno
	 * continua fechando pelo botão normal de confirmar do jogador 1 — não há
	 * segunda fase, nem troca de lado: quem escolhe pelos dois escolhe os dois
	 * e confirma uma vez.
	 */
	void AddPlayerTwoAction(const FBattleAction& Action);
	void ClearPlayerTwoActions();
	const TArray<FBattleAction>& GetPlayerTwoActions() const { return PlayerTwoManualActions; }

	/** Nome de apresentação do pet, ou vazio se ele não tem um. */
	FString GetPresentationNameForPet(uint8 PetId) const;

	/** Tipo do pet, ou "?" se desconhecido — nunca vazio. */
	FString GetPresentationTypeForPet(uint8 PetId) const;

	/** Nomes dos golpes do pet daquele lado, na ordem do slot. */
	TArray<FString> GetMoveNamesForSide(uint8 Side) const;

	/**
	 * O golpe daquele índice está liberado para o pet daquele lado?
	 *
	 * Consulta, não decisão: quem RECUSA é UBattleActionQueueComponent. A tela
	 * pergunta para não oferecer o que seria recusado — se ela decidisse,
	 * haveria duas regras, e elas concordariam até a primeira edição.
	 */
	bool IsMoveUnlockedForSide(uint8 Side, int32 MoveIndex) const;

	/** "exige Voo 12", ou vazio quando o golpe não exige nada. */
	FText GetMoveRequirementTextForSide(uint8 Side, int32 MoveIndex) const;

private:
	void ResolveTurnWithCommits(const FTurnCommit& LocalCommit, const FTurnCommit& OpponentCommit);

public:

	/** 0 = escolhendo pelo jogador, 1 = pelo oponente. Sempre 0 com o modo desligado. */
	uint8 GetSideBeingChosen() const { return bAwaitingOpponentChoice ? OpponentSideForChoice() : LocalPlayerSide; }

	uint8 GetControlledPlayerNumber() const { return LocalPlayerSide + 1; }

	/** Só para teste: montar cenários de fim de batalha sem jogar dez turnos. */
	FBattleState& GetMutableCurrentState() { return CurrentState; }

private:
	uint8 OpponentSideForChoice() const { return LocalPlayerSide == 0 ? 1 : 0; }

	bool bControlsBothSides = false;
	bool bAwaitingOpponentChoice = false;
	FTurnCommit StoredLocalCommit;

	// Rascunho POR JOGADOR: trocar de jogador controlado não pode apagar o que
	// o outro já tinha escolhido. Indexado por lado (0 e 1).
	TArray<FBattleAction> DraftsBySide[2];

	TArray<FBattleAction> PlayerTwoManualActions;

	// Traço do turno que ENCERROU a batalha, guardado até a reprodução acabar.
	TArray<FBattleEvent> PendingEndOfBattleTrace;

	// Commits do turno em reprodução, para a narração saber QUAL golpe caiu.
	//
	// O evento do núcleo não carrega o índice do golpe, e forçá-lo num campo
	// livre (FromCell) seria dar dois significados ao mesmo byte — o tipo de
	// economia que produz defeito calado. A arena já tem os commits: basta
	// guardá-los enquanto o traço é reproduzido.
	FTurnCommit LastCommitBySide[2];

	uint16 FindPostureFlagsForPet(uint8 PetId) const;

	// T5 (colecao-e-captura) 🧠: varre o trace por BatalhaEncerrada; se o
	// jogador local venceu, captura o pet do lado OPOSTO — nunca o
	// próprio pet do jogador.
	/** Garante que o pet do jogador está na coleção — é dele que a XP vive. */
	/** Diz à fila o que ESTE pet pode escolher (DP-skill-02). */
	/** Escolhe e aplica um layout de arena, se a montagem não trouxe um. */
	void ShowResultWidgetIfConfigured(const FBattleEvent& EndEvent);

	void ApplyArenaLayoutIfNeeded();

	/** Diz quem enfrenta quem, e quem tem vantagem, ANTES da primeira escolha. */
	void AnnounceMatchup();

	void ApplySkillsToActionQueue();



	void RegisterOwnPetInCollection();

	void CheckForCapture(const TArray<FBattleEvent>& Trace);

	// T4 (niveis-experiencia-evolucao): credita XP ao pet do JOGADOR
	// LOCAL (nunca o oponente) se o CatalogId dele já está na coleção —
	// em qualquer resultado (vitória/derrota/empate), quantidades
	// diferentes. Pet ainda não capturado não gera XP fantasma.
	void GrantExperienceIfOwned(const TArray<FBattleEvent>& Trace);

	/**
	 * Soma o que o pet do jogador ganhou de atributo NESTE turno.
	 *
	 * Chamado a cada turno resolvido, nos dois caminhos (local e rede); a
	 * gravação acontece uma vez só, junto da XP, no fim da batalha. Somar na
	 * hora e gravar no fim é o que evita uma escrita de save por turno sem
	 * perder o que aconteceu nos turnos do meio.
	 */
	void AccumulateAttributeGains(const TArray<FBattleEvent>& Trace);

	/**
	 * Tranca os golpes cujo requisito o pet do jogador ainda não alcança.
	 *
	 * Avaliado UMA vez, na montagem, contra a coleção — e não a cada clique:
	 * o atributo não muda no meio da batalha, e reavaliar por clique só
	 * abriria a chance de o botão e a fila discordarem entre um e outro.
	 */
	void ApplyMoveRequirementsToActionQueue();

	FPetAttributeGains AccumulatedAttributeGains;

	/**
	 * Diz na tela o que cresceu, como `Camuflagem 3 → 4`.
	 *
	 * Atributo que sobe sem aparecer é atributo que o jogador descobre — se
	 * descobrir — abrindo o save. O antes e o depois vão os dois: só o número
	 * final não deixa ver que houve ganho.
	 */
	void ShowAttributeGains(const FPetPresentationInfo& Presentation,
		const FOwnedPetInstance& Antes, const FOwnedPetInstance& Depois) const;

	/**
	 * Anuncia o golpe que ABRIU com o que o pet acabou de ganhar.
	 *
	 * É o retorno que fecha a volta: sem ele, o jogador vê o atributo subir
	 * numa tela e descobre o golpe novo — se descobrir — numa batalha
	 * seguinte, sem ligar uma coisa à outra.
	 *
	 * Compara o requisito contra o ANTES e o DEPOIS. Só olhar o depois
	 * anunciaria de novo, a cada batalha, todo golpe já destravado.
	 */
	void AnnounceMovesUnlockedBy(const FPetPresentationInfo& Presentation,
		const FOwnedPetInstance& Antes, const FOwnedPetInstance& Depois) const;

	// Dispara OnBattleFinished se o trace contiver BatalhaEncerrada, no
	// máximo uma vez por arena.
	void AnnounceBattleFinishedIfEnded(const TArray<FBattleEvent>& Trace);

	void LogCommit(const TCHAR* Quem, const FTurnCommit& Commit) const;

	/** Destrava a fila para a próxima rodada, se a batalha ainda não acabou. */
	/** Fim de batalha e abertura do turno seguinte, depois da reprodução. */
	void FinishPlaybackAndSettleTurn();

	void OpenNextTurnIfBattleContinues();

	/**
	 * Desenha a grade 3x3 no mundo, com a coordenada de cada casa e quem está
	 * nela. É o que torna posição e direção LEGÍVEIS — teria mostrado na hora
	 * que "Baixo" andava para a direita, e mostra a coabitação sem precisar
	 * de explicação. Só desenha com bs.ShowBattleDebug ligado.
	 */
	void DrawDebugGrid() const;

	/**
	 * Pinta a arena assim que o nível sobe, sem esperar batalha. Aberto no
	 * editor, o tabuleiro precisa existir antes de alguém apertar jogar.
	 */
	virtual void BeginPlay() override;

	/** Monta chão, moldura e lajes no construtor, com malha e material já atribuídos. */
	void BuildArenaGeometry();


	/**
	 * Mostra, escala, gira e pinta o que ocupa UMA casa bloqueada — e o
	 * esconde em toda casa que não é.
	 *
	 * A casa bloqueada era só uma laje mais alta: quem olhava via piso, e
	 * "não dá para andar aqui" ficava por conta de quem já sabia da regra.
	 */
	bool RefreshCellObstacle(int32 CellIndex, int32 Column, int32 Row, uint8 CellProperty);

	UMaterialInterface* ResolveTileMaterial(uint8 CellProperty) const;

	// A rodada seguinte só abre quando a reprodução do trace termina: abrir
	// antes deixaria o jogador escolhendo o próximo turno enquanto o anterior
	// ainda está acontecendo na tela.
	bool bWaitingForPlaybackToOpenNextTurn = false;

	bool bHasAnnouncedBattleFinished = false;
};

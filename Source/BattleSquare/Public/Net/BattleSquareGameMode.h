// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "World/WaterFooting.h"
#include "World/WorldDiscovery.h"
#include "World/WorldMapPins.h"
#include "UI/WorldMapProjection.h"
#include "Math/RandomStream.h"
#include "Engine/TimerHandle.h"
#include "GameFramework/GameModeBase.h"
#include "Net/BattleRoomRegistry.h"
#include "Net/BattleTurnCoordinator.h"
#include "Battle/BattleArena.h"
#include "Environment/WorldTimeOfDay.h"
#include "Data/BattleDataTranslator.h"
#include "Meta/PetCollectionSaveGame.h"
#include "BattleSquareGameMode.generated.h"

/**
 * Declaração adiantada COM O TIPO BASE, e o `: uint8` não é enfeite.
 *
 * Sem ele o compilador declara um enum NOVO de tipo diferente, e o erro que
 * sai fala de "tipo incompleto" em cinco arquivos que não têm nada a ver.
 */
enum class EVillageBuilding : uint8;
enum class ESettlementKind : uint8;

class ABattleSquarePlayerController;

// T7–T9 (tasks.md, Sala e Pareamento Simples): fiação fina — liga
// UBattleRoomRegistry (lógica pura, T2–T6) a jogadores/atores reais.
// Toda decisão de "pode ou não pode" já mora no registro; este ator só
// reage aos delegates dele e monta o que o Combate Online já construiu
// (ABattleArena, UBattleTurnCoordinator, UBattleNetCommitComponent).
class AForestBackdrop;
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
	/**
	 * Força o carregamento das malhas pesadas ENQUANTO a tela cobre.
	 *
	 * Não acelera nada, e é importante não confundir: move o custo para um
	 * momento em que existe tela, em vez de deixá-lo cair no meio do jogo.
	 */
	void WarmUpHeavyAssets();

	/**
	 * O mundo está montado?
	 *
	 * É um PORTÃO, e não um aviso: enquanto for falso, o jogo não roda — o
	 * jogador não simula, e o que vier a depender do mundo pergunta aqui em
	 * vez de descobrir do jeito difícil.
	 *
	 * Nasceu de um defeito que o usuário viu antes de mim: o jogador caía pelo
	 * cenário porque a montagem acontece no Tick, e ele existia e simulava
	 * física durante os quadros em que ainda não havia chão. A tela de
	 * carregamento cobria isso — e cobrir um mundo vivo pela metade é
	 * diferente de não o deixar viver antes da hora.
	 */
	bool IsWorldReady() const { return WorldEncounterFlow != nullptr; }

	/**
	 * Impede o jogador de cair num mundo que ainda não foi montado.
	 *
	 * A montagem roda no Tick e depende do pawn; até ela acontecer, não há
	 * chão. Com a tela de carregamento por cima ele não tem o que fazer mesmo,
	 * e física rodando num mundo inexistente é a definição do problema.
	 */
	void FreezePlayerWhileWorldIsNotReady(bool bFreeze);

	void SpawnWorldScenery();

	/**
	 * Monta o que está perto e derruba o que ficou para trás.
	 *
	 * A ilha cresceu de 6000 para 20000 de raio — vinte vezes a área. Plantar
	 * tudo de uma vez é exatamente o que o usuário pediu para não acontecer:
	 * "não podemos deixar ficar devagar, devemos recarregar por mapa".
	 *
	 * Roda no MESMO temporizador lento da descoberta. Um pedaço tem 6400
	 * unidades e ninguém o atravessa em meio segundo; chamar isto a cada
	 * quadro seria refazer sessenta vezes por segundo uma conta cuja resposta
	 * só muda quando se cruza uma fronteira.
	 *
	 * O que decide é `RegionResidency`, que é planejador puro e já tem teste.
	 * Aqui só se cumpre a ordem: nascer ator, chamar `BuildRegion`, destruir.
	 */
	void RefreshRegionResidency();

	/** Faz nascer o ator de um pedaço, já plantado com o bioma do lugar. */
	void BuildResidentChunk(const FIntPoint& Chunk);

	/**
	 * Os pedaços vivos, pelo endereço deles.
	 *
	 * Fraco de propósito: o ator é `RF_Transient` e a viagem para a arena
	 * leva o nível junto. Ponteiro forte manteria vivo o que o mundo já
	 * desmontou, e a residência passaria a achar montado o que não existe.
	 */
	TMap<FIntPoint, TWeakObjectPtr<AForestBackdrop>> ResidentChunks;

	/**
	 * A altura do chão do mundo, achada por traço uma vez na montagem.
	 *
	 * Guardada porque cada pedaço novo nasce nela, e refazer o traço a cada
	 * um deles perguntaria ao chão que o pedaço anterior acabou de criar —
	 * empilhando terra sobre terra a cada passo do jogador.
	 */
	float WorldGroundZ = 0.0f;

	FTimerHandle ResidencyTimer;

	/**
	 * Registra onde o jogador está, e grava se algo novo apareceu.
	 *
	 * No MESMO temporizador lento do painel do mundo: descoberta é regional, e
	 * uma região tem 800 unidades — ninguém a atravessa entre dois quadros.
	 * Marcar a cada Tick seria gastar sessenta chamadas por segundo para
	 * responder a mesma coisa.
	 *
	 * Só grava quando descobre: sem isso, cada passo escreveria o save inteiro
	 * no disco.
	 */
	void RefreshWorldDiscovery();

	/** O que este jogador já viu. Carregado do save ao montar o mundo. */
	FWorldDiscovery WorldDiscovery;

	FTimerHandle DiscoveryTimer;

	/**
	 * O terreno do mundo em pedaços, montado UMA vez.
	 *
	 * Ele não muda: a mata, a serra e a água estão onde a montagem as pôs.
	 * Refazer isso a cada atualização do mapa seria varrer centenas de
	 * instâncias de malha duas vezes por segundo para chegar sempre à mesma
	 * resposta.
	 */
	TArray<FWorldMapTerrainTile> WorldTerrainTiles;

	void BuildWorldTerrainTiles();

	/**
	 * Ergue a vila inicial no bloco 0,0.
	 *
	 * Antes dela, o jogador nascia num descampado com cinco clareiras — e a
	 * primeira sessão jogada disse exatamente isso: o mundo não parecia um
	 * lugar. A vila é o primeiro lugar que existe por decisão, e não por
	 * geração.
	 */
	void SpawnStartingVillage();

public:
	/**
	 * Marca o lugar onde o jogador está — ou apaga, se já houver marcação aqui.
	 *
	 * NA POSIÇÃO DELE, e não onde ele clicar no mapa: clique em widget de
	 * viewport já falhou três vezes neste projeto (o painel de depuração
	 * inteiro nasceu disso), e L-038 diz que caminho que falha três vezes é a
	 * dependência errada. Marcar onde se está sempre funciona, e é o gesto que
	 * o jogador faz mais — "isto aqui eu quero achar de novo".
	 */
	void ToggleMapPinHere(EWorldPinKind Kind);

private:
	FWorldMapPins MapPins;

	/**
	 * Escreve o painel do mundo: seu pet, seus atributos, quem está por perto.
	 *
	 * Num TEMPORIZADOR lento, não a cada quadro: nada disso muda em 16ms, e um
	 * painel reescrito 60 vezes por segundo é custo puro. A coleção só é
	 * relida quando pode ter mudado — o disco não tem por que ser tocado
	 * enquanto o jogador só anda.
	 */
	void RefreshWorldStatus();

	/** Alimenta o minimapa e o mapa completo. */
	void RefreshWorldMap();

	FTimerHandle WorldMapTimer;

	/** Passo do mapa. Mais curto que o do painel — ver o .cpp. */
	UPROPERTY(config, EditDefaultsOnly, Category = "Mundo")
	float WorldMapRefreshSeconds = 0.1f;

	void ReloadOwnedPetSnapshot();

	FTimerHandle WorldStatusTimer;
	FOwnedPetInstance CachedOwnedPet;
	bool bHasCachedOwnedPet = false;

	/** De quantos em quantos segundos o painel do mundo é reescrito. */
	UPROPERTY(config, EditDefaultsOnly, Category = "Mundo")
	float WorldStatusRefreshSeconds = 0.5f;

	/**
	 * Cria os campos de treino em volta do ponto de partida.
	 *
	 * Um por atributo, em círculo: o jogador vê as cinco clareiras coloridas
	 * de onde nasce e escolhe para onde ir. Espalhá-los sem que nenhum seja
	 * visível do início transformaria a escolha em busca ao acaso.
	 */
	void SpawnTrainingFields();

	/**
	 * Acumula o treino de quem está DENTRO de um campo.
	 *
	 * O tempo é o do JOGO ABERTO — o delta entre duas passagens deste
	 * temporizador — e não o relógio do sistema. É de propósito, e é a metade
	 * da decisão que ainda é sua (DP-atr-10): adiantar o relógio da máquina
	 * não adianta este treino, e nenhuma conta ou rede é exigida para treinar.
	 * O custo é o outro lado: não avança com o jogo fechado.
	 *
	 * Trocar para relógio de servidor depois mexe SÓ nesta função — a regra de
	 * rendimento (FTrainingFieldRules) já recebe o tempo pronto e não sabe de
	 * onde ele veio.
	 */
	void TickTrainingFields();

	/** Põe a hora e a fase no painel — relógio que não se vê não existe. */
	void TickWorldClock();

	/** Sorteia o tempo desta hora, entrega ao sol e escreve no painel. */
	void MostrarTempoDoMundo();

	/**
	 * Escreve no painel o que há no céu: fase da lua, eclipse e cometa.
	 *
	 * Linha separada da do tempo porque as duas mudam em ritmos diferentes: o
	 * céu leva dias para virar, e juntar tudo numa linha faria a fase da lua
	 * piscar junto com a chuva.
	 */
	void MostrarCeuDoMundo();

	/**
	 * Escreve no painel o evento de terra daqui e desta hora, e levanta o mar
	 * quando o tsunami sobe.
	 *
	 * Quem escreve a linha é quem move a água, de propósito: duas contas do
	 * mesmo tsunami — uma para o texto, outra para o mar — concordariam até a
	 * primeira edição, e aí o painel diria TSUNAMI com a água parada (L-032).
	 */
	void MostrarEventosDoMundo();

	/** O mar que fecha o mundo, para o tsunami ter o que levantar. */
	UPROPERTY(Transient)
	TObjectPtr<class AWorldBoundaryWater> AguaDoMundo;

	/** O relevo construído, para o painel poder dizer em que terreno se pisa. */
	UPROPERTY()
	TObjectPtr<class ATerrainMesh> RelevoDoMundo;

	/** O traçado assado de que este mundo saiu. Guardado para não recarregar. */
	UPROPERTY()
	TObjectPtr<const class UIslandBakedPlan> TracadoAssado;

	/**
	 * O passo do jogador em TERRA, guardado na primeira vez e nunca relido.
	 *
	 * Reler a velocidade atual multiplicaria o fator por si mesmo a cada passo
	 * dentro da água, e o jogador pararia de andar sem nada acusar.
	 */
	UPROPERTY()
	float PassoEmTerraUnidades = 0.0f;

	/** Quantos usos do solo o mundo ergueu. Zero é mundo sem motivo para andar. */
	UPROPERTY()
	int32 UsosDoSoloConstruidos = 0;

	/** Diz no painel de que deus é o templo ou a ruína em que se está. */
	/**
	 * ALGUÉM CRUZOU A PORTA DE UM PRÉDIO.
	 *
	 * Só ANUNCIA. Não cobra, não cura e não vende — entrar não é
	 * comprometer-se, e a vila que cobra por atravessar a calçada é a vila que
	 * ninguém atravessa duas vezes.
	 */
	void AnunciarPortaCruzada(EVillageBuilding Predio,
		ESettlementKind DeQueVila, bool bEntrou);

	/** A linha da carteira no painel. Chave fixa: é estado, não evento. */
	void MostrarCarteira() const;

	/**
	 * EM QUE PRÉDIO o jogador está, dito pelas portas da CI2.
	 *
	 * `bIsInsideBuilding` falso é "na rua". O par prédio+vila só vale enquanto
	 * verdadeiro — ler sem checar é ler o último prédio visitado.
	 */
	bool bIsInsideBuilding = false;

	// `{}` e não um valor nomeado: aqui o enum só existe como declaração
	// adiantada, e o par abaixo nunca é lido com `bIsInsideBuilding` falso.
	EVillageBuilding CurrentBuilding{};
	ESettlementKind CurrentBuildingKind{};

	/** A travessia sob os pés, e o aviso da ponte destruída antes da tentativa. */
	void AnunciarTravessiaPerto(const UIslandBakedPlan& Baked, const FVector2D& PositionUnits);

	/** O lugar que a carta conta e não aponta, revelado por andar até ele. */
	void AnunciarAchadoEscondido(const UIslandBakedPlan& Baked, const FVector2D& PositionUnits);

	void AnunciarSagradoPerto(const class UIslandBakedPlan& Assado, const FVector2D& Onde);

	/** Ergue os usos do solo do assado. Devolve quantos. */
	int32 ConstruirUsosDoSolo(const class UIslandBakedPlan& Assado,
		const struct FActorSpawnParameters& Parametros);

	/** Aplica ao movimento o que o pé encontrou. */
	void AplicarChaoMolhado(const APawn* Jogador, EWaterFooting Chao,
		const FVector2D& Fluxo, int32 ForcaPorMil);

public:
	/**
	 * PÕE um item na mochila do jogador. Ferramenta de desenvolvimento.
	 *
	 * Existe porque não há loja, espólio nem recompensa ainda, e sem uma
	 * maneira de conseguir um item a mochila seria um sistema que ninguém
	 * consegue exercitar — testes verdes sobre um eixo que o jogador não
	 * alcança, que é exatamente o defeito que a invariante 11 nomeia.
	 */
	bool GiveItem(const FString& ItemId, int32 Quantity);

	/** VESTE no pet do jogador, tirando da mochila. */
	bool EquipItemOnOwnedPet(const FString& ItemId);

	/** TIRA e devolve à mochila. */
	bool UnequipItemFromOwnedPet(const FString& ItemId);

	/** USA um consumível da mochila. */
	bool UseItem(const FString& ItemId);

private:

	/** A altura de repouso do mar, para a onda subir DE algum lugar. */
	float AguaEmRepousoZ = 0.0f;

	/** A luz do mundo, para o painel saber que horas o sol acha que são. */
	UPROPERTY(Transient)
	TObjectPtr<class ABattleSceneLighting> CenaDoMundo;

	/**
	 * A aurora sobre a geleira, para o relógio ter o que acender.
	 *
	 * Guardada porque ela responde ao HORÁRIO: quem a plantasse e a esquecesse
	 * teria uma cortina de brilho fixo, e aurora que não apaga de dia é mancha
	 * verde no céu ao meio-dia.
	 */
	UPROPERTY(Transient)
	TObjectPtr<class AAuroraCurtain> AuroraDoCeu;

	FTimerHandle TrainingTimer;
	float TrainingCarrySeconds = 0.0f;
	FTrainerProfile CachedTrainer;

public:
	/**
	 * Vira especialista no atributo do campo em que você está.
	 *
	 * Ato DELIBERADO, nunca automático: a especialidade é escassa e a escolha
	 * não se desfaz, então gastá-la por estar parado no lugar errado seria
	 * punição por acidente. Exposto por console (`bs.Especializar`) porque
	 * clicar exigiria uma barra no mundo que ainda não existe.
	 */
	bool LearnSpecialtyOfCurrentField();

	/**
	 * Vende um pet da coleção, se o jogador estiver num lugar que compra.
	 *
	 * O gesto é DELIBERADO (console `bs.VenderPet`), nunca a porta: entrar não
	 * é comprometer-se. A porta só diz ONDE o jogador está — e é ela que
	 * responde "aqui não se vende".
	 */
	void SellOwnedPet(const FString& CatalogId);

	/**
	 * Raio do anel dos campos de treino.
	 *
	 * Era 1.800 — dezoito metros numa ilha de mil e quatrocentos. Os cinco
	 * cabiam num ponto, e a spec já dizia o efeito disso: "as cinco clareiras
	 * estão todas a 18 metros do centro, e por isso nenhuma é uma viagem".
	 *
	 * A carta do mundo mostrou o tamanho do problema, e o conserto é um número
	 * só. Fica FORA da clareira da vila e bem dentro do anel das outras vilas:
	 * cada campo vira destino, e a trilha para eles é caminhada, não passo.
	 */
	UPROPERTY(config, EditDefaultsOnly, Category = "Mundo")
	float TrainingFieldRingRadiusUnits = 22000.0f;

	/** Zero desliga os campos de treino. */
	UPROPERTY(config, EditDefaultsOnly, Category = "Mundo")
	bool bSpawnTrainingFields = true;

	/**
	 * A bolsa inicial do treinador (decisão 56).
	 *
	 * 100 não é redondo por acaso: quatro curas na cidade (25) e pouco mais
	 * que uma venda de pet (70) — a primeira venda ainda é um acontecimento.
	 * Config, porque é o tipo de número que se equilibra jogando, não
	 * recompilando.
	 */
	UPROPERTY(config, EditDefaultsOnly, Category = "Economia")
	int32 StartingMoney = 100;

	/**
	 * Repõe a população: tira de cena os derrotados e cria novos até o alvo.
	 *
	 * Sem isto o mundo ACABA — com um número fixo, seis batalhas esvaziavam o
	 * mapa e sobrava caminhar por um lugar onde nada mais acontece. E o pet
	 * derrotado continuava passeando como fantasma, o que é pior que sumir:
	 * dá a entender que a batalha não valeu.
	 */
	void MaintainEncounterPopulation();

	void SpawnOneEncounter(const FVector& Centro, FRandomStream& Sorteio, int32 SementeDoPasseio);

public:
	/**
	 * A fase do dia que vale AGORA para quem sorteia encontros.
	 *
	 * Uma função, e não um campo: guardar a fase obrigaria a atualizá-la, e o
	 * dia que ela ficasse velha o painel diria noite enquanto o mundo sorteia
	 * bicho de meio-dia (L-032). A hora vem do sol, que é quem a conta.
	 *
	 * Sem cena de mundo, a hora é `WorldStartHour`: o mundo ainda não começou
	 * a andar, e a hora em que ele abre é a única que existe.
	 */
	EDayPhase CurrentEncounterPhase() const;

private:

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

public:
	/**
	 * Só para o despejo do mapa ler os mesmos números que o mundo usa.
	 *
	 * Sem eles, quem desenha a carta transcreve — e transcrição é a cópia que
	 * concorda até a primeira edição (L-032).
	 */
	int32 GetWorldScenerySeedForMap() const { return WorldScenerySeed; }
	float GetTrainingFieldRingForMap() const { return TrainingFieldRingRadiusUnits; }

	/**
	 * A hora em que o mundo abre, de 0 a 24.
	 *
	 * Sete da manhã: o jogo começa de dia, porque começar no escuro faz quem
	 * abriu pela primeira vez achar que algo não carregou. Quem quiser
	 * verificar a noite, a aurora ou um bicho noturno muda isto e entra
	 * direto na hora — esperar dez minutos de relógio para medir a noite é
	 * medir a paciência.
	 */
	UPROPERTY(config, EditDefaultsOnly, Category = "Mundo")
	float WorldStartHour = 7.0f;

	/** Quanto dura um dia inteiro, em segundos de relógio de parede. */
	UPROPERTY(config, EditDefaultsOnly, Category = "Mundo")
	float WorldSecondsPerDay = 1200.0f;

	/** Ids do catálogo sorteados para os encontros. */
	UPROPERTY(config, EditDefaultsOnly, Category = "Mundo")
	TArray<FString> WorldEncounterCatalogIds;

	/**
	 * A volta ao mundo — e, na DERROTA, o renascimento.
	 *
	 * Recebe a arena como payload do bind porque o delegate não carrega nada,
	 * e é dela que se lê COMO terminou. Derrota acorda o jogador na porta do
	 * Centro de Recuperação mais perto de onde ele caiu (decisão 60).
	 */
	void HandleWorldBattleFinished(class ABattleArena* Arena);

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

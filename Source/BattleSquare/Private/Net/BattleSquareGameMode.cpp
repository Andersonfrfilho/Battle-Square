// Copyright 2026 Anderson. All Rights Reserved.

#include "Net/BattleSquareGameMode.h"
#include "Balance/TypeEffectivenessTable.h"
#include "World/EncounterRoamingComponent.h"
#include "World/WorldEncounterActor.h"
#include "Blueprint/UserWidget.h"
#include "Debug/BattleDebugToolbar.h"
#include "Debug/BattleDebugScreen.h"
#include "Battle/BattleActionQueueComponent.h"
#include "UI/BattleActionSelectorWidget.h"
#include "Net/BattleSquarePlayerController.h"
#include "Debug/BattleDebugHUD.h"
#include "Data/PetDataLoader.h"
#include "Meta/PetCollectionService.h"
#include "Meta/PetProgressionService.h"
#include "Meta/PetAttributeProgression.h"
#include "World/WorldEncounterFlow.h"
#include "World/WorldExplorerCharacter.h"
#include "Environment/ForestBackdrop.h"
#include "Environment/MountainRange.h"
#include "Environment/ScenaryClimate.h"
#include "Environment/SceneLighting.h"
#include "World/WorldStatusReadout.h"
#include "UI/WorldLoadingScreen.h"
#include "UI/WorldMapScreen.h"
#include "Environment/WorldBoundaryWater.h"
#include "World/WorldTrainingField.h"
#include "Battle/PetView.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Character.h"
#include "Engine/StaticMeshActor.h"
#include "World/TrainingFieldRules.h"
#include "Balance/PetTypeCatalog.h"
#include "Meta/PetMoveRequirements.h"
#include "Meta/TrainerSpecialtyRules.h"
#include "World/EncounterDetectionComponent.h"
#include "World/EncounterMatchAssembler.h"
#include "Battle/BattleArena.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Misc/Paths.h"

ABattleSquareGameMode::ABattleSquareGameMode()
{
	PlayerControllerClass = ABattleSquarePlayerController::StaticClass();
	HUDClass = ABattleDebugHUD::StaticClass();
	PrimaryActorTick.bCanEverTick = true;
	// Timeouts de sala são medidos em dezenas de segundos
	// (CommitTimeoutSeconds=45, AbandonTimeoutSeconds=120) — checar uma
	// vez por segundo é preciso o bastante e barato mesmo com muitas
	// salas ativas (TMap pequeno, sem I/O).
	PrimaryActorTick.TickInterval = 1.0f;
}

void ABattleSquareGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	// Antes de qualquer pawn nascer: se o nível declara um pawn de exploração,
	// é ele que o jogador controla. Sem isto, a engine spawna um DefaultPawn
	// genérico, sem componente de encontro, e a corrente do mundo não sobe.
	if (!WorldExplorerPawnClassPath.IsValid())
	{
		return;
	}

	if (UClass* ExplorerClass = WorldExplorerPawnClassPath.TryLoadClass<APawn>())
	{
		DefaultPawnClass = ExplorerClass;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("ABattleSquareGameMode: WorldExplorerPawnClassPath não carregou (%s) — seguindo com o DefaultPawn da engine."),
			*WorldExplorerPawnClassPath.ToString());
	}
}

void ABattleSquareGameMode::BeginPlay()
{
	Super::BeginPlay();
	EnsureRoomRegistry();

	// A tela de carregamento sobe ANTES de qualquer montagem, e é o único
	// momento em que ela pode subir: a montagem acontece no Tick, num quadro
	// só, e uma tela mostrada junto com o trabalho não chega a ser desenhada.
	//
	// Ela não sobe num nível sem encontros configurados: ali não há nada a
	// esperar, e cobrir a tela para não esperar nada é pior que não cobrir.
	if (!WorldEncounterMirrorPath.IsEmpty())
	{
		FWorldLoadingScreen::Show(GetWorld());
		WarmUpHeavyAssets();
		RemoveStreamingTestCubes();
	}
}

void ABattleSquareGameMode::RemoveStreamingTestCubes()
{
	UWorld* World = GetWorld();
	if (!World || !bRemoveStreamingTestCubes)
	{
		return;
	}

	// A assinatura é ESTREITA de propósito: AStaticMeshActor com a malha de
	// cubo da engine. Nada que este projeto cria no mundo se parece com isso —
	// o pet é esfera, o campo de treino é cilindro, a mata é instanciada e a
	// água é cilindro. Uma varredura mais larga apagaria algo que alguém pôs
	// de propósito, e apagar em silêncio é o defeito que este paliativo não
	// pode ter.
	UStaticMesh* CuboDaEngine = LoadObject<UStaticMesh>(
		nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!CuboDaEngine)
	{
		return;
	}

	TArray<AActor*> Condenados;
	for (TActorIterator<AStaticMeshActor> It(World); It; ++It)
	{
		const UStaticMeshComponent* Malha = It->GetStaticMeshComponent();
		if (IsValid(*It) && Malha && Malha->GetStaticMesh() == CuboDaEngine)
		{
			Condenados.Add(*It);
		}
	}

	for (AActor* Cubo : Condenados)
	{
		Cubo->Destroy();
	}

	if (Condenados.IsEmpty())
	{
		return;
	}

	// BARULHENTO de propósito: paliativo silencioso vira permanente.
	UE_LOG(LogTemp, Warning,
		TEXT("[paliativo] %d cubos do teste de streaming removidos em tempo de jogo. ")
		TEXT("Eles são conteúdo do MAPA: apague-os no editor (World Outliner, filtro 'Cube') ")
		TEXT("e remova ABattleSquareGameMode::RemoveStreamingTestCubes."),
		Condenados.Num());

	FBattleDebugScreen::Show(
		FString::Printf(TEXT("%d cubos de teste removidos — apague-os no MAPA e tire este código"),
			Condenados.Num()),
		0.0f, FColor::Orange, /*Key=*/724);
}

void ABattleSquareGameMode::WarmUpHeavyAssets()
{
	// A MATA carrega 28 malhas distintas do disco, e ela as carrega no
	// CONSTRUTOR — ou seja, na primeira vez que um AForestBackdrop nasce.
	// Nascendo no meio da montagem, esse custo é um engasgo em pleno jogo;
	// forçando o objeto padrão a existir AQUI, ele acontece um quadro depois
	// da tela de carregamento subir, e fica coberto.
	//
	// Isto NÃO torna o carregamento mais rápido. Move para onde há tela,
	// que é diferente — e dizer o contrário seria mentir sobre o que se fez.
	const double Comecou = FPlatformTime::Seconds();

	AForestBackdrop::StaticClass()->GetDefaultObject();
	APetView::StaticClass()->GetDefaultObject();
	AWorldTrainingField::StaticClass()->GetDefaultObject();

	// MEDIDO, e não estimado: a próxima partida diz onde o tempo foi, em vez
	// de eu adivinhar. Foi assim que este projeto descobriu que a batalha
	// acontecia a um milhão de unidades da câmera.
	UE_LOG(LogTemp, Display,
		TEXT("[carregamento] malhas pesadas prontas em %.0f ms"),
		(FPlatformTime::Seconds() - Comecou) * 1000.0);
}

namespace
{
	/**
	 * Alcance do traço que procura o chão sob o jogador.
	 *
	 * Generoso de propósito: num nível cujo piso esteja bem abaixo do ponto de
	 * nascimento, um traço curto não acharia nada e a mata pousaria na altura
	 * do jogador — flutuando, que é pior que não existir.
	 */
	constexpr float TracoDeChaoUnidades = 100000.0f;

	/** Folga que evita as duas superfícies coplanares. */
	constexpr float FolgaDoChaoUnidades = 2.0f;

	/**
	 * Raio do chão da mata, EM CASAS. É a medida que AForestBackdrop usa
	 * internamente, e repeti-la aqui é o preço de a margem da água precisar
	 * coincidir com a borda da terra — com o número num lugar só, ele não
	 * discorda de si mesmo (L-032).
	 */
	constexpr float RaioDoChaoEmCasas = 30.0f;

	/**
	 * O pawn do jogador, com a MESMA tolerância que a detecção de encontro usa.
	 *
	 * `GetFirstPlayerController()->GetPawn()` pode ser nulo na montagem, e o
	 * código de encontro já previa isso caindo numa varredura. Quem cuidava do
	 * cenário não previa: o mundo ficaria com os campos de treino nascidos na
	 * origem e SEM CHÃO NENHUM, que é pior que nenhum dos dois.
	 */
	APawn* AcharPawnDoJogador(UWorld* World)
	{
		if (!World)
		{
			return nullptr;
		}

		if (const APlayerController* Controlador = World->GetFirstPlayerController())
		{
			if (APawn* Possuido = Controlador->GetPawn())
			{
				return Possuido;
			}
		}

		for (TActorIterator<APawn> Iterador(World); Iterador; ++Iterador)
		{
			if (IsValid(*Iterador))
			{
				return *Iterador;
			}
		}

		return nullptr;
	}

	TArray<uint8> MirrorKeyHexToBytes(const FString& Hex)
	{
		TArray<uint8> Bytes;
		Bytes.Reserve(Hex.Len() / 2);
		for (int32 Index = 0; Index + 1 < Hex.Len(); Index += 2)
		{
			Bytes.Add(static_cast<uint8>(FParse::HexNumber(*Hex.Mid(Index, 2))));
		}
		return Bytes;
	}
}

void ABattleSquareGameMode::FreezePlayerWhileWorldIsNotReady(bool bFreeze)
{
	ACharacter* Explorador = Cast<ACharacter>(AcharPawnDoJogador(GetWorld()));
	UCharacterMovementComponent* Movimento =
		Explorador ? Explorador->GetCharacterMovement() : nullptr;
	if (!Movimento)
	{
		return;
	}

	if (bFreeze)
	{
		// Já congelado? Sair cedo: reentrar todo quadro zeraria a velocidade
		// de novo, o que não faz mal, mas também não faz nada.
		if (Movimento->MovementMode == MOVE_None)
		{
			return;
		}

		Movimento->StopMovementImmediately();
		Movimento->SetMovementMode(MOVE_None);
		return;
	}

	if (Movimento->MovementMode != MOVE_None)
	{
		return;
	}

	// Volta CAINDO, e não andando: o chão acabou de nascer sob ele, e deixar a
	// engine resolver o contato é mais honesto que afirmar que ele já está em
	// pé sobre algo que pode não estar exatamente ali.
	Movimento->SetMovementMode(MOVE_Falling);
}

FString ABattleSquareGameMode::LoadConfiguredMirrorPets(TArray<FLoadedPetRecord>& OutPets) const
{
	if (WorldEncounterMirrorPath.IsEmpty())
	{
		return TEXT("WorldEncounterMirrorPath não configurado em DefaultGame.ini");
	}

	// Caminho relativo tem DOIS significados possíveis, e os dois aparecem de
	// verdade: o do .ini é relativo ao PROJETO ("Saved/..."), e o que a própria
	// engine devolve em FPaths::ProjectSavedDir() é relativo ao PROCESSO
	// ("../../../Projeto/Saved/..."). Resolver só de um jeito quebra o outro —
	// foi assim que o teste de bootstrap caiu. Tenta projeto, cai para processo.
	FString ResolvedMirrorPath = WorldEncounterMirrorPath;
	if (FPaths::IsRelative(ResolvedMirrorPath))
	{
		const FString ProjectRelative =
			FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(), WorldEncounterMirrorPath));
		ResolvedMirrorPath = FPaths::FileExists(ProjectRelative)
			? ProjectRelative
			: FPaths::ConvertRelativePathToFull(WorldEncounterMirrorPath);
	}

	int32 RejectedCount = 0;
	const bool bLoaded = FPetDataLoader::LoadVerifiedPets(
		ResolvedMirrorPath,
		MirrorKeyHexToBytes(WorldEncounterMirrorKeyHex),
		WorldEncounterMirrorPublicKeyPem,
		OutPets,
		RejectedCount);

	if (!bLoaded || OutPets.Num() == 0)
	{
		return FString::Printf(TEXT("espelho de pets não carregou em '%s'"), *ResolvedMirrorPath);
	}

	return FString();
}

FString ABattleSquareGameMode::SetUpWorldEncounterFlow()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return TEXT("sem UWorld");
	}

	// O pawn de exploração é quem carrega a detecção. Sem ele, não há
	// encontro possível — e isso é um nível legítimo, não um erro.
	//
	// O nível de verificação tem DOIS pawns com detecção (o de rota
	// determinística e o de exploração). Escolher "o primeiro que o iterador
	// achar" deixaria a decisão para a ordem de registro do mundo — o mesmo
	// não-determinismo silencioso que AD-004 recusa. A regra é explícita:
	// vence o pawn POSSUÍDO pelo jogador; só na ausência dele o primeiro serve.
	UEncounterDetectionComponent* Detection = nullptr;
	APawn* DetectingPawn = nullptr;

	if (const APlayerController* PlayerController = World->GetFirstPlayerController())
	{
		if (APawn* PossessedPawn = PlayerController->GetPawn())
		{
			if (UEncounterDetectionComponent* Found = PossessedPawn->FindComponentByClass<UEncounterDetectionComponent>())
			{
				Detection = Found;
				DetectingPawn = PossessedPawn;
			}
		}
	}

	if (!Detection)
	{
		for (TActorIterator<APawn> Iterator(World); Iterator; ++Iterator)
		{
			if (UEncounterDetectionComponent* Found = Iterator->FindComponentByClass<UEncounterDetectionComponent>())
			{
				Detection = Found;
				DetectingPawn = *Iterator;
				break;
			}
		}
	}

	if (!Detection)
	{
		// Transitório: o pawn pode chegar por streaming ou ser spawnado depois.
		bWorldEncounterSetupIsTransient = true;
		return TEXT("nenhum pawn com UEncounterDetectionComponent no nível");
	}

	bWorldEncounterSetupIsTransient = false;

	TArray<FLoadedPetRecord> Pets;
	if (const FString MirrorProblem = LoadConfiguredMirrorPets(Pets); !MirrorProblem.IsEmpty())
	{
		return MirrorProblem;
	}

	FEncounterMatchParams MatchParams;
	MatchParams.AvailablePets = Pets;
	MatchParams.PetCollectionSlotName = PetCollectionSlotName;
	// Sem pet declarado, o primeiro do espelho serve — é nível de teste, e
	// travar por configuração ausente aqui não ajudaria ninguém.
	MatchParams.PlayerCatalogId = WorldEncounterPlayerCatalogId.IsEmpty() ? Pets[0].Id : WorldEncounterPlayerCatalogId;

	WorldEncounterFlow = NewObject<UWorldEncounterFlow>(this);
	WorldEncounterFlow->Initialize(DetectingPawn, Detection, ABattleArena::StaticClass(), MatchParams);

	// A tela de ações se monta quando a batalha começa, não agora: montá-la
	// aqui deixaria botões por cima do mundo aberto o tempo todo.
	WorldEncounterFlow->OnWorldBattleStarted.AddUObject(
		this, &ABattleSquareGameMode::HandleWorldBattleStarted);

	FWorldLoadingProgress Progresso;
	Progresso.bTypeCatalogReady = !FPetTypeCatalog::Get().IsEmpty();
	Progresso.bMirrorVerified = true; // chegou aqui: o espelho carregou e verificou
	FWorldLoadingScreen::Update(Progresso);

	// Cada passo é CRONOMETRADO. O usuário relatou demora para a mata
	// aparecer; sem medir, o conserto seria palpite — e este projeto já
	// aprendeu que consertar por hipótese custa mais que medir uma vez.
	const double AntesDoCenario = FPlatformTime::Seconds();
	SpawnWorldScenery();
	Progresso.bSceneryBuilt = true;
	FWorldLoadingScreen::Update(Progresso);

	const double AntesDosEncontros = FPlatformTime::Seconds();
	SpawnRoamingEncounters();
	SpawnTrainingFields();
	Progresso.bEncountersPlaced = true;
	FWorldLoadingScreen::Update(Progresso);

	const double Terminou = FPlatformTime::Seconds();
	UE_LOG(LogTemp, Display,
		TEXT("[carregamento] cenário %.0f ms · encontros e campos %.0f ms"),
		(AntesDosEncontros - AntesDoCenario) * 1000.0,
		(Terminou - AntesDosEncontros) * 1000.0);

	FBattleDebugScreen::Show(
		FString::Printf(TEXT("carregamento: cenário %.0f ms, encontros %.0f ms"),
			(AntesDosEncontros - AntesDoCenario) * 1000.0,
			(Terminou - AntesDosEncontros) * 1000.0),
		0.0f, FColor::Silver, /*Key=*/723);

	ReloadOwnedPetSnapshot();
	if (WorldStatusRefreshSeconds > 0.0f)
	{
		World->GetTimerManager().SetTimer(WorldStatusTimer, this,
			&ABattleSquareGameMode::RefreshWorldStatus,
			WorldStatusRefreshSeconds, /*bLoop=*/true);

		// O MAPA anda mais rápido que o painel: meio segundo de atraso na
		// posição faz a seta pular atrás do jogador, e um mapa que persegue
		// quem o consulta é pior que nenhum.
		World->GetTimerManager().SetTimer(WorldMapTimer, this,
			&ABattleSquareGameMode::RefreshWorldMap,
			WorldMapRefreshSeconds, /*bLoop=*/true);

		// O treino usa o MESMO passo do painel: o jogador vê o número subir
		// no instante em que ele sobe, e não meio segundo depois.
		World->GetTimerManager().SetTimer(TrainingTimer, this,
			&ABattleSquareGameMode::TickTrainingFields,
			WorldStatusRefreshSeconds, /*bLoop=*/true);
	}
	return FString();
}

void ABattleSquareGameMode::EnsureRoomRegistry()
{
	if (RoomRegistry)
	{
		return;
	}

	RoomRegistry = NewObject<UBattleRoomRegistry>(this);
	RoomRegistry->OnRoomReady.AddUObject(this, &ABattleSquareGameMode::HandleRoomReady);
	RoomRegistry->OnRoomAbandoned.AddUObject(this, &ABattleSquareGameMode::HandleRoomAbandoned);
}

void ABattleSquareGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	EnsureRoomRegistry();

	if (!RoomRegistry || !GetWorld())
	{
		return;
	}

	const double CurrentTime = GetWorld()->GetTimeSeconds();
	// Ordem importa: CheckAbandonment primeiro (SALA-09, declara
	// vencedor enquanto ainda há um lado presente); CheckEmptyRooms
	// depois (SALA-11, só destrói o que os dois já abandonaram) — uma
	// sala recém-declarada abandonada continua existindo até seu próprio
	// timeout de "sala vazia" separado, dando tempo do jogador presente
	// ver o resultado (spec.md, edge case de fechamento de sala).
	RoomRegistry->CheckAbandonment(CurrentTime);
	RoomRegistry->CheckEmptyRooms(CurrentTime);

	// Encontros no mundo são opcionais: um nível sem pawn de exploração (ou
	// sem espelho configurado) segue funcionando exatamente como antes.
	if (!WorldEncounterFlow && bWorldEncounterSetupIsTransient)
	{
		// O jogador NÃO SIMULA enquanto o mundo não existe.
		//
		// A montagem depende do pawn e roda aqui, no Tick — e até ela
		// acontecer não há chão nenhum. O jogador nascia e CAÍA nesses
		// quadros, indo parar abaixo do chão antes de o chão ser criado; o
		// guarda de queda o devolvia, e ele caía de novo.
		//
		// Congelar é o conserto certo, e não engrossar o chão: com a tela de
		// carregamento por cima, ele não tem o que fazer mesmo. Física rodando
		// num mundo que ainda não foi montado é a definição do problema.
		FreezePlayerWhileWorldIsNotReady(true);

		const FString Problem = SetUpWorldEncounterFlow();
		if (Problem.IsEmpty())
		{
			UE_LOG(LogTemp, Display, TEXT("ABattleSquareGameMode: encontros de mundo ATIVOS."));

			// O mundo está montado: a tela sai. Sai AQUI, e não num
			// temporizador, porque o critério é o estado e não o relógio.
			FreezePlayerWhileWorldIsNotReady(false);
			FWorldLoadingScreen::Hide();

			// O mapa só aparece depois de haver mundo para mostrar: um
			// minimapa desenhando uma ilha vazia enquanto ela ainda é montada
			// mostraria um mundo que não existe.
			FWorldMapScreen::Show(GetWorld());
		}
		else if (!bHasLoggedWorldEncounterProblem || !bWorldEncounterSetupIsTransient)
		{
			// Falha permanente é logada como Warning e encerra as tentativas;
			// transitória é logada uma vez só, para o Tick 1x/s não inundar.
			bHasLoggedWorldEncounterProblem = true;
			if (bWorldEncounterSetupIsTransient)
			{
				UE_LOG(LogTemp, Log, TEXT("ABattleSquareGameMode: encontros de mundo ainda inativos — %s"), *Problem);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("ABattleSquareGameMode: encontros de mundo DESISTIDOS — %s"), *Problem);

				// Falha PERMANENTE: a tela diz o motivo em vez de girar para
				// sempre. Carregamento que nunca termina e nunca explica é o
				// pior dos dois mundos — o jogador não joga e não sabe por quê.
				FWorldLoadingProgress Falhou;
				Falhou.PermanentProblem = Problem;
				FWorldLoadingScreen::Update(Falhou);
			}
		}
	}
}

void ABattleSquareGameMode::Logout(AController* Exiting)
{
	EnsureRoomRegistry();

	if (ABattleSquarePlayerController* Controller = Cast<ABattleSquarePlayerController>(Exiting))
	{
		if (!Controller->CurrentRoomCode.IsEmpty() && RoomRegistry)
		{
			const double CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
			RoomRegistry->MarkDisconnected(Controller->CurrentRoomCode, Controller->CurrentSide, CurrentTime);
		}
	}

	Super::Logout(Exiting);
}

void ABattleSquareGameMode::RegisterControllerForRoom(const FString& Code, uint8 Side, ABattleSquarePlayerController* Controller)
{
	EnsureRoomRegistry();

	if (!Controller)
	{
		return;
	}

	Controller->CurrentRoomCode = Code;
	Controller->CurrentSide = Side;

	TArray<TWeakObjectPtr<ABattleSquarePlayerController>>& Controllers = RoomControllers.FindOrAdd(Code);
	Controllers.AddUnique(Controller);

	// Partida já montada (ex.: reconexão depois de HandleRoomReady já ter
	// rodado) — conecta este controller ao coordenador agora, não espera
	// um novo OnRoomReady que não vai disparar de novo.
	if (const FActiveMatch* Match = ActiveMatches.Find(Code))
	{
		if (Match->Coordinator && Controller->NetCommitComponent)
		{
			Controller->NetCommitComponent->SetServerCoordinator(Match->Coordinator, Side);
		}
	}
}

void ABattleSquareGameMode::ApplyOwnedPetProgressionBonus(const FString& SlotName, FPetState& PetState, const FPetPresentationInfo& Presentation)
{
	if (Presentation.CatalogId.IsEmpty())
	{
		return;
	}

	const TArray<FOwnedPetInstance> Collection = FPetCollectionService::LoadCollection(SlotName);
	const FOwnedPetInstance* OwnedInstance = Collection.FindByPredicate(
		[&Presentation](const FOwnedPetInstance& Instance) { return Instance.CatalogId == Presentation.CatalogId; });
	if (!OwnedInstance)
	{
		return;
	}

	FPetProgressionService::ApplyLevelBonus(PetState, FPetProgressionService::GetLevel(*OwnedInstance));

	// Reflexo e agressividade entram JUNTO do bônus de nível: os dois vêm da
	// mesma instância da coleção, e separá-los abriria a chance de um pet
	// entrar na batalha com o nível novo e os atributos velhos.
	FPetAttributeProgression::ApplyToBattleState(*OwnedInstance, PetState);
}

void ABattleSquareGameMode::HandleRoomReady(const FString& Code)
{
	TArray<FLoadedPetRecord> Pets;
	int32 RejectedCount = 0;
	const bool bLoaded = FPetDataLoader::LoadVerifiedPets(PetMirrorPath, PetMirrorEncryptionKey, PetMirrorPublicKeyPem, Pets, RejectedCount);

	if (!bLoaded || Pets.Num() < 2)
	{
		UE_LOG(LogTemp, Error, TEXT("ABattleSquareGameMode::HandleRoomReady: não foi possível montar pets para a sala %s (bLoaded=%d, PetsCarregados=%d)"), *Code, bLoaded ? 1 : 0, Pets.Num());
		return;
	}

	FPetState Side0Pet;
	FPetPresentationInfo Side0Presentation;
	FPetState Side1Pet;
	FPetPresentationInfo Side1Presentation;

	// Mesma correção da montagem de encontro: TranslateMatchup é o único que
	// aplica efetividade de tipo. Sem ele, os tipos existem no dado e não
	// mudam nada na luta.
	FTypeEffectivenessTable Efetividade;
	if (!FTypeEffectivenessTable::LoadFromJson(
		FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("TypeEffectiveness.json")), Efetividade))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("BattleSquareGameMode: tabela de efetividade nao carregou — combate neutro"));
	}

	FBattleDataTranslator::TranslateMatchup(Pets[0], Pets[1], Efetividade,
		/*LeftPetId=*/1, /*RightPetId=*/2,
		Side0Pet, Side0Presentation, Side1Pet, Side1Presentation);

	// T5 (niveis-experiencia-evolucao): pet de catálogo já capturado
	// entra na partida com o bônus de atributo do nível dele. Pet não
	// capturado (ou nível 1) fica exatamente como o catálogo — zero
	// regressão (NIVEL-09).
	ApplyOwnedPetProgressionBonus(PetCollectionSlotName, Side0Pet, Side0Presentation);
	ApplyOwnedPetProgressionBonus(PetCollectionSlotName, Side1Pet, Side1Presentation);

	FBattleState InitialState;
	InitialState.Pets.Add(Side0Pet);
	InitialState.Pets.Add(Side1Pet);
	InitialState.PlaceDuelistsAtStartingCells();

	// A UMIDADE do lugar entra no estado: é ela que decide se a poça vira lama
	// ou seca. Vem do mesmo clima que põe neve na serra — dois números
	// diferentes sobre o mesmo lugar, e não duas ideias de clima.
	InitialState.Humidity = static_cast<uint8>(FMath::Clamp(
		ScenaryClimate::HumidityPercent(ScenaryClimate::ConfiguredClimate()), 0, 100));



	TArray<FPetPresentationInfo> Presentations;
	Presentations.Add(Side0Presentation);
	Presentations.Add(Side1Presentation);

	AssembleMatchForRoom(Code, InitialState, Presentations);
}

void ABattleSquareGameMode::AssembleMatchForRoom(const FString& Code, const FBattleState& InitialState, const TArray<FPetPresentationInfo>& Presentations)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	ABattleArena* Arena = World->SpawnActor<ABattleArena>();
	if (!Arena)
	{
		return;
	}

	if (!Arena->BeginBattle(InitialState, Presentations))
	{
		// T7 (arenas-variadas): montagem rejeitada (ex.: pet em casa
		// bloqueada) — BeginBattle já logou o motivo. Nunca segue
		// montando uma partida meio-inicializada.
		Arena->Destroy();
		return;
	}

	UBattleTurnCoordinator* Coordinator = NewObject<UBattleTurnCoordinator>(this);
	Coordinator->BeginTurn(InitialState, World->GetTimeSeconds());
	Arena->ConfigureNetworkedOpponent(Coordinator);

	FActiveMatch Match;
	Match.Arena = Arena;
	Match.Coordinator = Coordinator;
	ActiveMatches.Add(Code, Match);

	ConnectControllersToCoordinator(Code, Coordinator);
}

void ABattleSquareGameMode::HandleRoomAbandoned(const FString& Code, uint8 PresentSide)
{
	const FActiveMatch* Match = ActiveMatches.Find(Code);
	if (!Match || !Match->Coordinator)
	{
		return;
	}

	Match->Coordinator->DeclareAbandonment(PresentSide);
}

void ABattleSquareGameMode::ConnectControllersToCoordinator(const FString& Code, UBattleTurnCoordinator* Coordinator)
{
	TArray<TWeakObjectPtr<ABattleSquarePlayerController>>* Controllers = RoomControllers.Find(Code);
	if (!Controllers)
	{
		return;
	}

	for (TWeakObjectPtr<ABattleSquarePlayerController>& WeakController : *Controllers)
	{
		if (ABattleSquarePlayerController* Controller = WeakController.Get())
		{
			if (Controller->NetCommitComponent)
			{
				Controller->NetCommitComponent->SetServerCoordinator(Coordinator, Controller->CurrentSide);
			}
		}
	}
}

void ABattleSquareGameMode::HandleWorldBattleStarted(ABattleArena* Arena)
{
	if (!Arena || !Arena->PlayerActionQueue)
	{
		return;
	}

	// Já existe uma? Desmonta antes: dois seletores ligados a filas
	// diferentes fariam metade dos cliques irem para a batalha anterior.
	TearDownBattleUi();

	UClass* WidgetClass = BattleActionSelectorWidgetClassPath.TryLoadClass<UBattleActionSelectorWidget>();
	APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!WidgetClass || !PlayerController)
	{
		// Sem tela de ações a batalha seria injogável. Melhor dizer isso alto
		// do que deixar o jogador achando que o jogo travou.
		FBattleDebugScreen::Show(
			TEXT("batalha do mundo SEM tela de ações — confira WorldBattleActionSelectorWidgetClassPath"),
			0.0f, FColor::Red, /*Key=*/710);
		return;
	}

	WorldBattleActionSelector = CreateWidget<UBattleActionSelectorWidget>(PlayerController, WidgetClass);
	if (!WorldBattleActionSelector)
	{
		return;
	}

	WorldBattleActionSelector->BindToQueue(Arena->PlayerActionQueue);
	WorldBattleActionSelector->AddToViewport();
	WorldBattleActionSelector->SetKeyboardFocus();

	FBattleDebugToolbar::Show(GetWorld());

	// A batalha é o jogo agora: o mouse precisa clicar nos botões.
	PlayerController->bShowMouseCursor = true;
	PlayerController->SetInputMode(
		FInputModeGameAndUI().SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock));

	Arena->OnBattleFinished.AddUObject(this, &ABattleSquareGameMode::HandleWorldBattleFinished);
}

void ABattleSquareGameMode::HandleWorldBattleFinished()
{
	TearDownBattleUi();

	// A batalha acabou de gravar experiência e atributo na coleção. Sem
	// reler aqui, o painel do mundo continuaria mostrando o pet de antes da
	// luta — e o jogador veria o ganho passar na tela da batalha e sumir.
	ReloadOwnedPetSnapshot();

	// De volta ao mundo: cursor escondido e input de jogo, senão o explorador
	// nasce sem responder e parece que a transição quebrou.
	if (APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
	{
		PlayerController->bShowMouseCursor = false;
		PlayerController->SetInputMode(FInputModeGameOnly());
	}
}

void ABattleSquareGameMode::TearDownBattleUi()
{
	if (WorldBattleActionSelector)
	{
		WorldBattleActionSelector->RemoveFromParent();
		WorldBattleActionSelector = nullptr;
	}

	FBattleDebugToolbar::Hide();
}

void ABattleSquareGameMode::SpawnTrainingFields()
{
	UWorld* World = GetWorld();
	if (!World || !bSpawnTrainingFields)
	{
		return;
	}

	// Campo colocado à mão manda — mesmo critério dos encontros e da mata.
	if (TActorIterator<AWorldTrainingField>(World))
	{
		return;
	}

	// MESMO pawn que a mata usou: campo de treino num lugar e chão em outro
	// deixaria os discos boiando fora da clareira.
	const APawn* Jogador = AcharPawnDoJogador(World);
	const FVector Centro = Jogador ? Jogador->GetActorLocation() : FVector::ZeroVector;

	// Os cinco atributos que existem, na mesma grafia do requisito de golpe.
	const TCHAR* Atributos[] = {
		TEXT("musculature"), TEXT("personality"),
		TEXT("camouflage"), TEXT("flight"), TEXT("underground"),
	};
	constexpr int32 Quantos = UE_ARRAY_COUNT(Atributos);

	for (int32 Indice = 0; Indice < Quantos; ++Indice)
	{
		const float Angulo = (2.0f * PI * static_cast<float>(Indice)) / static_cast<float>(Quantos);
		const FVector Onde = Centro + FVector(
			FMath::Cos(Angulo) * TrainingFieldRingRadiusUnits,
			FMath::Sin(Angulo) * TrainingFieldRingRadiusUnits,
			0.0f);

		FActorSpawnParameters Parametros;
		Parametros.ObjectFlags |= RF_Transient;

		AWorldTrainingField* Campo = World->SpawnActor<AWorldTrainingField>(
			AWorldTrainingField::StaticClass(), Onde, FRotator::ZeroRotator, Parametros);
		if (Campo)
		{
			Campo->TrainedAttribute = Atributos[Indice];
		}
	}

	FBattleDebugScreen::Show(
		FString::Printf(TEXT("%d campos de treino em volta de você"), Quantos),
		0.0f, FColor::Green, /*Key=*/722);
}

void ABattleSquareGameMode::TickTrainingFields()
{
	UWorld* World = GetWorld();
	if (!World || !bHasCachedOwnedPet)
	{
		return;
	}

	const APawn* Jogador = AcharPawnDoJogador(World);
	if (!Jogador)
	{
		return;
	}

	const AWorldTrainingField* Dentro = nullptr;
	for (TActorIterator<AWorldTrainingField> It(World); It; ++It)
	{
		if (IsValid(*It) && It->IsInside(Jogador->GetActorLocation()))
		{
			Dentro = *It;
			break;
		}
	}

	if (!Dentro)
	{
		// Sair do campo NÃO zera o resto acumulado: o jogador que deu dois
		// passos para fora e voltou não pode perder o que já tinha ganhado, ou
		// ficar parado passaria a render mais que se mover pelo mundo.
		FBattleDebugScreen::Show(TEXT(""), 0.0f, FColor::White, /*Key=*/745);
		FBattleDebugScreen::Show(TEXT(""), 0.0f, FColor::White, /*Key=*/746);
		return;
	}

	// DP-atr-09: o estudo do dono MULTIPLICA, nunca substitui. O treinador
	// especialista faz o MESMO treino render mais; ele nunca treina no lugar
	// do pet.
	const bool bEspecialista = FTrainerSpecialtyRules::IsSpecialistIn(
		CachedTrainer, Dentro->TrainedAttribute);

	const int32 Pontos = FTrainingFieldRules::PointsForTime(
		WorldStatusRefreshSeconds, bEspecialista, TrainingCarrySeconds);

	if (Pontos > 0)
	{
		TArray<FOwnedPetInstance> Colecao =
			FPetCollectionService::LoadCollection(PetCollectionSlotName);
		FOwnedPetInstance* Instancia = Colecao.FindByPredicate(
			[this](const FOwnedPetInstance& Item)
			{
				return Item.CatalogId == CachedOwnedPet.CatalogId;
			});

		if (Instancia)
		{
			FTrainingFieldRules::ApplyPoints(Dentro->TrainedAttribute, Pontos, *Instancia);
			FPetCollectionService::SaveCollection(PetCollectionSlotName, Colecao);

			// O retrato do painel vem junto: sem reler, o número treinado só
			// apareceria depois da próxima batalha.
			CachedOwnedPet = *Instancia;
		}
	}

	// A BARRA aparece sempre que se está no campo, mesmo sem ponto fechado:
	// treino que só dá sinal a cada seis segundos parece treino que não
	// começou, e o jogador sai antes de ver acontecer.
	const int32 PorCento = FMath::Clamp(
		FMath::RoundToInt((TrainingCarrySeconds / FTrainingFieldRules::SecondsPerPoint) * 100.0f),
		0, 100);

	const FString Atributo =
		FPetMoveRequirements::GetAttributeLabel(Dentro->TrainedAttribute).ToString();

	FBattleDebugScreen::Show(
		FString::Printf(TEXT("treinando %s — %d%% do próximo ponto%s"),
			*Atributo, PorCento,
			bEspecialista ? TEXT(" (especialista: +50%)") : TEXT("")),
		0.0f, FColor::Green, /*Key=*/745);

	// O que se PODE fazer aqui aparece junto do que está acontecendo. Uma
	// especialidade que o jogador só descobre lendo código é uma escolha que
	// ele nunca faz — e a escassez que a torna interessante vira só ausência.
	if (!bEspecialista)
	{
		const int32 Vagas = FTrainerSpecialtyRules::FreeSlots(CachedTrainer);
		FBattleDebugScreen::Show(
			Vagas > 0
				? FString::Printf(
					TEXT("você pode se especializar em %s — %d vaga(s), e a escolha NÃO se desfaz (bs.Especializar)"),
					*Atributo, Vagas)
				: FString::Printf(
					TEXT("sem vagas de especialidade — as suas já estão escolhidas")),
			0.0f, Vagas > 0 ? FColor::Yellow : FColor::Silver, /*Key=*/746);
	}
	else
	{
		FBattleDebugScreen::Show(TEXT(""), 0.0f, FColor::White, /*Key=*/746);
	}
}

void ABattleSquareGameMode::ReloadOwnedPetSnapshot()
{
	const TArray<FOwnedPetInstance> Colecao =
		FPetCollectionService::LoadCollection(PetCollectionSlotName);

	const FOwnedPetInstance* Meu = WorldEncounterPlayerCatalogId.IsEmpty()
		// Sem pet declarado, o primeiro da coleção serve — mesmo critério da
		// montagem de encontro, e pelo mesmo motivo: travar por configuração
		// ausente num nível de teste não ajudaria ninguém.
		? (Colecao.IsEmpty() ? nullptr : &Colecao[0])
		: Colecao.FindByPredicate(
			[this](const FOwnedPetInstance& Item)
			{
				return Item.CatalogId == WorldEncounterPlayerCatalogId;
			});

	bHasCachedOwnedPet = Meu != nullptr;
	if (Meu)
	{
		CachedOwnedPet = *Meu;
	}

	CachedTrainer = FPetCollectionService::LoadTrainerProfile(PetCollectionSlotName);

	// A força do golpe no MUNDO é a musculatura do pet, a mesma que a arena
	// usa. Sem esta linha, treinar musculatura num campo não mudaria nada
	// fora da batalha — e o jogador que treinou não veria diferença no lugar
	// onde ele está.
	if (AWorldExplorerCharacter* Explorador =
		Cast<AWorldExplorerCharacter>(AcharPawnDoJogador(GetWorld())))
	{
		Explorador->StrikeMusculature = bHasCachedOwnedPet ? CachedOwnedPet.Musculature : 0;
	}
}

bool ABattleSquareGameMode::LearnSpecialtyOfCurrentField()
{
	UWorld* World = GetWorld();
	const APawn* Jogador = AcharPawnDoJogador(World);
	if (!Jogador)
	{
		return false;
	}

	for (TActorIterator<AWorldTrainingField> It(World); It; ++It)
	{
		if (!IsValid(*It) || !It->IsInside(Jogador->GetActorLocation()))
		{
			continue;
		}

		if (!FTrainerSpecialtyRules::TryLearn(CachedTrainer, It->TrainedAttribute))
		{
			// A RECUSA diz o motivo. "Nada aconteceu" é indistinguível de
			// defeito, e aqui há três motivos diferentes para nada acontecer.
			FBattleDebugScreen::Show(
				FTrainerSpecialtyRules::IsSpecialistIn(CachedTrainer, It->TrainedAttribute)
					? TEXT("você já é especialista neste atributo")
					: TEXT("sem vagas de especialidade — as suas já estão escolhidas"),
				8.0f, FColor::Orange, /*Key=*/-1);
			return false;
		}

		FPetCollectionService::SaveTrainerProfile(PetCollectionSlotName, CachedTrainer);

		FBattleDebugScreen::Show(
			FString::Printf(TEXT("AGORA VOCÊ É ESPECIALISTA EM %s — treino aqui rende +50%%"),
				*FPetMoveRequirements::GetAttributeLabel(It->TrainedAttribute).ToString()),
			10.0f, FColor::Green, /*Key=*/-1);
		return true;
	}

	FBattleDebugScreen::Show(TEXT("você não está num campo de treino"),
		6.0f, FColor::Orange, /*Key=*/-1);
	return false;
}

void ABattleSquareGameMode::RefreshWorldMap()
{
	UWorld* World = GetWorld();
	const APawn* Jogador = AcharPawnDoJogador(World);
	if (!World || !Jogador || !FWorldMapScreen::IsVisible())
	{
		return;
	}

	FWorldMapSnapshot Retrato;
	Retrato.PlayerXY = FVector2D(Jogador->GetActorLocation());
	Retrato.PlayerYawDegrees = Jogador->GetActorRotation().Yaw;
	Retrato.ShoreRadiusUnits = WorldSceneryCellSizeUnits * RaioDoChaoEmCasas;

	// CAMPOS DE TREINO na cor de cada atributo: são o único destino do mapa,
	// e um mapa sem destino é um radar.
	for (TActorIterator<AWorldTrainingField> It(World); It; ++It)
	{
		if (!IsValid(*It))
		{
			continue;
		}

		FWorldMapMarkerInfo Marcador;
		Marcador.WorldXY = FVector2D(It->GetActorLocation());
		Marcador.Color = AWorldTrainingField::ColorForAttribute(It->TrainedAttribute);
		Marcador.Kind = EWorldMapMarker::CampoDeTreino;
		Marcador.WorldRadiusUnits = It->FieldRadiusUnits;
		Retrato.Markers.Add(Marcador);
	}

	for (TActorIterator<AWorldEncounterActor> It(World); It; ++It)
	{
		if (!IsValid(*It) || It->bIsResolved)
		{
			continue;
		}

		FWorldMapMarkerInfo Marcador;
		Marcador.WorldXY = FVector2D(It->GetActorLocation());
		// Todos da MESMA cor, e não da cor do tipo deles: o mapa diz que há
		// alguém ali, não quem. Saber o tipo de longe tiraria do encontro a
		// única coisa que ele tem de surpresa.
		Marcador.Color = FLinearColor(0.90f, 0.45f, 0.20f);
		Marcador.Kind = EWorldMapMarker::Adversario;
		Marcador.WorldRadiusUnits = 140.0f;
		Retrato.Markers.Add(Marcador);
	}

	FWorldMapScreen::Update(Retrato);
}

void ABattleSquareGameMode::RefreshWorldStatus()
{
	UWorld* World = GetWorld();
	if (!World || !FBattleDebugScreen::IsEnabled())
	{
		return;
	}

	const APawn* Jogador = AcharPawnDoJogador(World);

	FWorldStatusSnapshot Retrato;
	Retrato.bHasOwnedPet = bHasCachedOwnedPet;
	Retrato.OwnedPet = CachedOwnedPet;

	float MenorDistancia = TNumericLimits<float>::Max();
	for (TActorIterator<AWorldEncounterActor> It(World); It; ++It)
	{
		if (!IsValid(*It) || It->bIsResolved)
		{
			continue;
		}

		++Retrato.EncountersAlive;
		if (Jogador)
		{
			MenorDistancia = FMath::Min(MenorDistancia,
				FVector::Dist(Jogador->GetActorLocation(), It->GetActorLocation()));
		}
	}

	Retrato.DistanceToNearestUnits = (Retrato.EncountersAlive > 0 && Jogador)
		? MenorDistancia
		: -1.0f;

	// Chaves FIXAS e consecutivas: a linha se reescreve no lugar em vez de
	// empilhar. Empilhando, o painel encheria sozinho em segundos e engoliria
	// tudo o que a batalha tem a dizer.
	const TArray<FWorldStatusLine> Linhas = FWorldStatusReadout::Build(Retrato);
	for (int32 Indice = 0; Indice < Linhas.Num(); ++Indice)
	{
		FBattleDebugScreen::Show(Linhas[Indice].Text.ToString(), 0.0f,
			Linhas[Indice].Color, /*Key=*/740 + Indice);
	}
}

void ABattleSquareGameMode::SpawnWorldScenery()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Sol primeiro: sem ele a mata nasce azul, e o defeito parece ser da mata.
	if (!ABattleSceneLighting::WorldAlreadyHasSun(World))
	{
		FActorSpawnParameters Parametros;
		Parametros.ObjectFlags |= RF_Transient;
		World->SpawnActor<ABattleSceneLighting>(
			ABattleSceneLighting::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Parametros);
	}

	// Mata já colocada à mão manda — mesmo critério dos encontros: criar por
	// cima duplicaria o que o autor do nível decidiu.
	if (TActorIterator<AForestBackdrop>(World))
	{
		FBattleDebugScreen::Show(TEXT("mundo já tem mata própria — nenhuma foi plantada"),
			8.0f, FColor::Silver, /*Key=*/721);
		return;
	}

	APawn* Jogador = AcharPawnDoJogador(World);
	if (!Jogador)
	{
		return;
	}

	// O chão da mata pousa no chão QUE JÁ EXISTE, achado por traço para baixo
	// a partir do jogador. Chutar Z=0 empilharia dois chãos disputando os
	// mesmos pixels — o mesmo z-fighting que fez a barra de vida piscar — e
	// num nível cujo piso não estivesse em zero a mata ficaria no ar.
	const FVector Origem = Jogador->GetActorLocation();
	FHitResult Toque;
	FCollisionQueryParams Consulta;
	Consulta.AddIgnoredActor(Jogador);

	const bool bAchouChao = World->LineTraceSingleByChannel(Toque, Origem,
		Origem - FVector(0.0f, 0.0f, TracoDeChaoUnidades), ECC_WorldStatic, Consulta);

	// Sem traço, o chão vai nos PÉS do jogador — não no centro dele.
	//
	// `GetActorLocation` devolve o meio da cápsula, e usá-lo punha o topo da
	// terra na altura da cintura: o jogador nascia enterrado até a metade ou,
	// pior, caía por dentro. Meia altura de cápsula é a diferença entre estar
	// em pé no chão e estar dentro dele.
	float AlturaDoChao = Toque.Location.Z;
	if (!bAchouChao)
	{
		float MeiaAltura = 0.0f;
		if (const ACharacter* Personagem = Cast<ACharacter>(Jogador))
		{
			MeiaAltura = Personagem->GetCapsuleComponent()
				? Personagem->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()
				: 0.0f;
		}
		AlturaDoChao = Origem.Z - MeiaAltura;
	}

	FActorSpawnParameters Parametros;
	Parametros.ObjectFlags |= RF_Transient;

	// Um fio ABAIXO do piso encontrado: encostar exato deixaria as duas
	// superfícies coplanares, e é isso que produz o brilho piscando.
	const FVector Onde(Origem.X, Origem.Y,
		AlturaDoChao - AForestBackdrop::GroundTopLocalZ() - FolgaDoChaoUnidades);

	AForestBackdrop* Mata = World->SpawnActor<AForestBackdrop>(
		AForestBackdrop::StaticClass(), Onde, FRotator::ZeroRotator, Parametros);
	if (!Mata)
	{
		return;
	}

	// Sem vazio reservado para câmera: no mundo aberto a câmera segue o
	// jogador, então não existe uma direção fixa a proteger — e reservar uma
	// abriria um buraco sem explicação no meio da floresta.
	Mata->BuildForest(WorldSceneryCellSizeUnits,
		static_cast<uint32>(WorldScenerySeed), FVector2D::ZeroVector);

	// A ÁGUA fecha o mundo. Vem junto da mata porque as duas dependem do mesmo
	// chão: a margem tem de coincidir com a borda da terra, e calcular a
	// margem noutro lugar produziria água por cima da grama ou terra boiando.
	const float RaioDaTerra = WorldSceneryCellSizeUnits * RaioDoChaoEmCasas;

	AWorldBoundaryWater* Agua = World->SpawnActor<AWorldBoundaryWater>(
		AWorldBoundaryWater::StaticClass(), Onde, FRotator::ZeroRotator, Parametros);
	if (Agua)
	{
		Agua->ShoreRadiusUnits = RaioDaTerra;
		Agua->BuildBoundary();
	}

	// A SERRA fecha o horizonte, e nasce do mesmo `Onde` da mata: a arena e o
	// mundo aberto precisam ler como o MESMO lugar, e o fundo é o que mais
	// denuncia dois cenários diferentes.
	AMountainRange* Serra = World->SpawnActor<AMountainRange>(
		AMountainRange::StaticClass(), Onde, FRotator::ZeroRotator, Parametros);
	if (Serra)
	{
		const EScenaryClimate Clima = ScenaryClimate::ConfiguredClimate();
		Serra->BuildRange(Clima, static_cast<uint32>(WorldScenerySeed));

		FBattleDebugScreen::Show(
			FString::Printf(TEXT("serra: %d corpos, %d com gelo (neve acima de %.0f m)"),
				Serra->GetPeakCount(), Serra->GetSnowCapCount(),
				ScenaryClimate::SnowLineMeters(Clima)),
			0.0f, FColor::Cyan, /*Key=*/725);
	}

	FBattleDebugScreen::Show(
		FString::Printf(TEXT("mundo: sol e mata plantados (chão em Z=%.0f%s)"),
			AlturaDoChao, bAchouChao ? TEXT("") : TEXT(", NÃO encontrado por traço")),
		0.0f, FColor::Green, /*Key=*/721);
}

void ABattleSquareGameMode::SpawnRoamingEncounters()
{
	UWorld* World = GetWorld();
	if (!World || WorldEncounterCount <= 0 || WorldEncounterCatalogIds.IsEmpty())
	{
		return;
	}

	// Já há encontros no nível? Então este mundo foi povoado à mão, e criar
	// mais por cima duplicaria o que o autor decidiu.
	TActorIterator<AWorldEncounterActor> Existente(World);
	if (Existente)
	{
		FBattleDebugScreen::Show(
			TEXT("mundo já tem encontros colocados à mão — nenhum foi criado"),
			8.0f, FColor::Silver, /*Key=*/720);
		return;
	}

	const APawn* Jogador = AcharPawnDoJogador(World);
	const FVector Centro = Jogador ? Jogador->GetActorLocation() : FVector::ZeroVector;

	FRandomStream Sorteio(WorldEncounterSeed);

	for (int32 Indice = 0; Indice < WorldEncounterCount; ++Indice)
	{
		SpawnOneEncounter(Centro, Sorteio, WorldEncounterSeed + Indice * 7919);
	}

	FBattleDebugScreen::Show(
		FString::Printf(TEXT("%d encontros povoaram o mundo, e eles ANDAM"), WorldEncounterCount),
		0.0f, FColor::Green, /*Key=*/720);

	// A reposição confere de tempos em tempos, e não a cada quadro: a
	// população muda por batalha, não por frame.
	if (EncounterPopulationCheckSeconds > 0.0f)
	{
		World->GetTimerManager().SetTimer(EncounterPopulationTimer, this,
			&ABattleSquareGameMode::MaintainEncounterPopulation,
			EncounterPopulationCheckSeconds, /*bLoop=*/true);
	}
}

void ABattleSquareGameMode::SpawnOneEncounter(const FVector& Centro, FRandomStream& Sorteio, int32 SementeDoPasseio)
{
	UWorld* World = GetWorld();
	if (!World || WorldEncounterCatalogIds.IsEmpty())
	{
		return;
	}

	// Longe o bastante para não disparar batalha no primeiro passo: nascer
	// dentro do raio de encontro tiraria o jogador do mundo antes de ele andar
	// — e, na reposição, o tiraria de volta assim que ele voltasse da batalha.
	const float Angulo = Sorteio.FRandRange(0.0f, 2.0f * PI);
	const float Distancia = FMath::Lerp(WorldEncounterSpawnRadiusUnits * 0.25f,
		WorldEncounterSpawnRadiusUnits, Sorteio.FRand());

	const FVector Posicao = Centro
		+ FVector(FMath::Cos(Angulo) * Distancia, FMath::Sin(Angulo) * Distancia, 0.0f);

	AWorldEncounterActor* Encontro = World->SpawnActor<AWorldEncounterActor>(
		AWorldEncounterActor::StaticClass(), Posicao, FRotator::ZeroRotator);
	if (!Encontro)
	{
		return;
	}

	const int32 CatalogoIndice = Sorteio.RandRange(0, WorldEncounterCatalogIds.Num() - 1);
	Encontro->CatalogId = FName(*WorldEncounterCatalogIds[CatalogoIndice]);

	UEncounterRoamingComponent* Passeio = NewObject<UEncounterRoamingComponent>(Encontro);
	Passeio->RegisterComponent();

	// Semente própria: dois encontros criados no mesmo quadro precisam andar
	// diferente, senão passeiam colados.
	Passeio->ConfigureRoaming(Posicao, SementeDoPasseio);
}

void ABattleSquareGameMode::MaintainEncounterPopulation()
{
	UWorld* World = GetWorld();
	if (!World || WorldEncounterCount <= 0 || WorldEncounterCatalogIds.IsEmpty())
	{
		return;
	}

	// Tira de cena os derrotados. Deixá-los passeando como fantasma é pior que
	// sumir: dá a entender que a batalha não valeu de nada.
	TArray<AWorldEncounterActor*> Derrotados;
	int32 Vivos = 0;

	for (TActorIterator<AWorldEncounterActor> It(World); It; ++It)
	{
		if (It->bIsResolved)
		{
			Derrotados.Add(*It);
		}
		else
		{
			++Vivos;
		}
	}

	for (AWorldEncounterActor* Derrotado : Derrotados)
	{
		Derrotado->Destroy();
	}

	const int32 Faltam = WorldEncounterCount - Vivos;
	if (Faltam <= 0)
	{
		return;
	}

	const APawn* Jogador = AcharPawnDoJogador(World);
	const FVector Centro = Jogador ? Jogador->GetActorLocation() : FVector::ZeroVector;

	// Semente avança a cada reposição: repor sempre com a mesma faria os
	// substitutos nascerem no mesmo lugar, e o mundo viraria um carrossel.
	for (int32 Indice = 0; Indice < Faltam; ++Indice)
	{
		++EncounterRefillCounter;
		FRandomStream Sorteio(WorldEncounterSeed + EncounterRefillCounter * 104729);
		SpawnOneEncounter(Centro, Sorteio, WorldEncounterSeed + EncounterRefillCounter * 7919);
	}

	FBattleDebugScreen::Show(
		FString::Printf(TEXT("%d encontro(s) reposto(s) — o mundo não acaba"), Faltam),
		8.0f, FColor::Green, /*Key=*/721);
}

// A especialidade é DELIBERADA: nada nela acontece por estar parado no lugar.
// Fora do Shipping por compilação, como o resto das ferramentas de
// desenvolvimento — quando houver barra no mundo, o botão substitui isto.
#if !UE_BUILD_SHIPPING
namespace
{
	FAutoConsoleCommandWithWorldAndArgs GEspecializarCommand(
		TEXT("bs.Especializar"),
		TEXT("Vira especialista no atributo do campo de treino em que você está. NÃO se desfaz."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(
			[](const TArray<FString>&, UWorld* World)
			{
				if (ABattleSquareGameMode* GameMode =
					World ? World->GetAuthGameMode<ABattleSquareGameMode>() : nullptr)
				{
					GameMode->LearnSpecialtyOfCurrentField();
				}
			}));
}
#endif // !UE_BUILD_SHIPPING

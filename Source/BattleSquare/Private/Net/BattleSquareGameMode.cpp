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

}

namespace
{
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

	SpawnRoamingEncounters();
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
		const FString Problem = SetUpWorldEncounterFlow();
		if (Problem.IsEmpty())
		{
			UE_LOG(LogTemp, Display, TEXT("ABattleSquareGameMode: encontros de mundo ATIVOS."));
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

	APawn* Jogador = World->GetFirstPlayerController()
		? World->GetFirstPlayerController()->GetPawn() : nullptr;
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

	APawn* Jogador = World->GetFirstPlayerController()
		? World->GetFirstPlayerController()->GetPawn() : nullptr;
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

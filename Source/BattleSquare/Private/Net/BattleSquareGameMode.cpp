// Copyright 2026 Anderson. All Rights Reserved.

#include "Net/BattleSquareGameMode.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Balance/TypeEffectivenessTable.h"
#include "World/EncounterRoamingComponent.h"
#include "World/WorldEncounterActor.h"
#include "Blueprint/UserWidget.h"
#include "Debug/BattleDebugToolbar.h"
#include "Debug/BattleDebugScreen.h"
#include "Battle/BattleActionQueueComponent.h"
#include "Battle/BattleTypes.h"
#include "UI/BattleActionSelectorWidget.h"
#include "Net/BattleSquarePlayerController.h"
#include "Debug/BattleDebugHUD.h"
#include "Data/PetDataLoader.h"
#include "Meta/PetCollectionService.h"
#include "Meta/PetProgressionService.h"
#include "Meta/PetAttributeProgression.h"
#include "World/WorldEncounterFlow.h"
#include "World/WorldExplorerCharacter.h"
#include "Environment/AuroraCurtain.h"
#include "Environment/CaveSystem.h"
#include "Environment/ForestBackdrop.h"
#include "Environment/FreshWater.h"
#include "Environment/IslandFeatureLayout.h"
#include "Environment/IslandGeography.h"
#include "Environment/MountainRange.h"
#include "Environment/RegionResidency.h"
#include "Environment/ScenaryClimate.h"
#include "Environment/SceneLighting.h"
#include "Environment/WorldTimeOfDay.h"
#include "Environment/WorldNightSky.h"
#include "Environment/WorldWeather.h"
#include "World/WorldStatusReadout.h"
#include "UI/WorldLoadingScreen.h"
#include "UI/WorldMapScreen.h"
#include "Environment/Volcano.h"
#include "Environment/WalkableMountain.h"
#include "Environment/WorldBoundaryWater.h"
#include "Environment/WorldEvents.h"
#include "World/WorldTrainingField.h"
#include "World/Village.h"
#include "World/RegionLayout.h"
#include "Battle/PetView.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Character.h"
#include "Engine/StaticMeshActor.h"
#include "World/IslandBakedPlan.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "World/AqueductMesh.h"
#include "World/GroundUseActor.h"
#include "World/CrossingMesh.h"
#include "World/RiverMesh.h"
#include "World/WaterFooting.h"
#include "World/TerrainMesh.h"
#include "World/TrailMesh.h"
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
	}
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

	/**
	 * O clima do LUGAR onde o jogador está.
	 *
	 * A ilha tem cinco setores de bioma e vinte mil unidades de raio: um clima
	 * lido do `.ini` para o mundo inteiro faria o deserto chover e o glaciar
	 * suar. Quem responde é a geografia, pela posição.
	 *
	 * Sem pawn a resposta é a da origem — que é a casa, mata temperada, o
	 * mesmo que o `.ini` dizia antes. O padrão não muda; só deixa de ser o
	 * único.
	 */
	FVector2D OndeOJogadorEsta(UWorld* World)
	{
		const APawn* Jogador = AcharPawnDoJogador(World);
		return Jogador ? FVector2D(Jogador->GetActorLocation()) : FVector2D::ZeroVector;
	}

	EScenaryClimate ClimaOndeOJogadorEsta(UWorld* World)
	{
		return IslandGeography::ClimateAt(OndeOJogadorEsta(World));
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
	SpawnStartingVillage();
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

		// A DESCOBERTA também: uma região tem 800 unidades e ninguém a
		// atravessa entre dois quadros. O que ela carrega vem do save, e é o
		// que faz o mapa lembrar de uma sessão para a outra.
		WorldDiscovery = FPetCollectionService::LoadDiscovery(PetCollectionSlotName);
		BuildWorldTerrainTiles();
		MapPins = FPetCollectionService::LoadMapPins(PetCollectionSlotName);
		World->GetTimerManager().SetTimer(DiscoveryTimer, this,
			&ABattleSquareGameMode::RefreshWorldDiscovery,
			WorldStatusRefreshSeconds, /*bLoop=*/true);

		// A RESIDÊNCIA no mesmo passo: um pedaço tem 6400 unidades, oito
		// vezes a região da descoberta, então quem não atravessa uma região
		// em meio segundo muito menos atravessa um pedaço.
		World->GetTimerManager().SetTimer(ResidencyTimer, this,
			&ABattleSquareGameMode::RefreshRegionResidency,
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

	TickWorldClock();

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
	//
	// O TEMPO entra junto: chover encharca o campo, e sem isto o número saía
	// do clima e ignorava o céu que o jogador acabou de atravessar. Sem cena
	// de mundo (sala aberta direto, teste) o céu é limpo, que é o que sempre
	// foi.
	InitialState.Humidity = static_cast<uint8>(FMath::Clamp(
		WorldWeather::HumidityPercent(
			ClimaOndeOJogadorEsta(GetWorld()),
			CenaDoMundo ? CenaDoMundo->GetWeather() : EWeather::Clear), 0, 100));


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

void ABattleSquareGameMode::SpawnStartingVillage()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Assentamento posto à mão manda — mesmo critério dos encontros, da mata e
	// dos campos de treino. Quem colocou um no editor quis aquele.
	if (TActorIterator<AVillage>(World))
	{
		return;
	}

	FActorSpawnParameters Parametros;
	Parametros.ObjectFlags |= RF_Transient;

	int32 Erguidos = 0;
	int32 Predios = 0;

	// A REGIÃO inteira, e não só a vila de casa. Uma vila só faz o resto do
	// mapa ser paisagem: sem academia paga, sem mercado e sem a arena da
	// região, andar 700 metros não leva a lugar nenhum — e este projeto já
	// pagou por regra completa que ninguém alcançava (L-041).
	for (const FSettlementPlacement& Assentamento : RegionLayout::Plan())
	{
		const FVector Onde(Assentamento.CenterUnits.X, Assentamento.CenterUnits.Y, 0.0f);

		AVillage* Vila = World->SpawnActor<AVillage>(AVillage::StaticClass(),
			Onde, FRotator::ZeroRotator, Parametros);
		if (!Vila)
		{
			continue;
		}

		// O tipo ANTES de erguer: ele decide o traçado, e mudá-lo depois não
		// move prédio nenhum.
		Vila->SetSettlementKind(Assentamento.Kind);
		Vila->BuildVillage();

		++Erguidos;
		Predios += Vila->GetBuiltCount();
	}

	FBattleDebugScreen::Show(
		FString::Printf(TEXT("regiao: %d assentamentos, %d predios de pe"), Erguidos, Predios),
		0.0f, FColor(200, 180, 120), /*Key=*/744);
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
	// deixaria as clareiras boiando fora da mata.
	//
	// A altura é a dos PÉS do pawn, não a do centro dele. A origem do ator é a
	// base do marco, e no centro da cápsula o marco nasceria uma meia-altura
	// acima do chão — pedra flutuando, que é o mesmo defeito de aparência que
	// esta mudança veio consertar.
	const APawn* Jogador = AcharPawnDoJogador(World);
	FVector Centro = Jogador ? Jogador->GetActorLocation() : FVector::ZeroVector;
	if (Jogador)
	{
		Centro.Z -= Jogador->GetSimpleCollisionHalfHeight();
	}

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

			// REMONTA depois de atribuir. `BeginPlay` já correu dentro do
			// `SpawnActor`, com o atributo ainda no padrão — sem esta linha os
			// cinco campos nasciam com o marco e a cor da MUSCULATURA, cinco
			// clareiras idênticas dizendo que treinavam coisas diferentes.
			Campo->RebuildMarker();
		}
	}

	FBattleDebugScreen::Show(
		FString::Printf(TEXT("%d campos de treino em volta de você"), Quantos),
		0.0f, FColor::Green, /*Key=*/722);
}

void ABattleSquareGameMode::TickWorldClock()
{
	if (!CenaDoMundo || !CenaDoMundo->IsDayCycleRunning())
	{
		return;
	}

	const float Hora = CenaDoMundo->GetHour();
	const EDayPhase Fase = WorldTimeOfDay::PhaseAtHour(Hora);

	// A cor da linha é a cor do céu: quem passa o olho pelo painel lê a hora
	// antes de ler o número, do mesmo jeito que lê a hora pela luz na tela.
	FColor CorDaFase = FColor::White;
	switch (Fase)
	{
	case EDayPhase::Dawn:  CorDaFase = FColor::Orange; break;
	case EDayPhase::Day:   CorDaFase = FColor::Yellow; break;
	case EDayPhase::Dusk:  CorDaFase = FColor::Orange; break;
	case EDayPhase::Night: CorDaFase = FColor::Cyan;   break;
	}

	const int32 Horas = FMath::FloorToInt(Hora);
	const int32 Minutos = FMath::FloorToInt((Hora - Horas) * 60.0f);

	FBattleDebugScreen::Show(
		FString::Printf(TEXT("%02d:%02d — %s (sol %.0f°)"),
			Horas, Minutos,
			WorldTimeOfDay::PhaseDebugName(Fase),
			WorldTimeOfDay::SunElevationDegrees(Hora)),
		0.0f, CorDaFase, /*Key=*/750);

	MostrarTempoDoMundo();
	MostrarCeuDoMundo();
	MostrarEventosDoMundo();
}

void ABattleSquareGameMode::MostrarTempoDoMundo()
{
	// O tempo é função da semente do mundo, do clima do lugar e das horas
	// corridas. Quem sorteia é esta conta, uma só: o ator de luz recebe o
	// resultado, e o painel escreve o MESMO resultado (L-032).
	const EScenaryClimate Clima = ClimaOndeOJogadorEsta(GetWorld());

	const EWeather Tempo = WorldWeather::WeatherAt(
		static_cast<uint32>(WorldScenerySeed),
		Clima,
		CenaDoMundo->GetElapsedHours());

	CenaDoMundo->SetWeather(Tempo);

	// A cor sobe de tom com a severidade, porque a linha é lida de relance: o
	// grau da chuva precisa saltar antes da palavra ser lida.
	FColor CorDoTempo = FColor::Yellow;
	switch (Tempo)
	{
	case EWeather::Storm:    CorDoTempo = FColor::Magenta; break;
	case EWeather::Downpour: CorDoTempo = FColor::Cyan;    break;
	case EWeather::Rain:     CorDoTempo = FColor::Blue;    break;
	case EWeather::Drizzle:  CorDoTempo = FColor::Turquoise; break;
	case EWeather::Overcast: CorDoTempo = FColor::Silver;  break;
	case EWeather::Cloudy:   CorDoTempo = FColor::White;   break;
	case EWeather::Clear:    CorDoTempo = FColor::Yellow;  break;
	}

	// A umidade entra na linha porque é ela que atravessa para a batalha: ver
	// o número subir na chuva é ver, antes de lutar, que o campo vai estar
	// enlameado.
	const int32 Umidade = WorldWeather::HumidityPercent(Clima, Tempo);

	// Duas marcas e não uma escolha entre elas: numa tempestade o campo vem
	// alagado E enlameado, e mostrar só a mais chamativa esconderia metade do
	// que vai mudar no tabuleiro.
	FString Marca;
	if (WorldWeather::IsFlooding(Tempo))
	{
		Marca += TEXT(" (ALAGADO)");
	}
	if (Umidade >= MudMinHumidity)
	{
		Marca += TEXT(" (campo de LAMA)");
	}

	FBattleDebugScreen::Show(
		FString::Printf(TEXT("tempo: %s em %s — umidade %d%%%s"),
			WorldWeather::WeatherDebugName(Tempo),
			IslandGeography::BiomeDebugName(
				IslandGeography::BiomeAt(OndeOJogadorEsta(GetWorld()))),
			Umidade,
			*Marca),
		0.0f, CorDoTempo, /*Key=*/751);
}

void ABattleSquareGameMode::MostrarCeuDoMundo()
{
	const float Hora = CenaDoMundo->GetHour();
	const float Corridas = CenaDoMundo->GetElapsedHours();

	const float Fase = WorldNightSky::MoonPhaseFraction(Corridas);
	const ESkyEclipse Eclipse = WorldNightSky::EclipseAt(Hora, Corridas);

	// A cor grita o que é raro: eclipse lunar em vermelho porque a lua FICA
	// vermelha, solar em laranja porque é o sol que apaga. Céu comum fica
	// discreto — linha que chama atenção todo dia não chama atenção nenhuma.
	FColor CorDoCeu = FColor(150, 150, 170);
	FString Linha;

	if (Eclipse == ESkyEclipse::None)
	{
		Linha = FString::Printf(TEXT("céu: lua %s (%.0f%% acesa)"),
			WorldNightSky::PhaseDebugName(WorldNightSky::PhaseOf(Fase)),
			WorldNightSky::MoonLitFraction(Fase) * 100.0f);
	}
	else
	{
		CorDoCeu = Eclipse == ESkyEclipse::Lunar ? FColor::Red : FColor::Orange;
		Linha = FString::Printf(TEXT("céu: %s — %.0f%% de profundidade"),
			WorldNightSky::EclipseDebugName(Eclipse),
			WorldNightSky::EclipseDepth(Hora, Corridas) * 100.0f);
	}

	// Estrela e cometa entram na MESMA linha porque são a mesma pergunta: o
	// que se vê olhando para cima agora.
	const float Estrelas = WorldNightSky::StarBrightness(Hora, CenaDoMundo->GetWeather());
	if (Estrelas > 0.05f)
	{
		Linha += FString::Printf(TEXT(" | estrelas %.0f%%"), Estrelas * 100.0f);
	}

	if (WorldNightSky::CometVisible(static_cast<uint32>(WorldScenerySeed), Corridas))
	{
		Linha += FString::Printf(TEXT(" | COMETA a %.0f° de altura"),
			WorldNightSky::CometElevationDegrees(static_cast<uint32>(WorldScenerySeed), Corridas));
	}

	const float Aurora = WorldNightSky::AuroraStrength(ClimaOndeOJogadorEsta(GetWorld()), Hora);
	if (Aurora > 0.05f)
	{
		CorDoCeu = FColor::Green;
		Linha += TEXT(" | AURORA BOREAL");
	}

	// A CORTINA obedece ao clima da GELEIRA, e a linha acima ao clima de quem
	// anda: são duas perguntas diferentes — "está acima de mim?" e "está acima
	// da geleira?" — e as duas passam pela mesma `AuroraStrength`, que continua
	// sendo a única a decidir quando há aurora (L-032).
	//
	// Sem isto a cortina só acenderia para quem já estivesse no gelo, e o
	// jogador nunca veria de longe o que o faria ir até lá.
	if (AuroraDoCeu)
	{
		AuroraDoCeu->SetStrength(WorldNightSky::AuroraStrength(
			IslandGeography::ClimateOf(EIslandBiome::Glacier), Hora));
	}

	FBattleDebugScreen::Show(Linha, 0.0f, CorDoCeu, /*Key=*/761);
}

void ABattleSquareGameMode::MostrarEventosDoMundo()
{
	const FVector2D Aqui = OndeOJogadorEsta(GetWorld());
	const float Corridas = CenaDoMundo->GetElapsedHours();
	const uint32 Semente = static_cast<uint32>(WorldScenerySeed);

	// O mar sobe mesmo que o jogador esteja no miolo da ilha: a onda existe na
	// costa toda, e amarrá-la a quem está olhando faria o mar descer sozinho
	// quando alguém andasse para dentro.
	const float SubiuAOnda = WorldEvents::TsunamiRiseUnits(Semente, Corridas);
	if (AguaDoMundo)
	{
		FVector OndeOMarEsta = AguaDoMundo->GetActorLocation();
		OndeOMarEsta.Z = AguaEmRepousoZ + SubiuAOnda;
		AguaDoMundo->SetActorLocation(OndeOMarEsta);
	}

	const EWorldEvent Evento = WorldEvents::EventAt(Semente, Aqui, Corridas);
	if (Evento == EWorldEvent::None)
	{
		// Linha vazia e não "calmo": o painel tem doze linhas, e uma delas
		// dizendo "nada" todo o tempo custa o espaço de uma que diz algo.
		FBattleDebugScreen::Show(TEXT(""), 0.0f, FColor::White, /*Key=*/763);
		return;
	}

	// A cor sobe com a severidade, como na linha do tempo: o evento é lido de
	// relance, e quem está fugindo de um tsunami não vai ler a palavra.
	FColor CorDoEvento = FColor::Yellow;
	switch (Evento)
	{
	case EWorldEvent::Tsunami:    CorDoEvento = FColor::Magenta; break;
	case EWorldEvent::Hurricane:  CorDoEvento = FColor::Cyan;    break;
	case EWorldEvent::Earthquake: CorDoEvento = FColor::Red;     break;
	default: break;
	}

	FString Linha = FString::Printf(TEXT("%s %.0f%%"),
		WorldEvents::EventDebugName(Evento),
		WorldEvents::EventStrength(Semente, Aqui, Corridas) * 100.0f);

	if (Evento == EWorldEvent::Earthquake)
	{
		// A distância ao epicentro explica a força que a linha acabou de dizer;
		// sem ela, "TERREMOTO 30%" parece um tremor fraco em vez de um forte
		// sentido de longe.
		Linha += FString::Printf(TEXT(" | epicentro a %.0fm"),
			FVector2D::Distance(Aqui, WorldEvents::EarthquakeEpicenterUnits(Semente, Corridas)) * 0.01f);
	}
	else if (Evento == EWorldEvent::Tsunami)
	{
		Linha += FString::Printf(TEXT(" | o mar subiu %.0fm"), SubiuAOnda * 0.01f);
	}

	FBattleDebugScreen::Show(Linha, 0.0f, CorDoEvento, /*Key=*/763);
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

void ABattleSquareGameMode::ToggleMapPinHere(EWorldPinKind Kind)
{
	const APawn* Jogador = AcharPawnDoJogador(GetWorld());
	if (!Jogador)
	{
		return;
	}

	const FVector2D Onde(Jogador->GetActorLocation());
	const FWorldMapPins::EResult Resultado = MapPins.ToggleAt(Onde, Kind);

	// TODA marcação vira linha na tela, inclusive a recusada. Gesto que não
	// responde parece gesto que não chegou, e foi assim que três tentativas de
	// usar a tecla de copiar viraram três rodadas perdidas.
	switch (Resultado)
	{
	case FWorldMapPins::EResult::Posta:
		FBattleDebugScreen::Show(
			FString::Printf(TEXT("marcado aqui (%d de %d)"),
				MapPins.Pins.Num(), FWorldMapPins::MaxPins),
			4.0f, FColor(255, 220, 120), /*Key=*/-1);
		break;

	case FWorldMapPins::EResult::Apagada:
		FBattleDebugScreen::Show(TEXT("marcação apagada"),
			4.0f, FColor(255, 220, 120), /*Key=*/-1);
		break;

	case FWorldMapPins::EResult::Cheio:
		// RECUSA com motivo, e sem trocar a mais antiga: perder uma marcação
		// que se pôs de propósito, em silêncio, é pior que não marcar.
		FBattleDebugScreen::Show(
			FString::Printf(TEXT("mapa cheio: %d marcações. Apague uma marcando em cima dela."),
				FWorldMapPins::MaxPins),
			6.0f, FColor::Orange, /*Key=*/-1);
		return;
	}

	FPetCollectionService::SaveMapPins(PetCollectionSlotName, MapPins);
}

void ABattleSquareGameMode::BuildWorldTerrainTiles()
{
	// A GEOGRAFIA responde, não o que está plantado.
	//
	// Antes o mapa deduzia mata CONTANDO troncos no mundo. Com a ilha montada
	// por pedaços residentes, só os nove à volta do jogador existem — o mapa
	// esvaziaria e repovoaria atrás de quem anda, e diria que o deserto do
	// outro lado da ilha é clareira porque ninguém está lá.
	//
	// `IslandGeography` sabe o que cada ponto é sem que nada esteja montado, e
	// é a mesma fonte que decide o bioma de cada pedaço plantado. Uma verdade
	// só (L-032): o mapa não pode discordar do chão.
	WorldTerrainTiles.Reset();

	const float RaioDaTerra = IslandGeography::LandRadiusUnits();
	const float Lado = FWorldMapProjection::TerrainTileSideUnits(RaioDaTerra);
	const int32 Alcance = FWorldMapProjection::TerrainTilesAcross / 2;

	for (int32 Coluna = -Alcance; Coluna <= Alcance; ++Coluna)
	{
		for (int32 Linha = -Alcance; Linha <= Alcance; ++Linha)
		{
			FWorldMapTerrainTile Pedaco;
			Pedaco.WorldXY = FVector2D((Coluna + 0.5f) * Lado, (Linha + 0.5f) * Lado);

			// A ORDEM é a regra: a água vence o bioma. Um pedaço é uma coisa
			// só, e sem ordem declarada quem vence é a ordem em que os `if`
			// foram escritos — que ninguém revisa.
			Pedaco.Kind = IslandGeography::IsOnLand(Pedaco.WorldXY)
				? FWorldMapProjection::TerrainForBiome(
					IslandGeography::BiomeAt(Pedaco.WorldXY))
				: EWorldMapTerrain::Agua;

			WorldTerrainTiles.Add(Pedaco);
		}
	}

	FBattleDebugScreen::Show(
		FString::Printf(TEXT("mapa: %d pedaços de terreno"), WorldTerrainTiles.Num()),
		0.0f, FColor(150, 200, 255), /*Key=*/743);
}

void ABattleSquareGameMode::RefreshWorldDiscovery()
{
	const APawn* Jogador = AcharPawnDoJogador(GetWorld());
	if (!Jogador)
	{
		return;
	}

	const int32 Novas = WorldDiscovery.MarkSeenFrom(
		FVector2D(Jogador->GetActorLocation()));
	if (Novas == 0)
	{
		return;
	}

	FPetCollectionService::SaveDiscovery(PetCollectionSlotName, WorldDiscovery);

	FBattleDebugScreen::Show(
		FString::Printf(TEXT("mapa: %d regiões conhecidas"),
			WorldDiscovery.DiscoveredCount()),
		0.0f, FColor(150, 200, 255), /*Key=*/742);
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
	Retrato.ShoreRadiusUnits = IslandGeography::LandRadiusUnits();
	Retrato.Discovery = WorldDiscovery;
	Retrato.Terrain = WorldTerrainTiles;
	Retrato.Pins = MapPins;

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
		ABattleSceneLighting* Luz = World->SpawnActor<ABattleSceneLighting>(
			ABattleSceneLighting::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Parametros);

		// No MUNDO o dia corre. Na arena não — ver `StartDayCycle`.
		if (Luz)
		{
			Luz->SetSecondsPerDay(WorldSecondsPerDay);
			Luz->StartDayCycle(WorldStartHour);
			CenaDoMundo = Luz;
		}
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

	// A mata deixa de ser UMA e passa a ser os pedaços à volta de quem anda.
	//
	// Um disco só cobrindo os 20000 de raio teria vinte vezes a área de hoje
	// viva ao mesmo tempo, e o pedido foi explícito: "não podemos deixar ficar
	// devagar, devemos recarregar por mapa". Nove pedaços de 6400 acompanham
	// o jogador; o resto da ilha não existe enquanto ninguém está lá.
	//
	// Sem vazio reservado para câmera: no mundo aberto a câmera segue o
	// jogador, então não existe uma direção fixa a proteger — e reservar uma
	// abriria um buraco sem explicação no meio da floresta.
	const float RaioDaTerra = IslandGeography::LandRadiusUnits();
	WorldGroundZ = Onde.Z;
	RefreshRegionResidency();

	// O RELEVO vem antes de tudo o que se apoia nele. Sem terreno o rio flutua,
	// a trilha não sobe nada e o barranco não barra ninguém — e até aqui a
	// altura calculada nunca virava geometria.
	ATerrainMesh* Relevo = World->SpawnActor<ATerrainMesh>(
		ATerrainMesh::StaticClass(), Onde, FRotator::ZeroRotator, Parametros);
	if (Relevo)
	{
		// Pela porta COM guarda: assado de outra configuração faria a ilha
		// subir inteira, parecendo certa, sendo de um mundo que já não existe.
		if (const UIslandBakedPlan* Assado = IslandBakedPlan::LoadForWorld())
		{
			Relevo->BuildFrom(*Assado);
			RelevoDoMundo = Relevo;
			TracadoAssado = Assado;

			// A ÁGUA CORRENTE vem logo depois do relevo, e nunca antes: a
			// lâmina se assenta na altura do chão, e sem chão ela sairia toda
			// no zero — a bacia inteira boiando num plano.
			ARiverMesh* Rios = World->SpawnActor<ARiverMesh>(
				ARiverMesh::StaticClass(), Onde, FRotator::ZeroRotator, Parametros);
			if (Rios)
			{
				Rios->BuildFrom(*Assado);
			}

			// AS TRILHAS por último entre as três: elas atravessam a água (56
			// travessias), e a ponte só faz sentido com o rio já no lugar.
			ATrailMesh* Trilhas = World->SpawnActor<ATrailMesh>(
				ATrailMesh::StaticClass(), Onde, FRotator::ZeroRotator, Parametros);
			if (Trilhas)
			{
				Trilhas->BuildFrom(*Assado);
			}

			// AS TRAVESSIAS por último: elas são a obra onde a trilha encontra
			// a água, e precisam das duas já no lugar.
			ACrossingMesh* Travessias = World->SpawnActor<ACrossingMesh>(
				ACrossingMesh::StaticClass(), Onde, FRotator::ZeroRotator, Parametros);
			if (Travessias)
			{
				Travessias->BuildFrom(*Assado);
			}

			// OS AQUEDUTOS: a obra que leva agua a vila sem agua perto.
			AAqueductMesh* Aquedutos = World->SpawnActor<AAqueductMesh>(
				AAqueductMesh::StaticClass(), Onde, FRotator::ZeroRotator, Parametros);
			if (Aquedutos)
			{
				Aquedutos->BuildFrom(*Assado);
			}

			// O USO DO SOLO por ultimo: e o que enche o mundo de MOTIVO para
			// andar, e ele se apoia em tudo o que veio antes.
			UsosDoSoloConstruidos = ConstruirUsosDoSolo(*Assado, Parametros);
		}
	}

	// A ÁGUA fecha o mundo. Vem junto da mata porque as duas dependem do mesmo
	// chão: a margem tem de coincidir com a borda da terra, e calcular a
	// margem noutro lugar produziria água por cima da grama ou terra boiando.
	AWorldBoundaryWater* Agua = World->SpawnActor<AWorldBoundaryWater>(
		AWorldBoundaryWater::StaticClass(), Onde, FRotator::ZeroRotator, Parametros);
	if (Agua)
	{
		Agua->ShoreRadiusUnits = RaioDaTerra;
		Agua->BuildBoundary();
		AguaDoMundo = Agua;
		AguaEmRepousoZ = static_cast<float>(Agua->GetActorLocation().Z);
	}

	// A SERRA fecha o horizonte, e nasce do mesmo `Onde` da mata: a arena e o
	// mundo aberto precisam ler como o MESMO lugar, e o fundo é o que mais
	// denuncia dois cenários diferentes.
	AMountainRange* Serra = World->SpawnActor<AMountainRange>(
		AMountainRange::StaticClass(), Onde, FRotator::ZeroRotator, Parametros);
	if (Serra)
	{
		Serra->BuildRangeAcrossIsland(static_cast<uint32>(WorldScenerySeed));

		// Sem "neve acima de X m": a serra atravessa os setores, e cada pico
		// tem a sua linha. Um número só aqui seria o de um clima que já não
		// existe, e mentira na tela é pior que silêncio.
		FBattleDebugScreen::Show(
			FString::Printf(TEXT("serra: %d corpos, %d com gelo (linha da neve por setor)"),
				Serra->GetPeakCount(), Serra->GetSnowCapCount()),
			0.0f, FColor::Cyan, /*Key=*/725);
	}

	// Montanhas que se sobem e cavernas que se percorrem, no anel entre os
	// campos de treino e a beira da água. Onde cada uma cai é decisão de
	// `IslandFeatureLayout`, que já foi conferida sem abrir o jogo: quina de
	// caverna dentro de montanha é defeito que só se acha indo até lá a pé.
	int32 MontanhasPlantadas = 0;
	int32 CavernasPlantadas = 0;
	int32 MaiorCavernaPlantada = 0;
	int32 CavernasDeLava = 0;
	int32 CavernasDeAgua = 0;
	int32 CavernasFechadas = 0;
	int32 PatamaresDaTrilhaMaisLonga = 0;
	int32 VulcoesPlantados = 0;
	int32 DerramesDoVulcao = 0;

	// As grutas das cachoeiras entram na MESMA lista, porque são cavernas: quem
	// planta não precisa saber que a posição de uma veio do rio e a da outra do
	// anel. Um segundo laço só para elas duplicaria o despacho e a contagem.
	TArray<IslandFeatureLayout::FFeaturePlacement> PecasDaIlha = IslandFeatureLayout::Plan();
	const TArray<IslandFeatureLayout::FFeaturePlacement> Grutas = FreshWater::PlanGrottoes();
	const int32 GrutasDeCachoeira = Grutas.Num();
	PecasDaIlha.Append(Grutas);

	for (const IslandFeatureLayout::FFeaturePlacement& Peca : PecasDaIlha)
	{
		const FVector2D Deslocamento = Peca.CenterUnits();
		const FVector OndeNaIlha(Origem.X + Deslocamento.X, Origem.Y + Deslocamento.Y, AlturaDoChao);

		// A conta mora no TRAÇADO, não aqui: quem desenha o mapa de uma caverna
		// precisa da mesma semente, e ela não pode existir só onde o ator nasce.
		const uint32 SementeDaPeca =
			IslandFeatureLayout::SeedForPlacement(WorldScenerySeed, Peca);

		// `switch` sem `default`, e é de propósito: quando a montanha era o
		// caso especial e TODO o resto caía na caverna, o vulcão nasceu como
		// caverna sem ninguém escrever nada errado. Aqui, tipo novo sem caso
		// não compila — o compilador cobra o despacho no lugar do jogo.
		switch (Peca.Feature)
		{
		case IslandFeatureLayout::EIslandFeature::WalkableMountain:
		{
			AWalkableMountain* Montanha = World->SpawnActor<AWalkableMountain>(
				AWalkableMountain::StaticClass(), OndeNaIlha, FRotator::ZeroRotator, Parametros);
			if (Montanha)
			{
				Montanha->BuildMountain(SementeDaPeca);
				++MontanhasPlantadas;
				PatamaresDaTrilhaMaisLonga = FMath::Max(
					PatamaresDaTrilhaMaisLonga, Montanha->GetTrailSteps().Num());
			}
			break;
		}

		case IslandFeatureLayout::EIslandFeature::Cave:
		{
			ACaveSystem* Caverna = World->SpawnActor<ACaveSystem>(
				ACaveSystem::StaticClass(), OndeNaIlha, FRotator::ZeroRotator, Parametros);
			if (Caverna)
			{
				FCaveRecipe Receita;
				// Retangular: `CaveOtherSide` zero quer dizer "igual ao primeiro",
				// que é o que as cavernas postas à mão continuam sendo.
				Receita.Columns = Peca.CaveSide;
				Receita.Rows = (Peca.CaveOtherSide > 0) ? Peca.CaveOtherSide : Peca.CaveSide;
				Receita.Seed = SementeDaPeca;
				Receita.Flavor = Peca.CaveFlavor;
				Caverna->BuildCave(Receita);

				++CavernasPlantadas;
				MaiorCavernaPlantada = FMath::Max(MaiorCavernaPlantada, Peca.CaveSide);
				CavernasDeLava += (Peca.CaveFlavor == ECaveFlavor::Lava) ? 1 : 0;
				CavernasDeAgua += (Peca.CaveFlavor == ECaveFlavor::Water) ? 1 : 0;
				CavernasFechadas += IsCaveExplorable(Peca.CaveFlavor) ? 0 : 1;
			}
			break;
		}

		case IslandFeatureLayout::EIslandFeature::Volcano:
		{
			AVolcano* Vulcao = World->SpawnActor<AVolcano>(
				AVolcano::StaticClass(), OndeNaIlha, FRotator::ZeroRotator, Parametros);
			if (Vulcao)
			{
				Vulcao->BuildVolcano(SementeDaPeca);
				++VulcoesPlantados;
				DerramesDoVulcao = FMath::Max(DerramesDoVulcao, Vulcao->GetFlowSteps().Num());
			}
			break;
		}
		}
	}

	FBattleDebugScreen::Show(
		FString::Printf(TEXT("montanhas: %d com trilha (a mais longa tem %d patamares)"),
			MontanhasPlantadas, PatamaresDaTrilhaMaisLonga),
		0.0f, FColor::Yellow, /*Key=*/726);

	FBattleDebugScreen::Show(
		FString::Printf(
			TEXT("cavernas: %d (maior %dx%d) — %d de lava, %d de água (%d em cachoeira), ")
			TEXT("%d de boca fechada"),
			CavernasPlantadas, MaiorCavernaPlantada, MaiorCavernaPlantada,
			CavernasDeLava, CavernasDeAgua, GrutasDeCachoeira, CavernasFechadas),
		0.0f, FColor::Orange, /*Key=*/727);

	FBattleDebugScreen::Show(
		FString::Printf(TEXT("vulcões: %d com cratera de lava (%d pedaços de derrame)"),
			VulcoesPlantados, DerramesDoVulcao),
		0.0f, FColor::Red, /*Key=*/728);

	// A AURORA fica sobre a geleira, e não sobre quem anda. Amarrá-la ao
	// jogador daria uma cortina que nunca se alcança nem se deixa para trás —
	// e aurora que acompanha o passo é lanterna, não céu.
	//
	// Nasce apagada e é o relógio que a acende, em `MostrarCeuDoMundo`.
	const FVector2D CentroDaAurora = AAuroraCurtain::SkyCenterUnits();
	AAuroraCurtain* Aurora = World->SpawnActor<AAuroraCurtain>(
		AAuroraCurtain::StaticClass(),
		FVector(Origem.X + CentroDaAurora.X, Origem.Y + CentroDaAurora.Y, AlturaDoChao),
		FRotator::ZeroRotator, Parametros);
	if (Aurora)
	{
		Aurora->BuildCurtain(static_cast<uint32>(WorldScenerySeed));
		AuroraDoCeu = Aurora;
	}

	FBattleDebugScreen::Show(
		FString::Printf(TEXT("mundo: sol e %d pedaços plantados (chão em Z=%.0f%s)"),
			ResidentChunks.Num(), AlturaDoChao,
			bAchouChao ? TEXT("") : TEXT(", NÃO encontrado por traço")),
		0.0f, FColor::Green, /*Key=*/721);
}

void ABattleSquareGameMode::BuildResidentChunk(const FIntPoint& Pedaco)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FVector2D Centro = RegionResidency::ChunkCenterUnits(Pedaco);

	FActorSpawnParameters Parametros;
	Parametros.ObjectFlags |= RF_Transient;

	AForestBackdrop* Mata = World->SpawnActor<AForestBackdrop>(
		AForestBackdrop::StaticClass(),
		FVector(Centro.X, Centro.Y, WorldGroundZ), FRotator::ZeroRotator, Parametros);
	if (!Mata)
	{
		return;
	}

	// O bioma sai do CENTRO do pedaço, uma vez. Perguntar por planta faria a
	// fronteira do deserto serpentear entre as árvores, e o chão — que é uma
	// peça só — continuaria tendo de escolher um dos dois de qualquer jeito.
	Mata->BuildRegion(WorldSceneryCellSizeUnits,
		RegionResidency::ChunkSeed(static_cast<uint32>(WorldScenerySeed), Pedaco),
		IslandGeography::BiomeAt(Centro),
		RegionResidency::ChunkSideUnits());

	ResidentChunks.Add(Pedaco, Mata);
}

int32 ABattleSquareGameMode::ConstruirUsosDoSolo(
	const UIslandBakedPlan& Assado, const FActorSpawnParameters& Parametros)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return 0;
	}

	int32 Erguidos = 0;
	TMap<EGroundUse, int32> PorUso;

	for (const FBakedGroundUse& Mancha : Assado.GroundUses)
	{
		// Assenta no CHÃO que a superfície tem. Um uso do solo na altura zero
		// ficaria enterrado no morro ou boiando sobre o vale, e a contagem
		// continuaria certa.
		const FVector Onde(Mancha.CenterUnits.X, Mancha.CenterUnits.Y,
			Assado.HeightAt(Mancha.CenterUnits));

		AGroundUseActor* Ator = World->SpawnActor<AGroundUseActor>(
			AGroundUseActor::StaticClass(), Onde, FRotator::ZeroRotator, Parametros);
		if (!Ator)
		{
			continue;
		}

		FGroundUsePatch Patch;
		Patch.Use = Mancha.Use;
		Patch.CenterUnits = Mancha.CenterUnits;
		Patch.HalfExtentUnits = Mancha.HalfExtentUnits;
		Patch.bYieldsWater = Mancha.bYieldsWater;
		Patch.Deity = Mancha.Deity;

		if (!Ator->ConfigureFor(Patch))
		{
			// Uso sem linha na tabela e mancha INVISIVEL no mapa. Some calada,
			// e a contagem de atores continua batendo — o padrao exato que
			// esta feature existe para nao repetir.
			FBattleDebugScreen::Show(
				FString::Printf(TEXT("MUNDO: uso do solo SEM FORMA: %s"),
					AGroundUseActor::UseDebugName(Mancha.Use)),
				30.0f, FColor::Red, /*Key=*/-1);
			Ator->Destroy();
			continue;
		}

		++Erguidos;
		PorUso.FindOrAdd(Mancha.Use) += 1;
	}

	FString Resumo;
	for (const TPair<EGroundUse, int32>& Par : PorUso)
	{
		Resumo += FString::Printf(TEXT("%s%s %d"), Resumo.IsEmpty() ? TEXT("") : TEXT(", "),
			AGroundUseActor::UseDebugName(Par.Key), Par.Value);
	}

	FBattleDebugScreen::Show(
		FString::Printf(TEXT("MUNDO: %d usos do solo — %s"), Erguidos, *Resumo),
		30.0f, FColor(180, 220, 150), /*Key=*/750);

	return Erguidos;
}

void ABattleSquareGameMode::AplicarChaoMolhado(const APawn* Jogador, EWaterFooting Chao)
{
	const ACharacter* Personagem = Cast<ACharacter>(Jogador);
	UCharacterMovementComponent* Movimento =
		Personagem ? Personagem->GetCharacterMovement() : nullptr;
	if (!Movimento)
	{
		return;
	}

	// A velocidade em TERRA é guardada na primeira vez, e nunca relida depois.
	// Reler a atual multiplicaria o fator por si mesmo a cada passo dentro da
	// água, e o jogador pararia de andar sem nada acusar.
	if (PassoEmTerraUnidades <= 0.0f)
	{
		PassoEmTerraUnidades = Movimento->MaxWalkSpeed;
	}

	Movimento->MaxWalkSpeed =
		PassoEmTerraUnidades * WaterFooting::SpeedMultiplierFor(Chao);
}

void ABattleSquareGameMode::RefreshRegionResidency()
{
	UWorld* World = GetWorld();
	const APawn* Jogador = AcharPawnDoJogador(World);
	if (!World || !Jogador)
	{
		return;
	}

	// Entrada morta sai ANTES do plano. O ator é transitório e a viagem para a
	// arena leva o nível junto: sem esta limpeza, a residência acharia montado
	// um pedaço que já não existe e o jogador voltaria da batalha pisando no
	// vazio.
	for (auto It = ResidentChunks.CreateIterator(); It; ++It)
	{
		if (!It.Value().IsValid())
		{
			It.RemoveCurrent();
		}
	}

	// EM QUE TERRENO O JOGADOR PISA — antes de qualquer saída antecipada.
	//
	// A cor no chão diz o terreno de longe; o painel diz QUAL é, e é o painel
	// que transforma "aquilo ali é mais escuro" em informação verificável
	// contra a carta. Chave fixa: é estado que muda ao andar, não evento.
	if (TracadoAssado)
	{
		const FVector2D Onde(Jogador->GetActorLocation());
		FBattleDebugScreen::Show(
			FString::Printf(TEXT("terreno: %s"),
				UIslandBakedPlan::BandDebugName(TracadoAssado->BandAt(Onde))),
			0.0f, FColor(200, 200, 150), /*Key=*/747);

		// A ÁGUA MOLHA. Rio desenhado que não muda nada ao ser pisado é
		// enfeite: lê como obstáculo e se comporta como chão — e é essa
		// promessa quebrada que esvazia as 56 travessias do traçado.
		const EWaterFooting Chao = WaterFooting::At(*TracadoAssado, Onde);
		AplicarChaoMolhado(Jogador, Chao);

		FBattleDebugScreen::Show(
			FString::Printf(TEXT("pisando: %s (passo %.0f%%)"),
				WaterFooting::DebugName(Chao),
				WaterFooting::SpeedMultiplierFor(Chao) * 100.0f),
			0.0f,
			Chao == EWaterFooting::Seco ? FColor(200, 200, 150) : FColor(120, 180, 255),
			/*Key=*/748);
	}

	TSet<FIntPoint> Vivos;
	ResidentChunks.GetKeys(Vivos);

	const RegionResidency::FResidencyChange Mudanca =
		RegionResidency::PlanChange(Vivos, FVector2D(Jogador->GetActorLocation()));
	if (Mudanca.ToBuild.IsEmpty() && Mudanca.ToDrop.IsEmpty())
	{
		return;
	}

	// DERRUBA primeiro: montar antes deixaria o pico de memória com os nove
	// antigos e os novos ao mesmo tempo, que é justamente o pico que a
	// residência existe para não ter.
	for (const FIntPoint& Pedaco : Mudanca.ToDrop)
	{
		TWeakObjectPtr<AForestBackdrop> Mata;
		if (ResidentChunks.RemoveAndCopyValue(Pedaco, Mata) && Mata.IsValid())
		{
			Mata->Destroy();
		}
	}

	for (const FIntPoint& Pedaco : Mudanca.ToBuild)
	{
		BuildResidentChunk(Pedaco);
	}

	FBattleDebugScreen::Show(
		FString::Printf(TEXT("mapa: %d pedaços vivos (+%d, -%d)"),
			ResidentChunks.Num(), Mudanca.ToBuild.Num(), Mudanca.ToDrop.Num()),
		0.0f, FColor(150, 220, 150), /*Key=*/744);
}

namespace
{
	/**
	 * Quantos encontros vivos de cada hora, para o painel.
	 *
	 * Existe porque "seis encontros povoaram o mundo" não diz se o peso da
	 * hora funcionou. Contar na tela é o que transforma "confie no sorteio" em
	 * "às três da manhã são cinco noturnos e um diurno" — e é isso que se lê
	 * numa rodada, sem instrumentar nada.
	 */
	FString DescreverAtividadesDosEncontros(UWorld* Mundo)
	{
		int32 Diurnos = 0;
		int32 Tardios = 0;
		int32 Noturnos = 0;

		for (TActorIterator<AWorldEncounterActor> It(Mundo); It; ++It)
		{
			switch (WorldTimeOfDay::ActivityForSpecies(It->CatalogId.ToString()))
			{
			case EPetActivity::Diurnal:     ++Diurnos; break;
			case EPetActivity::Crepuscular: ++Tardios; break;
			case EPetActivity::Nocturnal:   ++Noturnos; break;
			}
		}

		return FString::Printf(TEXT("%d diurnos, %d tardios, %d noturnos"),
			Diurnos, Tardios, Noturnos);
	}
}

EDayPhase ABattleSquareGameMode::CurrentEncounterPhase() const
{
	const float Hora = (CenaDoMundo && CenaDoMundo->IsDayCycleRunning())
		? CenaDoMundo->GetHour()
		: WorldStartHour;

	return WorldTimeOfDay::PhaseAtHour(Hora);
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

	// Com uma espécie só configurada, a contagem sai igual a toda hora — e sem
	// dizer isso na tela, o painel parece um peso quebrado em vez de um
	// catálogo curto. Aviso, não conserto: quem configura é o `.ini`.
	const FString Ressalva = (WorldEncounterCatalogIds.Num() > 1)
		? FString()
		: TEXT(" [catalogo de 1 especie: a hora ainda nao muda quem aparece]");

	FBattleDebugScreen::Show(
		FString::Printf(TEXT("%d encontros povoaram o mundo (%s) — %s%s"),
			WorldEncounterCount,
			WorldTimeOfDay::PhaseDebugName(CurrentEncounterPhase()),
			*DescreverAtividadesDosEncontros(World),
			*Ressalva),
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

	// Pesado pela hora, e não sorteio igual: a espécie noturna aparece de noite
	// e mal aparece ao meio-dia. Sem isto o ciclo do dia era só a luz mudando
	// de cor, e não havia motivo nenhum para esperar anoitecer.
	const int32 CatalogoIndice = WorldTimeOfDay::PickSpeciesForPhase(
		WorldEncounterCatalogIds, CurrentEncounterPhase(), Sorteio);
	if (CatalogoIndice == INDEX_NONE)
	{
		Encontro->Destroy();
		return;
	}

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

	// A hora da reposição também: quem volta da batalha de noite precisa ver
	// que os substitutos são noturnos, e não os mesmos bichos de manhã.
	FBattleDebugScreen::Show(
		FString::Printf(TEXT("%d reposto(s) (%s) — %s"), Faltam,
			WorldTimeOfDay::PhaseDebugName(CurrentEncounterPhase()),
			*DescreverAtividadesDosEncontros(World)),
		8.0f, FColor::Green, /*Key=*/721);
}

// A especialidade é DELIBERADA: nada nela acontece por estar parado no lugar.
// Fora do Shipping por compilação, como o resto das ferramentas de
// desenvolvimento — quando houver barra no mundo, o botão substitui isto.
#if !UE_BUILD_SHIPPING
namespace
{
	/**
	 * `bs.ShotAfter <segundos>` — tira uma captura depois de esperar.
	 *
	 * Existe porque `HighResShot` no `-ExecCmds` dispara no INSTANTE do boot, e
	 * o que ele fotografa é a tela de carregamento: o mundo ainda não montou.
	 * Foi exatamente o que aconteceu na primeira tentativa, e a imagem saiu um
	 * retângulo creme.
	 *
	 * Com isto, quem verifica de fora consegue pedir "abra o jogo, espere o
	 * mundo, e fotografe" numa linha só — e passa a ENXERGAR o que antes só
	 * dava para deduzir do log. Boa parte do roteiro manual é sobre o que
	 * aparece na tela, e o que aparece na tela cabe numa imagem.
	 *
	 * Ferramenta de DESENVOLVIMENTO, compilada fora do Shipping.
	 */
	FAutoConsoleCommandWithWorldAndArgs GShotAfterCommand(
		TEXT("bs.ShotAfter"),
		TEXT("Tira uma captura depois de N segundos (padrão 8). Espera o mundo montar."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(
			[](const TArray<FString>& Args, UWorld* World)
			{
				if (!World)
				{
					return;
				}

				const float Espera = Args.Num() > 0
					? FMath::Clamp(FCString::Atof(*Args[0]), 0.5f, 120.0f)
					: 8.0f;

				FTimerHandle Disparo;
				World->GetTimerManager().SetTimer(Disparo,
					FTimerDelegate::CreateLambda([World]()
					{
						// Pelo console, e não por API: `HighResShot` já resolve
						// caminho, formato e nome do arquivo, e reimplementar
						// isso seria uma segunda fonte para a mesma coisa.
						GEngine->Exec(World, TEXT("HighResShot 1280x720"));
					}),
					Espera, /*bLoop=*/false);
			}));

	FAutoConsoleCommandWithWorldAndArgs GMarcarCommand(
		TEXT("bs.Marcar"),
		TEXT("Marca o lugar onde você está no mapa — ou apaga, se já houver marcação aqui. "
			 "Argumento opcional: interesse (padrão), perigo, destino."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(
			[](const TArray<FString>& Args, UWorld* World)
			{
				ABattleSquareGameMode* GameMode =
					World ? World->GetAuthGameMode<ABattleSquareGameMode>() : nullptr;
				if (!GameMode)
				{
					return;
				}

				// Tipo desconhecido vira INTERESSE em vez de recusar: quem
				// digitou errado quis marcar, e negar o gesto por causa da
				// palavra perde a marcação junto.
				EWorldPinKind Tipo = EWorldPinKind::Interesse;
				if (Args.Num() > 0)
				{
					if (Args[0].Equals(TEXT("perigo"), ESearchCase::IgnoreCase))
					{
						Tipo = EWorldPinKind::Perigo;
					}
					else if (Args[0].Equals(TEXT("destino"), ESearchCase::IgnoreCase))
					{
						Tipo = EWorldPinKind::Destino;
					}
				}

				GameMode->ToggleMapPinHere(Tipo);
			}));

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

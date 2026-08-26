// Copyright 2026 Anderson. All Rights Reserved.

#include "Net/BattleSquareGameMode.h"
#include "Net/BattleSquarePlayerController.h"
#include "Data/PetDataLoader.h"
#include "Meta/PetCollectionService.h"
#include "Meta/PetProgressionService.h"
#include "World/WorldEncounterFlow.h"
#include "World/EncounterDetectionComponent.h"
#include "World/EncounterMatchAssembler.h"
#include "Battle/BattleArena.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

ABattleSquareGameMode::ABattleSquareGameMode()
{
	PlayerControllerClass = ABattleSquarePlayerController::StaticClass();
	PrimaryActorTick.bCanEverTick = true;
	// Timeouts de sala são medidos em dezenas de segundos
	// (CommitTimeoutSeconds=45, AbandonTimeoutSeconds=120) — checar uma
	// vez por segundo é preciso o bastante e barato mesmo com muitas
	// salas ativas (TMap pequeno, sem I/O).
	PrimaryActorTick.TickInterval = 1.0f;
}

void ABattleSquareGameMode::BeginPlay()
{
	Super::BeginPlay();
	EnsureRoomRegistry();

}

namespace
{
	TArray<uint8> HexStringToBytes(const FString& Hex)
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
		return TEXT("nenhum pawn com UEncounterDetectionComponent no nível");
	}
	if (WorldEncounterMirrorPath.IsEmpty())
	{
		return TEXT("WorldEncounterMirrorPath não configurado em DefaultGame.ini");
	}

	TArray<FLoadedPetRecord> Pets;
	int32 RejectedCount = 0;
	const bool bLoaded = FPetDataLoader::LoadVerifiedPets(
		WorldEncounterMirrorPath,
		HexStringToBytes(WorldEncounterMirrorKeyHex),
		WorldEncounterMirrorPublicKeyPem,
		Pets,
		RejectedCount);

	if (!bLoaded || Pets.Num() == 0)
	{
		return TEXT("espelho de pets não carregou");
	}

	FEncounterMatchParams MatchParams;
	MatchParams.AvailablePets = Pets;
	MatchParams.PetCollectionSlotName = PetCollectionSlotName;
	// Sem pet declarado, o primeiro do espelho serve — é nível de teste, e
	// travar por configuração ausente aqui não ajudaria ninguém.
	MatchParams.PlayerCatalogId = WorldEncounterPlayerCatalogId.IsEmpty() ? Pets[0].Id : WorldEncounterPlayerCatalogId;

	WorldEncounterFlow = NewObject<UWorldEncounterFlow>(this);
	WorldEncounterFlow->Initialize(DetectingPawn, Detection, ABattleArena::StaticClass(), MatchParams);
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
	if (!WorldEncounterFlow)
	{
		const FString Problem = SetUpWorldEncounterFlow();
		if (Problem.IsEmpty())
		{
			UE_LOG(LogTemp, Display, TEXT("ABattleSquareGameMode: encontros de mundo ATIVOS."));
		}
		else if (!bHasLoggedWorldEncounterProblem)
		{
			// Uma vez só: o Tick roda 1x/s e isto viraria enxurrada de log.
			bHasLoggedWorldEncounterProblem = true;
			UE_LOG(LogTemp, Log, TEXT("ABattleSquareGameMode: encontros de mundo ainda inativos — %s"), *Problem);
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
	FBattleDataTranslator::TranslatePet(Pets[0], /*PetId=*/1, /*Side=*/0, /*Column=*/1, /*Row=*/1, Side0Pet, Side0Presentation);

	FPetState Side1Pet;
	FPetPresentationInfo Side1Presentation;
	FBattleDataTranslator::TranslatePet(Pets[1], /*PetId=*/2, /*Side=*/1, /*Column=*/2, /*Row=*/1, Side1Pet, Side1Presentation);

	// T5 (niveis-experiencia-evolucao): pet de catálogo já capturado
	// entra na partida com o bônus de atributo do nível dele. Pet não
	// capturado (ou nível 1) fica exatamente como o catálogo — zero
	// regressão (NIVEL-09).
	ApplyOwnedPetProgressionBonus(PetCollectionSlotName, Side0Pet, Side0Presentation);
	ApplyOwnedPetProgressionBonus(PetCollectionSlotName, Side1Pet, Side1Presentation);

	FBattleState InitialState;
	InitialState.Pets.Add(Side0Pet);
	InitialState.Pets.Add(Side1Pet);

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

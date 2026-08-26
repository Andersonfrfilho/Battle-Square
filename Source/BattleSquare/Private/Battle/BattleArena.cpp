// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattleArena.h"
#include "Camera/CameraComponent.h"
#include "Battle/DumbOpponentAI.h"
#include "Battle/BattleResolver.h"
#include "Battle/BattleOutcome.h"

ABattleArena::ABattleArena()
{
	PrimaryActorTick.bCanEverTick = false;

	ArenaRoot = CreateDefaultSubobject<USceneComponent>(TEXT("ArenaRoot"));
	SetRootComponent(ArenaRoot);

	ArenaCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ArenaCamera"));
	ArenaCamera->SetupAttachment(ArenaRoot);

	PlayerActionQueue = CreateDefaultSubobject<UBattleActionQueueComponent>(TEXT("PlayerActionQueue"));
	PlayerActionQueue->OnCommitted.AddDynamic(this, &ABattleArena::HandlePlayerCommitted);

	// Diorama fixo: recuada e elevada, olhando para o centro da grade
	// (DP-09 — tilt-shift/câmera fixa só nesta cena). Valores calibrados
	// para uma grade 3x3 de CellSize padrão; escala junto se CellSize
	// mudar via BeginPlay/ajuste de instância.
	ArenaCamera->SetRelativeLocation(FVector(-600.0f, 0.0f, 500.0f));
	ArenaCamera->SetRelativeRotation(FRotator(-38.0f, 0.0f, 0.0f));
	ArenaCamera->FieldOfView = 45.0f;
	ArenaCamera->AspectRatio = 16.0f / 9.0f;
}

FVector ABattleArena::GetCellWorldLocation(uint8 Column, uint8 Row) const
{
	const float OffsetX = (static_cast<float>(Column) - 1.0f) * CellSize;
	const float OffsetY = (static_cast<float>(Row) - 1.0f) * CellSize;
	return GetActorLocation() + FVector(OffsetX, OffsetY, 0.0f);
}

void ABattleArena::SpawnPetViews(const FBattleState& InitialState, const TArray<FPetPresentationInfo>& Presentations)
{
	SpawnedPetViews.Reset();

	UWorld* World = GetWorld();
	for (const FPetState& Pet : InitialState.Pets)
	{
		const FPetPresentationInfo* Presentation = Presentations.FindByPredicate(
			[&Pet](const FPetPresentationInfo& Info) { return Info.PetId == Pet.PetId; });
		if (!Presentation)
		{
			continue;
		}

		APetView* View = World
			? World->SpawnActor<APetView>(GetCellWorldLocation(Pet.Column, Pet.Row), FRotator::ZeroRotator)
			: NewObject<APetView>(this);

		if (!View)
		{
			continue;
		}

		View->SetInitialState(Pet, *Presentation);
		SpawnedPetViews.Add(View);
	}
}

bool ABattleArena::IsPointInCameraFrustum(const FVector& WorldPoint) const
{
	const FVector CameraLocation = ArenaCamera->GetComponentLocation();
	const FVector Forward = ArenaCamera->GetForwardVector();
	const FVector Right = ArenaCamera->GetRightVector();
	const FVector Up = ArenaCamera->GetUpVector();

	const FVector ToPoint = WorldPoint - CameraLocation;
	const float DepthZ = FVector::DotProduct(ToPoint, Forward);
	if (DepthZ <= 0.0f)
	{
		return false;
	}

	const float HorizontalHalfFovRad = FMath::DegreesToRadians(ArenaCamera->FieldOfView) * 0.5f;
	const float VerticalHalfFovRad = FMath::Atan(FMath::Tan(HorizontalHalfFovRad) / ArenaCamera->AspectRatio);

	const float HalfWidthAtDepth = DepthZ * FMath::Tan(HorizontalHalfFovRad);
	const float HalfHeightAtDepth = DepthZ * FMath::Tan(VerticalHalfFovRad);

	const float RightOffset = FVector::DotProduct(ToPoint, Right);
	const float UpOffset = FVector::DotProduct(ToPoint, Up);

	return FMath::Abs(RightOffset) <= HalfWidthAtDepth && FMath::Abs(UpOffset) <= HalfHeightAtDepth;
}

bool ABattleArena::AreAllGridCellsInCameraFrustum() const
{
	for (uint8 Column = 0; Column < 3; ++Column)
	{
		for (uint8 Row = 0; Row < 3; ++Row)
		{
			if (!IsPointInCameraFrustum(GetCellWorldLocation(Column, Row)))
			{
				return false;
			}
		}
	}
	return true;
}

bool ABattleArena::BeginBattle(const FBattleState& InitialState, const TArray<FPetPresentationInfo>& Presentations)
{
	// T7 (arenas-variadas, ARENA-02, edge case da spec): montagem que
	// posicionaria um pet numa casa bloqueada falha explicitamente, alto
	// e claro — nunca reposiciona silenciosamente.
	for (const FPetState& Pet : InitialState.Pets)
	{
		if (InitialState.CellLayout.IsValidIndex(CellLayoutIndex(Pet.Column, Pet.Row))
			&& InitialState.CellLayout[CellLayoutIndex(Pet.Column, Pet.Row)] == static_cast<uint8>(ECellProperty::Blocked))
		{
			UE_LOG(LogTemp, Error, TEXT("ABattleArena::BeginBattle: pet %d posicionado numa casa bloqueada (%d,%d) — montagem rejeitada"),
				Pet.PetId, Pet.Column, Pet.Row);
			return false;
		}
	}

	CurrentState = InitialState;
	SpawnPetViews(CurrentState, Presentations);

	if (!TracePlayer)
	{
		TracePlayer = NewObject<UBattleTracePlayer>(this);
		TracePlayer->OnEventApplied.AddUObject(this, &ABattleArena::DispatchEventToPetViews);
	}

	return true;
}

void ABattleArena::DispatchEventToPetViews(const FBattleEvent& Event)
{
	for (const TObjectPtr<APetView>& View : SpawnedPetViews)
	{
		if (View && (View->GetPetId() == Event.ActorId || View->GetPetId() == Event.TargetId))
		{
			View->ApplyEvent(Event);
		}
	}
}

void ABattleArena::ConfigureNetworkedOpponent(UBattleTurnCoordinator* InCoordinator)
{
	ServerCoordinator = InCoordinator;
	if (ServerCoordinator)
	{
		ServerCoordinator->OnTurnResolved.AddUObject(this, &ABattleArena::HandleCoordinatorTurnResolved);
	}
}

void ABattleArena::HandleCoordinatorTurnResolved(const FBattleState& NextState, const TArray<FBattleEvent>& Trace)
{
	CurrentState = NextState;
	if (TracePlayer)
	{
		TracePlayer->PlayTrace(Trace);
	}
}

void ABattleArena::HandlePlayerCommitted()
{
	const FTurnCommit PlayerCommit = PlayerActionQueue->BuildCommit();

	// T8 (tasks.md, Combate Online, NET-09/NET-10): oponente real presente
	// (ServerCoordinator setado por ConfigureNetworkedOpponent) — o
	// resultado chega via HandleCoordinatorTurnResolved, não aqui.
	// FDumbOpponentAI nunca é chamado neste caminho.
	if (ServerCoordinator)
	{
		ServerCoordinator->SubmitCommit(/*Side=*/0, PlayerCommit);
		return;
	}

	// Sem oponente humano (Standalone): comportamento idêntico ao de
	// antes desta feature — IA gera o commit dela (Side=1 por convenção),
	// o resolvedor real roda com os dois commits, e o trace resultante
	// anima as views. Nenhum cálculo de batalha aqui — só orquestração.
	const FTurnCommit OpponentCommit = FDumbOpponentAI::GenerateRandomValidCommit(CurrentState, /*Side=*/1, CurrentState.Random);

	FBattleResolveResult Result = FBattleResolver::ResolveTurn(CurrentState, PlayerCommit, OpponentCommit);
	// ResolveTurn nunca decide vitória/derrota por design (BattleOutcome.h:
	// "separação deliberada") — quem chama precisa avaliar depois. Achado
	// real durante escala-pets-skills: nem aqui nem UBattleTurnCoordinator
	// chamavam isto, então BatalhaEncerrada nunca disparava em produção.
	BattleOutcome::EvaluateOutcome(Result.NextState, Result.Trace);
	CurrentState = Result.NextState;

	if (TracePlayer)
	{
		TracePlayer->PlayTrace(Result.Trace);
	}
}

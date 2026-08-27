// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattleArena.h"

#include "Debug/BattleDebugScreen.h"

DEFINE_LOG_CATEGORY(LogBattleArena);
#include "Camera/CameraComponent.h"
#include "Battle/DumbOpponentAI.h"
#include "Battle/BattleResolver.h"
#include "Battle/BattleOutcome.h"
#include "Meta/PetCollectionService.h"
#include "Meta/PetProgressionService.h"

ABattleArena::ABattleArena()
{
	PrimaryActorTick.bCanEverTick = true;

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
	// A câmera fica recuada em -X olhando ao longo de +X: na tela, +Y é a
	// DIREITA e +X é o FUNDO. Mapear Coluna->X e Linha->Y (o óbvio) fazia
	// "Baixo" andar para a direita e "Direita" andar para o fundo — foi o que
	// se viu jogando. Linha vira o eixo vertical da tela, coluna o horizontal.
	const float OffsetX = -(static_cast<float>(Row) - 1.0f) * CellSize;
	const float OffsetY = (static_cast<float>(Column) - 1.0f) * CellSize;
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

	// T4 (colecao-e-captura): retido para CheckForCapture (T5) consultar
	// CatalogId/Name/Type quando a batalha terminar — FPetState já não
	// carrega isso (AD-012).
	PresentationsByPetId.Reset();
	for (const FPetPresentationInfo& Presentation : Presentations)
	{
		PresentationsByPetId.Add(Presentation.PetId, Presentation);
	}

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

			// A conversão casa -> mundo é da ARENA, não da view: é ela que
			// conhece CellSize e a origem da grade. A view só sabe em que
			// casa está. Sem isto o pet muda de casa no estado e não sai do
			// lugar na tela.
			View->SetActorLocation(GetCellWorldLocation(View->GetColumn(), View->GetRow()));
			View->RefreshBodyAppearance();
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
	CheckForCapture(Trace);
	GrantExperienceIfOwned(Trace);
	AnnounceBattleFinishedIfEnded(Trace);

	if (TracePlayer)
	{
		TracePlayer->StartPlayback(Trace);
		bWaitingForPlaybackToOpenNextTurn = true;
	}
	else
	{
		OpenNextTurnIfBattleContinues();
	}
}

void ABattleArena::AnnounceBattleFinishedIfEnded(const TArray<FBattleEvent>& Trace)
{
	if (bHasAnnouncedBattleFinished)
	{
		return;
	}

	for (const FBattleEvent& Event : Trace)
	{
		if (Event.Type == EBattleEventType::BatalhaEncerrada)
		{
			bHasAnnouncedBattleFinished = true;
			OnBattleFinished.Broadcast();
			return;
		}
	}
}

void ABattleArena::CheckForCapture(const TArray<FBattleEvent>& Trace)
{
	for (const FBattleEvent& Event : Trace)
	{
		if (Event.Type != EBattleEventType::BatalhaEncerrada)
		{
			continue;
		}

		// Só vitória do jogador LOCAL captura — derrota, empate (Value ==
		// 0xFF) e vitória do outro lado nunca capturam (COLECAO-04).
		if (Event.Value != static_cast<int32>(LocalPlayerSide))
		{
			return;
		}

		// T5 🧠: o pet capturado é o do lado OPOSTO ao vencedor — nunca o
		// próprio pet do jogador.
		const uint8 OpponentSide = (LocalPlayerSide == 0) ? 1 : 0;
		const FPetState* OpponentPet = CurrentState.Pets.FindByPredicate(
			[OpponentSide](const FPetState& Pet) { return Pet.Side == OpponentSide; });
		if (!OpponentPet)
		{
			return;
		}

		const FPetPresentationInfo* Presentation = PresentationsByPetId.Find(OpponentPet->PetId);
		if (!Presentation || Presentation->CatalogId.IsEmpty())
		{
			return;
		}

		FOwnedPetInstance Instance;
		Instance.CatalogId = Presentation->CatalogId;
		Instance.Name = Presentation->Name;
		Instance.Type = Presentation->Type;
		FPetCollectionService::CaptureIfNew(PetCollectionSlotName, Instance);
		return;
	}
}

void ABattleArena::GrantExperienceIfOwned(const TArray<FBattleEvent>& Trace)
{
	for (const FBattleEvent& Event : Trace)
	{
		if (Event.Type != EBattleEventType::BatalhaEncerrada)
		{
			continue;
		}

		int32 ExperienceAmount = BattlePetProgressionConstants::ExperienceForLoss;
		if (Event.Value == static_cast<int32>(LocalPlayerSide))
		{
			ExperienceAmount = BattlePetProgressionConstants::ExperienceForWin;
		}
		else if (Event.Value == 0xFF)
		{
			ExperienceAmount = BattlePetProgressionConstants::ExperienceForDraw;
		}

		// XP vai para o pet do JOGADOR LOCAL — nunca o oponente, mesmo
		// que ele também esteja na coleção (edge case: os dois lados são
		// pets que o jogador possui, via Standalone contra a própria IA).
		const FPetState* OwnPet = CurrentState.Pets.FindByPredicate(
			[this](const FPetState& Pet) { return Pet.Side == LocalPlayerSide; });
		if (!OwnPet)
		{
			return;
		}

		const FPetPresentationInfo* Presentation = PresentationsByPetId.Find(OwnPet->PetId);
		if (!Presentation || Presentation->CatalogId.IsEmpty())
		{
			return;
		}

		TArray<FOwnedPetInstance> Collection = FPetCollectionService::LoadCollection(PetCollectionSlotName);
		FOwnedPetInstance* OwnedInstance = Collection.FindByPredicate(
			[Presentation](const FOwnedPetInstance& Instance) { return Instance.CatalogId == Presentation->CatalogId; });
		if (!OwnedInstance)
		{
			return; // pet do jogador ainda não capturado — nenhuma XP fantasma
		}

		FPetProgressionService::GrantExperience(*OwnedInstance, ExperienceAmount);
		FPetCollectionService::SaveCollection(PetCollectionSlotName, Collection);
		return;
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

	LogCommit(TEXT("jogador"), PlayerCommit);
	LogCommit(TEXT("oponente"), OpponentCommit);

	FBattleResolveResult Result = FBattleResolver::ResolveTurn(CurrentState, PlayerCommit, OpponentCommit);
	// ResolveTurn nunca decide vitória/derrota por design (BattleOutcome.h:
	// "separação deliberada") — quem chama precisa avaliar depois. Achado
	// real durante escala-pets-skills: nem aqui nem UBattleTurnCoordinator
	// chamavam isto, então BatalhaEncerrada nunca disparava em produção.
	BattleOutcome::EvaluateOutcome(Result.NextState, Result.Trace);
	CurrentState = Result.NextState;

	for (const FPetState& Pet : CurrentState.Pets)
	{
		UE_LOG(LogBattleArena, Display, TEXT("  lado %d terminou em (%d,%d) com %d/%d de vida"),
			static_cast<int32>(Pet.Side), static_cast<int32>(Pet.Column),
			static_cast<int32>(Pet.Row), Pet.Health, Pet.MaxHealth);

		// Chave por lado: a linha de cada pet se ATUALIZA no lugar, em vez de
		// empilhar uma nova a cada turno.
		FBattleDebugScreen::Show(
			FString::Printf(TEXT("lado %d: casa (%d,%d)  vida %d/%d"),
				static_cast<int32>(Pet.Side), static_cast<int32>(Pet.Column),
				static_cast<int32>(Pet.Row), Pet.Health, Pet.MaxHealth),
			30.0f, FColor::Green, /*Key=*/100 + Pet.Side);
	}

	if (TracePlayer)
	{
		TracePlayer->StartPlayback(Result.Trace);
		bWaitingForPlaybackToOpenNextTurn = true;
	}
	else
	{
		OpenNextTurnIfBattleContinues();
	}
}

void ABattleArena::OpenNextTurnIfBattleContinues()
{
	// Batalha encerrada NÃO abre turno novo: a fila travada é o que impede o
	// jogador de escolher ações para uma partida que já acabou.
	if (bHasAnnouncedBattleFinished || !PlayerActionQueue)
	{
		return;
	}

	PlayerActionQueue->BeginNewTurn();
}

void ABattleArena::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bWaitingForPlaybackToOpenNextTurn || !TracePlayer)
	{
		return;
	}

	if (!TracePlayer->Advance(DeltaSeconds))
	{
		bWaitingForPlaybackToOpenNextTurn = false;
		OpenNextTurnIfBattleContinues();
	}
}

void ABattleArena::LogCommit(const TCHAR* Quem, const FTurnCommit& Commit) const
{
	// Permanente, e em categoria própria: "o inimigo não fez nada" foi a
	// pergunta mais cara desta feature, e responder exigia justamente isto.
	// Filtrar com: Log LogBattleArena Off
	for (int32 Slot = 0; Slot < FTurnCommit::ActionsPerTurn; ++Slot)
	{
		const FBattleAction& Action = Commit.Actions[Slot];
		const FString Linha = FString::Printf(TEXT("%s %d: %s %s"),
			Quem, Slot + 1,
			*StaticEnum<EActionType>()->GetNameStringByValue(static_cast<int64>(Action.Type)),
			*StaticEnum<EBattleDirection>()->GetNameStringByValue(static_cast<int64>(Action.Direction)));

		UE_LOG(LogBattleArena, Display, TEXT("[turno] %s"), *Linha);
		FBattleDebugScreen::Show(Linha, 8.0f,
			FCString::Strcmp(Quem, TEXT("oponente")) == 0 ? FColor::Orange : FColor::Cyan);
	}
}

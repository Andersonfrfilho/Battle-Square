// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattleArena.h"
#include "EngineUtils.h"
#include "Battle/BattleNarration.h"

#include "Debug/BattleDebugScreen.h"
#include "DrawDebugHelpers.h"

DEFINE_LOG_CATEGORY(LogBattleArena);
#include "Camera/CameraComponent.h"
#include "Battle/TacticalOpponentAI.h"
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
	// Os pets já nascem olhando um para o outro.
	RefreshGazes();
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
	NarrateEvent(Event);


	for (const TObjectPtr<APetView>& View : SpawnedPetViews)
	{
		if (View && (View->GetPetId() == Event.ActorId || View->GetPetId() == Event.TargetId))
		{
			View->ApplyEvent(Event);

			// A conversão casa -> mundo é da ARENA, não da view: é ela que
			// conhece CellSize e a origem da grade. A view só sabe em que
			// casa está. Sem isto o pet muda de casa no estado e não sai do
			// lugar na tela.
			// Desliza em vez de aparecer: o teleporte não deixava ver QUEM
			// andou nem em que ordem.
			View->GlideTo(GetCellWorldLocation(View->GetColumn(), View->GetRow()));
			View->RefreshBodyAppearance();
		}
	}

	// Depois de QUALQUER reposicionamento: quem andou passa a olhar de outro
	// ângulo, e quem ficou parado também, porque o alvo mudou de casa.
	RefreshGazes();
}

uint8 ABattleArena::FindPostureFlagsForPet(uint8 PetId) const
{
	for (const FPetState& Pet : CurrentState.Pets)
	{
		if (Pet.PetId == PetId)
		{
			return Pet.PostureFlags;
		}
	}
	return 0;
}

void ABattleArena::RefreshGazes()
{
	for (const TObjectPtr<APetView>& View : SpawnedPetViews)
	{
		if (!View || View->IsDefeated())
		{
			continue;
		}

		for (const TObjectPtr<APetView>& Other : SpawnedPetViews)
		{
			if (!Other || Other == View || Other->GetSide() == View->GetSide() || Other->IsDefeated())
			{
				continue;
			}

			// DP-ia-04: o olhar segue o que o adversário FEZ. É a metade
			// visível de uma regra que, sem isto, o jogador só sentiria pelo
			// dano que não veio.
			const uint8 Flags = FindPostureFlagsForPet(Other->GetPetId());
			if ((Flags & static_cast<uint8>(EBattlePostureFlags::Camouflaged)) != 0)
			{
				View->LoseSightOfTarget();
			}
			else if ((Flags & static_cast<uint8>(EBattlePostureFlags::Flying)) != 0)
			{
				View->LookUp();
			}
			else if ((Flags & static_cast<uint8>(EBattlePostureFlags::Underground)) != 0)
			{
				View->LookDown();
			}
			else
			{
				View->LookAtLocation(Other->GetActorLocation());
			}
			break;
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

			// Quem venceu, na tela. "Nada aconteceu" era duas coisas ao mesmo
			// tempo: a transição não voltava, E o jogador não tinha como saber
			// se tinha ganho — o silêncio parecia defeito nas duas pontas.
			const bool bEmpate = (Event.Value == 0xFF);
			const bool bVenceu = (Event.Value == static_cast<int32>(LocalPlayerSide));

			FBattleDebugScreen::Show(
				bEmpate ? TEXT("=== EMPATE ===")
					: (bVenceu ? TEXT("=== VOCÊ VENCEU ===") : TEXT("=== VOCÊ PERDEU ===")),
				0.0f, bVenceu ? FColor::Green : (bEmpate ? FColor::Silver : FColor::Red),
				/*Key=*/950);

			FBattleNarrationFeed::Push(
				FText::FromString(bEmpate ? TEXT("A batalha terminou empatada.")
					: (bVenceu ? TEXT("Você venceu a batalha!") : TEXT("Você perdeu a batalha."))),
				bVenceu ? FColor::Green : (bEmpate ? FColor::Silver : FColor::Red));

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
	// A IA local nunca é chamada neste caminho.
	if (ServerCoordinator)
	{
		ServerCoordinator->SubmitCommit(/*Side=*/0, PlayerCommit);
		return;
	}

	// Modo de teste: a MESMA pessoa escolhe pelos dois lados. A primeira
	// escolha fica guardada e a fila reabre; a segunda fecha o turno.
	if (bControlsBothSides)
	{
		if (!bAwaitingOpponentChoice)
		{
			StoredLocalCommit = PlayerCommit;
			bAwaitingOpponentChoice = true;

			FBattleDebugScreen::Show(
				TEXT("agora escolha pelo JOGADOR 2"),
				8.0f, FColor::Orange, 800);

			PlayerActionQueue->BeginNewTurn();
			return;
		}

		bAwaitingOpponentChoice = false;
		FBattleDebugScreen::Show(TEXT("turno resolvido — próxima escolha é do jogador 1"), 8.0f, FColor::Cyan, 800);
		ResolveTurnWithCommits(StoredLocalCommit, PlayerCommit);
		return;
	}

	// Sem oponente humano (Standalone): comportamento idêntico ao de
	// antes desta feature — IA gera o commit dela (Side=1 por convenção),
	// o resolvedor real roda com os dois commits, e o trace resultante
	// anima as views. Nenhum cálculo de batalha aqui — só orquestração.
	// A IA joga pelo lado que o jogador NÃO está controlando. Fixar Side=1
	// aqui faria a troca de jogador produzir dois commits para o mesmo pet.
	const uint8 BotSide = (LocalPlayerSide == 0) ? 1 : 0;

	FTurnCommit OtherCommit;
	if (PlayerTwoManualActions.Num() > 0)
	{
		// Escolhas à mão do jogador 2 substituem o bot. Faltando ações, o
		// núcleo completa com Aguardar — mesma regra do commit normal.
		for (int32 Slot = 0; Slot < FTurnCommit::ActionsPerTurn; ++Slot)
		{
			OtherCommit.Actions[Slot] = PlayerTwoManualActions.IsValidIndex(Slot)
				? PlayerTwoManualActions[Slot]
				: FBattleAction{ EActionType::Aguardar, EBattleDirection::Nenhuma };
		}
	}
	else
	{
		OtherCommit = FTacticalOpponentAI::GenerateCommit(CurrentState, BotSide, CurrentState.Random);
	}

	ResolveTurnWithCommits(PlayerCommit, OtherCommit);
}

/**
 * A resolução do turno, a partir dos dois commits já formados.
 *
 * Separada porque agora há DOIS caminhos até aqui — a IA gerando o commit do
 * oponente, e a pessoa escolhendo pelos dois lados. Duplicar a orquestração
 * faria os dois divergirem na primeira mudança.
 */
void ABattleArena::ResolveTurnWithCommits(const FTurnCommit& LocalCommit, const FTurnCommit& OpponentCommit)
{
	// Escolhas são de UM turno. Sobreviver faria o seguinte começar com as do
	// anterior já marcadas.
	DraftsBySide[0].Reset();
	DraftsBySide[1].Reset();
	ClearPlayerTwoActions();

	LogCommit(*FString::Printf(TEXT("jogador %d"), static_cast<int32>(GetControlledPlayerNumber())), LocalCommit);
	LogCommit(TEXT("bot"), OpponentCommit);

	// ResolveTurn recebe por LADO (esquerdo = 0, direito = 1), não por "quem
	// escolheu". Com o jogador controlando o lado 1, passar na ordem de quem
	// escolheu trocaria as ações entre os pets — e o turno inteiro sairia
	// espelhado, de um jeito plausível o bastante para não parecer defeito.
	const bool bPlayerIsLeft = (LocalPlayerSide == 0);
	const FTurnCommit& LeftCommit = bPlayerIsLeft ? LocalCommit : OpponentCommit;
	const FTurnCommit& RightCommit = bPlayerIsLeft ? OpponentCommit : LocalCommit;

	FBattleResolveResult Result = FBattleResolver::ResolveTurn(CurrentState, LeftCommit, RightCommit);
	// ResolveTurn nunca decide vitória/derrota por design (BattleOutcome.h:
	// "separação deliberada") — quem chama precisa avaliar depois. Achado
	// real durante escala-pets-skills: nem aqui nem UBattleTurnCoordinator
	// chamavam isto, então BatalhaEncerrada nunca disparava em produção.
	BattleOutcome::EvaluateOutcome(Result.NextState, Result.Trace);
	CurrentState = Result.NextState;

	// Captura, XP e anúncio do fim viviam SÓ no caminho de rede. A batalha
	// local resolvia o turno, avaliava o desfecho e não contava a ninguém —
	// então derrotar o inimigo no mundo não devolvia o jogador ao mundo, e ele
	// ficava preso numa arena de uma partida já terminada.
	//
	// M1–M4 nunca jogaram uma partida até o fim por uma tela; o caminho local
	// nunca tinha chegado até aqui.
	CheckForCapture(Result.Trace);
	GrantExperienceIfOwned(Result.Trace);
	AnnounceBattleFinishedIfEnded(Result.Trace);

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

	DrawDebugGrid();

	// O deslize e o OLHAR avançam juntos: sem atualizar o olhar durante o
	// movimento, o pet chegaria olhando para onde o outro estava antes.
	for (const TObjectPtr<APetView>& View : SpawnedPetViews)
	{
		if (View)
		{
			View->AdvanceGlide(DeltaSeconds);
		}
	}
	RefreshGazes();

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
		// "jogador 2" lia-se como um SEGUNDO jogador; é a ação 2 do mesmo
		// pet. O rótulo agora diz isso.
		const FString Linha = FString::Printf(TEXT("%s - acao %d/3: %s %s"),
			Quem, Slot + 1,
			*StaticEnum<EActionType>()->GetNameStringByValue(static_cast<int64>(Action.Type)),
			*StaticEnum<EBattleDirection>()->GetNameStringByValue(static_cast<int64>(Action.Direction)));

		UE_LOG(LogBattleArena, Display, TEXT("[turno] %s"), *Linha);
		FBattleDebugScreen::Show(Linha, 8.0f,
			FCString::Strcmp(Quem, TEXT("oponente")) == 0 ? FColor::Orange : FColor::Cyan);
	}
}

void ABattleArena::DrawDebugGrid() const
{
	if (!FBattleDebugScreen::IsEnabled() || !GetWorld())
	{
		return;
	}

	constexpr uint8 GridSize = 3;
	const float HalfCell = CellSize * 0.5f;

	for (uint8 Row = 0; Row < GridSize; ++Row)
	{
		for (uint8 Column = 0; Column < GridSize; ++Column)
		{
			const FVector Centro = GetCellWorldLocation(Column, Row);

			// Quem está aqui? Coabitação (DP-02) faz DOIS pets caberem na
			// mesma casa, e é justamente isso que precisa ficar visível.
			FString Ocupantes;
			for (const FPetState& Pet : CurrentState.Pets)
			{
				if (Pet.Column == Column && Pet.Row == Row)
				{
					Ocupantes += FString::Printf(TEXT(" [lado %d]"), static_cast<int32>(Pet.Side));
				}
			}

			const FColor CorDaCasa = Ocupantes.IsEmpty() ? FColor(80, 80, 80) : FColor::Yellow;
			DrawDebugBox(GetWorld(), Centro, FVector(HalfCell, HalfCell, 2.0f),
				CorDaCasa, /*bPersistent=*/false, /*LifeTime=*/-1.0f, /*DepthPriority=*/0, /*Thickness=*/2.0f);

			DrawDebugString(GetWorld(), Centro + FVector(0.0f, 0.0f, 10.0f),
				FString::Printf(TEXT("(%d,%d)%s"), Column, Row, *Ocupantes),
				nullptr, CorDaCasa, /*Duration=*/0.0f, /*bDrawShadow=*/true, /*FontScale=*/1.1f);
		}
	}
}

void ABattleArena::NarrateEvent(const FBattleEvent& Event)
{
	const FPetPresentationInfo* Actor = PresentationsByPetId.Find(Event.ActorId);
	const FPetPresentationInfo* Target = PresentationsByPetId.Find(Event.TargetId);

	const FText Frase = FBattleNarration::Describe(Event,
		Actor ? Actor->Name : FString(),
		Target ? Target->Name : FString());

	if (Frase.IsEmpty())
	{
		return;
	}

	// Cor pelo lado de quem AGIU: numa troca rápida de golpes, saber de quem
	// foi a jogada importa mais que ler a frase inteira.
	FColor Cor = FColor::White;
	for (const TObjectPtr<APetView>& View : SpawnedPetViews)
	{
		if (View && View->GetPetId() == Event.ActorId)
		{
			Cor = View->GetSide() == 0 ? FColor::Cyan : FColor::Orange;
			break;
		}
	}

	FBattleNarrationFeed::Push(Frase, Cor);
}

FString ABattleArena::GetPresentationNameForPet(uint8 PetId) const
{
	const FPetPresentationInfo* Presentation = PresentationsByPetId.Find(PetId);
	return Presentation ? Presentation->Name : FString();
}

void ABattleArena::AddPlayerTwoAction(const FBattleAction& Action)
{
	if (PlayerTwoManualActions.Num() >= FTurnCommit::ActionsPerTurn)
	{
		FBattleDebugScreen::Show(TEXT("jogador 2 já tem 3 ações"), 6.0f, FColor::Orange, 801);
		return;
	}

	PlayerTwoManualActions.Add(Action);

	FBattleDebugScreen::Show(
		FString::Printf(TEXT("jogador 2: %d/3 ações escolhidas"), PlayerTwoManualActions.Num()),
		0.0f, FColor::Orange, /*Key=*/801);
}

void ABattleArena::ClearPlayerTwoActions()
{
	PlayerTwoManualActions.Reset();
	FBattleDebugScreen::Show(TEXT("jogador 2: 0/3 ações escolhidas"), 0.0f, FColor::Orange, 801);
}

void ABattleArena::SwapControlledPlayer()
{
	// Guarda o rascunho de quem está saindo e repõe o de quem entra.
	//
	// A primeira versão ZERAVA tudo ao trocar, para não aplicar ao pet errado
	// o que foi pensado para o outro. O motivo era legítimo; a solução,
	// grosseira: separar os rascunhos resolve o mesmo problema sem jogar fora
	// o trabalho de quem está jogando.
	if (PlayerActionQueue)
	{
		DraftsBySide[LocalPlayerSide] = PlayerActionQueue->GetConfirmedActions();
	}

	LocalPlayerSide = (LocalPlayerSide == 0) ? 1 : 0;

	if (PlayerActionQueue)
	{
		PlayerActionQueue->RestoreConfirmedActions(DraftsBySide[LocalPlayerSide]);
	}

	FBattleDebugScreen::Show(
		FString::Printf(TEXT("controlando o JOGADOR %d — o bot joga pelo outro"),
			static_cast<int32>(GetControlledPlayerNumber())),
		8.0f, LocalPlayerSide == 0 ? FColor::Cyan : FColor::Orange, /*Key=*/800);
}

void ABattleArena::SetControllingBothSides(bool bEnabled)
{
	bControlsBothSides = bEnabled;

	// Trocar de modo no meio de um turno deixaria uma escolha guardada sem
	// nunca ser resolvida, e o turno seguinte usaria o commit de antes.
	bAwaitingOpponentChoice = false;

	FBattleDebugScreen::Show(
		bEnabled
			? TEXT("controlando jogador 1 e jogador 2: escolha pelo 1, depois pelo 2")
			: TEXT("controle duplo desligado — o jogador 2 volta a decidir sozinho"),
		8.0f, FColor::Orange, 800);
}

// Ferramenta de desenvolvimento, e só. Fora do Shipping por compilação, não
// por disciplina: um jogo publicado onde qualquer um digita o comando e joga
// pelos dois lados não é o mesmo jogo.
#if !UE_BUILD_SHIPPING
namespace
{
	FAutoConsoleCommandWithWorldAndArgs GControlOpponentCommand(
		TEXT("bs.ControlOpponent"),
		TEXT("1 para escolher as ações dos DOIS lados; 0 para o oponente voltar a decidir sozinho."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(
			[](const TArray<FString>& Args, UWorld* World)
			{
				if (!World)
				{
					return;
				}

				const bool bEnable = Args.Num() == 0 || Args[0] != TEXT("0");
				for (TActorIterator<ABattleArena> It(World); It; ++It)
				{
					It->SetControllingBothSides(bEnable);
				}
			}));
}
#endif // !UE_BUILD_SHIPPING

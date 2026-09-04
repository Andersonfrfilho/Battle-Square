// Copyright 2026 Anderson. All Rights Reserved.

#include "World/WorldExplorerCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/StaticMeshComponent.h"
#include "Debug/BattleDebugScreen.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "World/WorldTraversalMotion.h"
#include "World/EncounterDetectionComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "UI/WorldMapScreen.h"
#include "Debug/BattleDebugScreen.h"
#include "Environment/ForestBackdrop.h"
#include "EngineUtils.h"
#include "World/WorldObstacleBreaking.h"
#include "World/MountedMovement.h"
#include "World/MountFatigue.h"
#include "World/MountEligibility.h"
#include "World/ResourceCatalog.h"
#include "World/ResourceGathering.h"
#include "Environment/IslandGeography.h"
#include "Net/BattleSquareGameMode.h"
#include "Environment/RegionResidency.h"
#include "World/WorldCellKey.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "InputModifiers.h"
#include "InputCoreTypes.h"

AWorldExplorerCharacter::AWorldExplorerCharacter()
{
	// Sem isto o guarda-queda nunca roda, e a queda infinita continua.
	PrimaryActorTick.bCanEverTick = true;

	// CORPO VISÍVEL. ACharacter traz malha esqueletal sem asset: o jogador
	// dirigiria um corpo invisível em terceira pessoa, e nenhum teste acusa.
	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(RootComponent);
	// Sem colisão: quem colide é a cápsula do Character. Uma segunda forma
	// colidindo brigaria com ela e produziria empurrões inexplicáveis.
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	FacingMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FacingMarker"));
	FacingMarker->SetupAttachment(RootComponent);
	FacingMarker->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CorpoMesh(
		TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (CorpoMesh.Succeeded())
	{
		BodyMesh->SetStaticMesh(CorpoMesh.Object);
		BodyMesh->SetRelativeScale3D(FVector(BodyScale));
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> MarcaMesh(
		TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (MarcaMesh.Succeeded())
	{
		// A esfera é simétrica: sem uma marca à frente, não dá para saber para
		// onde o personagem está virado — e direção é o que mais importa num
		// mundo onde encostar em alguém inicia uma batalha.
		FacingMarker->SetStaticMesh(MarcaMesh.Object);
		FacingMarker->SetRelativeScale3D(FVector(0.25f, 0.25f, 0.25f));
		FacingMarker->SetRelativeLocation(FVector(FacingMarkerForwardUnits, 0.0f, 0.0f));
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> CorpoMaterial(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (CorpoMaterial.Succeeded())
	{
		BodyMesh->SetMaterial(0, CorpoMaterial.Object);
		FacingMarker->SetMaterial(0, CorpoMaterial.Object);
	}

	// DP-trav-04: a câmera orbita livre e o corpo vira para onde anda. Sem
	// isto o personagem fica travado apontando para a câmera e "anda de
	// lado", que é comportamento de shooter, não de explorador.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->bOrientRotationToMovement = true;
		Movement->RotationRate = FRotator(0.0, WorldTraversal::CharacterRotationRateYawDegreesPerSecond, 0.0);
	}

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = WorldTraversal::CameraArmLengthUnits;
	CameraBoom->bUsePawnControlRotation = true;
	// É isto, e só isto, que impede a câmera de entrar na parede.
	CameraBoom->bDoCollisionTest = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	// A rotação vive num lugar só: o braço.
	FollowCamera->bUsePawnControlRotation = false;

	// DP-trav-05: encontro é o componente da feature anterior, sem exceção.
	EncounterDetection = CreateDefaultSubobject<UEncounterDetectionComponent>(TEXT("EncounterDetection"));
}

void AWorldExplorerCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	UE_LOG(LogTemp, Display, TEXT("[TRAVERSAL] NotifyControllerChanged: controller=%s IMC=%s Move=%s Look=%s"),
		PlayerController ? TEXT("ok") : TEXT("NULO"),
		TraversalMappingContext ? TEXT("ok") : TEXT("NULO"),
		MoveAction ? TEXT("ok") : TEXT("NULO"),
		LookAction ? TEXT("ok") : TEXT("NULO"));

	if (!PlayerController)
	{
		return;
	}

	PlayerController->PlayerCameraManager->ViewPitchMin = WorldTraversal::CameraPitchMinDegrees;
	PlayerController->PlayerCameraManager->ViewPitchMax = WorldTraversal::CameraPitchMaxDegrees;

	// Contexto ausente é configuração incompleta, não motivo para cair.
	if (!TraversalMappingContext)
	{
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer());
	if (!Subsystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TRAVERSAL] subsistema de Enhanced Input NULO"));
		return;
	}

	const int32 MappingsNoAsset = TraversalMappingContext->GetMappings().Num();
	UE_LOG(LogTemp, Display, TEXT("[TRAVERSAL] mapeamentos no asset: %d"), MappingsNoAsset);

	// Reserva: um asset sem mapeamento não move ninguém, e descobrir isso só
	// pelo silêncio custou caro. Construir o contexto pela API da engine
	// (MapKey) não depende de como a ferramenta de autoria escreveu o asset.
	UInputMappingContext* ContextToApply = TraversalMappingContext;
	if (MappingsNoAsset == 0 && MoveAction && LookAction)
	{
		UInputMappingContext* Fallback = NewObject<UInputMappingContext>(this);
		Fallback->MapKey(MoveAction, EKeys::W);
		Fallback->MapKey(MoveAction, EKeys::S).Modifiers.Add(NewObject<UInputModifierNegate>(Fallback));

		FEnhancedActionKeyMapping& Right = Fallback->MapKey(MoveAction, EKeys::D);
		Right.Modifiers.Add(NewObject<UInputModifierSwizzleAxis>(Fallback));

		FEnhancedActionKeyMapping& Left = Fallback->MapKey(MoveAction, EKeys::A);
		Left.Modifiers.Add(NewObject<UInputModifierSwizzleAxis>(Fallback));
		Left.Modifiers.Add(NewObject<UInputModifierNegate>(Fallback));

		Fallback->MapKey(LookAction, EKeys::Mouse2D);

		ContextToApply = Fallback;
		UE_LOG(LogTemp, Warning, TEXT("[TRAVERSAL] asset sem mapeamentos — usando contexto de RESERVA construído em C++ (%d)"),
			Fallback->GetMappings().Num());
	}

	Subsystem->AddMappingContext(ContextToApply, /*Priority=*/0);
	UE_LOG(LogTemp, Display, TEXT("[TRAVERSAL] mapping context REGISTRADO (%d mapeamentos)"),
		ContextToApply->GetMappings().Num());
}

void AWorldExplorerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	UE_LOG(LogTemp, Display, TEXT("[TRAVERSAL] SetupPlayerInputComponent: enhanced=%s Move=%s"),
		EnhancedInput ? TEXT("ok") : TEXT("NULO"), MoveAction ? TEXT("ok") : TEXT("NULO"));
	if (!EnhancedInput)
	{
		return;
	}

	// Sonda crua, no caminho legado: dispara se a tecla chegar ao jogo, mesmo
	// que o Enhanced Input não roteie nada.
	PlayerInputComponent->BindKey(EKeys::W, IE_Pressed, this, &AWorldExplorerCharacter::LogRawKeyProbe);

	// Pular, correr e trocar de câmera por TECLA DIRETA, e não por asset de
	// Enhanced Input.
	//
	// DP-trav-06 manda as teclas virem de asset, e continua valendo para andar
	// e olhar — que já têm assets. Criar asset novo depende do editor, e a
	// ausência dele degradaria para "não pula, não corre", exatamente o tipo de
	// recurso invisível que este projeto passou o dia removendo. Quando os
	// assets existirem, eles se somam a isto sem conflito.
	PlayerInputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindKey(EKeys::SpaceBar, IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindKey(EKeys::LeftShift, IE_Pressed, this, &AWorldExplorerCharacter::StartSprinting);
	PlayerInputComponent->BindKey(EKeys::LeftShift, IE_Released, this, &AWorldExplorerCharacter::StopSprinting);

	PlayerInputComponent->BindKey(EKeys::Y, IE_Pressed, this, &AWorldExplorerCharacter::CycleCameraMode);

	// M abre e fecha o mapa completo. Ligado AQUI, no pawn, e não no ouvinte
	// de Slate que a depuração usa: mapa é jogo, e precisa existir em
	// Shipping. Este é o mesmo caminho de input do pulo e da corrida, que
	// comprovadamente funciona neste projeto — as teclas que falharam três
	// vezes eram as amarradas fora do pawn.
	PlayerInputComponent->BindKey(EKeys::M, IE_Pressed, this, &AWorldExplorerCharacter::ToggleWorldMap);

	// Botão esquerdo: golpe no mundo. É o que derruba árvore e pedra — a
	// mesma regra da arena, onde força derruba o obstáculo do tabuleiro. Um
	// pet que derruba a pedra lá e não derruba a árvore aqui ensinaria duas
	// coisas contraditórias sobre o mesmo bicho.
	PlayerInputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed,
		this, &AWorldExplorerCharacter::StrikeForward);

	if (MoveAction)
	{
		EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AWorldExplorerCharacter::HandleMove);
	}

	if (LookAction)
	{
		EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &AWorldExplorerCharacter::HandleLook);
	}
}

void AWorldExplorerCharacter::HandleMove(const FInputActionValue& Value)
{
	const AController* OwningController = GetController();
	if (!OwningController)
	{
		return;
	}

	FWorldTraversalMotionParams Params;
	Params.MovementInput = Value.Get<FVector2D>();
	Params.CameraRotation = OwningController->GetControlRotation();

	const FVector MoveDirection = FWorldTraversalMotion::ComputeMoveDirection(Params);
	UE_LOG(LogTemp, Display, TEXT("[TRAVERSAL] HandleMove entrada=(%.2f,%.2f) direcao=(%.2f,%.2f,%.2f)"),
		Params.MovementInput.X, Params.MovementInput.Y, MoveDirection.X, MoveDirection.Y, MoveDirection.Z);
	if (MoveDirection.IsNearlyZero())
	{
		return;
	}

	AddMovementInput(MoveDirection);
}

void AWorldExplorerCharacter::HandleLook(const FInputActionValue& Value)
{
	const FVector2D LookInput = Value.Get<FVector2D>();
	AddControllerYawInput(LookInput.X);
	AddControllerPitchInput(LookInput.Y);
}

void AWorldExplorerCharacter::LogRawKeyProbe()
{
	UE_LOG(LogTemp, Warning, TEXT("[TRAVERSAL] SONDA: a tecla W CHEGOU ao jogo (binding cru)."));
}

void AWorldExplorerCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Azul, contra o laranja dos inimigos: a cor é o que separa você deles a
	// distância, antes de qualquer detalhe de forma.
	if (BodyMesh)
	{
		if (UMaterialInstanceDynamic* Material = BodyMesh->CreateDynamicMaterialInstance(0))
		{
			Material->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.1f, 0.45f, 0.95f));
		}
	}
	if (FacingMarker)
	{
		if (UMaterialInstanceDynamic* Material = FacingMarker->CreateDynamicMaterialInstance(0))
		{
			Material->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.95f, 0.95f, 0.95f));
		}
	}
}

void AWorldExplorerCharacter::StrikeForward()
{
	UWorld* Mundo = GetWorld();
	if (!Mundo)
	{
		return;
	}

	AForestBackdrop* Mata = nullptr;
	for (TActorIterator<AForestBackdrop> It(Mundo); It; ++It)
	{
		if (IsValid(*It))
		{
			Mata = *It;
			break;
		}
	}

	if (!Mata)
	{
		return;
	}

	TArray<int32> Chaves;
	const TArray<FWorldObstacleCandidate> Candidatos = Mata->CollectObstaclesNear(
		GetActorLocation(), FWorldObstacleBreaking::ReachUnits, Chaves);

	const int32 Alvo = FWorldObstacleBreaking::FindTarget(
		GetActorLocation(), GetActorForwardVector(), Candidatos);

	if (Alvo == INDEX_NONE || !Chaves.IsValidIndex(Alvo))
	{
		// GOLPE NO VAZIO É DITO. Sem isto, bater sem alvo e bater num alvo
		// indestrutível produzem o mesmo silêncio, e o jogador conclui que o
		// botão não funciona.
		FBattleDebugScreen::Show(TEXT("golpe no vazio"), 2.0f, FColor::Silver, /*Key=*/760);
		return;
	}

	const int32 Dano = FWorldObstacleBreaking::DamageFromMusculature(StrikeMusculature);
	const bool bCaiu = Mata->DamageObstacle(Chaves[Alvo], Dano);

	// O PROGRESSO aparece, e não só o fim. Bater três vezes numa árvore sem
	// nenhum sinal é indistinguível de bater no vazio — e aí o jogador para
	// antes do golpe que a derrubaria.
	const int32 QueFalta = FMath::Max(0, Candidatos[Alvo].RemainingHealth - Dano);
	FBattleDebugScreen::Show(
		bCaiu
			? TEXT("derrubou!")
			: *FString::Printf(TEXT("golpe: %d de dano, falta %d"), Dano, QueFalta),
		2.0f, bCaiu ? FColor::Green : FColor::Yellow, /*Key=*/760);

	// A árvore CAIU deixa marca no mundo compartilhado (MV3): grava no servidor
	// carimbada pela posição, para o próximo jogador ver o mesmo vazio. A
	// identidade é a célula quantizada, a mesma do replantio.
	if (bCaiu)
	{
		if (ABattleSquareGameMode* Modo = GetWorld()->GetAuthGameMode<ABattleSquareGameMode>())
		{
			const FVector2D Onde(Candidatos[Alvo].Location);
			const FString ChunkKey = WorldCellKey::ChunkKeyOf(RegionResidency::ChunkAt(Onde));
			const FString CellKey = WorldCellKey::CellKeyOf(Onde, Modo->WorldSceneryCellSizeUnits);
			Modo->RecordTreeCutInBackground(ChunkKey, CellKey);
		}
	}

	// A ARVORE DERRUBADA RENDE MADEIRA (MV7, decisao 68): quanto depende da
	// ferramenta (machado) e de o pet ajudar (68-c) — pela regra pura da
	// colheita, nao por uma conta solta aqui.
	if (bCaiu)
	{
		ResourceGathering::FGatherContext Contexto;
		Contexto.Tool = CurrentTool;
		Contexto.bPetHelps = bPetNearbyToGather;
		const int32 Colhido = ResourceGathering::Yield(EWorldResource::Madeira, Contexto);
		GatheredWood += Colhido;
		FBattleDebugScreen::Show(
			Colhido > 0
				? *FString::Printf(TEXT("colheu %d de madeira (total: %d)%s"),
					Colhido, GatheredWood, bPetNearbyToGather ? TEXT(" — o pet ajudou") : TEXT(""))
				: TEXT("sem machado: a arvore caiu, mas nao rendeu madeira"),
			3.0f, FColor(200, 170, 120), /*Key=*/734);
	}
}

void AWorldExplorerCharacter::ToggleWorldMap()
{
	FWorldMapScreen::ToggleFullMap();
}

void AWorldExplorerCharacter::RememberSafeGround()
{
	LastSafeGround = GetActorLocation();
	bHasSafeGround = true;
}

void AWorldExplorerCharacter::CheckFallGuard()
{
	if (GetActorLocation().Z > FallGuardZLimit)
	{
		return;
	}

	// Sem marca conhecida, o ponto de partida é o melhor palpite disponível —
	// e é infinitamente melhor que continuar caindo.
	const FVector Destino = bHasSafeGround
		? LastSafeGround + FVector(0.0f, 0.0f, RespawnLiftUnits)
		: FVector(0.0f, 0.0f, RespawnLiftUnits);

	SetActorLocation(Destino);

	// Zerar a velocidade: sem isso ele reaparece já caindo na mesma
	// velocidade acumulada e atravessa o piso de novo no quadro seguinte.
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
	}

	FBattleDebugScreen::Show(
		TEXT("caiu para fora do mundo — devolvido à última terra firme"),
		6.0f, FColor::Orange, /*Key=*/700);
}

void AWorldExplorerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Terra firme só conta quando ele está DE FATO no chão: gravar durante a
	// queda registraria o próprio buraco como lugar seguro.
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		if (Movement->IsMovingOnGround())
		{
			RememberSafeGround();
		}
	}

	// CANSAÇO DE MONTARIA (MT2): so montado, e proporcional ao PESO de custo do
	// trecho — subir cansa mais que descer, pelos pesos que a trilha ja usa. E
	// valor proprio, alheio ao combate: batalha nenhuma o toca.
	if (WalkSpeedBeforeMount > 0.0f)
	{
		const FVector Agora = GetActorLocation();
		if (!LastFatiguePos.IsZero())
		{
			const float Horizontal = FVector2D::Distance(
				FVector2D(Agora), FVector2D(LastFatiguePos));
			if (Horizontal > 1.0f)
			{
				const float Peso = (Agora.Z >= LastFatiguePos.Z)
					? IslandGeography::UphillCostWeight()
					: IslandGeography::DownhillCostWeight();
				const float FadigaTrecho = MountFatigue::FatigueForStretch(
					Horizontal, Peso, MountFatigueRatePerUnit);
				// MT3: o PESO do pet montado multiplica o cansaco — nunca bloqueia,
				// so cansa mais (o multiplicador tem teto finito). Peso zero (nenhum
				// pet definido, dado antigo) cai no neutro.
				const float MultPeso = (MountedPetWeight > 0.0f)
					? MountFatigue::WeightMultiplier(
						MountedPetWeight, MountReferenceWeight, MountMaxWeightMultiplier)
					: 1.0f;
				AccumulatedMountFatigue += MountFatigue::FatigueWithWeight(FadigaTrecho, MultPeso);
				FBattleDebugScreen::Show(
					FString::Printf(TEXT("cansaco (montado): %.0f"), AccumulatedMountFatigue),
					0.0f, FColor(210, 180, 120), /*Key=*/733);
			}
		}
		LastFatiguePos = Agora;
	}
	else
	{
		LastFatiguePos = FVector::ZeroVector;
	}

	CheckFallGuard();
}

void AWorldExplorerCharacter::SetMounted(bool bWantMounted)
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (!Movement)
	{
		return;
	}

	const bool bMounted = WalkSpeedBeforeMount > 0.0f;
	if (bWantMounted == bMounted)
	{
		return;
	}

	if (bWantMounted)
	{
		// MT4: so monta pet que o DADO diz montavel. Recusado, a tela diz por que
		// — e nada muda. Dado antigo (default false) cai aqui, nunca monta por acaso.
		if (!MountEligibility::CanMount(bCurrentPetMountable))
		{
			FBattleDebugScreen::Show(
				MountEligibility::RefusalReason().ToString(),
				4.0f, FColor(210, 150, 120), /*Key=*/732);
			return;
		}

		// Guarda a velocidade a pe e sobe pela conta de montaria (MT1) — o
		// ganho vem da fonte unica, nao de um multiplicador solto aqui.
		WalkSpeedBeforeMount = Movement->MaxWalkSpeed;
		Movement->MaxWalkSpeed =
			MountedMovement::MountedBaseSpeed(WalkSpeedBeforeMount, MountSpeedMultiplier);
		FBattleDebugScreen::Show(TEXT("montado"), 0.0f, FColor(150, 220, 150), /*Key=*/732);
	}
	else
	{
		Movement->MaxWalkSpeed = WalkSpeedBeforeMount;
		WalkSpeedBeforeMount = 0.0f;
		FBattleDebugScreen::Show(TEXT("a pe"), 0.0f, FColor::Silver, /*Key=*/732);
	}
}

void AWorldExplorerCharacter::StartSprinting()
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (!Movement || WalkSpeedBeforeSprint > 0.0f)
	{
		return;
	}

	// Guarda a velocidade ANTES de multiplicar. Multiplicar sobre o valor
	// atual faria cada toque acelerar de novo, e em três toques o explorador
	// atravessaria o mundo.
	WalkSpeedBeforeSprint = Movement->MaxWalkSpeed;
	Movement->MaxWalkSpeed = WalkSpeedBeforeSprint * SprintSpeedMultiplier;

	FBattleDebugScreen::Show(TEXT("correndo"), 0.0f, FColor::Cyan, /*Key=*/730);
}

void AWorldExplorerCharacter::StopSprinting()
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (!Movement || WalkSpeedBeforeSprint <= 0.0f)
	{
		return;
	}

	Movement->MaxWalkSpeed = WalkSpeedBeforeSprint;
	WalkSpeedBeforeSprint = 0.0f;

	FBattleDebugScreen::Show(TEXT("andando"), 0.0f, FColor::Silver, /*Key=*/730);
}

void AWorldExplorerCharacter::CycleCameraMode()
{
	CameraMode = static_cast<ECameraMode>(
		(static_cast<uint8>(CameraMode) + 1) % static_cast<uint8>(ECameraMode::MAX));
	ApplyCameraMode();
}

void AWorldExplorerCharacter::ApplyCameraMode()
{
	if (!CameraBoom || !BodyMesh)
	{
		return;
	}

	const TCHAR* Nome = TEXT("terceira pessoa");

	switch (CameraMode)
	{
	case ECameraMode::PrimeiraPessoa:
		CameraBoom->TargetArmLength = FirstPersonArmLengthUnits;
		CameraBoom->SetRelativeRotation(FRotator::ZeroRotator);
		// O próprio corpo some: com o braço em zero, a câmera fica DENTRO da
		// esfera e o jogador veria o interior dela.
		BodyMesh->SetVisibility(false);
		Nome = TEXT("primeira pessoa");
		break;

	case ECameraMode::DeCima:
		CameraBoom->TargetArmLength = TopDownArmLengthUnits;
		CameraBoom->SetRelativeRotation(FRotator(-70.0f, 0.0f, 0.0f));
		BodyMesh->SetVisibility(true);
		Nome = TEXT("de cima");
		break;

	default:
		CameraBoom->TargetArmLength = WorldTraversal::CameraArmLengthUnits;
		CameraBoom->SetRelativeRotation(FRotator::ZeroRotator);
		BodyMesh->SetVisibility(true);
		break;
	}

	FBattleDebugScreen::Show(FString::Printf(TEXT("câmera: %s"), Nome),
		0.0f, FColor::Cyan, /*Key=*/731);
}

// A MONTARIA pelo console (MT1): monta/desmonta sem depender de input bindado.
static FAutoConsoleCommandWithWorldAndArgs GMontarCommand(
	TEXT("bs.Montar"),
	TEXT("bs.Montar [on|off] — monta (on) ou desmonta (off); montado anda mais rapido."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(
		[](const TArray<FString>& Args, UWorld* World)
		{
			const APlayerController* Ctrl = World ? World->GetFirstPlayerController() : nullptr;
			AWorldExplorerCharacter* Explorador =
				Ctrl ? Cast<AWorldExplorerCharacter>(Ctrl->GetPawn()) : nullptr;
			if (!Explorador)
			{
				return;
			}
			Explorador->SetMounted(Args.Num() < 1 || Args[0].Equals(TEXT("on"), ESearchCase::IgnoreCase));
		}));

// Marca o pet candidato como montavel (MT4, dev): sem isto, bs.Montar recusa.
static FAutoConsoleCommandWithWorldAndArgs GPetMontavelCommand(
	TEXT("bs.PetMontavel"),
	TEXT("bs.PetMontavel [on|off] — marca se o pet candidato pode ser montado (MT4)."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(
		[](const TArray<FString>& Args, UWorld* World)
		{
			const APlayerController* Ctrl = World ? World->GetFirstPlayerController() : nullptr;
			AWorldExplorerCharacter* Explorador =
				Ctrl ? Cast<AWorldExplorerCharacter>(Ctrl->GetPawn()) : nullptr;
			if (Explorador)
			{
				Explorador->SetCurrentPetMountable(
					Args.Num() < 1 || Args[0].Equals(TEXT("on"), ESearchCase::IgnoreCase));
			}
		}));

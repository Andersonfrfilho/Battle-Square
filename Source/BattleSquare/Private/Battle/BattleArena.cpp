// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattleArena.h"

#include "Environment/ForestBackdrop.h"
#include "Environment/SceneLighting.h"
#include "Battle/PetOwnerView.h"
#include "UI/BattleResultWidget.h"
#include "Blueprint/UserWidget.h"
#include "Misc/Paths.h"
#include "Misc/ConfigCacheIni.h"
#include "Balance/PetSkillCatalog.h"
#include "Balance/ArenaLayoutCatalog.h"
#include "EngineUtils.h"
#include "Battle/BattleNarration.h"

#include "Debug/BattleDebugScreen.h"
#include "DrawDebugHelpers.h"

DEFINE_LOG_CATEGORY(LogBattleArena);
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Components/PrimitiveComponent.h"
#include "CollisionQueryParams.h"
#include "UObject/ConstructorHelpers.h"
#include "Battle/TacticalOpponentAI.h"
#include "Battle/BattleResolver.h"
#include "Battle/BattleOutcome.h"
#include "Meta/PetCollectionService.h"
#include "Meta/PetProgressionService.h"

namespace ArenaGeometria
{
	/** Aresta do cubo da engine. Toda escala abaixo é fração disto. */
	constexpr float CuboDaEngine = 100.0f;

	/**
	 * Fundo comum de todas as lajes. A altura por terreno mexe no TOPO, nunca
	 * no fundo: fundos diferentes deixariam o tabuleiro vazado por baixo, que
	 * é justamente o ângulo da câmera do diorama.
	 */
	constexpr float FundoDoTabuleiro = -80.0f;

	/**
	 * Topo da laje por terreno. Neutro fica logo ABAIXO de zero para o pet
	 * não afundar; água afunda de verdade (é onde se submerge) e casa
	 * bloqueada sobe, porque ninguém pisa nela.
	 */
	constexpr float SuperficieNeutra = -4.0f;
	constexpr float SuperficieDaAgua = -22.0f;
	constexpr float SuperficieBloqueada = 22.0f;

	/**
	 * Casa sem regra nenhuma NÃO tem vão: ela é o próprio chão da clareira, e
	 * quem delimita a grade é a linha desenhada. O vão em toda casa era o que
	 * fazia o tabuleiro ler como placa quadriculada posta sobre o cenário.
	 *
	 * Só o que carrega regra fica recuado — e aí o recuo tem serventia: a
	 * água, o dano, o bônus e o bloqueio precisam ser achados de relance.
	 */
	constexpr float ProporcaoDaLajeNeutra = 1.0f;
	constexpr float ProporcaoDaLajeComRegra = 0.90f;

	constexpr float EspessuraDoChao = 24.0f;
	constexpr float MargemDoChao = 95.0f;

	/**
	 * Coordenada do CENTRO da grade naquele eixo, em casas.
	 *
	 * Numa grade de lado ímpar cai numa casa (1.0 para 3); numa de lado par
	 * cai ENTRE duas (1.5 para 4). Arredondar para casa inteira jogaria o
	 * tabuleiro meia casa para fora do centro da clareira em todo campo par.
	 */
	float CentroDaGrade(int32 Lado)
	{
		return (static_cast<float>(Lado) - 1.0f) * 0.5f;
	}

	float ProporcaoDaLajePara(uint8 Propriedade)
	{
		return static_cast<ECellProperty>(Propriedade) == ECellProperty::None
			? ProporcaoDaLajeNeutra
			: ProporcaoDaLajeComRegra;
	}

	/** A linha desenhada assenta ESTE tanto acima da superfície da laje. */
	constexpr float FolgaDaGradeDesenhada = 4.0f;

	/** De onde e até onde a sonda procura o chão do mundo sob o encontro. */
	constexpr float AlturaDaSondaDeChao = 200.0f;
	constexpr float AlcanceDaSondaDeChao = 5000.0f;
}

void ABattleArena::ResolveConfiguredGridSize()
{
	if (GConfig)
	{
		GConfig->GetInt(TEXT("/Script/BattleSquare.BattleArena"),
			TEXT("GridColumns"), GridColumns, GGameIni);
		GConfig->GetInt(TEXT("/Script/BattleSquare.BattleArena"),
			TEXT("GridRows"), GridRows, GGameIni);
	}

	// Recorta em vez de recusar: número errado num .ini não deve impedir a
	// arena de existir — deve virar a grade válida mais próxima.
	GridColumns = FMath::Clamp(GridColumns, BattleGridMinSide, BattleGridMaxSide);
	GridRows = FMath::Clamp(GridRows, BattleGridMinSide, BattleGridMaxSide);
}

int32 ABattleArena::GetActiveGridColumns() const
{
	// Durante a batalha manda o ESTADO, porque é a grade dele que entrou no
	// hash; fora dela, a configurada — que é o que o construtor usou para
	// criar as lajes.
	return CurrentState.Pets.IsEmpty()
		? GridColumns
		: static_cast<int32>(CurrentState.GridColumns);
}

int32 ABattleArena::GetActiveGridRows() const
{
	return CurrentState.Pets.IsEmpty()
		? GridRows
		: static_cast<int32>(CurrentState.GridRows);
}

ABattleArena::ABattleArena()
{
	PrimaryActorTick.bCanEverTick = true;

	// Lido do .ini AQUI, e não deixado para o carregamento normal de
	// UPROPERTY(config): as lajes são subobjetos criados neste construtor, e
	// nesse momento o CDO ainda não recebeu a configuração. Sem esta leitura
	// o campo do .ini mudaria o número guardado e não o número de lajes.
	ResolveConfiguredGridSize();

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
	//
	// Recuada, erguida e MENOS mergulhada do que era (-38, 45mm): com o
	// mergulho antigo o quadro inteiro era tabuleiro, e o que estivesse
	// atrás dele subia para fora do topo da tela. Não havia lugar onde um
	// fundo pudesse existir — daí "não tem fundo".
	//
	// Aqui o tabuleiro ocupa a faixa de baixo e sobra quadro para a mata.
	// O horizonte continua fora: enquadrar 3x3 legível E horizonte exige uma
	// câmera tão rasa que as casas ficam de perfil, e a leitura da grade é
	// regra, não decoração.
	ArenaCamera->SetRelativeLocation(FVector(-880.0f, 0.0f, 620.0f));
	ArenaCamera->SetRelativeRotation(FRotator(-26.0f, 0.0f, 0.0f));
	ArenaCamera->FieldOfView = 58.0f;
	ArenaCamera->AspectRatio = 16.0f / 9.0f;

	BuildArenaGeometry();
}

float ABattleArena::GetCellSurfaceHeight(uint8 CellProperty)
{
	switch (static_cast<ECellProperty>(CellProperty))
	{
	case ECellProperty::Water:
		return ArenaGeometria::SuperficieDaAgua;
	case ECellProperty::Blocked:
		return ArenaGeometria::SuperficieBloqueada;
	default:
		return ArenaGeometria::SuperficieNeutra;
	}
}

void ABattleArena::BeginPlay()
{
	Super::BeginPlay();

	// Erguer o ator INTEIRO: GetCellWorldLocation deriva da posição dele, então
	// pets, grade desenhada e câmera sobem juntos. Erguer só as lajes deixaria
	// os pets flutuando no antigo plano zero.
	AddActorWorldOffset(FVector(0.0f, 0.0f, BoardElevation));

	// A luz vem ANTES da mata: sem sol, o verde das folhas chega na tela
	// lavado de azul, e a mata parece um problema de material que não é.
	SpawnSceneLighting();
	RefreshTileVisuals();
	SpawnForestBackdrop();
}

void ABattleArena::RefreshClearingGround()
{
	using namespace ArenaGeometria;

	if (!ArenaFloorMesh)
	{
		return;
	}

	// A clareira é REDONDA: o contorno quadrado era metade da impressão de
	// "placa posta em cima do cenário". Raio pela diagonal, senão os cantos
	// do tabuleiro ficam pendurados fora da terra.
	// Raio pela DIAGONAL do tabuleiro, que num campo retangular não é o
	// mesmo que meia largura: usar só um dos lados deixaria os cantos do
	// eixo maior pendurados fora da terra.
	const float MeiaLargura = CellSize * static_cast<float>(GetActiveGridColumns()) * 0.5f;
	const float MeiaAltura = CellSize * static_cast<float>(GetActiveGridRows()) * 0.5f;
	const float RaioDaClareira =
		FMath::Sqrt(MeiaLargura * MeiaLargura + MeiaAltura * MeiaAltura) + MargemDoChao;

	// Topo NO NÍVEL da casa neutra: assim a casa sem regra fica rente à terra
	// e o tabuleiro deixa de ser um platô. O que sobra em relevo é só o que
	// tem regra — água afunda, pedra sobe.
	const float TopoDaTerra = SuperficieNeutra;
	// Fundo no chão da mata, o mesmo BoardElevation que ergueu o ator: sem
	// isto o tabuleiro fica FLUTUANDO sobre o cenário.
	const float BaseDaTerra = -BoardElevation;
	const float AlturaDaTerra = FMath::Max(TopoDaTerra - BaseDaTerra, EspessuraDoChao);

	ArenaFloorMesh->SetRelativeScale3D(FVector(
		(RaioDaClareira * 2.0f) / CuboDaEngine,
		(RaioDaClareira * 2.0f) / CuboDaEngine,
		AlturaDaTerra / CuboDaEngine));
	ArenaFloorMesh->SetRelativeLocation(
		FVector(0.0f, 0.0f, TopoDaTerra - AlturaDaTerra * 0.5f));
}

void ABattleArena::SpawnSceneLighting()
{
	UWorld* World = GetWorld();
	if (!World || SceneLighting)
	{
		return;
	}

	// Mapa que já tem sol próprio manda: dois sóis somam intensidade e
	// devolvem a cena lavada por outro caminho.
	if (ABattleSceneLighting::WorldAlreadyHasSun(World))
	{
		return;
	}

	FActorSpawnParameters Parametros;
	Parametros.Owner = this;
	SceneLighting = World->SpawnActor<ABattleSceneLighting>(
		ABattleSceneLighting::StaticClass(), GetActorLocation(), FRotator::ZeroRotator, Parametros);

	FBattleDebugScreen::Show(
		TEXT("cenario: sol e ceu acesos por codigo (o mapa nao tem luz)"),
		0.0f, FColor::Yellow, /*Key=*/22);
}

void ABattleArena::SpawnOwnerViews(const FBattleState& InitialState)
{
	using namespace ArenaGeometria;

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (const TObjectPtr<APetOwnerView>& Anterior : SpawnedOwnerViews)
	{
		if (Anterior)
		{
			Anterior->Destroy();
		}
	}
	SpawnedOwnerViews.Reset();

	// Um dono por LADO que tem pet em campo — nunca um por pet: dois pets do
	// mesmo treinador são dois bichos, não dois donos.
	TSet<uint8> LadosEmCampo;
	for (const FPetState& Pet : InitialState.Pets)
	{
		LadosEmCampo.Add(Pet.Side);
	}

	// O dono fica além da BORDA LATERAL (eixo das colunas, que é o -Y/+Y da
	// tela), então quem manda aqui é a largura, não a altura.
	const float MeiaLargura = CellSize * static_cast<float>(GetActiveGridColumns()) * 0.5f;
	const float AfastamentoDoDono = MeiaLargura + MargemDoChao * 0.5f;

	for (const uint8 Lado : LadosEmCampo)
	{
		// Side 0 é a esquerda, e na tela a esquerda é -Y (a câmera olha ao
		// longo de +X). O mesmo mapeamento de GetCellWorldLocation.
		const float DeslocamentoY = (Lado == 0 ? -1.0f : 1.0f) * AfastamentoDoDono;
		const FVector Onde = GetActorLocation() + FVector(0.0f, DeslocamentoY, SuperficieNeutra);
		// Virado para o centro: dono de costas para a briga não é dono.
		const FRotator Olhando = FVector(0.0f, -DeslocamentoY, 0.0f).Rotation();

		APetOwnerView* Dono = World->SpawnActor<APetOwnerView>(
			APetOwnerView::StaticClass(), Onde, Olhando);
		if (!Dono)
		{
			continue;
		}
		Dono->SetSide(Lado);
		SpawnedOwnerViews.Add(Dono);
	}
}

void ABattleArena::SpawnForestBackdrop()
{
	UWorld* World = GetWorld();
	if (!World || ForestBackdrop)
	{
		return;
	}

	// Ao NÍVEL do chão, não ao do tabuleiro: a arena foi erguida
	// BoardElevation acima do plano do mundo, e a mata cresce no plano.
	// Descer aqui deriva do mesmo número que ergueu — dois valores
	// discordariam na primeira edição (L-032/L-033).
	const FVector ChaoDaMata = GetActorLocation() - FVector(0.0f, 0.0f, BoardElevation);

	FActorSpawnParameters Parametros;
	Parametros.Owner = this;
	ForestBackdrop = World->SpawnActor<AForestBackdrop>(
		AForestBackdrop::StaticClass(), ChaoDaMata, FRotator::ZeroRotator, Parametros);
	if (!ForestBackdrop)
	{
		return;
	}

	const FVector CameraLocal = ArenaCamera ? ArenaCamera->GetRelativeLocation() : FVector::ZeroVector;
	ForestBackdrop->BuildForest(CellSize, static_cast<uint32>(ForestSeed),
		FVector2D(CameraLocal.X, CameraLocal.Y));

	// Painel de desenvolvimento, não texto de jogador: FString aqui é a
	// mesma escolha das outras linhas de diagnóstico, e some no Shipping.
	FBattleDebugScreen::Show(
		FString::Printf(TEXT("cenario: mata de floresta com %d elementos"),
			ForestBackdrop->GetPlantedCount()),
		0.0f, FColor::Green, /*Key=*/21);
}

void ABattleArena::BuildArenaGeometry()
{
	using namespace ArenaGeometria;

	// Conteúdo da engine, não vendorizado — mesmo princípio de AD-019.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CuboDaArena(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CilindroDaArena(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

	// Caminhos dos materiais autorados em /Game/Arena/Materials. Soft: o CDO
	// não carrega textura nenhuma no boot do módulo, e o editor continua
	// podendo trocar a paleta sem recompilar.
	// Chão e casa neutra saem da PALETA DA MATA, não da paleta da arena: é o
	// que faz o tabuleiro ser parte do cenário em vez de um objeto pousado
	// nele. O que continua com material próprio é o que carrega regra.
	FloorMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Environment/Nature/dirt.dirt")));
	NeutralTileMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Environment/Nature/dirt.dirt")));
	WaterTileMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Arena/Materials/MI_Tile_Water.MI_Tile_Water")));
	DamageTileMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Arena/Materials/MI_Tile_Damage.MI_Tile_Damage")));
	BuffTileMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Arena/Materials/MI_Tile_Buff.MI_Tile_Buff")));
	BlockedTileMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Arena/Materials/MI_Tile_Blocked.MI_Tile_Blocked")));

	ArenaFloorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ArenaFloor"));
	ArenaFloorMesh->SetupAttachment(ArenaRoot);
	// Sem colisão em nada do tabuleiro: quem decide onde o pet está é o
	// núcleo. Geometria que empurra seria uma segunda fonte de verdade.
	ArenaFloorMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RefreshClearingGround();

	// Nasce do tamanho da casa NEUTRA; quem recua o que tem regra é
	// RefreshTileVisuals, que é o único lugar que conhece o tabuleiro.
	const float LadoDaLaje = CellSize * ProporcaoDaLajeNeutra;

	CellTileMeshes.Reset();
	for (int32 Linha = 0; Linha < GridRows; ++Linha)
	{
		for (int32 Coluna = 0; Coluna < GridColumns; ++Coluna)
		{
			UStaticMeshComponent* Laje = CreateDefaultSubobject<UStaticMeshComponent>(
				FName(*FString::Printf(TEXT("CellTile_%d_%d"), Coluna, Linha)));
			Laje->SetupAttachment(ArenaRoot);
			Laje->SetCollisionEnabled(ECollisionEnabled::NoCollision);

			// Mesmo mapeamento de eixos de GetCellWorldLocation, e pelo mesmo
			// motivo: linha é o eixo vertical da tela, coluna o horizontal.
			const float DeslocamentoX =
				-(static_cast<float>(Linha) - CentroDaGrade(GridRows)) * CellSize;
			const float DeslocamentoY =
				(static_cast<float>(Coluna) - CentroDaGrade(GridColumns)) * CellSize;

			const float Espessura = SuperficieNeutra - FundoDoTabuleiro;
			Laje->SetRelativeScale3D(FVector(LadoDaLaje / CuboDaEngine,
				LadoDaLaje / CuboDaEngine, Espessura / CuboDaEngine));
			Laje->SetRelativeLocation(FVector(DeslocamentoX, DeslocamentoY,
				(SuperficieNeutra + FundoDoTabuleiro) * 0.5f));

			CellTileMeshes.Add(Laje);
		}
	}

	// A malha é atribuída AQUI, no construtor. Componente criado sem asset
	// passa em todo teste de lógica e não existe na tela — foi assim três
	// vezes neste projeto (pets, inimigos do mundo, o próprio jogador).
	if (CilindroDaArena.Succeeded())
	{
		ArenaFloorMesh->SetStaticMesh(CilindroDaArena.Object);
	}
	if (CuboDaArena.Succeeded())
	{
		for (UStaticMeshComponent* Laje : CellTileMeshes)
		{
			Laje->SetStaticMesh(CuboDaArena.Object);
		}
	}
}

UMaterialInterface* ABattleArena::ResolveTileMaterial(uint8 CellProperty) const
{
	switch (static_cast<ECellProperty>(CellProperty))
	{
	case ECellProperty::Water:
		return WaterTileMaterial.LoadSynchronous();
	case ECellProperty::Damage:
		return DamageTileMaterial.LoadSynchronous();
	case ECellProperty::Buff:
		return BuffTileMaterial.LoadSynchronous();
	case ECellProperty::Blocked:
		return BlockedTileMaterial.LoadSynchronous();
	default:
		return NeutralTileMaterial.LoadSynchronous();
	}
}

void ABattleArena::RefreshTileVisuals()
{
	using namespace ArenaGeometria;

	// O que o mundo emprestou vem primeiro: a arena que nasce de um encontro
	// deve parecer o lugar onde ele aconteceu, não um cenário à parte.
	UMaterialInterface* MaterialDoChao = AdoptedFloorMaterial
		? AdoptedFloorMaterial.Get()
		: FloorMaterial.LoadSynchronous();
	if (MaterialDoChao)
	{
		ArenaFloorMesh->SetMaterial(0, MaterialDoChao);
	}

	// A clareira acompanha o tabuleiro: se o tamanho da casa ou a elevação
	// mudarem, a terra embaixo muda junto em vez de virar uma placa solta.
	RefreshClearingGround();

	for (int32 Linha = 0; Linha < GetActiveGridRows(); ++Linha)
	{
		for (int32 Coluna = 0; Coluna < GetActiveGridColumns(); ++Coluna)
		{
			const int32 Indice = CurrentState.CellIndex(Coluna, Linha);
			if (!CellTileMeshes.IsValidIndex(Indice) || !CellTileMeshes[Indice])
			{
				continue;
			}

			const uint8 Propriedade = GetCellProperty(
				static_cast<uint8>(Coluna), static_cast<uint8>(Linha));

			const float Superficie = GetCellSurfaceHeight(Propriedade);
			const float Espessura = Superficie - FundoDoTabuleiro;

			UStaticMeshComponent* Laje = CellTileMeshes[Indice];
			const float LadoDaLaje = CellSize * ProporcaoDaLajePara(Propriedade);
			Laje->SetRelativeScale3D(FVector(LadoDaLaje / CuboDaEngine,
				LadoDaLaje / CuboDaEngine, Espessura / CuboDaEngine));

			const FVector PosicaoAtual = Laje->GetRelativeLocation();
			Laje->SetRelativeLocation(FVector(PosicaoAtual.X, PosicaoAtual.Y,
				(Superficie + FundoDoTabuleiro) * 0.5f));

			if (UMaterialInterface* MaterialDaLaje = ResolveTileMaterial(Propriedade))
			{
				Laje->SetMaterial(0, MaterialDaLaje);
			}
		}
	}
}

FVector ABattleArena::GetCellWorldLocation(uint8 Column, uint8 Row) const
{
	// A câmera fica recuada em -X olhando ao longo de +X: na tela, +Y é a
	// DIREITA e +X é o FUNDO. Mapear Coluna->X e Linha->Y (o óbvio) fazia
	// "Baixo" andar para a direita e "Direita" andar para o fundo — foi o que
	// se viu jogando. Linha vira o eixo vertical da tela, coluna o horizontal.
	const float OffsetX =
		-(static_cast<float>(Row) - ArenaGeometria::CentroDaGrade(GetActiveGridRows())) * CellSize;
	const float OffsetY =
		(static_cast<float>(Column) - ArenaGeometria::CentroDaGrade(GetActiveGridColumns())) * CellSize;

	// O Z vem do TERRENO, não do plano do ator. Enquanto esta função devolvia
	// zero, a laje tinha relevo e o pet não: sobre a água ele ficava no ar, e
	// sobre a pedra, enterrado nela. A altura já tinha dona
	// (GetCellSurfaceHeight) — faltava esta função perguntar a ela.
	return GetActorLocation() + FVector(OffsetX, OffsetY, GetCellSurfaceHeightAt(Column, Row));
}

uint8 ABattleArena::GetCellProperty(uint8 Column, uint8 Row) const
{
	const int32 Indice = CurrentState.CellIndex(Column, Row);
	return CurrentState.CellLayout.IsValidIndex(Indice)
		? CurrentState.CellLayout[Indice]
		: static_cast<uint8>(ECellProperty::None);
}

float ABattleArena::GetCellSurfaceHeightAt(uint8 Column, uint8 Row) const
{
	return GetCellSurfaceHeight(GetCellProperty(Column, Row));
}

bool ABattleArena::AdoptAmbienceFromWorldLocation(const FVector& WorldLocation)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const FVector Alto = WorldLocation + FVector(0.0f, 0.0f, ArenaGeometria::AlturaDaSondaDeChao);
	const FVector Baixo = WorldLocation - FVector(0.0f, 0.0f, ArenaGeometria::AlcanceDaSondaDeChao);

	FHitResult Chao;
	FCollisionQueryParams Consulta(SCENE_QUERY_STAT(ArenaAmbience), /*bTraceComplex=*/false, this);
	if (!World->LineTraceSingleByChannel(Chao, Alto, Baixo, ECC_Visibility, Consulta))
	{
		FBattleDebugScreen::Show(TEXT("ambiente: sem chão sob o encontro — paleta própria"),
			0.0f, FColor::Silver, /*Key=*/20);
		return false;
	}

	const UPrimitiveComponent* Terreno = Chao.GetComponent();
	UMaterialInterface* MaterialDoLugar = Terreno ? Terreno->GetMaterial(0) : nullptr;
	if (!MaterialDoLugar)
	{
		FBattleDebugScreen::Show(TEXT("ambiente: chão sem material — paleta própria"),
			0.0f, FColor::Silver, /*Key=*/20);
		return false;
	}

	AdoptedFloorMaterial = MaterialDoLugar;
	RefreshTileVisuals();

	// Sem esta linha, "a arena parece o mundo" é opinião: ninguém distingue
	// adoção bem-sucedida de coincidência de paleta olhando a tela.
	FBattleDebugScreen::Show(
		FString::Printf(TEXT("ambiente: chão herdado do mundo (%s)"), *MaterialDoLugar->GetName()),
		0.0f, FColor::Green, /*Key=*/20);
	return true;
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

		// Dois bichos gerados podem sair parecidos por acaso, e olhar não
		// distingue "o gerador não variou" de "variou pouco desta vez". A
		// linha diz os números, então uma partida responde qual dos dois é.
		const FPetMorphology& Corpo = View->GetMorphology();
		FBattleDebugScreen::Show(
			FString::Printf(
				TEXT("corpo %d: tronco %.2fx%.2fx%.2f  perna %.0f  cabeca %.2f  cauda %.2f"),
				Pet.PetId, Corpo.BodyScale.X, Corpo.BodyScale.Y, Corpo.BodyScale.Z,
				Corpo.LegClearanceUnits, Corpo.HeadScale, Corpo.TailScale.Z),
			0.0f, FColor::Cyan, /*Key=*/30 + Pet.PetId);
	}
	// Os pets já nascem olhando um para o outro.
	RefreshGazes();

	// Os donos entram junto com os bichos: pet em campo sem ninguém dele do
	// lado de fora foi a primeira coisa que faltou quando se olhou a tela.
	SpawnOwnerViews(InitialState);
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
	for (int32 Column = 0; Column < GetActiveGridColumns(); ++Column)
	{
		for (int32 Row = 0; Row < GetActiveGridRows(); ++Row)
		{
			if (!IsPointInCameraFrustum(GetCellWorldLocation(
				static_cast<uint8>(Column), static_cast<uint8>(Row))))
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
		if (InitialState.CellLayout.IsValidIndex(InitialState.CellIndex(Pet.Column, Pet.Row))
			&& InitialState.CellLayout[InitialState.CellIndex(Pet.Column, Pet.Row)] == static_cast<uint8>(ECellProperty::Blocked))
		{
			UE_LOG(LogTemp, Error, TEXT("ABattleArena::BeginBattle: pet %d posicionado numa casa bloqueada (%d,%d) — montagem rejeitada"),
				Pet.PetId, Pet.Column, Pet.Row);
			return false;
		}
	}

	CurrentState = InitialState;

	// ANTES da checagem de casa bloqueada não daria certo: a escolha precisa
	// do estado montado para saber onde os pets estão. Por isso ela própria
	// recusa layouts que bloqueiem casa inicial.
	ApplyArenaLayoutIfNeeded();

	// Depois do layout, nunca antes: é ele que diz qual casa é água, dano ou
	// pedra, e a laje só sabe a própria altura sabendo o terreno.
	RefreshTileVisuals();

	SpawnPetViews(CurrentState, Presentations);

	// T4 (colecao-e-captura): retido para CheckForCapture (T5) consultar
	// CatalogId/Name/Type quando a batalha terminar — FPetState já não
	// carrega isso (AD-012).
	PresentationsByPetId.Reset();
	for (const FPetPresentationInfo& Presentation : Presentations)
	{
		PresentationsByPetId.Add(Presentation.PetId, Presentation);
	}

	// DEPOIS do mapa de apresentação: é dele que sai o id de catálogo, e
	// chamar antes procurava numa tabela ainda vazia.
	RegisterOwnPetInCollection();
	ApplySkillsToActionQueue();
	AnnounceMatchup();

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
	AccumulateAttributeGains(Trace);
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
			//
			// A tradução vem da MESMA função que a tela de resultado usa: eu
			// tinha escrito esta escada de if aqui também, e duas cópias da
			// mesma regra concordam só até a primeira edição.
			const EBattleResultOutcome Desfecho = BattleOutcomeForLocalPlayer(Event, LocalPlayerSide);
			const bool bEmpate = (Desfecho == EBattleResultOutcome::Empate);
			const bool bVenceu = (Desfecho == EBattleResultOutcome::Vitoria);

			ShowResultWidgetIfConfigured(Event);

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

TArray<EActionType> ABattleArena::GetAvailableActionsForSide(uint8 Side) const
{
	const FString Caminho = PetSkillCatalogPath.IsEmpty()
		? FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("PetSkills.json"))
		: PetSkillCatalogPath;

	FPetSkillCatalog Catalogo;
	if (!FPetSkillCatalog::LoadFromJson(Caminho, Catalogo))
	{
		return FPetSkillCatalog::GetUniversalActions();
	}

	const FPetState* Pet = CurrentState.Pets.FindByPredicate(
		[Side](const FPetState& Candidate) { return Candidate.Side == Side; });
	if (!Pet)
	{
		return FPetSkillCatalog::GetUniversalActions();
	}

	const FPetPresentationInfo* Presentation = PresentationsByPetId.Find(Pet->PetId);
	return Catalogo.GetAvailableActionsForType(Presentation ? Presentation->Type : FString());
}

void ABattleArena::ShowResultWidgetIfConfigured(const FBattleEvent& EndEvent)
{
	// A tela de resultado existia desde M1 e NINGUÉM a mostrava — o comentário
	// dela dizia "quem chama isto é ABattleArena", e ABattleArena não chamava.
	//
	// Sem classe configurada, nada acontece: o feed de combate já anuncia o
	// desfecho, e uma tela obrigatória exigiria um asset que talvez não exista.
	UClass* ClasseDaTela = BattleResultWidgetClassPath.TryLoadClass<UBattleResultWidget>();
	if (!ClasseDaTela)
	{
		return;
	}

	APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!PlayerController)
	{
		return;
	}

	UBattleResultWidget* Tela = CreateWidget<UBattleResultWidget>(PlayerController, ClasseDaTela);
	if (!Tela)
	{
		return;
	}

	Tela->ApplyBattleEndedEvent(EndEvent, LocalPlayerSide);
	Tela->AddToViewport();
}

void ABattleArena::ApplyArenaLayoutIfNeeded()
{
	// Montagem que já trouxe terreno manda: só preenche quem chegou neutro.
	// Sobrescrever apagaria a arena escolhida por quem montou a partida.
	const bool bTodaNeutra = !CurrentState.CellLayout.ContainsByPredicate(
		[](uint8 Propriedade) { return Propriedade != static_cast<uint8>(ECellProperty::None); });
	if (!bTodaNeutra)
	{
		return;
	}

	const FString Caminho = ArenaLayoutCatalogPath.IsEmpty()
		? FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("Arenas.json"))
		: ArenaLayoutCatalogPath;

	FArenaLayoutCatalog Catalogo;
	if (!FArenaLayoutCatalog::LoadFromJson(Caminho, Catalogo))
	{
		FBattleDebugScreen::Show(TEXT("catálogo de arenas não carregou — arena neutra"),
			8.0f, FColor::Orange, /*Key=*/954);
		return;
	}

	const TArray<FString> Nomes = Catalogo.GetSortedLayoutNames();
	if (Nomes.IsEmpty())
	{
		return;
	}

	// Escolha derivada da semente da partida, e SEM consumir o gerador do
	// núcleo: gastar números dele aqui deslocaria toda a batalha e quebraria
	// os snapshots de determinismo.
	//
	// A semente passa por uma MISTURA antes de virar índice. Usá-la crua (ou
	// dividida) faz sementes pequenas e vizinhas caírem sempre na mesma arena:
	// escrevi `Semente / 1000` primeiro e toda partida abriu no mesmo lugar,
	// porque a divisão zerava tudo abaixo de mil.
	const uint64 Misturada =
		CurrentState.Random.State * 6364136223846793005ULL + 1442695040888963407ULL;
	const uint64 Semente = Misturada >> 33;

	for (int32 Tentativa = 0; Tentativa < Nomes.Num(); ++Tentativa)
	{
		const int32 Indice = static_cast<int32>((Semente + Tentativa) % Nomes.Num());

		TArray<uint8> Layout;
		if (!Catalogo.GetLayoutByName(Nomes[Indice], Layout)
			|| Layout.Num() != CurrentState.CellLayout.Num())
		{
			continue;
		}

		// Layout que bloqueia casa inicial faria BeginBattle rejeitar a
		// montagem, e a batalha simplesmente não abriria. Tenta o próximo.
		const bool bBloqueiaAlguem = CurrentState.Pets.ContainsByPredicate(
			[this, &Layout](const FPetState& Pet)
			{
				return Layout[CurrentState.CellIndex(Pet.Column, Pet.Row)] == static_cast<uint8>(ECellProperty::Blocked);
			});
		if (bBloqueiaAlguem)
		{
			continue;
		}

		CurrentState.CellLayout = Layout;
		FBattleDebugScreen::Show(FString::Printf(TEXT("arena: %s"), *Nomes[Indice]),
			0.0f, FColor::Cyan, /*Key=*/954);
		return;
	}
}

void ABattleArena::AnnounceMatchup()
{
	const FPetState* Meu = CurrentState.Pets.FindByPredicate(
		[this](const FPetState& Pet) { return Pet.Side == LocalPlayerSide; });
	const FPetState* Dele = CurrentState.Pets.FindByPredicate(
		[this](const FPetState& Pet) { return Pet.Side != LocalPlayerSide; });
	if (!Meu || !Dele)
	{
		return;
	}

	const FPetPresentationInfo* MinhaInfo = PresentationsByPetId.Find(Meu->PetId);
	const int32 Efetividade = MinhaInfo ? MinhaInfo->EffectivenessPercent : 100;

	// ANTES da primeira escolha, não depois do primeiro golpe: saber que se
	// está em desvantagem muda a jogada — descobrir isso pelo dano recebido é
	// aprender tarde demais para agir.
	const TCHAR* Vantagem =
		Efetividade > 100 ? TEXT("  (você tem VANTAGEM de tipo)")
		: Efetividade < 100 ? TEXT("  (você está em DESVANTAGEM de tipo)")
		: TEXT("  (tipos neutros entre si)");

	FBattleDebugScreen::Show(
		FString::Printf(TEXT("%s [%s]  x  %s [%s]%s"),
			*GetPresentationNameForPet(Meu->PetId), *GetPresentationTypeForPet(Meu->PetId),
			*GetPresentationNameForPet(Dele->PetId), *GetPresentationTypeForPet(Dele->PetId),
			Vantagem),
		0.0f,
		Efetividade > 100 ? FColor::Green : (Efetividade < 100 ? FColor::Orange : FColor::Silver),
		/*Key=*/955);
}

void ABattleArena::ApplySkillsToActionQueue()
{
	if (!PlayerActionQueue)
	{
		return;
	}

	const FString Caminho = PetSkillCatalogPath.IsEmpty()
		? FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("PetSkills.json"))
		: PetSkillCatalogPath;

	FPetSkillCatalog Catalogo;
	if (!FPetSkillCatalog::LoadFromJson(Caminho, Catalogo))
	{
		// DP-skill-04: sem catálogo, ninguém fica sem ação — todo pet volta a
		// ter os seis universais, que é o comportamento de antes desta feature.
		PlayerActionQueue->SetAvailableActions(FPetSkillCatalog::GetUniversalActions());
		FBattleDebugScreen::Show(
			TEXT("catálogo de skills não carregou — só as ações universais"),
			8.0f, FColor::Orange, /*Key=*/953);
		return;
	}

	const FPetState* OwnPet = CurrentState.Pets.FindByPredicate(
		[this](const FPetState& Pet) { return Pet.Side == LocalPlayerSide; });
	if (!OwnPet)
	{
		return;
	}

	const FPetPresentationInfo* Presentation = PresentationsByPetId.Find(OwnPet->PetId);
	const FString Tipo = Presentation ? Presentation->Type : FString();

	const TArray<EActionType> Disponiveis = Catalogo.GetAvailableActionsForType(Tipo);
	PlayerActionQueue->SetAvailableActions(Disponiveis);

	const TArray<EActionType> Skills = Catalogo.GetSkillsForType(Tipo);
	FBattleDebugScreen::Show(
		Skills.IsEmpty()
			? FString::Printf(TEXT("%s (tipo %s) não tem skill própria"),
				Presentation ? *Presentation->Name : TEXT("seu pet"), *Tipo)
			: FString::Printf(TEXT("%s (tipo %s) tem %d skill(s) própria(s)"),
				Presentation ? *Presentation->Name : TEXT("seu pet"), *Tipo, Skills.Num()),
		0.0f, FColor::Cyan, /*Key=*/953);
}

void ABattleArena::RegisterOwnPetInCollection()
{
	// O pet com que você LUTA é seu.
	//
	// A única coisa que povoava a coleção era capturar o oponente derrotado —
	// o próprio pet do jogador nunca entrava, então a experiência dele não
	// tinha onde cair e o jogador não progredia NUNCA. A recusa de XP estava
	// certa; o que faltava era isto.
	//
	// O oponente NÃO entra aqui: ele se ganha vencendo, e adicioná-lo ao
	// começar daria de graça o que a captura deveria custar.
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

	FOwnedPetInstance Instance;
	Instance.CatalogId = Presentation->CatalogId;
	Instance.Name = Presentation->Name;
	Instance.Type = Presentation->Type;

	if (FPetCollectionService::CaptureIfNew(PetCollectionSlotName, Instance))
	{
		FBattleDebugScreen::Show(
			FString::Printf(TEXT("%s entrou na sua coleção"), *Presentation->Name),
			0.0f, FColor::Green, /*Key=*/952);
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

void ABattleArena::ShowAttributeGains(const FPetPresentationInfo& Presentation,
	const FOwnedPetInstance& Antes, const FOwnedPetInstance& Depois) const
{
	if (AccumulatedAttributeGains.IsEmpty())
	{
		// Batalha sem ganho nenhum DIZ isso. O silêncio aqui seria
		// indistinguível de atributo que não está sendo gravado — e essa
		// dúvida já custou uma investigação inteira com a XP.
		FBattleDebugScreen::Show(TEXT("nenhum atributo mudou nesta batalha"),
			0.0f, FColor::Silver, /*Key=*/950);
		return;
	}

	struct FLinhaDeAtributo
	{
		FText Rotulo;
		int32 Antes;
		int32 Depois;
	};

	const FLinhaDeAtributo Linhas[] = {
		{ NSLOCTEXT("PetAttributes", "Musculatura", "Musculatura"),
		  Antes.Musculature, Depois.Musculature },
		{ NSLOCTEXT("PetAttributes", "Personalidade", "Personalidade"),
		  Antes.Personality, Depois.Personality },
		{ NSLOCTEXT("PetAttributes", "Camuflagem", "Camuflagem"),
		  Antes.SkillProficiency[FPetAttributeProgression::Camouflage],
		  Depois.SkillProficiency[FPetAttributeProgression::Camouflage] },
		{ NSLOCTEXT("PetAttributes", "Voo", "Voo"),
		  Antes.SkillProficiency[FPetAttributeProgression::Flight],
		  Depois.SkillProficiency[FPetAttributeProgression::Flight] },
		{ NSLOCTEXT("PetAttributes", "Subsolo", "Subsolo"),
		  Antes.SkillProficiency[FPetAttributeProgression::Underground],
		  Depois.SkillProficiency[FPetAttributeProgression::Underground] },
	};

	for (const FLinhaDeAtributo& Linha : Linhas)
	{
		if (Linha.Antes == Linha.Depois)
		{
			continue;
		}

		// Argumentos NOMEADOS: em alemão o número e o rótulo não caem na
		// mesma ordem, e posicional obrigaria o tradutor a reordenar o que
		// não é dele.
		const FText Texto = FText::Format(
			NSLOCTEXT("PetAttributes", "AtributoSubiu", "{Pet}: {Atributo} {Antes} → {Depois}"),
			FFormatNamedArguments{
				{ TEXT("Pet"), FText::FromString(Presentation.Name) },
				{ TEXT("Atributo"), Linha.Rotulo },
				{ TEXT("Antes"), FText::AsNumber(Linha.Antes) },
				{ TEXT("Depois"), FText::AsNumber(Linha.Depois) },
			});

		FBattleDebugScreen::Show(Texto.ToString(), 0.0f,
			Linha.Depois > Linha.Antes ? FColor::Green : FColor::Orange, /*Key=*/-1);
		FBattleNarrationFeed::Push(Texto,
			Linha.Depois > Linha.Antes ? FColor::Green : FColor::Orange);
	}
}

void ABattleArena::AccumulateAttributeGains(const TArray<FBattleEvent>& Trace)
{
	const FPetState* OwnPet = CurrentState.Pets.FindByPredicate(
		[this](const FPetState& Pet) { return Pet.Side == LocalPlayerSide; });
	if (!OwnPet)
	{
		return;
	}

	AccumulatedAttributeGains.Add(
		FPetAttributeProgression::ComputeGains(Trace, OwnPet->PetId));
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
		// Cada desistência abaixo DIZ o motivo na tela.
		//
		// Elas eram silenciosas, e o usuário terminou uma batalha sem receber
		// XP nenhuma sem ter como saber por quê. Recusa sem motivo visível é
		// indistinguível de defeito — e neste caso a recusa até era correta.
		const FPetState* OwnPet = CurrentState.Pets.FindByPredicate(
			[this](const FPetState& Pet) { return Pet.Side == LocalPlayerSide; });
		if (!OwnPet)
		{
			FBattleDebugScreen::Show(TEXT("sem XP: nenhum pet do seu lado no estado final"),
				0.0f, FColor::Orange, /*Key=*/951);
			return;
		}

		const FPetPresentationInfo* Presentation = PresentationsByPetId.Find(OwnPet->PetId);
		if (!Presentation || Presentation->CatalogId.IsEmpty())
		{
			FBattleDebugScreen::Show(TEXT("sem XP: seu pet não tem id de catálogo"),
				0.0f, FColor::Orange, /*Key=*/951);
			return;
		}

		TArray<FOwnedPetInstance> Collection = FPetCollectionService::LoadCollection(PetCollectionSlotName);
		FOwnedPetInstance* OwnedInstance = Collection.FindByPredicate(
			[Presentation](const FOwnedPetInstance& Instance) { return Instance.CatalogId == Presentation->CatalogId; });
		if (!OwnedInstance)
		{
			// Recusa CORRETA: XP para um pet que não está na coleção seria XP
			// fantasma, gravada em nada. O que faltava era dizer isso.
			FBattleDebugScreen::Show(
				FString::Printf(TEXT("sem XP: '%s' ainda não está na sua coleção"), *Presentation->Name),
				0.0f, FColor::Orange, /*Key=*/951);
			return;
		}

		FPetProgressionService::GrantExperience(*OwnedInstance, ExperienceAmount);

		// Atributo grava JUNTO da XP, na mesma coleção e no mesmo save. Duas
		// escritas separadas abririam a janela em que uma acontece e a outra
		// não, e o pet terminaria a batalha com nível novo e músculo velho.
		const FOwnedPetInstance AntesDosGanhos = *OwnedInstance;
		FPetAttributeProgression::Apply(*OwnedInstance, AccumulatedAttributeGains);

		FPetCollectionService::SaveCollection(PetCollectionSlotName, Collection);

		ShowAttributeGains(*Presentation, AntesDosGanhos, *OwnedInstance);

		FBattleDebugScreen::Show(
			FString::Printf(TEXT("+%d de experiência para %s (total %d)"),
				ExperienceAmount, *Presentation->Name, OwnedInstance->Experience),
			0.0f, FColor::Green, /*Key=*/951);

		FBattleNarrationFeed::Push(
			FText::FromString(FString::Printf(TEXT("%s ganhou %d de experiência."),
				*Presentation->Name, ExperienceAmount)),
			FColor::Green);
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
		OtherCommit = FTacticalOpponentAI::GenerateCommit(
			CurrentState, BotSide, CurrentState.Random, GetAvailableActionsForSide(BotSide));
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

	LastCommitBySide[0] = LeftCommit;
	LastCommitBySide[1] = RightCommit;

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
	AccumulateAttributeGains(Result.Trace);
	GrantExperienceIfOwned(Result.Trace);

	// O anúncio do fim ESPERA a reprodução. Anunciando aqui, a transição
	// arranca o jogador da arena antes de o golpe final aparecer — e a
	// mensagem de quem venceu vai junto. Foi descrito como "apenas saiu da
	// tela".
	PendingEndOfBattleTrace = Result.Trace;

	for (const FPetState& Pet : CurrentState.Pets)
	{
		UE_LOG(LogBattleArena, Display, TEXT("  lado %d terminou em (%d,%d) com %d/%d de vida"),
			static_cast<int32>(Pet.Side), static_cast<int32>(Pet.Column),
			static_cast<int32>(Pet.Row), Pet.Health, Pet.MaxHealth);

		// Chave por lado: a linha de cada pet se ATUALIZA no lugar, em vez de
		// empilhar uma nova a cada turno.
		FBattleDebugScreen::Show(
			// NOME e TIPO, não "lado N".
			//
			// A efetividade passou a doer mais ou menos conforme o tipo, e o
			// jogador não via tipo nenhum: "é super efetivo" vira surpresa em
			// vez de decisão quando não se sabe o que está em campo. Tipo na
			// tela é o que transforma a tabela em algo que se joga.
			FString::Printf(TEXT("%s [%s]: casa (%d,%d)  vida %d/%d"),
				*GetPresentationNameForPet(Pet.PetId),
				*GetPresentationTypeForPet(Pet.PetId),
				static_cast<int32>(Pet.Column),
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
		FinishPlaybackAndSettleTurn();
	}
}

void ABattleArena::FinishPlaybackAndSettleTurn()
{
	// Ordem: primeiro o fim da batalha, depois a abertura do turno seguinte.
	// Invertida, a fila reabriria por um instante numa partida já decidida.
	if (PendingEndOfBattleTrace.Num() > 0)
	{
		const TArray<FBattleEvent> Trace = MoveTemp(PendingEndOfBattleTrace);
		PendingEndOfBattleTrace.Reset();
		AnnounceBattleFinishedIfEnded(Trace);
	}

	OpenNextTurnIfBattleContinues();
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
		FinishPlaybackAndSettleTurn();
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

	const int32 Colunas = GetActiveGridColumns();
	const int32 Linhas = GetActiveGridRows();
	const float HalfCell = CellSize * 0.5f;

	// O tamanho do campo aparece no painel: grade errada é o tipo de coisa
	// que se vê num relance e não se deduz de log nenhum.
	FBattleDebugScreen::Show(FString::Printf(TEXT("grade: %dx%d"), Colunas, Linhas),
		0.0f, FColor::Silver, /*Key=*/953);

	for (int32 Linha = 0; Linha < Linhas; ++Linha)
	{
		for (int32 Coluna = 0; Coluna < Colunas; ++Coluna)
		{
			const uint8 Row = static_cast<uint8>(Linha);
			const uint8 Column = static_cast<uint8>(Coluna);
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

			// O QUE a casa é, não só onde ela fica.
			//
			// Submergir exige água, casa de dano fere, casa de bônus ajuda — e
			// nada disso aparecia. Uma regra que depende do terreno numa grade
			// que não mostra o terreno é uma regra que o jogador só descobre
			// perdendo.
			const uint8 Propriedade = GetCellProperty(Column, Row);

			FString NomeDaCasa;
			FColor CorDoTerreno = FColor(80, 80, 80);

			switch (static_cast<ECellProperty>(Propriedade))
			{
			case ECellProperty::Water:
				NomeDaCasa = TEXT(" ÁGUA");
				CorDoTerreno = FColor(60, 140, 255);
				break;
			case ECellProperty::Damage:
				NomeDaCasa = TEXT(" DANO");
				CorDoTerreno = FColor(230, 70, 40);
				break;
			case ECellProperty::Buff:
				NomeDaCasa = TEXT(" BÔNUS");
				CorDoTerreno = FColor(80, 220, 120);
				break;
			case ECellProperty::Blocked:
				NomeDaCasa = TEXT(" BLOQUEADA");
				CorDoTerreno = FColor(140, 140, 140);
				break;
			default:
				break;
			}

			// Ocupação em AMARELO por cima do terreno: saber quem está onde
			// decide o turno, e o terreno decide o seguinte.
			const FColor CorDaCasa = Ocupantes.IsEmpty() ? CorDoTerreno : FColor::Yellow;

			// Centro já vem NA superfície da casa — a altura mora em
			// GetCellWorldLocation, e a linha só acrescenta a folga para não
			// brigar com a face da laje. Somá-la aqui outra vez punha o
			// contorno ao dobro da altura do terreno.
			const FVector CentroNaSuperficie = Centro
				+ FVector(0.0f, 0.0f, ArenaGeometria::FolgaDaGradeDesenhada);

			DrawDebugBox(GetWorld(), CentroNaSuperficie, FVector(HalfCell, HalfCell, 2.0f),
				CorDaCasa, /*bPersistent=*/false, /*LifeTime=*/-1.0f, /*DepthPriority=*/0, /*Thickness=*/2.0f);

			// Casa de terreno ganha um plano sólido: contorno sozinho some
			// contra o tabuleiro quando se olha de cima, e é exatamente aí que
			// se procura a água.
			if (Propriedade != static_cast<uint8>(ECellProperty::None))
			{
				DrawDebugSolidBox(GetWorld(), CentroNaSuperficie, FVector(HalfCell * 0.85f, HalfCell * 0.85f, 1.0f),
					FColor(CorDoTerreno.R, CorDoTerreno.G, CorDoTerreno.B, 90),
					/*bPersistent=*/false, /*LifeTime=*/-1.0f);
			}

			DrawDebugString(GetWorld(), CentroNaSuperficie + FVector(0.0f, 0.0f, 10.0f),
				FString::Printf(TEXT("(%d,%d)%s%s"), Column, Row, *NomeDaCasa, *Ocupantes),
				nullptr, CorDaCasa, /*Duration=*/0.0f, /*bDrawShadow=*/true, /*FontScale=*/1.1f);
		}
	}
}

FString ABattleArena::FindMoveNameForEvent(const FBattleEvent& Event) const
{
	const FPetState* Pet = CurrentState.Pets.FindByPredicate(
		[&Event](const FPetState& Candidate) { return Candidate.PetId == Event.ActorId; });
	if (!Pet || Pet->Side > 1 || Event.SlotIndex >= FTurnCommit::ActionsPerTurn)
	{
		return FString();
	}

	const FBattleAction& Acao = LastCommitBySide[Pet->Side].Actions[Event.SlotIndex];
	if (!BattleActionRequiresMove(Acao.Type))
	{
		return FString();
	}

	const TArray<FString> Nomes = GetMoveNamesForSide(Pet->Side);
	const uint8 Indice = GetMoveIndexFromAction(Acao);

	// Pet sem nome cadastrado para aquele slot não ganha linha: "usou golpe 2"
	// não diz mais que o silêncio, e ainda ocupa espaço no feed.
	return Nomes.IsValidIndex(Indice) ? Nomes[Indice] : FString();
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

	// O NOME do golpe que caiu, logo depois do acerto.
	//
	// Sem isto, escolher entre quatro golpes é escolher entre quatro nomes que
	// nunca reaparecem: o jogador não tem como ligar a escolha ao resultado, e
	// a decisão da fatia 2 não ensina nada.
	if (Event.Type == EBattleEventType::AtaqueAcertou && Actor)
	{
		const FString NomeDoGolpe = FindMoveNameForEvent(Event);
		if (!NomeDoGolpe.IsEmpty())
		{
			FBattleNarrationFeed::Push(
				FText::Format(NSLOCTEXT("BattleNarration", "GolpeUsado", "— usou {Move}"),
					FFormatNamedArguments{ { TEXT("Move"), FText::FromString(NomeDoGolpe) } }),
				Cor);
		}
	}

	// Efetividade só faz sentido no golpe que ACERTOU: dizê-la num erro ou num
	// movimento seria ruído, e o jogador aprende associando ao dano que veio.
	if (Event.Type == EBattleEventType::AtaqueAcertou && Actor)
	{
		if (Actor->EffectivenessPercent > 100)
		{
			FBattleNarrationFeed::Push(
				NSLOCTEXT("BattleNarration", "SuperEfetivo", "É super efetivo!"),
				FColor::Green);
		}
		else if (Actor->EffectivenessPercent < 100)
		{
			FBattleNarrationFeed::Push(
				NSLOCTEXT("BattleNarration", "PoucoEfetivo", "Não é muito efetivo..."),
				FColor(150, 150, 150));
		}
	}
}

FString ABattleArena::GetPresentationNameForPet(uint8 PetId) const
{
	const FPetPresentationInfo* Presentation = PresentationsByPetId.Find(PetId);
	return Presentation ? Presentation->Name : FString();
}

TArray<FString> ABattleArena::GetMoveNamesForSide(uint8 Side) const
{
	const FPetState* Pet = CurrentState.Pets.FindByPredicate(
		[Side](const FPetState& Candidate) { return Candidate.Side == Side; });
	if (!Pet)
	{
		return {};
	}

	const FPetPresentationInfo* Presentation = PresentationsByPetId.Find(Pet->PetId);
	return Presentation ? Presentation->MoveNames : TArray<FString>();
}

FString ABattleArena::GetPresentationTypeForPet(uint8 PetId) const
{
	const FPetPresentationInfo* Presentation = PresentationsByPetId.Find(PetId);

	// Sem tipo conhecido diz "?" em vez de vazio: colchete vazio pareceria
	// defeito da tela, e não dado ausente.
	return (Presentation && !Presentation->Type.IsEmpty()) ? Presentation->Type : FString(TEXT("?"));
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

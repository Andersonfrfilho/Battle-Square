// Copyright 2026 Anderson. All Rights Reserved.

#include "World/Village.h"
#include "World/VillageBuildingArt.h"
#include "Environment/BiomeTint.h"
#include "Environment/IslandGeography.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "Engine/StaticMesh.h"
#include "Environment/ScenaryPalette.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace Vila
{
	/** O cubo da engine mede 100 unidades de lado. */
	constexpr float CuboDaEngine = 100.0f;

	/** O telhado é um pouco MAIOR que a parede: beiral se lê de longe. */
	constexpr float BeiralUnidades = 60.0f;
	constexpr float AlturaDoTelhado = 90.0f;

	/**
	 * A CALÇADA: quanto a porta se estende além da parede.
	 *
	 * Ela existe porque o prédio BLOQUEIA. Um gatilho do tamanho exato da
	 * parede nunca dispararia — ninguém consegue estar dentro de uma caixa
	 * sólida —, e a porta seria um evento que só o teste vê.
	 *
	 * Maior que a largura de quem anda, para o passo não atravessar a faixa
	 * entre dois quadros e sair sem nunca ter entrado.
	 */
	constexpr float CalcadaDaPorta = 160.0f;

	/**
	 * A porta DESENHADA: o vão na fachada (decisão 65, emendada — "casas
	 * devem ter portas"). O gatilho invisível AVISA; esta CONVIDA — prédio
	 * com função se lê de fora, sem precisar encostar.
	 */
	constexpr float LarguraDaPorta = 130.0f;
	constexpr float AlturaDaPorta = 250.0f;
	constexpr float EspessuraDaPorta = 16.0f;

	/** Madeira escura, igual em toda vila: porta é convenção, não identidade. */
	const FLinearColor CorDaPorta(0.24f, 0.17f, 0.11f);

	/**
	 * A cor de cada prédio, e ela é INFORMAÇÃO.
	 *
	 * O Centro de Recuperação precisa se achar sem placa — é onde se corre
	 * quando o pet está mal. As casas são surdas de propósito: elas são
	 * silhueta, e competir com o que tem função faria a vila inteira gritar.
	 */
	FLinearColor CorDoPredio(EVillageBuilding Predio)
	{
		switch (Predio)
		{
			case EVillageBuilding::CentroDeRecuperacao: return FLinearColor(0.90f, 0.35f, 0.32f);
			case EVillageBuilding::Escola:              return FLinearColor(0.45f, 0.60f, 0.85f);
			case EVillageBuilding::Arena:               return FLinearColor(0.85f, 0.68f, 0.30f);
			case EVillageBuilding::Marco:               return FLinearColor(0.70f, 0.72f, 0.75f);
			case EVillageBuilding::Praca:               return FLinearColor(0.52f, 0.47f, 0.38f);
			case EVillageBuilding::Casa:                return FLinearColor(0.58f, 0.50f, 0.42f);
			// Cores próprias, e é o que faz reconhecer o assentamento de longe
			// sem placa: verde de academia, roxo de mercado, pedra escura de
			// portão.
			case EVillageBuilding::Academia:            return FLinearColor(0.35f, 0.75f, 0.45f);
			case EVillageBuilding::Mercado:             return FLinearColor(0.62f, 0.40f, 0.78f);
			case EVillageBuilding::Portao:              return FLinearColor(0.30f, 0.32f, 0.36f);

			// O MERCADO DO LAGO, e as três cores contam o material, não a
			// função: palafita é madeira clara de estaca, que vive molhada e
			// desbota; passarela é a mesma madeira mais escura de pisada; e a
			// chinampa é a única coisa VERDE sobre a água — ela é canteiro, e
			// verde sobre azul é o que diz "aqui se planta" de longe.
			case EVillageBuilding::Palafita:            return FLinearColor(0.72f, 0.60f, 0.44f);
			case EVillageBuilding::Passarela:           return FLinearColor(0.55f, 0.44f, 0.32f);
			case EVillageBuilding::Chinampa:            return FLinearColor(0.30f, 0.62f, 0.28f);
		}
		return FLinearColor(0.5f, 0.5f, 0.5f);
	}

	/** Telhado sempre mais escuro que a parede: é o que separa os dois de longe. */
	FLinearColor CorDoTelhado(EVillageBuilding Predio)
	{
		return CorDoPredio(Predio) * 0.45f;
	}

	void Pintar(UStaticMeshComponent* Componente, const FLinearColor& Cor)
	{
		UMaterialInterface* Base = ScenaryPalette::ColorableBaseMaterial();
		if (!Componente || !Base)
		{
			return;
		}

		UMaterialInstanceDynamic* Tinta =
			UMaterialInstanceDynamic::Create(Base, Componente);
		if (!Tinta)
		{
			return;
		}

		Tinta->SetVectorParameterValue(TEXT("Color"), Cor);
		Componente->SetMaterial(0, Tinta);
	}
}

FLinearColor AVillage::BuildingColor(EVillageBuilding Predio)
{
	return Vila::CorDoPredio(Predio);
}

FLinearColor AVillage::BuildingColorInBiome(EVillageBuilding Predio, EIslandBiome Biome)
{
	// A identidade do prédio (a base) vestida pelo tom do bioma. Multiplicação,
	// não substituição: o roxo do Mercado continua roxo, só muda de clima.
	return Vila::CorDoPredio(Predio) * BiomeTint::Of(Biome);
}

FLinearColor AVillage::FallbackColor()
{
	// O MESMO literal do `default`, num lugar só? Não: o literal fica lá, e
	// esta função devolve a cor de um valor de enum que NÃO tem case — assim
	// ela continua certa se alguém mudar o cinza.
	return Vila::CorDoPredio(static_cast<EVillageBuilding>(255));
}

AVillage::AVillage()
{
	PrimaryActorTick.bCanEverTick = false;

	VillageRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VillageRoot"));
	SetRootComponent(VillageRoot);

	// A MALHA vem no construtor, e não em BeginPlay: ator que nasce sem asset
	// atribuído passa em toda bateria e não existe na tela — três vezes neste
	// projeto, e hoje há sonda para isso.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cubo(
		ScenaryPalette::PrimitiveMeshPath(EScenaryPrimitive::Cube));
	if (Cubo.Succeeded())
	{
		BoxMesh = Cubo.Object;
	}
}

void AVillage::BeginPlay()
{
	Super::BeginPlay();
	BuildVillage();
}

void AVillage::BuildVillage()
{
	for (TObjectPtr<UStaticMeshComponent>& Componente : BuiltMeshes)
	{
		if (Componente) { Componente->DestroyComponent(); }
	}
	for (TObjectPtr<UStaticMeshComponent>& Componente : BuiltRoofs)
	{
		if (Componente) { Componente->DestroyComponent(); }
	}
	for (TObjectPtr<UStaticMeshComponent>& Componente : DoorMeshes)
	{
		if (Componente) { Componente->DestroyComponent(); }
	}
	BuiltMeshes.Reset();
	BuiltRoofs.Reset();
	DoorMeshes.Reset();

	if (!BoxMesh)
	{
		return;
	}

	// A VILA VESTE O BIOMA (MB3): o tom sai do bioma do LUGAR onde ela está,
	// pela fonte única BiomeTint. A identidade do prédio sobrevive; só o clima
	// da cor muda entre a Geleira e o Vulcão.
	const EIslandBiome BiomaDaVila =
		IslandGeography::BiomeAt(FVector2D(GetActorLocation()));

	int32 Indice = 0;
	for (const FVillagePlacement& Peca : VillageLayout::PlanFor(Kind))
	{
		UStaticMeshComponent* Parede = NewObject<UStaticMeshComponent>(this,
			*FString::Printf(TEXT("Predio_%d"), Indice));
		Parede->SetupAttachment(VillageRoot);
		Parede->RegisterComponent();

		// O PRÉDIO VESTE MALHA DE VERDADE quando o dado aponta uma (AR8).
		// Sem override, cai no cubo de sempre — o jogo fecha verde SEM pacote
		// (invariante 20). Asset que não carrega também cai no cubo: papel sem
		// malha nunca fica invisível.
		const FString CaminhoDaMalha = VillageBuildingArt::MeshPathFor(Peca.Building);
		UStaticMesh* MalhaDoPredio = CaminhoDaMalha.IsEmpty()
			? nullptr
			: LoadObject<UStaticMesh>(nullptr, *CaminhoDaMalha);
		const bool bMalhaPropria = (MalhaDoPredio != nullptr);

		// ToRawPtr no BoxMesh: ele e TObjectPtr, e o ternario com ponteiro cru
		// fica ambiguo (os dois lados se convertem um no outro).
		Parede->SetStaticMesh(bMalhaPropria ? MalhaDoPredio : ToRawPtr(BoxMesh));

		// A ESCALA SAI DA CAIXA MEDIDA, não de um 100 fixo. Malha autorada tem
		// caixa própria; dividir por 100 (o lado do cubo da engine) a deformaria
		// — o mesmo motivo pelo qual ForestBackdrop mede a árvore em vez de
		// fixar altura por espécie.
		const FVector2D Pegada(Peca.HalfExtentUnits.X * 2.0f, Peca.HalfExtentUnits.Y * 2.0f);
		const FVector CaixaDaMalha = bMalhaPropria
			? MalhaDoPredio->GetBoundingBox().GetSize()
			: FVector(Vila::CuboDaEngine, Vila::CuboDaEngine, Vila::CuboDaEngine);

		Parede->SetRelativeScale3D(
			VillageBuildingArt::ScaleToFootprint(CaixaDaMalha, Pegada, Peca.HeightUnits));
		Parede->SetRelativeLocation(FVector(
			Peca.OffsetUnits.X, Peca.OffsetUnits.Y, Peca.HeightUnits * 0.5f));

		// BLOQUEIA. Cenário sólido sem colisão é o defeito que o usuário
		// relatou atravessando montanha, e uma vila que se atravessa não é
		// um lugar.
		Parede->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Parede->SetCollisionResponseToAllChannels(ECR_Block);

		Vila::Pintar(Parede, AVillage::BuildingColorInBiome(Peca.Building, BiomaDaVila));
		BuiltMeshes.Add(Parede);

		// A PRAÇA não tem telhado: ela é chão, e um telhado sobre ela seria
		// um galpão no meio da vila. E o prédio que veste MALHA PRÓPRIA também
		// não leva: ele já tem telhado no modelo, e um cubo por cima viraria
		// caixa sobre casa.
		if (Peca.Building != EVillageBuilding::Praca && !bMalhaPropria)
		{
			UStaticMeshComponent* Telhado = NewObject<UStaticMeshComponent>(this,
				*FString::Printf(TEXT("Telhado_%d"), Indice));
			Telhado->SetupAttachment(VillageRoot);
			Telhado->RegisterComponent();
			Telhado->SetStaticMesh(BoxMesh);

			Telhado->SetRelativeScale3D(FVector(
				(Peca.HalfExtentUnits.X * 2.0f + Vila::BeiralUnidades) / Vila::CuboDaEngine,
				(Peca.HalfExtentUnits.Y * 2.0f + Vila::BeiralUnidades) / Vila::CuboDaEngine,
				Vila::AlturaDoTelhado / Vila::CuboDaEngine));
			Telhado->SetRelativeLocation(FVector(Peca.OffsetUnits.X, Peca.OffsetUnits.Y,
				Peca.HeightUnits + Vila::AlturaDoTelhado * 0.5f));

			Telhado->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			Telhado->SetCollisionResponseToAllChannels(ECR_Block);

			Vila::Pintar(Telhado, AVillage::BuildingColorInBiome(Peca.Building, BiomaDaVila) * 0.45f);
			BuiltRoofs.Add(Telhado);
		}

		// A PORTA DESENHADA, virada para a PRAÇA — é de lá que se chega, e
		// porta nos fundos é porta que ninguém acha. A praça fica de fora:
		// ela é chão, e chão não tem fachada.
		if (VillageLayout::HasDoor(Peca.Building)
			&& Peca.Building != EVillageBuilding::Praca)
		{
			// A fachada é a FACE dominante no rumo da praça: prédio de quina
			// (as casas em ±650,±650) ganha a porta numa face, nunca na
			// aresta — porta na quina é porta em lugar nenhum.
			const FVector2D RumoDaPraca = Peca.OffsetUnits.IsNearlyZero()
				? FVector2D(0.0f, 1.0f)
				: -Peca.OffsetUnits.GetSafeNormal();
			const bool bFaceEmX = FMath::Abs(RumoDaPraca.X) >= FMath::Abs(RumoDaPraca.Y);
			const FVector2D Normal = bFaceEmX
				? FVector2D(FMath::Sign(RumoDaPraca.X), 0.0f)
				: FVector2D(0.0f, FMath::Sign(RumoDaPraca.Y));

			// A porta cabe no prédio que a recebe: o Marco é estreito e
			// baixo, e uma porta de tamanho fixo o engoliria.
			const float Largura = FMath::Min(Vila::LarguraDaPorta,
				(bFaceEmX ? Peca.HalfExtentUnits.Y : Peca.HalfExtentUnits.X) * 0.9f);
			const float Altura = FMath::Min(Vila::AlturaDaPorta, Peca.HeightUnits * 0.7f);

			const float MeioLadoDaFace =
				bFaceEmX ? Peca.HalfExtentUnits.X : Peca.HalfExtentUnits.Y;

			UStaticMeshComponent* Vao = NewObject<UStaticMeshComponent>(this,
				*FString::Printf(TEXT("Vao_%d"), Indice));
			Vao->SetupAttachment(VillageRoot);
			Vao->RegisterComponent();
			Vao->SetStaticMesh(BoxMesh);

			Vao->SetRelativeScale3D(FVector(
				(bFaceEmX ? Vila::EspessuraDaPorta : Largura) / Vila::CuboDaEngine,
				(bFaceEmX ? Largura : Vila::EspessuraDaPorta) / Vila::CuboDaEngine,
				Altura / Vila::CuboDaEngine));

			// Meio corpo para FORA da parede: rente, o z-fighting a apagaria;
			// funda, viraria caixa de correio.
			Vao->SetRelativeLocation(FVector(
				Peca.OffsetUnits.X + Normal.X * (MeioLadoDaFace + Vila::EspessuraDaPorta * 0.5f),
				Peca.OffsetUnits.Y + Normal.Y * (MeioLadoDaFace + Vila::EspessuraDaPorta * 0.5f),
				Altura * 0.5f));

			// Decoração: quem bloqueia é a parede atrás. Porta que empurra
			// seria a calçada bloqueando com outra roupa.
			Vao->SetCollisionEnabled(ECollisionEnabled::NoCollision);

			Vila::Pintar(Vao, Vila::CorDaPorta);
			DoorMeshes.Add(Vao);
		}

		// A PORTA, e só onde há função atrás dela.
		//
		// Ela é uma FAIXA EM VOLTA da parede, e não a parede: o prédio bloqueia
		// (é o que faz a vila ser um lugar), então um gatilho do tamanho dele
		// nunca dispararia — ninguém consegue estar dentro de uma caixa
		// sólida. Quem chega para na calçada, e é ali que a porta tem de estar.
		if (VillageLayout::HasDoor(Peca.Building))
		{
			UBoxComponent* Porta = NewObject<UBoxComponent>(this,
				*FString::Printf(TEXT("Porta_%d"), Indice));
			Porta->SetupAttachment(VillageRoot);
			Porta->RegisterComponent();

			Porta->SetBoxExtent(FVector(
				Peca.HalfExtentUnits.X + Vila::CalcadaDaPorta,
				Peca.HalfExtentUnits.Y + Vila::CalcadaDaPorta,
				Peca.HeightUnits * 0.5f));
			Porta->SetRelativeLocation(FVector(
				Peca.OffsetUnits.X, Peca.OffsetUnits.Y, Peca.HeightUnits * 0.5f));

			// NÃO BLOQUEIA. Um gatilho que bloqueia é parede, e empararedaria
			// justamente o prédio que ele existe para deixar usar.
			Porta->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			Porta->SetCollisionResponseToAllChannels(ECR_Ignore);
			Porta->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
			Porta->SetGenerateOverlapEvents(true);

			// E ela é INVISÍVEL: quem vê a porta é quem entra nela, e uma
			// caixa desenhada em volta de cada prédio seria a vila dentro de
			// gaiolas.
			Porta->SetHiddenInGame(true);

			Porta->OnComponentBeginOverlap.AddDynamic(this, &AVillage::HandleDoorBegin);
			Porta->OnComponentEndOverlap.AddDynamic(this, &AVillage::HandleDoorEnd);

			Doors.Add(Porta);
			DoorBuildings.Add(Peca.Building);
		}

		++Indice;
	}
}

EVillageBuilding AVillage::GetDoorBuilding(int32 Qual) const
{
	return DoorBuildings.IsValidIndex(Qual)
		? DoorBuildings[Qual] : EVillageBuilding::Casa;
}

void AVillage::HandleDoorBegin(UPrimitiveComponent* Porta, AActor* Quem,
	UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	AnunciarPorta(Porta, Quem, /*bEntrou=*/true);
}

void AVillage::HandleDoorEnd(UPrimitiveComponent* Porta, AActor* Quem,
	UPrimitiveComponent*, int32)
{
	AnunciarPorta(Porta, Quem, /*bEntrou=*/false);
}

void AVillage::AnunciarPorta(UPrimitiveComponent* Porta, AActor* Quem, bool bEntrou)
{
	// CAMINHO ÚNICO para entrar e sair, porque só o sentido muda. Dois corpos
	// quase iguais divergem na primeira edição — e aqui a divergência seria
	// entrar num prédio e sair de outro.
	if (!Quem || !Porta)
	{
		return;
	}

	// SÓ QUEM JOGA cruza porta. Sem isto, cada pet solto e cada inimigo do
	// mundo passando pela calçada anunciaria uma visita que ninguém fez.
	if (!Cast<APawn>(Quem))
	{
		return;
	}

	const int32 Qual = Doors.IndexOfByKey(Porta);
	if (Qual == INDEX_NONE)
	{
		return;
	}

	// ENTRAR NÃO É COMPROMETER-SE, e é por isso que aqui só se ANUNCIA.
	//
	// Nada é cobrado, curado nem vendido neste caminho — quem escuta decide, e
	// decidir exige um gesto do jogador. Uma vila que cobra por atravessar a
	// calçada é uma vila que ninguém atravessa duas vezes.
	OnDoorCrossed.Broadcast(DoorBuildings[Qual], Kind, bEntrou, Qual);
}

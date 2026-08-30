// Copyright 2026 Anderson. All Rights Reserved.

#include "Environment/ForestBackdrop.h"
#include "Battle/BattleArena.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

namespace
{
	// Nome próprio (L-042): helper homônimo em outro arquivo de teste vira
	// sobrecarga ambígua quando o unity build junta os dois.
	UWorld* CreateForestTestWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
		FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
		WorldContext.SetCurrentWorld(World);
		World->InitializeActorsForPlay(FURL());
		return World;
	}

	void DestroyForestTestWorld(UWorld* World)
	{
		if (!World)
		{
			return;
		}
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
	}

	constexpr float CasaDeTeste = 150.0f;
	const FVector2D CameraDeTeste(-880.0f, 0.0f);
}

/**
 * O defeito que já aconteceu TRÊS vezes neste projeto: componente visual
 * criado sem asset atribuído. Ele passa em todo teste de lógica e não existe
 * na tela. Aqui a asserção é sobre a ATRIBUIÇÃO, não sobre o desenho.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FForestBackdropAssignsMeshToEverySpeciesTest,
	"BattleSquare.ForestBackdrop.AssignsMeshToEverySpecies",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FForestBackdropAssignsMeshToEverySpeciesTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateForestTestWorld();
	AForestBackdrop* Mata = World->SpawnActor<AForestBackdrop>();
	if (!TestNotNull(TEXT("AForestBackdrop spawna sem crash"), Mata))
	{
		DestroyForestTestWorld(World);
		return false;
	}

	TestNotNull(TEXT("O disco de chão tem malha atribuída"), Mata->GetGroundMesh());
	if (Mata->GetGroundMesh())
	{
		TestNotNull(TEXT("O disco de chão tem malha atribuída"),
			Mata->GetGroundMesh()->GetStaticMesh().Get());
	}

	const TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>>& Especies = Mata->GetSpeciesClusters();
	TestTrue(TEXT("A mata declara espécies"), Especies.Num() > 0);
	for (const TObjectPtr<UHierarchicalInstancedStaticMeshComponent>& Grupo : Especies)
	{
		if (!TestNotNull(TEXT("Espécie da mata existe"), Grupo.Get()))
		{
			continue;
		}
		TestNotNull(*FString::Printf(TEXT("Espécie '%s' tem malha atribuída"), *Grupo->GetName()),
			Grupo->GetStaticMesh().Get());
	}

	DestroyForestTestWorld(World);
	return true;
}

/**
 * A mata é cenário. Se ela nasce dentro do tabuleiro, vira regra de jogo por
 * acidente — e uma pedra em cima de uma casa muda o que o jogador lê ali.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FForestBackdropKeepsBoardAndCameraClearTest,
	"BattleSquare.ForestBackdrop.KeepsBoardAndCameraClear",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FForestBackdropKeepsBoardAndCameraClearTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateForestTestWorld();
	AForestBackdrop* Mata = World->SpawnActor<AForestBackdrop>();
	if (!TestNotNull(TEXT("AForestBackdrop spawna sem crash"), Mata))
	{
		DestroyForestTestWorld(World);
		return false;
	}

	Mata->BuildForest(CasaDeTeste, /*Seed=*/7u, CameraDeTeste);

	const float FolgaDoTabuleiro = Mata->BoardClearanceInCells * CasaDeTeste;
	const float FolgaDaCamera = Mata->CameraClearanceInCells * CasaDeTeste;

	int32 DentroDoTabuleiro = 0;
	int32 NaLenteDaCamera = 0;
	for (const FVector& Onde : Mata->GetPlantedLocations())
	{
		const FVector2D NoChao(Onde.X, Onde.Y);
		if (NoChao.Size() < FolgaDoTabuleiro)
		{
			++DentroDoTabuleiro;
		}
		if (FVector2D::Distance(NoChao, CameraDeTeste) < FolgaDaCamera)
		{
			++NaLenteDaCamera;
		}
	}

	TestEqual(TEXT("Nenhuma planta nasce sobre o tabuleiro"), DentroDoTabuleiro, 0);
	TestEqual(TEXT("Nenhuma planta nasce colada na câmera"), NaLenteDaCamera, 0);

	DestroyForestTestWorld(World);
	return true;
}

/**
 * Mata vazia é o modo de falhar silencioso desta feature: tudo compila, o
 * ator existe, e a tela continua sem fundo. A contagem é o que separa
 * "plantou" de "rodou".
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FForestBackdropActuallyPlantsSomethingTest,
	"BattleSquare.ForestBackdrop.ActuallyPlantsSomething",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FForestBackdropActuallyPlantsSomethingTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateForestTestWorld();
	AForestBackdrop* Mata = World->SpawnActor<AForestBackdrop>();
	if (!TestNotNull(TEXT("AForestBackdrop spawna sem crash"), Mata))
	{
		DestroyForestTestWorld(World);
		return false;
	}

	Mata->BuildForest(CasaDeTeste, /*Seed=*/7u, CameraDeTeste);

	// Metade do pedido é folga generosa: a rejeição por folga descarta
	// algumas, e o número exato depende da semente. O que não pode é zero,
	// nem "quase nada".
	TestTrue(TEXT("A mata planta um número substancial de elementos"),
		Mata->GetPlantedCount() > 150);

	// Toda espécie contribui: uma que planta zero é uma malha que não
	// carregou, e isso não pode passar calado.
	int32 EspeciesVazias = 0;
	for (const TObjectPtr<UHierarchicalInstancedStaticMeshComponent>& Grupo : Mata->GetSpeciesClusters())
	{
		if (Grupo && Grupo->GetInstanceCount() == 0)
		{
			++EspeciesVazias;
		}
	}
	TestEqual(TEXT("Nenhuma espécie fica sem uma única planta"), EspeciesVazias, 0);

	DestroyForestTestWorld(World);
	return true;
}

/**
 * A mesma semente dá a mesma mata, e sementes diferentes dão matas
 * diferentes. Sem a primeira metade, uma captura de tela não vale amanhã;
 * sem a segunda, o sorteio não sorteia nada.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FForestBackdropIsDeterministicPerSeedTest,
	"BattleSquare.ForestBackdrop.IsDeterministicPerSeed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FForestBackdropIsDeterministicPerSeedTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateForestTestWorld();
	AForestBackdrop* Mata = World->SpawnActor<AForestBackdrop>();
	if (!TestNotNull(TEXT("AForestBackdrop spawna sem crash"), Mata))
	{
		DestroyForestTestWorld(World);
		return false;
	}

	Mata->BuildForest(CasaDeTeste, /*Seed=*/7u, CameraDeTeste);
	const TArray<FVector> Primeira = Mata->GetPlantedLocations();

	Mata->BuildForest(CasaDeTeste, /*Seed=*/7u, CameraDeTeste);
	const TArray<FVector> Repetida = Mata->GetPlantedLocations();

	Mata->BuildForest(CasaDeTeste, /*Seed=*/8u, CameraDeTeste);
	const TArray<FVector> Outra = Mata->GetPlantedLocations();

	TestEqual(TEXT("A mesma semente planta a mesma quantidade"), Repetida.Num(), Primeira.Num());
	bool bIguais = Repetida.Num() == Primeira.Num();
	for (int32 Indice = 0; bIguais && Indice < Primeira.Num(); ++Indice)
	{
		bIguais = Primeira[Indice].Equals(Repetida[Indice], 0.01f);
	}
	TestTrue(TEXT("A mesma semente planta a mata no mesmo lugar"), bIguais);

	int32 Diferentes = 0;
	const int32 QuantasComparar = FMath::Min(Primeira.Num(), Outra.Num());
	for (int32 Indice = 0; Indice < QuantasComparar; ++Indice)
	{
		if (!Primeira[Indice].Equals(Outra[Indice], 0.01f))
		{
			++Diferentes;
		}
	}
	TestTrue(TEXT("Semente diferente dá uma mata diferente"),
		QuantasComparar > 0 && Diferentes > QuantasComparar / 2);

	DestroyForestTestWorld(World);
	return true;
}

/**
 * A escala sai da caixa MEDIDA da malha, não de um número por espécie: é
 * isso que faz espécies de tamanhos diferentes lerem como uma mata só.
 * Medir a altura já plantada é o que denuncia um número mágico voltando —
 * uma árvore de 40 unidades some, uma de 4000 come a tela.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FForestBackdropDerivesScaleFromMeshBoundsTest,
	"BattleSquare.ForestBackdrop.DerivesScaleFromMeshBounds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FForestBackdropDerivesScaleFromMeshBoundsTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateForestTestWorld();
	AForestBackdrop* Mata = World->SpawnActor<AForestBackdrop>();
	if (!TestNotNull(TEXT("AForestBackdrop spawna sem crash"), Mata))
	{
		DestroyForestTestWorld(World);
		return false;
	}

	Mata->BuildForest(CasaDeTeste, /*Seed=*/7u, CameraDeTeste);

	// Diorama de 3x3 casas: nada rasteiro abaixo de meia casa nem arbóreo
	// acima de cinco. Fora dessa faixa a mata não é mata, é escala errada.
	const float AlturaMinima = 0.1f * CasaDeTeste;
	const float AlturaMaxima = 5.0f * CasaDeTeste;

	int32 ForaDaFaixa = 0;
	int32 Medidas = 0;
	for (const TObjectPtr<UHierarchicalInstancedStaticMeshComponent>& Grupo : Mata->GetSpeciesClusters())
	{
		if (!Grupo || !Grupo->GetStaticMesh())
		{
			continue;
		}
		const float AlturaDaMalha = Grupo->GetStaticMesh()->GetBoundingBox().GetSize().Z;
		FTransform Onde;
		for (int32 Instancia = 0; Instancia < Grupo->GetInstanceCount(); ++Instancia)
		{
			if (!Grupo->GetInstanceTransform(Instancia, Onde))
			{
				continue;
			}
			++Medidas;
			const float AlturaPlantada = AlturaDaMalha * Onde.GetScale3D().Z;
			if (AlturaPlantada < AlturaMinima || AlturaPlantada > AlturaMaxima)
			{
				++ForaDaFaixa;
			}
		}
	}

	TestTrue(TEXT("Há instâncias para medir"), Medidas > 0);
	TestEqual(TEXT("Toda planta cabe na faixa de altura do diorama"), ForaDaFaixa, 0);

	DestroyForestTestWorld(World);
	return true;
}

/**
 * A mata pode estar perfeita e não aparecer: basta ninguém spawnar. Este é
 * o teste que separa "a classe funciona" de "o jogo tem fundo".
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FForestBackdropIsSpawnedByArenaTest,
	"BattleSquare.ForestBackdrop.IsSpawnedByArena",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FForestBackdropIsSpawnedByArenaTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateForestTestWorld();
	ABattleArena* Arena = World->SpawnActor<ABattleArena>();
	if (!TestNotNull(TEXT("ABattleArena spawna sem crash"), Arena))
	{
		DestroyForestTestWorld(World);
		return false;
	}

	Arena->DispatchBeginPlay();

	AForestBackdrop* Mata = Arena->GetForestBackdrop();
	TestNotNull(TEXT("A arena spawna a mata ao começar a partida"), Mata);
	if (Mata)
	{
		TestTrue(TEXT("A mata spawnada pela arena tem elementos plantados"),
			Mata->GetPlantedCount() > 0);
	}

	DestroyForestTestWorld(World);
	return true;
}

/**
 * A mata precisa ser PINTADA, não herdar a cor do pacote.
 *
 * O chão vinha vestido com `leafsGreen` — o MESMO material das folhas das
 * árvores. Chão e copa eram uma cor só, e o quadro inteiro lia como uma
 * mancha verde-água. Trocar um material do kit por outro moveria o problema:
 * os dez são variações de uma faixa estreita.
 *
 * A asserção é sobre a PINTURA ter acontecido, não sobre o desenho — o mesmo
 * modo de falhar de componente criado sem asset atribuído.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FForestBackdropPaintsEverySpeciesFromThePaletteTest,
	"BattleSquare.ForestBackdrop.PaintsEverySpeciesFromThePalette",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FForestBackdropPaintsEverySpeciesFromThePaletteTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateForestTestWorld();
	AForestBackdrop* Mata = World->SpawnActor<AForestBackdrop>();
	if (!TestNotNull(TEXT("AForestBackdrop spawna sem crash"), Mata))
	{
		DestroyForestTestWorld(World);
		return false;
	}

	Mata->BuildForest(CasaDeTeste, /*Seed=*/7u, CameraDeTeste);

	int32 EspeciesSemTinta = 0;
	for (const TObjectPtr<UHierarchicalInstancedStaticMeshComponent>& Grupo : Mata->GetSpeciesClusters())
	{
		if (!Grupo || !Grupo->GetStaticMesh())
		{
			continue;
		}
		if (!Cast<UMaterialInstanceDynamic>(Grupo->GetMaterial(0)))
		{
			++EspeciesSemTinta;
		}
	}
	TestEqual(TEXT("toda especie sai da nossa paleta, nao da cor do pacote"),
		EspeciesSemTinta, 0);

	// E o chão também: era ELE que vestia o material das folhas.
	UStaticMeshComponent* Chao = Mata->GetGroundMesh();
	if (TestNotNull(TEXT("a mata tem chao"), Chao))
	{
		TestNotNull(TEXT("o chao da mata e' pintado pela paleta"),
			Cast<UMaterialInstanceDynamic>(Chao->GetMaterial(0)));
	}

	DestroyForestTestWorld(World);
	return true;
}

/**
 * O chão emprestado pelo mundo continua vencendo a paleta.
 *
 * A batalha que nasce de um encontro acontece no chão daquele lugar; se a
 * paleta passasse por cima, a arena voltaria a parecer outro lugar — que é
 * exatamente o defeito que o empréstimo resolveu.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FForestBackdropLetsTheWorldGroundWinTest,
	"BattleSquare.ForestBackdrop.LetsTheWorldGroundWin",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FForestBackdropLetsTheWorldGroundWinTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateForestTestWorld();
	AForestBackdrop* Mata = World->SpawnActor<AForestBackdrop>();
	if (!TestNotNull(TEXT("AForestBackdrop spawna sem crash"), Mata))
	{
		DestroyForestTestWorld(World);
		return false;
	}

	UMaterialInterface* DoMundo = LoadObject<UMaterialInterface>(
		nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (!TestNotNull(TEXT("o material de emprestimo carrega"), DoMundo))
	{
		DestroyForestTestWorld(World);
		return false;
	}

	Mata->BuildForest(CasaDeTeste, /*Seed=*/7u, CameraDeTeste);
	Mata->SetGroundMaterialOverride(DoMundo);

	TestEqual(TEXT("o chao emprestado pelo mundo vence a paleta"),
		Mata->GetGroundMesh()->GetMaterial(0), DoMundo);

	DestroyForestTestWorld(World);
	return true;
}

// Copyright 2026 Anderson. All Rights Reserved.

#include "Environment/ForestBackdrop.h"
#include "Environment/RegionResidency.h"
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

	/** Lado do ladrilho nos testes de pedaço — o mesmo que a residência usa. */
	constexpr float LadoDoPedacoDeTeste = 6400.0f;

	/**
	 * Quantas instâncias existem nas espécies cujo nome contém `Trecho`.
	 *
	 * Pelo NOME e não pelo índice: a tabela de espécies é reordenável, e um
	 * teste amarrado à posição dela passaria a medir outra planta em silêncio
	 * na primeira reordenação.
	 */
	int32 ContaEspeciesDoPedaco(const AForestBackdrop* Mata, const TCHAR* Trecho)
	{
		int32 Total = 0;
		for (const TObjectPtr<UHierarchicalInstancedStaticMeshComponent>& Grupo : Mata->GetSpeciesClusters())
		{
			if (Grupo && Grupo->GetName().Contains(Trecho))
			{
				Total += Grupo->GetInstanceCount();
			}
		}
		return Total;
	}
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

// O CHÃO DA MATA COLIDE. Ele é o piso do mundo, não pintura de fundo.
//
// Nasceu sem colisão porque a mata começou como cenário de FUNDO da arena,
// onde ninguém anda. No mundo aberto ele é a ilha — e enquanto os 225 cubos do
// teste de streaming estiveram lá, eles seguravam o jogador e esconderam isto.
// Ao removê-los, o jogador passou a cair pelo chão.
//
// A lição é geral, e por isso este teste existe: o que segurava o jogador não
// era o que parecia segurá-lo. Andaime que sustenta produção não é andaime —
// é estrutura que ninguém declarou.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FForestGroundIsWalkableTest,
	"BattleSquare.Environment.ForestBackdrop.GroundIsWalkable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FForestGroundIsWalkableTest::RunTest(const FString& Parameters)
{
	const AForestBackdrop* Padrao = GetDefault<AForestBackdrop>();
	const UStaticMeshComponent* Chao = Padrao->GetGroundMesh();

	TestTrue(TEXT("O chão existe"), Chao != nullptr);
	if (!Chao)
	{
		return false;
	}

	TestTrue(TEXT("E COLIDE — dá para andar em cima dele"),
		Chao->GetCollisionEnabled() != ECollisionEnabled::NoCollision);
	TestTrue(TEXT("E bloqueia o pawn, em vez de só responder a consulta"),
		Chao->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Block);

	return true;
}


/**
 * O ladrilho é QUADRADO, e a planta cai dentro dele.
 *
 * Pedaço que ladrilha o mundo encosta nos vizinhos pelos quatro lados. Uma
 * planta fora do quadrado nasce no ladrilho do lado — e, na costura, duas
 * árvores no mesmo lugar.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FForestRegionPlantsInsideItsSquareTest,
	"BattleSquare.Environment.ForestBackdrop.RegionPlantsInsideItsSquare",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FForestRegionPlantsInsideItsSquareTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateForestTestWorld();
	AForestBackdrop* Mata = World->SpawnActor<AForestBackdrop>();
	if (!TestNotNull(TEXT("AForestBackdrop spawna sem crash"), Mata))
	{
		DestroyForestTestWorld(World);
		return false;
	}

	Mata->BuildRegion(CasaDeTeste, 7u, EIslandBiome::Forest, LadoDoPedacoDeTeste);

	const float Meio = LadoDoPedacoDeTeste * 0.5f;
	int32 ForaDoQuadrado = 0;
	for (const FVector& Onde : Mata->GetPlantedLocations())
	{
		if (FMath::Abs(Onde.X) > Meio || FMath::Abs(Onde.Y) > Meio)
		{
			++ForaDoQuadrado;
		}
	}

	// Uma asserção para o conjunto, e não uma por planta: cento e tantos
	// acertos silenciosos afogariam a única que falhasse.
	TestEqual(TEXT("Nenhuma planta nasce fora do ladrilho"), ForaDoQuadrado, 0);
	TestTrue(TEXT("E o ladrilho de mata não sai vazio"), Mata->GetPlantedCount() > 0);

	const UStaticMeshComponent* Chao = Mata->GetGroundMesh();
	TestTrue(TEXT("O chão do pedaço é o cubo, não o cilindro"),
		Chao->GetStaticMesh() && Chao->GetStaticMesh()->GetName().Contains(TEXT("Cube")));
	// Ele mede MAIS que o pedaço, e a diferença é o que costura a grade: dois
	// ladrilhos que só se encostam deixam fresta na diagonal.
	TestEqual(TEXT("E ele mede o lado transbordado em X"),
		static_cast<float>(Chao->GetRelativeScale3D().X) * 100.0f,
		AForestBackdrop::RegionGroundSideUnits(LadoDoPedacoDeTeste));
	TestEqual(TEXT("E o mesmo em Y — quadrado, não retângulo"),
		static_cast<float>(Chao->GetRelativeScale3D().Y),
		static_cast<float>(Chao->GetRelativeScale3D().X));

	DestroyForestTestWorld(World);
	return true;
}

/**
 * Deserto não tem capim, e geleira não tem flor.
 *
 * A tabela de espécies é UMA; o bioma a filtra. Se o filtro não morder, o
 * deserto sai igual à mata com outra cor de chão — que é exatamente o que se
 * queria evitar ao dividir a ilha em setores.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FForestRegionFiltersSpeciesByBiomeTest,
	"BattleSquare.Environment.ForestBackdrop.RegionFiltersSpeciesByBiome",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FForestRegionFiltersSpeciesByBiomeTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateForestTestWorld();
	AForestBackdrop* Mata = World->SpawnActor<AForestBackdrop>();
	if (!TestNotNull(TEXT("AForestBackdrop spawna sem crash"), Mata))
	{
		DestroyForestTestWorld(World);
		return false;
	}

	Mata->BuildRegion(CasaDeTeste, 11u, EIslandBiome::Desert, LadoDoPedacoDeTeste);
	TestEqual(TEXT("O deserto não tem capim"),
		ContaEspeciesDoPedaco(Mata, TEXT("grass")), 0);
	TestEqual(TEXT("Nem flor"),
		ContaEspeciesDoPedaco(Mata, TEXT("flower")), 0);
	TestEqual(TEXT("Nem árvore"),
		ContaEspeciesDoPedaco(Mata, TEXT("tree")), 0);
	TestTrue(TEXT("Mas tem pedra — é o que sobra onde não há mata"),
		ContaEspeciesDoPedaco(Mata, TEXT("rock")) > 0);

	// O MESMO ator, outro bioma: é assim que a residência reaproveita o
	// ladrilho ao trocar de setor. O pinheiro que sobrasse ficaria de pé no
	// meio da duna.
	Mata->BuildRegion(CasaDeTeste, 11u, EIslandBiome::Forest, LadoDoPedacoDeTeste);
	TestTrue(TEXT("Remontado como mata, o capim volta"),
		ContaEspeciesDoPedaco(Mata, TEXT("grass")) > 0);

	Mata->BuildRegion(CasaDeTeste, 11u, EIslandBiome::Volcano, LadoDoPedacoDeTeste);
	TestEqual(TEXT("E remontado como vulcão, ele some de novo"),
		ContaEspeciesDoPedaco(Mata, TEXT("grass")), 0);

	DestroyForestTestWorld(World);
	return true;
}

/**
 * Cada bioma pinta o SEU chão, e nenhum deles repete o do vizinho.
 *
 * Chão de bioma novo sem `case` na paleta sai verde de mata — o modo de
 * falhar que este projeto já pagou três vezes, e que nenhum teste de lógica
 * pega.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FForestRegionGivesEachBiomeItsGroundTest,
	"BattleSquare.Environment.ForestBackdrop.RegionGivesEachBiomeItsGround",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FForestRegionGivesEachBiomeItsGroundTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateForestTestWorld();
	AForestBackdrop* Mata = World->SpawnActor<AForestBackdrop>();
	if (!TestNotNull(TEXT("AForestBackdrop spawna sem crash"), Mata))
	{
		DestroyForestTestWorld(World);
		return false;
	}

	const EIslandBiome Biomas[] = {
		EIslandBiome::Desert, EIslandBiome::Glacier,
		EIslandBiome::Volcano, EIslandBiome::Beach,
		EIslandBiome::Swamp };

	TSet<EScenaryRole> Vistos;
	for (const EIslandBiome Bioma : Biomas)
	{
		Mata->BuildRegion(CasaDeTeste, 3u, Bioma, LadoDoPedacoDeTeste);
		const EScenaryRole Papel = Mata->GetRegionGroundRole();

		TestTrue(TEXT("Bioma fora da mata tem papel de chão próprio"),
			Papel != EScenaryRole::Count);
		TestFalse(TEXT("E nenhum bioma repete o chão de outro"), Vistos.Contains(Papel));
		Vistos.Add(Papel);
	}

	// A mata é a ausência de bioma: ela usa a cor de chão de sempre, a mesma
	// que a arena empresta quando a batalha nasce ali.
	Mata->BuildRegion(CasaDeTeste, 3u, EIslandBiome::Forest, LadoDoPedacoDeTeste);
	TestEqual(TEXT("A mata continua com a cor de chão de sempre"),
		static_cast<int32>(Mata->GetRegionGroundRole()),
		static_cast<int32>(EScenaryRole::Count));

	DestroyForestTestWorld(World);
	return true;
}

/**
 * O brejo NÃO é a mata com outra cor.
 *
 * Ele nasceu sem `case` na tabela de presença e caiu no retorno da floresta:
 * a geografia sabia que ali era pântano, o clima sabia, o pino do mapa sabia,
 * e a tela desenhava mata. Defeito que nenhum teste de lógica alcança, porque
 * toda lógica estava certa — só o desenho é que não era o do lugar.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FForestReliefShapesTheFlatBiomesTest,
	"BattleSquare.Environment.ForestBackdrop.ODesertoEAGeleiraTemRelevo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FForestReliefShapesTheFlatBiomesTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateForestTestWorld();
	AForestBackdrop* Mata = World->SpawnActor<AForestBackdrop>();
	if (!TestNotNull(TEXT("AForestBackdrop spawna sem crash"), Mata))
	{
		DestroyForestTestWorld(World);
		return false;
	}

	UHierarchicalInstancedStaticMeshComponent* Montes = Mata->GetReliefMounds();
	if (!TestNotNull(TEXT("o pedaço tem agrupamento de relevo"), Montes))
	{
		DestroyForestTestWorld(World);
		return false;
	}

	// Componente criado não é componente visível: sem malha, o monte passa em
	// toda contagem e não existe na tela. É o defeito que já apareceu três
	// vezes neste projeto, e aqui ele custaria o bioma inteiro.
	TestNotNull(TEXT("e o agrupamento de relevo tem malha atribuída"),
		Montes->GetStaticMesh().Get());

	// O defeito que o jogador viu: "areia plana com pedra em cima não é
	// deserto". Quem não tem relevo nenhum é um plano de uma cor só.
	Mata->BuildRegion(CasaDeTeste, 5u, EIslandBiome::Desert, LadoDoPedacoDeTeste);
	const int32 DunasDoDeserto = Montes->GetInstanceCount();
	TestTrue(TEXT("o deserto tem duna"), DunasDoDeserto > 0);

	Mata->BuildRegion(CasaDeTeste, 5u, EIslandBiome::Glacier, LadoDoPedacoDeTeste);
	TestTrue(TEXT("a geleira tem gelo quebrado"), Montes->GetInstanceCount() > 0);

	// A mata já tem copa para dar escala; monte ali seria bola no meio da
	// clareira. E este é o caminho que o ator percorre de verdade — ele vira
	// pedaço e volta a ser mata sem nunca ser destruído.
	Mata->BuildForest(CasaDeTeste, 5u, CameraDeTeste);
	TestEqual(TEXT("a mata não ganha monte nenhum"), Montes->GetInstanceCount(), 0);

	// O monte sobe do chão pela METADE: centro no topo do piso, para a duna
	// encontrar o plano numa curva em vez de num degrau.
	Mata->BuildRegion(CasaDeTeste, 5u, EIslandBiome::Desert, LadoDoPedacoDeTeste);
	TestEqual(TEXT("e a mesma semente devolve o mesmo deserto"),
		Montes->GetInstanceCount(), DunasDoDeserto);

	FTransform Primeira;
	Montes->GetInstanceTransform(0, Primeira);
	TestEqual(TEXT("o centro do monte fica no topo do chão"),
		static_cast<float>(Primeira.GetLocation().Z), AForestBackdrop::GroundTopLocalZ());
	TestTrue(TEXT("e ele tem altura de verdade"),
		Primeira.GetScale3D().Z > KINDA_SMALL_NUMBER);

	DestroyForestTestWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FForestSwampIsNotJustDarkForestTest,
	"BattleSquare.Environment.ForestBackdrop.OBrejoNaoEAMataComOutraCor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FForestSwampIsNotJustDarkForestTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateForestTestWorld();
	AForestBackdrop* Mata = World->SpawnActor<AForestBackdrop>();
	if (!TestNotNull(TEXT("AForestBackdrop spawna sem crash"), Mata))
	{
		DestroyForestTestWorld(World);
		return false;
	}

	Mata->BuildRegion(CasaDeTeste, 7u, EIslandBiome::Forest, LadoDoPedacoDeTeste);
	const int32 ArvoresDaMata = ContaEspeciesDoPedaco(Mata, TEXT("tree"));
	const int32 TroncosDaMata = ContaEspeciesDoPedaco(Mata, TEXT("log"))
		+ ContaEspeciesDoPedaco(Mata, TEXT("stump"));

	Mata->BuildRegion(CasaDeTeste, 7u, EIslandBiome::Swamp, LadoDoPedacoDeTeste);
	const int32 ArvoresDoBrejo = ContaEspeciesDoPedaco(Mata, TEXT("tree"));
	const int32 TroncosDoBrejo = ContaEspeciesDoPedaco(Mata, TEXT("log"))
		+ ContaEspeciesDoPedaco(Mata, TEXT("stump"));

	// A copa é o que separa os dois de longe: brejo é mata que não drenou, e
	// árvore alta não fica de pé em chão encharcado.
	TestTrue(TEXT("O brejo tem MENOS copa que a mata"),
		ArvoresDoBrejo < ArvoresDaMata);

	// E o tronco caído é o que ele tem de sobra — é o que a copa virou.
	TestTrue(TEXT("E não tem menos tronco caído que ela"),
		TroncosDoBrejo >= TroncosDaMata);

	// Sem capim nenhum o brejo viraria lama pelada, que é deserto escuro.
	TestTrue(TEXT("O junco continua lá"),
		ContaEspeciesDoPedaco(Mata, TEXT("grass")) > 0);

	DestroyForestTestWorld(World);
	return true;
}

/**
 * O adensamento acompanha a ÁREA, não o número de ladrilhos.
 *
 * Sem isto, um pedaço grande sai pelado e um pequeno sai entupido — e nada no
 * mundo explica a diferença, porque a diferença é do código.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FForestRegionKeepsDensityAcrossSizesTest,
	"BattleSquare.Environment.ForestBackdrop.RegionKeepsDensityAcrossSizes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FForestRegionKeepsDensityAcrossSizesTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateForestTestWorld();
	AForestBackdrop* Mata = World->SpawnActor<AForestBackdrop>();
	if (!TestNotNull(TEXT("AForestBackdrop spawna sem crash"), Mata))
	{
		DestroyForestTestWorld(World);
		return false;
	}

	Mata->BuildRegion(CasaDeTeste, 5u, EIslandBiome::Forest, LadoDoPedacoDeTeste);
	const int32 Pequeno = Mata->GetPlantedCount();

	Mata->BuildRegion(CasaDeTeste, 5u, EIslandBiome::Forest, LadoDoPedacoDeTeste * 2.0f);
	const int32 Grande = Mata->GetPlantedCount();

	TestTrue(TEXT("O ladrilho pequeno tem planta"), Pequeno > 0);

	// Dobrar o lado quadruplica a área. O arredondamento por espécie afasta a
	// conta do quatro exato, então a cobrança é de FAIXA — apertada o bastante
	// para reprovar contagem fixa (1x) e crescimento linear (2x).
	TestTrue(TEXT("Dobrar o lado quadruplica o povoamento, e não o dobra"),
		Grande >= Pequeno * 7 / 2 && Grande <= Pequeno * 9 / 2);

	DestroyForestTestWorld(World);
	return true;
}

/**
 * A mesma semente dá o mesmo pedaço.
 *
 * O jogador que sai de um ladrilho e volta a ele precisa reencontrar as
 * mesmas árvores. Sem isto, andar em círculo replanta a floresta.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FForestRegionRepeatsForTheSameSeedTest,
	"BattleSquare.Environment.ForestBackdrop.RegionRepeatsForTheSameSeed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FForestRegionRepeatsForTheSameSeedTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateForestTestWorld();
	AForestBackdrop* Mata = World->SpawnActor<AForestBackdrop>();
	if (!TestNotNull(TEXT("AForestBackdrop spawna sem crash"), Mata))
	{
		DestroyForestTestWorld(World);
		return false;
	}

	Mata->BuildRegion(CasaDeTeste, 21u, EIslandBiome::Beach, LadoDoPedacoDeTeste);
	const TArray<FVector> Primeira = Mata->GetPlantedLocations();
	if (!TestTrue(TEXT("A praia não sai vazia"), Primeira.Num() > 0))
	{
		DestroyForestTestWorld(World);
		return false;
	}

	Mata->BuildRegion(CasaDeTeste, 21u, EIslandBiome::Beach, LadoDoPedacoDeTeste);
	const TArray<FVector> Segunda = Mata->GetPlantedLocations();

	if (!TestEqual(TEXT("Duas montagens plantam a mesma quantidade"),
		Segunda.Num(), Primeira.Num()))
	{
		DestroyForestTestWorld(World);
		return false;
	}

	int32 Diferentes = 0;
	for (int32 Indice = 0; Indice < Primeira.Num(); ++Indice)
	{
		if (!Primeira[Indice].Equals(Segunda[Indice], 0.01))
		{
			++Diferentes;
		}
	}
	TestEqual(TEXT("E cada planta cai no mesmo lugar"), Diferentes, 0);

	// Outra semente é outro lugar — sem isto, "determinístico" poderia ser só
	// uma tabela fixa disfarçada.
	Mata->BuildRegion(CasaDeTeste, 22u, EIslandBiome::Beach, LadoDoPedacoDeTeste);
	const TArray<FVector> Outra = Mata->GetPlantedLocations();
	TestTrue(TEXT("Semente diferente move as plantas"),
		Outra.Num() != Primeira.Num() || !Outra[0].Equals(Primeira[0], 0.01));

	DestroyForestTestWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FForestRegionGroundOverlapsItsNeighbourTest,
	"BattleSquare.Environment.ForestBackdrop.OChaoDeUmPedacoInvadeOVizinho",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FForestRegionGroundOverlapsItsNeighbourTest::RunTest(const FString& Parameters)
{
	// Encostar não é cobrir. Dois pedaços do mesmo lado numa grade daquele
	// lado dividem uma aresta exata, e na diagonal quatro cantos dividem um
	// PONTO — que é por onde se cai. O chão precisa passar da borda.
	const float LadoDoPedaco = RegionResidency::ChunkSideUnits();
	const float LadoDoChao = AForestBackdrop::RegionGroundSideUnits(LadoDoPedaco);

	if (!TestTrue(TEXT("O chão é maior que o pedaço que ele cobre"),
		LadoDoChao > LadoDoPedaco))
	{
		return false;
	}

	// Dois vizinhos, centros a um lado de distância. A borda de um passa da
	// borda do outro pela metade do transbordo, de cada lado.
	const float MetadeDoChao = 0.5f * LadoDoChao;
	const float Invasao = 2.0f * MetadeDoChao - LadoDoPedaco;

	TestTrue(TEXT("E a invasão é larga o bastante para ninguém caber na fresta"),
		Invasao >= 100.0f);

	// Crescer o mundo não pode encolher a costura: a sobreposição é uma
	// FRAÇÃO do lado, não um número fixo que um dia vira desprezível.
	const float LadoGrande = AForestBackdrop::RegionGroundSideUnits(4.0f * LadoDoPedaco);
	TestTrue(TEXT("Pedaço maior transborda proporcionalmente mais"),
		LadoGrande - 4.0f * LadoDoPedaco > Invasao);

	return true;
}

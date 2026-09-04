// Copyright 2026 Anderson. All Rights Reserved.

#include "Environment/ForestBackdrop.h"
#include "Environment/BiomeFlora.h"
#include "Environment/FreshWater.h"
#include "Environment/IslandGeography.h"
#include "Environment/RegionResidency.h"
#include "Environment/ScenaryPalette.h"
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

	// Toda espécie DO ELENCO contribui: uma que planta zero é uma malha que
	// não carregou, e isso não pode passar calado. Antes o teste cobrava
	// isso de todas as 28, e cobrar de todas é dizer que os seis biomas
	// plantam o mesmo — que era o defeito.
	const TArray<FString> NomesDasEspecies = AForestBackdrop::SpeciesNames();
	const TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>>& Grupos =
		Mata->GetSpeciesClusters();

	int32 EspeciesVazias = 0;
	int32 EspeciesForaDoElenco = 0;
	for (int32 Indice = 0; Indice < Grupos.Num() && Indice < NomesDasEspecies.Num(); ++Indice)
	{
		if (!Grupos[Indice])
		{
			continue;
		}

		const bool bDoElenco = BiomeFlora::FitsBiome(NomesDasEspecies[Indice], EIslandBiome::Forest);
		const int32 Plantadas = Grupos[Indice]->GetInstanceCount();

		if (bDoElenco && Plantadas == 0)
		{
			++EspeciesVazias;
		}
		if (!bDoElenco && Plantadas > 0)
		{
			++EspeciesForaDoElenco;
		}
	}
	TestEqual(TEXT("Nenhuma espécie do elenco fica sem uma única planta"), EspeciesVazias, 0);

	// A outra metade, e a que o jogador reclamou: agulha de pedra e conífera
	// de geleira não têm o que fazer na mata de casa.
	TestEqual(TEXT("Nada de fora do elenco da floresta planta"), EspeciesForaDoElenco, 0);

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

	// Só o elenco da floresta: espécie que este bioma não planta também não
	// pinta, e cobrar tinta dela mediria a lista de elenco, não a paleta.
	const TArray<FString> NomesDaPaleta = AForestBackdrop::SpeciesNames();
	const TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>>& GruposDaPaleta =
		Mata->GetSpeciesClusters();

	int32 EspeciesSemTinta = 0;
	for (int32 Indice = 0; Indice < GruposDaPaleta.Num() && Indice < NomesDaPaleta.Num(); ++Indice)
	{
		const TObjectPtr<UHierarchicalInstancedStaticMeshComponent>& Grupo = GruposDaPaleta[Indice];
		if (!Grupo || !Grupo->GetStaticMesh())
		{
			continue;
		}
		if (!BiomeFlora::FitsBiome(NomesDaPaleta[Indice], EIslandBiome::Forest))
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

	// LONGE DA ORIGEM, e é uma correção: a origem virou a praça da vila, e
	// nada se planta lá. Medido ali, o pedaço pequeno perdia proporcionalmente
	// mais do que o grande, e a densidade parecia mudar com o tamanho quando
	// quem mudava era a clareira.
	//
	// O teste sempre quis medir mata comum; a origem só era conveniente.
	Mata->SetActorLocation(FVector(500000.0f, 500000.0f, 0.0f));

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

namespace
{
	/**
	 * Um pedaço apoiado na LINHA D'ÁGUA, que é onde a orla existe.
	 *
	 * Nome próprio (L-042): dois auxiliares homônimos em arquivos diferentes
	 * viram sobrecarga no unity build, e a que ganha é a que o compilador viu
	 * primeiro.
	 */
	FVector CentroDoPedacoDaOrlaDeTeste()
	{
		// Um pouco para dentro da costa: assim tanto a faixa molhada (que fica
		// por dentro) quanto a espuma (que fica por fora) caem no ladrilho.
		return FVector(IslandGeography::LandRadiusUnits() - 200.0f, 0.0f, 0.0f);
	}

	/**
	 * O pedaço centrado nas ÁRVORES da beira, que ficam bem mais para dentro.
	 *
	 * Existe porque a praia deixou de caber num pedaço. Ela é uma FRAÇÃO do
	 * raio (8%), então numa ilha de 1,4 km ela tem 112 metros — e o pedaço tem
	 * 64. Com a ilha de 20.000 a faixa inteira cabia num ladrilho e um centro
	 * só servia a todos os testes da orla; deixou de servir.
	 *
	 * Não é defeito da praia nem do pedaço: é a suposição de que os dois
	 * tinham tamanhos comparáveis, verdadeira num tamanho de ilha só.
	 */
	FVector CentroDoPedacoDasArvoresDaOrla()
	{
		const float RecuoDasArvores = IslandGeography::BeachWidthUnits() * 0.55f;
		return FVector(IslandGeography::LandRadiusUnits() - RecuoDasArvores, 0.0f, 0.0f);
	}

	/** A que distância do CENTRO DA ILHA está esta instância. */
	float RaioDaInstanciaDaOrla(const AForestBackdrop* Mata,
		const UHierarchicalInstancedStaticMeshComponent* Grupo, int32 Instancia)
	{
		FTransform Onde;
		Grupo->GetInstanceTransform(Instancia, Onde);
		const FVector Mundo = Mata->GetActorLocation() + Onde.GetLocation();
		return FVector2D(Mundo.X, Mundo.Y).Size();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FForestShoreWetsOnlyTheWaterlineTest,
	"BattleSquare.Environment.ForestBackdrop.ShoreWetsOnlyTheWaterline",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FForestShoreWetsOnlyTheWaterlineTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateForestTestWorld();
	if (!World)
	{
		return false;
	}

	// A praia da borda: aqui a costa passa por dentro do ladrilho.
	AForestBackdrop* NaBorda = World->SpawnActor<AForestBackdrop>(
		AForestBackdrop::StaticClass(), FTransform(CentroDoPedacoDaOrlaDeTeste()));
	// E uma praia no MIOLO da ilha, que não existe no mundo mas existe como
	// chamada: é o caso que distingue "o bioma é praia" de "o mar está aqui".
	AForestBackdrop* NoMiolo = World->SpawnActor<AForestBackdrop>();
	if (!NaBorda || !NoMiolo)
	{
		DestroyForestTestWorld(World);
		return false;
	}

	NaBorda->BuildRegion(CasaDeTeste, 4242u, EIslandBiome::Beach, LadoDoPedacoDeTeste);
	NoMiolo->BuildRegion(CasaDeTeste, 4242u, EIslandBiome::Beach, LadoDoPedacoDeTeste);

	const UHierarchicalInstancedStaticMeshComponent* Molhada = NaBorda->GetShoreWetSand();
	if (!TestNotNull(TEXT("a areia molhada existe como componente"), Molhada))
	{
		DestroyForestTestWorld(World);
		return false;
	}

	TestTrue(TEXT("o pedaço da costa ganha areia molhada"),
		Molhada->GetInstanceCount() > 0);
	TestEqual(TEXT("o pedaço do miolo não ganha nenhuma"),
		NoMiolo->GetShoreWetSand()->GetInstanceCount(), 0);

	// E ela é MOLHADA: a faixa fica por DENTRO da linha d'água, nunca no mar.
	const float RaioDaCosta = IslandGeography::LandRadiusUnits();
	for (int32 Instancia = 0; Instancia < Molhada->GetInstanceCount(); ++Instancia)
	{
		if (!TestTrue(TEXT("nenhuma laje de areia cai fora da costa"),
			RaioDaInstanciaDaOrla(NaBorda, Molhada, Instancia) <= RaioDaCosta + 1.0f))
		{
			break;
		}
	}

	// Trocar o bioma APAGA a orla: o mesmo ator é reciclado entre pedaços.
	NaBorda->BuildRegion(CasaDeTeste, 4242u, EIslandBiome::Glacier, LadoDoPedacoDeTeste);
	TestEqual(TEXT("virar geleira não deixa areia molhada para trás"),
		NaBorda->GetShoreWetSand()->GetInstanceCount(), 0);

	DestroyForestTestWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FForestShorePutsFoamOutsideTheSandTest,
	"BattleSquare.Environment.ForestBackdrop.ShorePutsFoamOutsideTheSand",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FForestShorePutsFoamOutsideTheSandTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateForestTestWorld();
	if (!World)
	{
		return false;
	}

	AForestBackdrop* Mata = World->SpawnActor<AForestBackdrop>(
		AForestBackdrop::StaticClass(), FTransform(CentroDoPedacoDaOrlaDeTeste()));
	if (!Mata)
	{
		DestroyForestTestWorld(World);
		return false;
	}

	Mata->BuildRegion(CasaDeTeste, 77u, EIslandBiome::Beach, LadoDoPedacoDeTeste);

	const UHierarchicalInstancedStaticMeshComponent* Molhada = Mata->GetShoreWetSand();
	const UHierarchicalInstancedStaticMeshComponent* Espuma = Mata->GetShoreFoam();
	if (!TestNotNull(TEXT("a espuma existe como componente"), Espuma))
	{
		DestroyForestTestWorld(World);
		return false;
	}

	if (!TestTrue(TEXT("há espuma e há areia molhada para comparar"),
		Espuma->GetInstanceCount() > 0 && Molhada->GetInstanceCount() > 0))
	{
		DestroyForestTestWorld(World);
		return false;
	}

	float AreiaMaisDistante = 0.0f;
	for (int32 Instancia = 0; Instancia < Molhada->GetInstanceCount(); ++Instancia)
	{
		AreiaMaisDistante = FMath::Max(AreiaMaisDistante,
			RaioDaInstanciaDaOrla(Mata, Molhada, Instancia));
	}

	float EspumaMaisPerto = TNumericLimits<float>::Max();
	for (int32 Instancia = 0; Instancia < Espuma->GetInstanceCount(); ++Instancia)
	{
		EspumaMaisPerto = FMath::Min(EspumaMaisPerto,
			RaioDaInstanciaDaOrla(Mata, Espuma, Instancia));
	}

	// A onda quebra DEPOIS da areia. Espuma por dentro seria espuma em terra
	// seca, e as duas na mesma faixa piscariam uma contra a outra.
	TestTrue(TEXT("a espuma toda fica além da areia molhada"),
		EspumaMaisPerto >= AreiaMaisDistante);

	DestroyForestTestWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FForestShoreStandsTreesOnTheSandTest,
	"BattleSquare.Environment.ForestBackdrop.ShoreStandsTreesOnTheSand",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FForestShoreStandsTreesOnTheSandTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateForestTestWorld();
	if (!World)
	{
		return false;
	}

	AForestBackdrop* Mata = World->SpawnActor<AForestBackdrop>(
		AForestBackdrop::StaticClass(), FTransform(CentroDoPedacoDasArvoresDaOrla()));
	if (!Mata)
	{
		DestroyForestTestWorld(World);
		return false;
	}

	Mata->BuildRegion(CasaDeTeste, 909u, EIslandBiome::Beach, LadoDoPedacoDeTeste);

	const UHierarchicalInstancedStaticMeshComponent* Beira = Mata->GetShoreTrees();
	if (!TestNotNull(TEXT("as árvores de beira existem como componente"), Beira))
	{
		DestroyForestTestWorld(World);
		return false;
	}

	// Praia sem nada em pé é uma rampa bege: não há o que dê escala nem o que
	// faça sombra, e o jogador chega na água sem ter visto que chegava.
	if (!TestTrue(TEXT("a praia tem algo em pé"), Beira->GetInstanceCount() > 0))
	{
		DestroyForestTestWorld(World);
		return false;
	}

	// E elas ficam na areia SECA, atrás da onda.
	const float RaioDaCosta = IslandGeography::LandRadiusUnits();
	const float RaioDaAreia = RaioDaCosta - IslandGeography::BeachWidthUnits();
	bool bAlgumaTombada = false;
	for (int32 Instancia = 0; Instancia < Beira->GetInstanceCount(); ++Instancia)
	{
		const float Raio = RaioDaInstanciaDaOrla(Mata, Beira, Instancia);
		if (!TestTrue(TEXT("nenhuma árvore nasce dentro do mar nem longe da praia"),
			Raio < RaioDaCosta && Raio > RaioDaAreia))
		{
			break;
		}

		FTransform Onde;
		Beira->GetInstanceTransform(Instancia, Onde);
		// Árvore de praia cresce torta: o vento vem sempre do mesmo lado.
		bAlgumaTombada = bAlgumaTombada
			|| FMath::Abs(static_cast<float>(Onde.Rotator().Pitch)) > 1.0f;
	}

	TestTrue(TEXT("elas se inclinam, não sobem em prumo"), bAlgumaTombada);

	Mata->BuildRegion(CasaDeTeste, 909u, EIslandBiome::Desert, LadoDoPedacoDeTeste);
	TestEqual(TEXT("o deserto do mesmo lugar não herda as árvores de beira"),
		Mata->GetShoreTrees()->GetInstanceCount(), 0);

	DestroyForestTestWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FForestShoreRepeatsForTheSamePlaceTest,
	"BattleSquare.Environment.ForestBackdrop.ShoreRepeatsForTheSamePlace",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FForestShoreRepeatsForTheSamePlaceTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateForestTestWorld();
	if (!World)
	{
		return false;
	}

	AForestBackdrop* Mata = World->SpawnActor<AForestBackdrop>(
		AForestBackdrop::StaticClass(), FTransform(CentroDoPedacoDaOrlaDeTeste()));
	if (!Mata)
	{
		DestroyForestTestWorld(World);
		return false;
	}

	// Duas montagens do MESMO pedaço, como acontece a cada ida e volta do
	// jogador: a praia precisa ser a mesma praia, e não outra igualmente
	// bonita.
	Mata->BuildRegion(CasaDeTeste, 31337u, EIslandBiome::Beach, LadoDoPedacoDeTeste);
	const int32 LajesDaPrimeira = Mata->GetShoreWetSand()->GetInstanceCount();
	const int32 ArvoresDaPrimeira = Mata->GetShoreTrees()->GetInstanceCount();

	FTransform PrimeiraArvore;
	if (ArvoresDaPrimeira > 0)
	{
		Mata->GetShoreTrees()->GetInstanceTransform(0, PrimeiraArvore);
	}

	Mata->BuildRegion(CasaDeTeste, 31337u, EIslandBiome::Beach, LadoDoPedacoDeTeste);

	TestEqual(TEXT("a mesma quantidade de areia molhada"),
		Mata->GetShoreWetSand()->GetInstanceCount(), LajesDaPrimeira);
	TestEqual(TEXT("a mesma quantidade de árvore de beira"),
		Mata->GetShoreTrees()->GetInstanceCount(), ArvoresDaPrimeira);

	if (ArvoresDaPrimeira > 0)
	{
		FTransform SegundaArvore;
		Mata->GetShoreTrees()->GetInstanceTransform(0, SegundaArvore);
		TestTrue(TEXT("e a primeira árvore está onde estava"),
			SegundaArvore.GetLocation().Equals(PrimeiraArvore.GetLocation(), 0.01));
	}

	DestroyForestTestWorld(World);
	return true;
}

namespace
{
	/**
	 * Um pedaço apoiado EM CIMA de um rio, num raio escolhido pelo curso.
	 *
	 * Nome próprio (L-042). O centro não é escrito à mão: quem decide por onde
	 * o rio passa é o `FreshWater`, e um ponto fixo aqui descolaria do curso na
	 * primeira vez que alguém mexesse na serpente.
	 */
	/**
	 * O primeiro curso que TEM lago, ou o primeiro de todos.
	 *
	 * `Cursos[0]` valia quando cada curso tinha lago e queda por construção.
	 * Hoje o lago nasce onde o leito achata e a queda onde ele despenca, e a
	 * maioria dos cursos não tem nenhum dos dois — pegar o índice zero era
	 * pegar um rio manso e cobrar dele uma cachoeira.
	 */
	const FreshWater::FRiverCourse* CursoComLago()
	{
		static TArray<FreshWater::FRiverCourse> Cursos = FreshWater::Plan();

		for (const FreshWater::FRiverCourse& Curso : Cursos)
		{
			if (Curso.HasLake() && Curso.PointsUnits.Num() > 8)
			{
				return &Curso;
			}
		}

		return Cursos.Num() > 0 ? &Cursos[0] : nullptr;
	}

	/** O primeiro curso que TEM queda. */
	const FreshWater::FRiverCourse* CursoComQueda()
	{
		static TArray<FreshWater::FRiverCourse> Cursos = FreshWater::Plan();

		for (const FreshWater::FRiverCourse& Curso : Cursos)
		{
			if (Curso.HasFall() && Curso.PointsUnits.Num() > 8)
			{
				return &Curso;
			}
		}

		return Cursos.Num() > 0 ? &Cursos[0] : nullptr;
	}

	FVector CentroDoPedacoDoRioDeTeste(float FracaoDoPercurso)
	{
		const TArray<FreshWater::FRiverCourse> Cursos = FreshWater::Plan();
		if (Cursos.Num() == 0)
		{
			return FVector::ZeroVector;
		}

		const FreshWater::FRiverCourse* Curso = CursoComLago();
		if (!Curso)
		{
			return FVector::ZeroVector;
		}

		const FVector2D Ponto = FreshWater::PointAtProgress(*Curso, FracaoDoPercurso);
		return FVector(Ponto.X, Ponto.Y, 0.0f);
	}

	/** Um pedaço no raio exato em que o primeiro rio vira lago. */
	FVector CentroDoPedacoDoLagoDeTeste()
	{
		const TArray<FreshWater::FRiverCourse> Cursos = FreshWater::Plan();
		if (Cursos.Num() == 0)
		{
			return FVector::ZeroVector;
		}

		// No OMBRO do lago, não no meio dele.
		//
		// O teste precisa de um pedaço que contenha as duas larguras — a calha
		// estreita e a lâmina larga. Centrado no meio do lago isso valia só
		// enquanto o lago era menor que um pedaço; hoje ele é mais comprido
		// que um, e o pedaço inteiro cai dentro da água.
		//
		// O ombro é achado PROCURANDO, não por um deslocamento escrito à mão:
		// é o primeiro ponto em que a calha passa do dobro da largura do rio.
		// Assim ele acompanha quando o lago mudar de tamanho de novo.
		const FreshWater::FRiverCourse& Tronco = *CursoComLago();
		const float DoRio = FreshWater::HalfWidthAtProgress(Tronco, 0.0f);

		float NoOmbro = Tronco.LakeAtProgress;
		for (int32 Passo = 0; Passo <= 200; ++Passo)
		{
			const float Onde = Tronco.LakeAtProgress * static_cast<float>(Passo) / 200.0f;
			if (FreshWater::HalfWidthAtProgress(Tronco, Onde) > DoRio * 2.0f)
			{
				NoOmbro = Onde;
				break;
			}
		}

		const FVector2D Ponto = FreshWater::PointAtProgress(*CursoComLago(), NoOmbro);
		return FVector(Ponto.X, Ponto.Y, 0.0f);
	}

	/** Um pedaço no raio exato da queda do primeiro rio. */
	FVector CentroDoPedacoDaQuedaDeTeste()
	{
		const TArray<FreshWater::FRiverCourse> Cursos = FreshWater::Plan();
		if (Cursos.Num() == 0)
		{
			return FVector::ZeroVector;
		}

		const FreshWater::FRiverCourse* DaQueda = CursoComQueda();
		const FVector2D Ponto = DaQueda
			? FreshWater::PointAtProgress(*DaQueda, DaQueda->FallAtProgress)
			: FVector2D::ZeroVector;
		return FVector(Ponto.X, Ponto.Y, 0.0f);
	}

	/** A altura local do topo de uma instância deste agrupamento. */
	float AlturaDaInstanciaDoRio(const UHierarchicalInstancedStaticMeshComponent* Grupo,
		int32 Instancia)
	{
		FTransform Onde;
		Grupo->GetInstanceTransform(Instancia, Onde);
		return static_cast<float>(Onde.GetLocation().Z);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FForestRiverRunsThroughTheChunkItCrossesTest,
	"BattleSquare.Environment.ForestBackdrop.RiverRunsThroughTheChunkItCrosses",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FForestRiverRunsThroughTheChunkItCrossesTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateForestTestWorld();
	if (!World)
	{
		AddError(TEXT("Sem mundo de teste"));
		return false;
	}

	AForestBackdrop* NoRio = World->SpawnActor<AForestBackdrop>(
		AForestBackdrop::StaticClass(), FTransform(CentroDoPedacoDoRioDeTeste(0.7f)));

	// O miolo da ilha fica dentro do anel dos montes: nenhum rio nasce lá.
	AForestBackdrop* NoMiolo = World->SpawnActor<AForestBackdrop>();
	if (!NoRio || !NoMiolo)
	{
		DestroyForestTestWorld(World);
		AddError(TEXT("Sem cenário"));
		return false;
	}

	NoRio->BuildRegion(CasaDeTeste, 4242u, EIslandBiome::Forest, LadoDoPedacoDeTeste);
	NoMiolo->BuildRegion(CasaDeTeste, 4242u, EIslandBiome::Forest, LadoDoPedacoDeTeste);

	TestTrue(TEXT("o pedaço por onde o rio passa tem lâmina d'água"),
		NoRio->GetRiverSurface()->GetInstanceCount() > 0);
	TestEqual(TEXT("e o pedaço do miolo não tem nenhuma"),
		NoMiolo->GetRiverSurface()->GetInstanceCount(), 0);

	// A lâmina fica ACIMA do chão, nunca dentro dele: foi afundar no chão que
	// produziu "parte da agua ele afunda".
	const float TopoDoChao = AForestBackdrop::GroundTopLocalZ();
	for (int32 Instancia = 0; Instancia < NoRio->GetRiverSurface()->GetInstanceCount();
		++Instancia)
	{
		TestTrue(TEXT("cada laje do rio corre por cima do chão"),
			AlturaDaInstanciaDoRio(NoRio->GetRiverSurface(), Instancia) > TopoDoChao);
	}

	// E a lâmina não fecha: o pet ATRAVESSA o rio, não esbarra nele.
	TestEqual(TEXT("a lâmina d'água não tem colisão"),
		static_cast<int32>(NoRio->GetRiverSurface()->GetCollisionEnabled()),
		static_cast<int32>(ECollisionEnabled::NoCollision));

	DestroyForestTestWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FForestRiverWidensIntoALakeTest,
	"BattleSquare.Environment.ForestBackdrop.RiverWidensIntoALake",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FForestRiverWidensIntoALakeTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateForestTestWorld();
	if (!World)
	{
		AddError(TEXT("Sem mundo de teste"));
		return false;
	}

	AForestBackdrop* NoLago = World->SpawnActor<AForestBackdrop>(
		AForestBackdrop::StaticClass(), FTransform(CentroDoPedacoDoLagoDeTeste()));
	const TArray<FreshWater::FRiverCourse> Cursos = FreshWater::Plan();
	if (!NoLago || Cursos.Num() == 0)
	{
		DestroyForestTestWorld(World);
		AddError(TEXT("Sem cenário"));
		return false;
	}

	NoLago->BuildRegion(CasaDeTeste, 909u, EIslandBiome::Forest, LadoDoPedacoDeTeste);

	// A comparação é entre LAJES, e não entre pedaços. O ladrilho tem 6400 de
	// lado e o rio inteiro cabe dentro dele: "o pedaço do lago" e "o pedaço do
	// rio" seriam o mesmo pedaço com dois nomes, e foi exatamente isso que a
	// primeira versão deste teste afirmou — e errou.
	//
	// A largura de uma laje é a escala em Y: depois do giro, Y atravessa a
	// calha. Se a laje mais larga não estiver no lago, ele não é lago — é rio
	// com outro nome.
	const UHierarchicalInstancedStaticMeshComponent* Agua = NoLago->GetRiverSurface();
	TestTrue(TEXT("o pedaço do lago tem água"), Agua->GetInstanceCount() > 0);

	float MaiorLargura = 0.0f;
	float RaioDaMaior = 0.0f;
	float MenorLargura = TNumericLimits<float>::Max();
	float RaioDaMenor = 0.0f;
	for (int32 Instancia = 0; Instancia < Agua->GetInstanceCount(); ++Instancia)
	{
		FTransform Onde;
		Agua->GetInstanceTransform(Instancia, Onde);
		const FVector Mundo = NoLago->GetActorLocation() + Onde.GetLocation();
		const float Raio = FVector2D(Mundo.X, Mundo.Y).Size();
		const float Largura = static_cast<float>(Onde.GetScale3D().Y);

		if (Largura > MaiorLargura)
		{
			MaiorLargura = Largura;
			RaioDaMaior = Raio;
		}
		if (Largura < MenorLargura)
		{
			MenorLargura = Largura;
			RaioDaMenor = Raio;
		}
	}

	// O lago é medido em PROGRESSO agora; para comparar com o raio das lajes,
	// pergunta-se o ponto e mede-se dele.
	const float RaioDoLago = static_cast<float>(
		FreshWater::PointAtProgress(*CursoComLago(), CursoComLago()->LakeAtProgress).Size());
	TestTrue(TEXT("a calha mais larga é bem mais larga que a mais estreita"),
		MaiorLargura > MenorLargura * 2.0f);
	TestTrue(TEXT("e ela está do lado do lago, não num trecho qualquer"),
		FMath::Abs(RaioDaMaior - RaioDoLago) < FMath::Abs(RaioDaMenor - RaioDoLago));

	DestroyForestTestWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FForestRiverFoamsOnlyWhereItFallsTest,
	"BattleSquare.Environment.ForestBackdrop.RiverFoamsOnlyWhereItFalls",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FForestRiverFoamsOnlyWhereItFallsTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateForestTestWorld();
	if (!World)
	{
		AddError(TEXT("Sem mundo de teste"));
		return false;
	}

	AForestBackdrop* NaQueda = World->SpawnActor<AForestBackdrop>(
		AForestBackdrop::StaticClass(), FTransform(CentroDoPedacoDaQuedaDeTeste()));
	if (!NaQueda)
	{
		DestroyForestTestWorld(World);
		AddError(TEXT("Sem cenário"));
		return false;
	}

	NaQueda->BuildRegion(CasaDeTeste, 55u, EIslandBiome::Forest, LadoDoPedacoDeTeste);

	const UHierarchicalInstancedStaticMeshComponent* Espuma = NaQueda->GetRiverFallFoam();
	TestTrue(TEXT("no degrau a água espuma"), Espuma->GetInstanceCount() > 0);

	// E espuma SÓ ali. O ladrilho é maior que o rio inteiro, então o pedaço da
	// queda contém também o lago e o trecho calmo: quem separa água batida de
	// água parada é o RAIO de cada laje, nunca o pedaço em que ela caiu.
	const TArray<FreshWater::FRiverCourse> Cursos = FreshWater::Plan();
	for (int32 Instancia = 0; Instancia < Espuma->GetInstanceCount(); ++Instancia)
	{
		FTransform Onde;
		Espuma->GetInstanceTransform(Instancia, Onde);
		const FVector Mundo = NaQueda->GetActorLocation() + Onde.GetLocation();
		const float Raio = FVector2D(Mundo.X, Mundo.Y).Size();

		bool bNumDegrau = false;
		for (const FreshWater::FRiverCourse& Curso : Cursos)
		{
			// Qual PONTO do curso está mais perto desta laje — e ele é quem diz
			// se aqui é degrau. Perguntar por raio pulava os trechos que
			// correm de lado.
			float NoCurso = 0.0f;
			FreshWater::NearestOn(Curso, FVector2D(Mundo.X, Mundo.Y), NoCurso);
			bNumDegrau = bNumDegrau || FreshWater::IsFallAtProgress(Curso, NoCurso);
		}

		TestTrue(TEXT("toda espuma está num degrau, e água parada não espuma"),
			bNumDegrau);
	}

	// A espuma vai por CIMA da lâmina: espuma na mesma altura da água briga
	// por pixel com ela e pisca conforme a câmera anda.
	if (NaQueda->GetRiverFallFoam()->GetInstanceCount() > 0
		&& NaQueda->GetRiverSurface()->GetInstanceCount() > 0)
	{
		TestTrue(TEXT("a espuma da queda fica acima da lâmina d'água"),
			AlturaDaInstanciaDoRio(NaQueda->GetRiverFallFoam(), 0)
			> AlturaDaInstanciaDoRio(NaQueda->GetRiverSurface(), 0));
	}

	DestroyForestTestWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FForestRiverTrailWalksTheBankTest,
	"BattleSquare.Environment.ForestBackdrop.RiverTrailWalksTheBank",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FForestRiverTrailWalksTheBankTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateForestTestWorld();
	if (!World)
	{
		AddError(TEXT("Sem mundo de teste"));
		return false;
	}

	AForestBackdrop* NoRio = World->SpawnActor<AForestBackdrop>(
		AForestBackdrop::StaticClass(), FTransform(CentroDoPedacoDoRioDeTeste(0.5f)));
	if (!NoRio)
	{
		DestroyForestTestWorld(World);
		AddError(TEXT("Sem cenário"));
		return false;
	}

	NoRio->BuildRegion(CasaDeTeste, 606u, EIslandBiome::Forest, LadoDoPedacoDeTeste);

	TestTrue(TEXT("quem segue o rio tem por onde andar"),
		NoRio->GetRiverTrail()->GetInstanceCount() > 0);

	// A trilha é chão pisado: fica MAIS BAIXA que a água, ou pareceria uma
	// ponte correndo ao lado do rio.
	if (NoRio->GetRiverTrail()->GetInstanceCount() > 0
		&& NoRio->GetRiverSurface()->GetInstanceCount() > 0)
	{
		TestTrue(TEXT("a trilha corre rente ao chão, abaixo da água"),
			AlturaDaInstanciaDoRio(NoRio->GetRiverTrail(), 0)
			< AlturaDaInstanciaDoRio(NoRio->GetRiverSurface(), 0));
	}

	// E ela nunca cai dentro da calha: trilha que atravessa a água é trilha
	// que some justamente onde o jogador precisaria dela.
	const FVector CentroDoPedaco = NoRio->GetActorLocation();
	const TArray<FreshWater::FRiverCourse> Cursos = FreshWater::Plan();
	for (int32 Instancia = 0; Instancia < NoRio->GetRiverTrail()->GetInstanceCount();
		++Instancia)
	{
		FTransform Onde;
		NoRio->GetRiverTrail()->GetInstanceTransform(Instancia, Onde);
		const FVector Mundo = CentroDoPedaco + Onde.GetLocation();
		const FVector2D Plano(Mundo.X, Mundo.Y);
		const float Raio = Plano.Size();

		for (const FreshWater::FRiverCourse& Curso : Cursos)
		{
			if (Raio < Curso.SourceRadiusUnits || Raio > Curso.MouthRadiusUnits)
			{
				continue;
			}

			float NoCurso = 0.0f;
			const float Ate = FreshWater::NearestOn(Curso, Plano, NoCurso);

			// A calha, com a tolerância de meia laje: a trilha de beira-rio é
			// posta em ladrilhos, e um ladrilho encostando na lâmina não é
			// trilha dentro da água — é a borda dos dois se tocando, que é o
			// que uma margem é.
			// A tolerância é a trilha INTEIRA, e não meia: o ladrilho é posto
			// centrado no eixo dela, e o que se quer garantir é que o CENTRO da
			// trilha não esteja na água — a borda tocando a lâmina é a margem,
			// que é onde uma trilha de beira-rio anda.
			TestTrue(TEXT("nenhum trecho de trilha cai dentro da água"),
				Ate > FreshWater::HalfWidthAtProgress(Curso, NoCurso)
					- FreshWater::TrailHalfWidthUnits() * 2.0f);
		}
	}

	DestroyForestTestWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FForestRiverLeavesNoWaterBehindTest,
	"BattleSquare.Environment.ForestBackdrop.RiverLeavesNoWaterBehind",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FForestRiverLeavesNoWaterBehindTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateForestTestWorld();
	if (!World)
	{
		AddError(TEXT("Sem mundo de teste"));
		return false;
	}

	// O ator é REAPROVEITADO de um pedaço para o outro. Sem limpeza, um trecho
	// de rio do pedaço anterior continuaria correndo por onde rio não passa.
	AForestBackdrop* Mata = World->SpawnActor<AForestBackdrop>(
		AForestBackdrop::StaticClass(), FTransform(CentroDoPedacoDoRioDeTeste(0.7f)));
	if (!Mata)
	{
		DestroyForestTestWorld(World);
		AddError(TEXT("Sem cenário"));
		return false;
	}

	Mata->BuildRegion(CasaDeTeste, 7u, EIslandBiome::Forest, LadoDoPedacoDeTeste);
	TestTrue(TEXT("o pedaço do rio nasce com água"),
		Mata->GetRiverSurface()->GetInstanceCount() > 0);

	Mata->SetActorLocation(FVector::ZeroVector);
	Mata->BuildRegion(CasaDeTeste, 7u, EIslandBiome::Forest, LadoDoPedacoDeTeste);
	TestEqual(TEXT("mudou para o miolo e a água ficou para trás"),
		Mata->GetRiverSurface()->GetInstanceCount(), 0);
	TestEqual(TEXT("a espuma também"),
		Mata->GetRiverFallFoam()->GetInstanceCount(), 0);
	TestEqual(TEXT("e a trilha de beira também"),
		Mata->GetRiverTrail()->GetInstanceCount(), 0);

	// A arena é um recorte de mata, não um trecho da ilha: rio nenhum ali.
	Mata->SetActorLocation(CentroDoPedacoDoRioDeTeste(0.7f));
	Mata->BuildRegion(CasaDeTeste, 7u, EIslandBiome::Forest, LadoDoPedacoDeTeste);
	Mata->BuildForest(CasaDeTeste, 7u, CameraDeTeste);
	TestEqual(TEXT("a arena não tem rio"),
		Mata->GetRiverSurface()->GetInstanceCount(), 0);

	DestroyForestTestWorld(World);
	return true;
}

namespace
{
	/**
	 * O lado do cubo da engine, que é a unidade em que a escala da laje fala.
	 *
	 * Nome próprio (L-042). Escrito aqui e não importado porque a constante do
	 * cenário é privada do `.cpp` dele — e um teste que enxerga o privado mede
	 * a implementação, não o resultado.
	 */
	constexpr float CilindroDaEngineNoTeste = 100.0f;

	/**
	 * A maior inclinação, em graus, de qualquer planta plantada neste cenário.
	 *
	 * Nome próprio (L-042). Mede o TOMBO e não o giro: quem gira em torno de si
	 * continua em pé, e árvore em pé é o que o pântano não tem.
	 */
	float MaiorTomboDoBrejoDeTeste(const AForestBackdrop* Mata)
	{
		float Maior = 0.0f;
		for (const TObjectPtr<UHierarchicalInstancedStaticMeshComponent>& Grupo :
			Mata->GetSpeciesClusters())
		{
			if (!Grupo)
			{
				continue;
			}
			for (int32 Instancia = 0; Instancia < Grupo->GetInstanceCount(); ++Instancia)
			{
				FTransform Onde;
				Grupo->GetInstanceTransform(Instancia, Onde);
				const FRotator Giro = Onde.GetRotation().Rotator();
				Maior = FMath::Max(Maior,
					static_cast<float>(FMath::Abs(Giro.Pitch)));
			}
		}
		return Maior;
	}

	/**
	 * Quantas FATIAS de planta estão pintadas com exatamente esta cor.
	 *
	 * A cor entra pronta, e não o papel: a paleta desvia madeira, folha e flor
	 * pelo NOME da fatia antes de olhar o papel, então `ColorFor(papel, fatia)`
	 * devolve marrom de casca para qualquer papel numa fatia de tronco. Cobrar
	 * aquilo acusaria de água toda planta de madeira do bioma.
	 */
	int32 FatiasComACorDoBrejoDeTeste(const AForestBackdrop* Mata, const FLinearColor& Cor)
	{
		int32 Total = 0;
		for (const TObjectPtr<UHierarchicalInstancedStaticMeshComponent>& Grupo :
			Mata->GetSpeciesClusters())
		{
			if (!Grupo)
			{
				continue;
			}
			const int32 Fatias = Grupo->GetMaterialSlotNames().Num();
			for (int32 Fatia = 0; Fatia < Fatias; ++Fatia)
			{
				UMaterialInstanceDynamic* Tinta =
					Cast<UMaterialInstanceDynamic>(Grupo->GetMaterial(Fatia));
				if (Tinta
					&& Tinta->K2_GetVectorParameterValue(TEXT("Color")).Equals(Cor, 0.001f))
				{
					++Total;
				}
			}
		}
		return Total;
	}
}

/**
 * O PÂNTANO TEM ÁGUA NA TELA, e ela é uma superfície deitada no chão.
 *
 * O defeito que este teste tranca já esteve no jogo: a pedra do brejo era
 * pintada com a cor da água parada, para haver "alguma água" no bioma. O que
 * aquilo produzia era uma pedra em forma de pedra e cor de poça — nem uma coisa
 * nem a outra —, e água de verdade não existia em lugar nenhum do pântano.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FForestSwampPoolsOnlyInTheSwampTest,
	"BattleSquare.Environment.ForestBackdrop.SwampPoolsOnlyInTheSwamp",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FForestSwampPoolsOnlyInTheSwampTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateForestTestWorld();
	if (!World)
	{
		AddError(TEXT("Sem mundo de teste"));
		return false;
	}

	AForestBackdrop* Mata = World->SpawnActor<AForestBackdrop>();
	if (!Mata)
	{
		DestroyForestTestWorld(World);
		AddError(TEXT("Sem cenário"));
		return false;
	}

	UHierarchicalInstancedStaticMeshComponent* Pocas = Mata->GetSwampPools();
	if (!TestNotNull(TEXT("o cenário tem componente de poça"), Pocas))
	{
		DestroyForestTestWorld(World);
		return false;
	}

	Mata->BuildRegion(CasaDeTeste, 31u, EIslandBiome::Swamp, LadoDoPedacoDeTeste);
	TestTrue(TEXT("o pântano nasce com água parada na tela"),
		Pocas->GetInstanceCount() > 0);

	// A poça é uma SUPERFÍCIE: rente ao chão, achatada e sem colisão. Laje
	// cúbica seria uma pedra azul, e laje que barra o passo é degrau — foi
	// degrau na água que produziu "parte da agua ele afunda".
	const float TopoDoChao = AForestBackdrop::GroundTopLocalZ();
	int32 Afundadas = 0;
	int32 Cubicas = 0;
	for (int32 Instancia = 0; Instancia < Pocas->GetInstanceCount(); ++Instancia)
	{
		FTransform Onde;
		Pocas->GetInstanceTransform(Instancia, Onde);
		if (Onde.GetLocation().Z <= TopoDoChao)
		{
			++Afundadas;
		}
		const FVector Escala = Onde.GetScale3D();
		if (Escala.Z >= Escala.X * 0.25 || !FMath::IsNearlyEqual(Escala.X, Escala.Y, 0.001))
		{
			++Cubicas;
		}
	}
	TestEqual(TEXT("nenhuma laje de poça afunda no chão"), Afundadas, 0);
	TestEqual(TEXT("toda laje de poça é uma lâmina achatada, não um cubo"), Cubicas, 0);
	TestEqual(TEXT("a poça não tem colisão"),
		static_cast<int32>(Pocas->GetCollisionEnabled()),
		static_cast<int32>(ECollisionEnabled::NoCollision));

	// E a água é do BREJO: o mesmo ator vira geleira no pedaço seguinte, e poça
	// esquecida seria água parada sobre o gelo.
	Mata->BuildRegion(CasaDeTeste, 31u, EIslandBiome::Glacier, LadoDoPedacoDeTeste);
	TestEqual(TEXT("a geleira não herda a poça do pântano"),
		Pocas->GetInstanceCount(), 0);

	Mata->BuildRegion(CasaDeTeste, 31u, EIslandBiome::Forest, LadoDoPedacoDeTeste);
	TestEqual(TEXT("nem a floresta"), Pocas->GetInstanceCount(), 0);

	DestroyForestTestWorld(World);
	return true;
}

/**
 * A cor da ÁGUA PARADA veste a água, e nenhuma planta.
 *
 * É a metade da regra que o contador de poças não cobre: se a pedra voltar a
 * ser pintada de poça, o pântano volta a ter duas coisas erradas em vez de uma
 * certa, e a tela fica igualzinha à de antes.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FForestSwampRocksAreNotPaintedAsWaterTest,
	"BattleSquare.Environment.ForestBackdrop.SwampRocksAreNotPaintedAsWater",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FForestSwampRocksAreNotPaintedAsWaterTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateForestTestWorld();
	if (!World)
	{
		AddError(TEXT("Sem mundo de teste"));
		return false;
	}

	AForestBackdrop* Mata = World->SpawnActor<AForestBackdrop>();
	if (!Mata)
	{
		DestroyForestTestWorld(World);
		AddError(TEXT("Sem cenário"));
		return false;
	}

	Mata->BuildRegion(CasaDeTeste, 77u, EIslandBiome::Swamp, LadoDoPedacoDeTeste);

	// `NAME_None` é de propósito: é a fatia que não dispara nenhum desvio por
	// nome, então o que volta é a cor CRUA da água parada.
	const FLinearColor CorDaAguaParada =
		ScenaryPalette::ColorFor(EScenaryRole::SwampWater, NAME_None);

	TestEqual(TEXT("nenhuma planta do pântano se faz passar por água parada"),
		FatiasComACorDoBrejoDeTeste(Mata, CorDaAguaParada), 0);

	UHierarchicalInstancedStaticMeshComponent* Pocas = Mata->GetSwampPools();
	if (TestNotNull(TEXT("o cenário tem componente de poça"), Pocas))
	{
		UMaterialInstanceDynamic* Tinta =
			Cast<UMaterialInstanceDynamic>(Pocas->GetMaterial(0));
		const TArray<FName> Fatias = Pocas->GetMaterialSlotNames();
		if (TestNotNull(TEXT("a poça sai da nossa paleta"), Tinta) && Fatias.Num() > 0)
		{
			TestTrue(TEXT("e a cor dela é a da água parada"),
				Tinta->K2_GetVectorParameterValue(TEXT("Color"))
					.Equals(CorDaAguaParada, 0.001f));
		}
	}

	DestroyForestTestWorld(World);
	return true;
}

/**
 * No brejo o tronco TOMBA, e é isso que separa pântano de mata escura.
 *
 * O tombo era fixo em quatro graus para todo bioma — um jitter que impede a
 * fileira de postes e que ninguém lê como inclinação. Sem tombo visível, o
 * pântano ficava sendo uma floresta com poças no chão.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FForestSwampTrunksLeanMoreThanTheForestTest,
	"BattleSquare.Environment.ForestBackdrop.SwampTrunksLeanMoreThanTheForest",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FForestSwampTrunksLeanMoreThanTheForestTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateForestTestWorld();
	if (!World)
	{
		AddError(TEXT("Sem mundo de teste"));
		return false;
	}

	AForestBackdrop* NoBrejo = World->SpawnActor<AForestBackdrop>();
	AForestBackdrop* NaMata = World->SpawnActor<AForestBackdrop>();
	if (!NoBrejo || !NaMata)
	{
		DestroyForestTestWorld(World);
		AddError(TEXT("Sem cenário"));
		return false;
	}

	// A MESMA semente nos dois: o que muda é o bioma, e é só o bioma que este
	// teste está medindo.
	NoBrejo->BuildRegion(CasaDeTeste, 909u, EIslandBiome::Swamp, LadoDoPedacoDeTeste);
	NaMata->BuildRegion(CasaDeTeste, 909u, EIslandBiome::Forest, LadoDoPedacoDeTeste);

	const float TomboDoBrejo = MaiorTomboDoBrejoDeTeste(NoBrejo);
	const float TomboDaMata = MaiorTomboDoBrejoDeTeste(NaMata);

	TestTrue(TEXT("a mata fica em pé — o desvio dela é jitter, não inclinação"),
		TomboDaMata < 8.0f);
	TestTrue(TEXT("e no brejo o tronco tomba bem mais que na mata"),
		TomboDoBrejo > TomboDaMata * 2.0f);

	DestroyForestTestWorld(World);
	return true;
}

/**
 * A BATALHA no pântano tem pântano na tela.
 *
 * É a resposta direta a "quando eu iniciei a arena ela não pegou a parte do
 * cenario". O diorama recusa relevo, orla e rio de propósito — os três precisam
 * saber onde ficam no mundo. A poça não: ela é propriedade do BIOMA, e nasce
 * onde o brejo estiver.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FForestSwampArenaGetsPoolsTest,
	"BattleSquare.Environment.ForestBackdrop.SwampArenaGetsPools",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FForestSwampArenaGetsPoolsTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateForestTestWorld();
	if (!World)
	{
		AddError(TEXT("Sem mundo de teste"));
		return false;
	}

	AForestBackdrop* Mata = World->SpawnActor<AForestBackdrop>();
	if (!Mata)
	{
		DestroyForestTestWorld(World);
		AddError(TEXT("Sem cenário"));
		return false;
	}

	UHierarchicalInstancedStaticMeshComponent* Pocas = Mata->GetSwampPools();
	if (!TestNotNull(TEXT("o cenário tem componente de poça"), Pocas))
	{
		DestroyForestTestWorld(World);
		return false;
	}

	Mata->BuildForest(CasaDeTeste, 55u, CameraDeTeste, EIslandBiome::Swamp);
	TestTrue(TEXT("a arena do pântano tem água parada"), Pocas->GetInstanceCount() > 0);

	const float RaioDoChao = Mata->GroundRadiusInCells * CasaDeTeste;
	const float FolgaDoTabuleiro = Mata->BoardClearanceInCells * CasaDeTeste;

	int32 ForaDoDisco = 0;
	int32 SobOTabuleiro = 0;
	for (int32 Instancia = 0; Instancia < Pocas->GetInstanceCount(); ++Instancia)
	{
		FTransform Onde;
		Pocas->GetInstanceTransform(Instancia, Onde);
		const FVector2D Plano(Onde.GetLocation().X, Onde.GetLocation().Y);
		// A METADE da laje é o que passa da borda, não o centro dela: laje
		// flutuando meio corpo fora do disco é o mesmo defeito que a poça
		// debaixo do tabuleiro, só do outro lado.
		const float MeiaLaje =
			static_cast<float>(Onde.GetScale3D().X) * CilindroDaEngineNoTeste * 0.5f;
		if (Plano.Size() + MeiaLaje > RaioDoChao)
		{
			++ForaDoDisco;
		}
		if (Plano.Size() - MeiaLaje < FolgaDoTabuleiro)
		{
			++SobOTabuleiro;
		}
	}
	TestEqual(TEXT("nenhuma laje de poça passa da borda do chão da arena"),
		ForaDoDisco, 0);
	TestEqual(TEXT("e nenhuma entra por baixo do tabuleiro"),
		SobOTabuleiro, 0);

	// E a arena que não é pântano continua seca.
	Mata->BuildForest(CasaDeTeste, 55u, CameraDeTeste, EIslandBiome::Forest);
	TestEqual(TEXT("a arena da floresta não tem poça"), Pocas->GetInstanceCount(), 0);

	DestroyForestTestWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FForestGrowsWithWorldAgeTest,
	"BattleSquare.Environment.Crescimento.MataCresceMasNaoSeMove",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FForestGrowsWithWorldAgeTest::RunTest(const FString&)
{
	// MV2 — o contrapeso: a MESMA semente, medida em duas idades do mundo,
	// muda a ESCALA mas NÃO a posição nem a espécie. Crescer não é replantar.
	UWorld* World = CreateForestTestWorld();

	AForestBackdrop* Jovem = World->SpawnActor<AForestBackdrop>();
	AForestBackdrop* Madura = World->SpawnActor<AForestBackdrop>();
	if (!Jovem || !Madura)
	{
		DestroyForestTestWorld(World);
		return false;
	}

	// Mesma semente, mesmo bioma, mesmo lado — só a idade do mundo difere.
	Jovem->BuildRegion(CasaDeTeste, 31u, EIslandBiome::Forest, LadoDoPedacoDeTeste, /*WorldAgeInDays=*/0);
	Madura->BuildRegion(CasaDeTeste, 31u, EIslandBiome::Forest, LadoDoPedacoDeTeste, /*WorldAgeInDays=*/3000);

	const auto& EspeciesJovem = Jovem->GetSpeciesClusters();
	const auto& EspeciesMadura = Madura->GetSpeciesClusters();
	TestEqual(TEXT("o número de espécies não muda com a idade"),
		EspeciesJovem.Num(), EspeciesMadura.Num());

	int32 InstanciasComparadas = 0;
	int32 MenoresQuandoJovem = 0;
	for (int32 E = 0; E < EspeciesJovem.Num(); ++E)
	{
		UHierarchicalInstancedStaticMeshComponent* J = EspeciesJovem[E].Get();
		UHierarchicalInstancedStaticMeshComponent* M = EspeciesMadura[E].Get();
		if (!J || !M)
		{
			continue;
		}
		// Mesma quantidade por espécie: nada nasceu nem sumiu com a idade.
		if (!TestEqual(TEXT("a espécie tem a mesma contagem nas duas idades"),
			J->GetInstanceCount(), M->GetInstanceCount()))
		{
			continue;
		}
		for (int32 I = 0; I < J->GetInstanceCount(); ++I)
		{
			FTransform Tj, Tm;
			J->GetInstanceTransform(I, Tj);
			M->GetInstanceTransform(I, Tm);

			// A POSIÇÃO vem só da semente — igual nas duas idades.
			TestTrue(TEXT("a árvore não se moveu ao crescer"),
				Tj.GetLocation().Equals(Tm.GetLocation(), 0.1f));

			// A ESCALA cresce: a jovem é menor ou igual; ao menos uma, menor.
			const float Ej = Tj.GetScale3D().X;
			const float Em = Tm.GetScale3D().X;
			TestTrue(TEXT("a muda nunca é maior que a adulta"), Ej <= Em + 0.001f);
			if (Ej < Em - 0.001f)
			{
				++MenoresQuandoJovem;
			}
			++InstanciasComparadas;
		}
	}

	TestTrue(TEXT("houve árvore para comparar"), InstanciasComparadas > 0);
	TestTrue(TEXT("a mata jovem é de fato menor — a idade entra na escala"),
		MenoresQuandoJovem > 0);

	DestroyForestTestWorld(World);
	return true;
}

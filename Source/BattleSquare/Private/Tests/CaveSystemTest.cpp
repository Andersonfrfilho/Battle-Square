// Copyright 2026 Anderson. All Rights Reserved.

#include "Environment/CaveSystem.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Environment/ScenaryPalette.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/AutomationTest.h"
#include "PhysicsEngine/BodySetup.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// Nome próprio (L-042): helper homônimo em outro arquivo de teste vira
	// sobrecarga ambígua quando o unity build junta os dois.
	UWorld* CreateCaveSystemTestWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
		FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
		WorldContext.SetCurrentWorld(World);
		World->InitializeActorsForPlay(FURL());
		return World;
	}

	void DestroyCaveSystemTestWorld(UWorld* World)
	{
		if (!World)
		{
			return;
		}
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
	}

	constexpr uint32 SementeDaCaverna = 20260902u;

	/** Quantas lajes de parede a planta EXIGE, contadas sem olhar o ator. */
	int32 LajesQueAPlantaExige(const CaveLabyrinth::FCaveGrid& Planta)
	{
		// Segmentos internos (2CR − C − R) mais a borda (2C + 2R), menos as
		// passagens escavadas e menos TODAS as bocas. Contar assim, e não
		// repetindo a regra de desenho do ator, é o que faz o teste ser uma
		// verificação e não um espelho (L-032).
		//
		// Eram "menos a boca", no singular, e valia quando a caverna tinha uma
		// saída só. Cada boca extra também abre a muralha, e ninguém as estava
		// descontando: a conta dava exatamente duas lajes a mais, com duas
		// bocas extras.
		const int32 C = Planta.Columns;
		const int32 R = Planta.Rows;
		const int32 Todos = (2 * C * R - C - R) + (2 * C + 2 * R);
		return Todos - CaveLabyrinth::OpenPassageCount(Planta)
			- 1 - Planta.ExtraMouths.Num();
	}

	/** A caixa que uma instância ocupa, no espaço do ator. */
	bool CaixaDaLajeDaCaverna(const UHierarchicalInstancedStaticMeshComponent* Pedra,
		int32 Qual, FBox& OutCaixa)
	{
		if (!Pedra || !Pedra->GetStaticMesh())
		{
			return false;
		}

		FTransform Laje;
		if (!Pedra->GetInstanceTransform(Qual, Laje, false))
		{
			return false;
		}

		const FVector Meio = 0.5f * Pedra->GetStaticMesh()->GetBoundingBox().GetSize()
			* Laje.GetScale3D();
		OutCaixa = FBox(Laje.GetLocation() - Meio, Laje.GetLocation() + Meio);
		return true;
	}

	/** Alguma laje destes dois componentes cobre este ponto? */
	bool PedraCobreOPonto(const ACaveSystem& Caverna, const FVector& Ponto)
	{
		const UHierarchicalInstancedStaticMeshComponent* Pedras[2] = {
			Caverna.GetWalls(), Caverna.GetShell() };

		for (const UHierarchicalInstancedStaticMeshComponent* Pedra : Pedras)
		{
			if (!Pedra)
			{
				continue;
			}
			for (int32 Qual = 0; Qual < Pedra->GetInstanceCount(); ++Qual)
			{
				FBox Caixa;
				if (CaixaDaLajeDaCaverna(Pedra, Qual, Caixa) && Caixa.IsInsideOrOn(Ponto))
				{
					return true;
				}
			}
		}

		return false;
	}

	/** Onde ficam todas as lajes de um componente, para comparar duas cavernas. */
	TArray<FVector> PosicoesDasLajesDaCaverna(const UHierarchicalInstancedStaticMeshComponent* Pedra)
	{
		TArray<FVector> Posicoes;
		if (!Pedra)
		{
			return Posicoes;
		}

		for (int32 Qual = 0; Qual < Pedra->GetInstanceCount(); ++Qual)
		{
			FTransform Laje;
			if (Pedra->GetInstanceTransform(Qual, Laje, false))
			{
				Posicoes.Add(Laje.GetLocation());
			}
		}

		return Posicoes;
	}

	/**
	 * Planta uma caverna com o sabor pedido.
	 *
	 * Funil ÚNICO: os testes de sabor e os de geometria precisam sair do mesmo
	 * lugar, senão o dia em que `FCaveRecipe` ganhar um campo só metade dos
	 * testes passa a exercitá-lo.
	 */
	ACaveSystem* PlantaCavernaComSabor(UWorld* World, int32 Colunas, int32 Linhas,
		uint32 Semente, ECaveFlavor Sabor)
	{
		ACaveSystem* Caverna = World->SpawnActor<ACaveSystem>();
		if (Caverna)
		{
			FCaveRecipe Receita;
			Receita.Columns = Colunas;
			Receita.Rows = Linhas;
			Receita.Seed = Semente;
			Receita.Flavor = Sabor;
			Caverna->BuildCave(Receita);
		}
		return Caverna;
	}

	ACaveSystem* PlantaCavernaDeTeste(UWorld* World, int32 Colunas, int32 Linhas, uint32 Semente)
	{
		return PlantaCavernaComSabor(World, Colunas, Linhas, Semente, ECaveFlavor::Dry);
	}

	/** Quantas instâncias um componente tem, sem estourar em ponteiro nulo. */
	int32 QuantasInstanciasNaCaverna(const UHierarchicalInstancedStaticMeshComponent* Pedra)
	{
		return Pedra ? Pedra->GetInstanceCount() : -1;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCaveSystemAssignsMeshesTest,
	"BattleSquare.CaveSystem.AssignsMeshes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCaveSystemAssignsMeshesTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateCaveSystemTestWorld();
	ACaveSystem* Caverna = PlantaCavernaDeTeste(World, ACaveSystem::SmallCaveSide,
		ACaveSystem::SmallCaveSide, SementeDaCaverna);

	if (!TestNotNull(TEXT("A caverna nasceu"), Caverna))
	{
		DestroyCaveSystemTestWorld(World);
		return false;
	}

	// Componente criado não é componente visível: o que falha calado é a malha
	// que nunca foi atribuída.
	TestNotNull(TEXT("O chão tem malha"), Caverna->GetFloor()->GetStaticMesh().Get());
	TestNotNull(TEXT("As paredes têm malha"), Caverna->GetWalls()->GetStaticMesh().Get());
	TestNotNull(TEXT("A muralha tem malha"), Caverna->GetShell()->GetStaticMesh().Get());

	DestroyCaveSystemTestWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCaveSystemBlocksTest,
	"BattleSquare.CaveSystem.Blocks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCaveSystemBlocksTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateCaveSystemTestWorld();
	ACaveSystem* Caverna = PlantaCavernaDeTeste(World, ACaveSystem::SmallCaveSide,
		ACaveSystem::SmallCaveSide, SementeDaCaverna);

	if (!TestNotNull(TEXT("A caverna nasceu"), Caverna))
	{
		DestroyCaveSystemTestWorld(World);
		return false;
	}

	UHierarchicalInstancedStaticMeshComponent* Pedras[3] = {
		Caverna->GetFloor(), Caverna->GetWalls(), Caverna->GetShell() };

	for (UHierarchicalInstancedStaticMeshComponent* Pedra : Pedras)
	{
		TestEqual(TEXT("A pedra colide"),
			Pedra->GetCollisionEnabled(), ECollisionEnabled::QueryAndPhysics);
		TestEqual(TEXT("A pedra barra o jogador"),
			Pedra->GetCollisionResponseToChannel(ECC_Pawn), ECR_Block);

		// Barrar por configuração não basta: sem geometria de colisão a parede
		// é atravessada mesmo declarada como bloqueio.
		UStaticMesh* Malha = Pedra->GetStaticMesh().Get();
		UBodySetup* Corpo = Malha ? Malha->GetBodySetup() : nullptr;
		TestNotNull(TEXT("A malha tem corpo de colisão"), Corpo);
		if (Corpo)
		{
			TestTrue(TEXT("O corpo tem forma"), Corpo->AggGeom.GetElementCount() > 0);
		}
	}

	DestroyCaveSystemTestWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCaveSystemMatchesThePlanTest,
	"BattleSquare.CaveSystem.MatchesThePlan",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCaveSystemMatchesThePlanTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateCaveSystemTestWorld();
	ACaveSystem* Caverna = PlantaCavernaDeTeste(World, ACaveSystem::LargeCaveSide,
		ACaveSystem::LargeCaveSide, SementeDaCaverna);

	if (!TestNotNull(TEXT("A caverna nasceu"), Caverna))
	{
		DestroyCaveSystemTestWorld(World);
		return false;
	}

	const CaveLabyrinth::FCaveGrid& Planta = Caverna->GetGrid();

	TestEqual(TEXT("Há um chão por casa"),
		Caverna->GetFloor()->GetInstanceCount(), Planta.CellCount());

	// A verga é a única laje que não corresponde a um segmento da planta, e há
	// UMA POR BOCA — a do sul mais as extras.
	//
	// Era "mais um" quando a caverna tinha uma saída só. A conta mudou junto
	// com a caverna, e a diferença de duas lajes foi o que revelou que as bocas
	// extras abriam a parede interna e não a muralha.
	const int32 Lajes = Caverna->GetWalls()->GetInstanceCount()
		+ Caverna->GetShell()->GetInstanceCount();
	TestEqual(TEXT("Há uma laje por parede que sobrou, mais uma verga por boca"),
		Lajes, LajesQueAPlantaExige(Planta) + 1 + Planta.ExtraMouths.Num());

	DestroyCaveSystemTestWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCaveSystemEntranceIsWalkableTest,
	"BattleSquare.CaveSystem.EntranceIsWalkable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCaveSystemEntranceIsWalkableTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateCaveSystemTestWorld();
	ACaveSystem* Caverna = PlantaCavernaDeTeste(World, ACaveSystem::LargeCaveSide,
		ACaveSystem::LargeCaveSide, SementeDaCaverna);

	if (!TestNotNull(TEXT("A caverna nasceu"), Caverna))
	{
		DestroyCaveSystemTestWorld(World);
		return false;
	}

	const CaveLabyrinth::FCaveGrid& Planta = Caverna->GetGrid();

	// O vão fica entre o lado de fora e o centro da primeira casa; a altura de
	// um metro é onde o corpo do jogador passa.
	const FVector Fora = Caverna->EntranceLocal();
	const FVector Dentro = Caverna->CellCenterLocal(Planta.EntranceColumn, 0);
	const FVector Vao = 0.5f * (Fora + Dentro) + FVector(0.0f, 0.0f, 100.0f);

	TestFalse(TEXT("Não há pedra na altura do corpo, no vão da boca"),
		PedraCobreOPonto(*Caverna, Vao));

	// E a verga existe: acima da altura das paredes de dentro, o mesmo vão é
	// pedra. Sem isso a boca é um entalhe na muralha, não uma passagem.
	const FVector Verga = 0.5f * (Fora + Dentro)
		+ FVector(0.0f, 0.0f, 0.5f * (Caverna->InnerWallHeightUnits()
			+ Caverna->OuterWallHeightUnits()));
	TestTrue(TEXT("Acima da boca há pedra"), PedraCobreOPonto(*Caverna, Verga));

	DestroyCaveSystemTestWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCaveSystemCorridorFitsThePlayerTest,
	"BattleSquare.CaveSystem.CorridorFitsThePlayer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCaveSystemCorridorFitsThePlayerTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateCaveSystemTestWorld();
	ACaveSystem* Caverna = PlantaCavernaDeTeste(World, ACaveSystem::MediumCaveSide,
		ACaveSystem::MediumCaveSide, SementeDaCaverna);

	if (!TestNotNull(TEXT("A caverna nasceu"), Caverna))
	{
		DestroyCaveSystemTestWorld(World);
		return false;
	}

	// Um labirinto por onde o jogador não passa é decoração. A cápsula padrão
	// tem 34 de raio; a folga precisa ser maior que isso com sobra, senão andar
	// vira raspar na parede.
	constexpr float LarguraMinimaDoCorredor = 120.0f;
	TestTrue(FString::Printf(TEXT("O corredor tem %.0f de folga"),
		Caverna->CorridorWidthUnits()),
		Caverna->CorridorWidthUnits() >= LarguraMinimaDoCorredor);

	DestroyCaveSystemTestWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCaveSystemLargeIsActuallyLargeTest,
	"BattleSquare.CaveSystem.LargeIsActuallyLarge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCaveSystemLargeIsActuallyLargeTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateCaveSystemTestWorld();
	ACaveSystem* Pequena = PlantaCavernaDeTeste(World, ACaveSystem::SmallCaveSide,
		ACaveSystem::SmallCaveSide, SementeDaCaverna);
	ACaveSystem* Grande = PlantaCavernaDeTeste(World, ACaveSystem::LargeCaveSide,
		ACaveSystem::LargeCaveSide, SementeDaCaverna);

	if (!TestNotNull(TEXT("A pequena nasceu"), Pequena) || !TestNotNull(TEXT("A grande nasceu"), Grande))
	{
		DestroyCaveSystemTestWorld(World);
		return false;
	}

	// "Algumas precisam ser grandes" é uma afirmação verificável, não um ajuste
	// de gosto: a grande ocupa mais chão E o caminho até o fundo é mais longo.
	TestTrue(TEXT("A grande ocupa mais chão"),
		Grande->FootprintUnits() > Pequena->FootprintUnits());

	int32 ColunaFundo = 0;
	int32 LinhaFundo = 0;
	const int32 PassosGrande = CaveLabyrinth::DeepestFrom(Grande->GetGrid(),
		Grande->GetGrid().EntranceColumn, 0, ColunaFundo, LinhaFundo);
	const int32 PassosPequena = CaveLabyrinth::DeepestFrom(Pequena->GetGrid(),
		Pequena->GetGrid().EntranceColumn, 0, ColunaFundo, LinhaFundo);

	TestTrue(FString::Printf(TEXT("O fundo da grande (%d passos) é mais longe que o da pequena (%d)"),
		PassosGrande, PassosPequena), PassosGrande > PassosPequena);

	DestroyCaveSystemTestWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCaveSystemIsDeterministicTest,
	"BattleSquare.CaveSystem.IsDeterministic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCaveSystemIsDeterministicTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateCaveSystemTestWorld();
	ACaveSystem* Uma = PlantaCavernaDeTeste(World, 9, 9, SementeDaCaverna);
	ACaveSystem* Outra = PlantaCavernaDeTeste(World, 9, 9, SementeDaCaverna);
	ACaveSystem* Diferente = PlantaCavernaDeTeste(World, 9, 9, SementeDaCaverna + 1);

	if (!TestNotNull(TEXT("A primeira nasceu"), Uma) || !TestNotNull(TEXT("A segunda nasceu"), Outra)
		|| !TestNotNull(TEXT("A terceira nasceu"), Diferente))
	{
		DestroyCaveSystemTestWorld(World);
		return false;
	}

	const TArray<FVector> Primeira = PosicoesDasLajesDaCaverna(Uma->GetWalls());
	const TArray<FVector> Segunda = PosicoesDasLajesDaCaverna(Outra->GetWalls());
	const TArray<FVector> Terceira = PosicoesDasLajesDaCaverna(Diferente->GetWalls());

	TestEqual(TEXT("A mesma semente dá o mesmo número de paredes"),
		Segunda.Num(), Primeira.Num());

	bool bTodasIguais = (Primeira.Num() == Segunda.Num());
	for (int32 Qual = 0; bTodasIguais && Qual < Primeira.Num(); ++Qual)
	{
		bTodasIguais = Primeira[Qual].Equals(Segunda[Qual], 0.01f);
	}
	TestTrue(TEXT("A mesma semente põe as paredes no mesmo lugar"), bTodasIguais);

	bool bAlgumaDiferente = (Primeira.Num() != Terceira.Num());
	for (int32 Qual = 0; !bAlgumaDiferente && Qual < Primeira.Num(); ++Qual)
	{
		bAlgumaDiferente = !Primeira[Qual].Equals(Terceira[Qual], 0.01f);
	}
	TestTrue(TEXT("Outra semente dá outro labirinto"), bAlgumaDiferente);

	DestroyCaveSystemTestWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCaveSystemInvalidGridBuildsNothingTest,
	"BattleSquare.CaveSystem.InvalidGridBuildsNothing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCaveSystemInvalidGridBuildsNothingTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateCaveSystemTestWorld();
	ACaveSystem* Caverna = PlantaCavernaDeTeste(World, 0, 9, SementeDaCaverna);

	if (!TestNotNull(TEXT("A caverna nasceu"), Caverna))
	{
		DestroyCaveSystemTestWorld(World);
		return false;
	}

	// Meia caverna é pior que caverna nenhuma: parede solta no mapa parece
	// ruína, e ninguém procura o defeito onde parece intenção.
	TestEqual(TEXT("Nenhum chão"), Caverna->GetFloor()->GetInstanceCount(), 0);
	TestEqual(TEXT("Nenhuma parede"), Caverna->GetWalls()->GetInstanceCount(), 0);
	TestEqual(TEXT("Nenhuma muralha"), Caverna->GetShell()->GetInstanceCount(), 0);

	DestroyCaveSystemTestWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCaveSystemPaintsEachFlavorItsOwnStoneTest,
	"BattleSquare.CaveSystem.PaintsEachFlavorItsOwnStone",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCaveSystemPaintsEachFlavorItsOwnStoneTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateCaveSystemTestWorld();

	// A pedra da caverna de lava é a pedra do vulcão, e não a pedra cinza das
	// outras: se as três saíssem da mesma cor, o sabor viraria um campo que só
	// o código enxerga.
	ACaveSystem* Seca = PlantaCavernaComSabor(World, ACaveSystem::SmallCaveSide,
		ACaveSystem::SmallCaveSide, SementeDaCaverna, ECaveFlavor::Dry);
	ACaveSystem* DeLava = PlantaCavernaComSabor(World, ACaveSystem::SmallCaveSide,
		ACaveSystem::SmallCaveSide, SementeDaCaverna, ECaveFlavor::Lava);

	if (!TestNotNull(TEXT("A seca nasceu"), Seca) || !TestNotNull(TEXT("A de lava nasceu"), DeLava))
	{
		DestroyCaveSystemTestWorld(World);
		return false;
	}

	UMaterialInstanceDynamic* PedraSeca =
		Cast<UMaterialInstanceDynamic>(Seca->GetWalls()->GetMaterial(0));
	UMaterialInstanceDynamic* PedraDeLava =
		Cast<UMaterialInstanceDynamic>(DeLava->GetWalls()->GetMaterial(0));

	if (TestNotNull(TEXT("A parede seca foi pintada"), PedraSeca)
		&& TestNotNull(TEXT("A parede de lava foi pintada"), PedraDeLava))
	{
		const FLinearColor CorSeca = PedraSeca->K2_GetVectorParameterValue(TEXT("Color"));
		const FLinearColor CorDeLava = PedraDeLava->K2_GetVectorParameterValue(TEXT("Color"));
		TestFalse(TEXT("A pedra da lava não é a pedra seca"), CorSeca.Equals(CorDeLava, 0.001f));
	}

	TestEqual(TEXT("O sabor fica guardado"),
		static_cast<int32>(DeLava->GetFlavor()), static_cast<int32>(ECaveFlavor::Lava));

	DestroyCaveSystemTestWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCaveSystemPoolsOnlyWhereThereIsSomethingToPoolTest,
	"BattleSquare.CaveSystem.PoolsOnlyWhereThereIsSomethingToPool",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCaveSystemPoolsOnlyWhereThereIsSomethingToPoolTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateCaveSystemTestWorld();

	ACaveSystem* Seca = PlantaCavernaComSabor(World, ACaveSystem::LargeCaveSide,
		ACaveSystem::LargeCaveSide, SementeDaCaverna, ECaveFlavor::Dry);
	ACaveSystem* DeAgua = PlantaCavernaComSabor(World, ACaveSystem::LargeCaveSide,
		ACaveSystem::LargeCaveSide, SementeDaCaverna, ECaveFlavor::Water);

	if (!TestNotNull(TEXT("A seca nasceu"), Seca) || !TestNotNull(TEXT("A de água nasceu"), DeAgua))
	{
		DestroyCaveSystemTestWorld(World);
		return false;
	}

	// Poça na caverna seca seria uma lâmina de nada: o chão pintado de azul sem
	// que exista água nenhuma naquele lugar do mapa.
	TestEqual(TEXT("A seca não tem poça"), QuantasInstanciasNaCaverna(Seca->GetPools()), 0);
	TestTrue(TEXT("A de água tem poça"), QuantasInstanciasNaCaverna(DeAgua->GetPools()) > 0);

	DestroyCaveSystemTestWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCaveSystemLightsOnlyTheLavaTest,
	"BattleSquare.CaveSystem.LightsOnlyTheLava",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCaveSystemLightsOnlyTheLavaTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateCaveSystemTestWorld();

	ACaveSystem* Seca = PlantaCavernaComSabor(World, ACaveSystem::MediumCaveSide,
		ACaveSystem::MediumCaveSide, SementeDaCaverna, ECaveFlavor::Dry);
	ACaveSystem* DeLava = PlantaCavernaComSabor(World, ACaveSystem::MediumCaveSide,
		ACaveSystem::MediumCaveSide, SementeDaCaverna, ECaveFlavor::Lava);

	if (!TestNotNull(TEXT("A seca nasceu"), Seca) || !TestNotNull(TEXT("A de lava nasceu"), DeLava))
	{
		DestroyCaveSystemTestWorld(World);
		return false;
	}

	// A brasa existe nas duas — o componente é sempre criado, para ninguém ter
	// de lembrar de checar ponteiro nulo — mas só ACENDE numa.
	if (TestNotNull(TEXT("A brasa da seca existe"), Seca->GetLavaGlow())
		&& TestNotNull(TEXT("A brasa da lava existe"), DeLava->GetLavaGlow()))
	{
		TestEqual(TEXT("A seca fica apagada"), Seca->GetLavaGlow()->Intensity, 0.0f);
		TestTrue(TEXT("A de lava acende"), DeLava->GetLavaGlow()->Intensity > 0.0f);

		// No chão ela acenderia a laje e deixaria a pedra em volta preta.
		TestTrue(TEXT("A brasa fica na altura da parede"),
			DeLava->GetLavaGlow()->GetRelativeLocation().Z > 0.0);
	}

	DestroyCaveSystemTestWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCaveSystemSealsTheMouthOfWhatCannotBeEnteredTest,
	"BattleSquare.CaveSystem.SealsTheMouthOfWhatCannotBeEntered",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCaveSystemSealsTheMouthOfWhatCannotBeEnteredTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateCaveSystemTestWorld();

	ACaveSystem* Aberta = PlantaCavernaComSabor(World, ACaveSystem::MediumCaveSide,
		ACaveSystem::MediumCaveSide, SementeDaCaverna, ECaveFlavor::Dry);
	ACaveSystem* Fechada = PlantaCavernaComSabor(World, ACaveSystem::MediumCaveSide,
		ACaveSystem::MediumCaveSide, SementeDaCaverna, ECaveFlavor::Lava);

	if (!TestNotNull(TEXT("A aberta nasceu"), Aberta) || !TestNotNull(TEXT("A fechada nasceu"), Fechada))
	{
		DestroyCaveSystemTestWorld(World);
		return false;
	}

	TestTrue(TEXT("A seca se explora"), Aberta->IsExplorable());
	TestFalse(TEXT("A de lava não se explora"), Fechada->IsExplorable());

	// A tampa é UMA laje a mais na muralha, e é ela que conta de longe que ali
	// não se entra. Deixar entrar e barrar na porta seria caminhar até lá para
	// descobrir que não valia.
	TestEqual(TEXT("A boca fechada é exatamente uma laje a mais"),
		QuantasInstanciasNaCaverna(Fechada->GetShell()),
		QuantasInstanciasNaCaverna(Aberta->GetShell()) + 1);

	DestroyCaveSystemTestWorld(World);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCaveSystemGrowsStoneSpikesInsideTheRockTest,
	"BattleSquare.CaveSystem.GrowsStoneSpikesInsideTheRock",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCaveSystemGrowsStoneSpikesInsideTheRockTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateCaveSystemTestWorld();

	ACaveSystem* Caverna = PlantaCavernaComSabor(World, ACaveSystem::LargeCaveSide,
		ACaveSystem::LargeCaveSide, SementeDaCaverna, ECaveFlavor::Dry);

	if (!TestNotNull(TEXT("A caverna nasceu"), Caverna))
	{
		DestroyCaveSystemTestWorld(World);
		return false;
	}

	const TArray<FVector> Pontas = PosicoesDasLajesDaCaverna(Caverna->GetSpikes());
	if (!TestTrue(TEXT("Há pontas de pedra"), Pontas.Num() > 0))
	{
		DestroyCaveSystemTestWorld(World);
		return false;
	}

	// Nenhuma pode ficar abaixo do chão nem acima da verga: pedra flutuando é o
	// mesmo erro de usar a primitiva como se fosse a coisa.
	bool bTodasDentro = true;
	for (const FVector& Ponta : Pontas)
	{
		bTodasDentro = bTodasDentro && Ponta.Z > -1.0 && Ponta.Z < 1000.0;
	}
	TestTrue(TEXT("Toda ponta está dentro da pedra"), bTodasDentro);

	// A mesma semente tem de dar a mesma caverna: sorteio que muda a cada
	// carregamento faz o mundo esquecer onde ficava o quê.
	ACaveSystem* Gemea = PlantaCavernaComSabor(World, ACaveSystem::LargeCaveSide,
		ACaveSystem::LargeCaveSide, SementeDaCaverna, ECaveFlavor::Dry);
	TestEqual(TEXT("A mesma semente dá as mesmas pontas"),
		QuantasInstanciasNaCaverna(Gemea->GetSpikes()), Pontas.Num());

	DestroyCaveSystemTestWorld(World);
	return true;
}

#endif

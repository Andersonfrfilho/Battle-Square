// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattleArena.h"
#include "Battle/BattleTypes.h"
#include "Environment/ForestBackdrop.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Misc/AutomationTest.h"
#include "UObject/SoftObjectPtr.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"

namespace ArenaCena
{
	UWorld* CriarMundoDaGeometria()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
		FWorldContext& Contexto = GEngine->CreateNewWorldContext(EWorldType::Game);
		Contexto.SetCurrentWorld(World);
		World->InitializeActorsForPlay(FURL());
		return World;
	}

	void DestruirMundoDaGeometria(UWorld* World)
	{
		if (!World)
		{
			return;
		}
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
	}

	/** Uma laje larga e sólida com material conhecido, para a sonda encontrar. */
	AActor* CriarChao(UWorld* World, const FVector& Onde, UMaterialInterface* Material)
	{
		AActor* Chao = World->SpawnActor<AActor>(AActor::StaticClass(), Onde, FRotator::ZeroRotator);
		UStaticMeshComponent* Malha = NewObject<UStaticMeshComponent>(Chao);
		Malha->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
		Malha->SetMaterial(0, Material);
		Malha->SetWorldScale3D(FVector(40.0f, 40.0f, 1.0f));
		Malha->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Malha->SetCollisionResponseToAllChannels(ECR_Block);
		Chao->SetRootComponent(Malha);
		Malha->RegisterComponent();
		return Chao;
	}
}

// O tabuleiro precisa EXISTIR na tela, não só na lógica.
//
// Três vezes neste projeto um ator nasceu com componente visual e sem asset:
// os pets, os inimigos do mundo e o próprio jogador. Nos três casos toda a
// lógica passava e não havia nada para ver. A arena é o quarto candidato — e
// é o maior, porque é o fundo de tudo.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArenaMeshesHaveAssignedAssetsTest,
	"BattleSquare.Battle.Arena.MeshesHaveAssignedAssets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArenaMeshesHaveAssignedAssetsTest::RunTest(const FString& Parameters)
{
	const ABattleArena* Padrao = GetDefault<ABattleArena>();

	const TArray<TObjectPtr<UStaticMeshComponent>>& Lajes = Padrao->GetCellTileMeshes();
	TestEqual(TEXT("Uma laje por casa da grade configurada"), Lajes.Num(),
		Padrao->GridColumns * Padrao->GridRows);
	for (const UStaticMeshComponent* Laje : Lajes)
	{
		TestTrue(TEXT("Laje existe e tem malha ATRIBUÍDA"),
			Laje != nullptr && Laje->GetStaticMesh() != nullptr);
	}

	return true;
}

// Malha sem material é malha cinza: metade do defeito, e a metade que faz
// água, dano e bônus ficarem indistinguíveis.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArenaMaterialsArePointedAtTest,
	"BattleSquare.Battle.Arena.MaterialsArePointedAt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArenaMaterialsArePointedAtTest::RunTest(const FString& Parameters)
{
	const ABattleArena* Padrao = GetDefault<ABattleArena>();

	const TPair<const TCHAR*, const TSoftObjectPtr<UMaterialInterface>*> Materiais[] = {
		{ TEXT("neutro"), &Padrao->NeutralTileMaterial },
		{ TEXT("água"), &Padrao->WaterTileMaterial },
		{ TEXT("dano"), &Padrao->DamageTileMaterial },
		{ TEXT("bônus"), &Padrao->BuffTileMaterial },
		{ TEXT("bloqueada"), &Padrao->BlockedTileMaterial },
	};

	for (const TPair<const TCHAR*, const TSoftObjectPtr<UMaterialInterface>*>& Par : Materiais)
	{
		TestTrue(FString::Printf(TEXT("Material de %s aponta para um asset"), Par.Key),
			!Par.Value->IsNull());
	}

	return true;
}

// A altura vem de UMA função, e é ela que a laje e a grade desenhada usam.
// Se voltarem a ser dois números, a água afunda a laje e deixa o contorno
// boiando — foi o modo de falhar de L-032 e L-033.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArenaSurfaceHeightVariesByTerrainTest,
	"BattleSquare.Battle.Arena.SurfaceHeightVariesByTerrain",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArenaSurfaceHeightVariesByTerrainTest::RunTest(const FString& Parameters)
{
	const float Neutra = ABattleArena::GetCellSurfaceHeight(static_cast<uint8>(ECellProperty::None));
	const float Agua = ABattleArena::GetCellSurfaceHeight(static_cast<uint8>(ECellProperty::Water));
	const float Bloqueada = ABattleArena::GetCellSurfaceHeight(static_cast<uint8>(ECellProperty::Blocked));

	TestTrue(TEXT("Água AFUNDA — é onde se submerge"), Agua < Neutra);
	TestTrue(TEXT("Bloqueada SOBE — ninguém pisa nela"), Bloqueada > Neutra);

	// Dano e bônus mudam de cor, não de altura: quem anda por cima delas anda
	// no mesmo plano do resto, e um degrau ali seria mentira sobre a regra.
	TestEqual(TEXT("Dano no plano de andar"),
		ABattleArena::GetCellSurfaceHeight(static_cast<uint8>(ECellProperty::Damage)), Neutra);
	TestEqual(TEXT("Bônus no plano de andar"),
		ABattleArena::GetCellSurfaceHeight(static_cast<uint8>(ECellProperty::Buff)), Neutra);

	return true;
}

// O pet fica NA SUPERFÍCIE da casa, seja ela qual for.
//
// Enquanto GetCellWorldLocation devolvia Z=0, a laje tinha relevo e o pet não:
// sobre a água ele boiava no ar, sobre a pedra ficava enterrado nela. A altura
// já tinha uma dona (GetCellSurfaceHeight) — o defeito foi esta função não
// perguntar a ela, que é o mesmo modo de falhar de L-032 e L-033.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArenaCellLocationFollowsTerrainTest,
	"BattleSquare.Battle.Arena.CellLocationFollowsTerrain",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArenaCellLocationFollowsTerrainTest::RunTest(const FString& Parameters)
{
	UWorld* World = ArenaCena::CriarMundoDaGeometria();
	ABattleArena* Arena = World->SpawnActor<ABattleArena>();

	FBattleState& Estado = Arena->GetMutableCurrentState();
	Estado.CellLayout[Estado.CellIndex(0, 0)] = static_cast<uint8>(ECellProperty::Water);
	Estado.CellLayout[Estado.CellIndex(2, 2)] = static_cast<uint8>(ECellProperty::Blocked);

	// Sem o redesenho o obstáculo ainda não tem tamanho nem assentamento, e
	// a casa bloqueada não teria topo para o pé encostar.
	Arena->RefreshTileVisuals();

	const float Base = Arena->GetActorLocation().Z;
	const float NaAgua = Arena->GetCellWorldLocation(0, 0).Z;
	const float NoPlano = Arena->GetCellWorldLocation(1, 1).Z;
	const float NaPedra = Arena->GetCellWorldLocation(2, 2).Z;

	TestTrue(TEXT("Na água o pé desce"), NaAgua < NoPlano);
	TestTrue(TEXT("Na pedra o pé sobe"), NaPedra > NoPlano);

	// Mesma fonte que dimensiona a laje: dois números divergem na primeira edição.
	TestEqual(TEXT("A altura é exatamente a superfície da casa"),
		NaAgua - Base,
		ABattleArena::GetCellSurfaceHeight(static_cast<uint8>(ECellProperty::Water)));

	// Na casa BLOQUEADA o pé não encosta na laje: encosta no TOPO do tronco,
	// porque quem está ali escalou. Antes disto o pet subia e renderizava por
	// dentro da pedra — de novo o defeito de afundar no tabuleiro.
	//
	// A medida sai da caixa do PRÓPRIO componente, e não de `altura * casa`:
	// a escala do obstáculo ainda é aparada para ele não invadir a casa
	// vizinha, então a conta refeita acharia um topo que a pedra larga não
	// tem.
	const TArray<TObjectPtr<UStaticMeshComponent>>& Obstaculos = Arena->GetCellObstacleMeshes();
	const int32 IndiceDaPedra = Estado.CellIndex(2, 2);
	if (!TestTrue(TEXT("a casa bloqueada tem componente de obstaculo"),
		Obstaculos.IsValidIndex(IndiceDaPedra) && Obstaculos[IndiceDaPedra] != nullptr))
	{
		ArenaCena::DestruirMundoDaGeometria(World);
		return false;
	}

	const FBoxSphereBounds CaixaDaPedra =
		Obstaculos[IndiceDaPedra]->CalcBounds(Obstaculos[IndiceDaPedra]->GetRelativeTransform());
	const float TopoDaPedra = static_cast<float>(CaixaDaPedra.Origin.Z + CaixaDaPedra.BoxExtent.Z);

	TestEqual(TEXT("Quem escalou fica em cima do obstaculo, nao dentro dele"),
		Arena->GetCellSurfaceHeightAt(2, 2), TopoDaPedra, 1.0f);
	TestTrue(TEXT("E isso e' bem acima da laje bloqueada"),
		Arena->GetCellSurfaceHeightAt(2, 2)
			> ABattleArena::GetCellSurfaceHeight(static_cast<uint8>(ECellProperty::Blocked))
				+ Arena->CellSize * 0.5f);

	ArenaCena::DestruirMundoDaGeometria(World);
	return true;
}

// A arena veste o lugar de onde o jogador veio.
//
// Ela nasce a um milhão de unidades do mundo (DP-enc-03): luz, céu e névoa são
// globais e já a alcançam, mas o CHÃO não — e era o chão que denunciava que a
// batalha se passava noutro lugar.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArenaAdoptsAmbienceFromWorldTest,
	"BattleSquare.Battle.Arena.AdoptsAmbienceFromWorld",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArenaAdoptsAmbienceFromWorldTest::RunTest(const FString& Parameters)
{
	UWorld* World = ArenaCena::CriarMundoDaGeometria();

	UMaterialInterface* MaterialDoLugar = LoadObject<UMaterialInterface>(
		nullptr, TEXT("/Engine/EngineMaterials/WorldGridMaterial.WorldGridMaterial"));
	if (!TestNotNull(TEXT("Material de referência carregou"), MaterialDoLugar))
	{
		ArenaCena::DestruirMundoDaGeometria(World);
		return false;
	}

	AActor* Terreno = ArenaCena::CriarChao(World, FVector::ZeroVector, MaterialDoLugar);
	TestNotNull(TEXT("Chão de teste criado"), Terreno);

	ABattleArena* Arena = World->SpawnActor<ABattleArena>(ABattleArena::StaticClass(),
		FVector(0.0f, 0.0f, 100000.0f), FRotator::ZeroRotator);

	TestTrue(TEXT("Adotou o ambiente do lugar do encontro"),
		Arena->AdoptAmbienceFromWorldLocation(FVector::ZeroVector));
	TestTrue(TEXT("E o chão da arena é o material DE LÁ"),
		Arena->GetAdoptedFloorMaterial() == MaterialDoLugar);

	ArenaCena::DestruirMundoDaGeometria(World);
	return true;
}

// Sem chão sondável a paleta autorada continua valendo: degrada, não quebra.
// A tela de batalha avulsa (BattleScreen) não nasce de encontro nenhum, e uma
// arena que recusasse existir sem mundo por baixo mataria justamente o caminho
// mais usado em desenvolvimento.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArenaKeepsOwnPaletteWithoutGroundTest,
	"BattleSquare.Battle.Arena.KeepsOwnPaletteWithoutGround",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArenaKeepsOwnPaletteWithoutGroundTest::RunTest(const FString& Parameters)
{
	UWorld* World = ArenaCena::CriarMundoDaGeometria();
	ABattleArena* Arena = World->SpawnActor<ABattleArena>();

	TestFalse(TEXT("Sem chão embaixo, não há o que adotar"),
		Arena->AdoptAmbienceFromWorldLocation(FVector(0.0f, 0.0f, 500000.0f)));
	TestNull(TEXT("E nada foi emprestado"), Arena->GetAdoptedFloorMaterial());

	ArenaCena::DestruirMundoDaGeometria(World);
	return true;
}

// O tabuleiro tem que ficar ACIMA do chão do nível.
//
// Medido em 2026-08-29: `Floor_0` de BattleScreen é um plano em Z=-0.5, e a
// laje de andar tem topo em -4. Sem elevação o tabuleiro nasce enterrado —
// o mesmo defeito de "existe na lógica, não existe na tela", só que por
// profundidade em vez de asset faltando.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArenaSitsAboveLevelGroundTest,
	"BattleSquare.Battle.Arena.SitsAboveLevelGround",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArenaSitsAboveLevelGroundTest::RunTest(const FString& Parameters)
{
	const ABattleArena* Padrao = GetDefault<ABattleArena>();

	// A água é o ponto mais fundo em que ainda se anda: se ela passa, todo o
	// resto do tabuleiro passa junto.
	const float TopoDaAgua = Padrao->BoardElevation
		+ ABattleArena::GetCellSurfaceHeight(static_cast<uint8>(ECellProperty::Water));

	TestTrue(TEXT("Até a casa mais funda fica acima do chão do nível"), TopoDaAgua > 0.0f);

	return true;
}

// A arena tem que ser PARTE DO CENÁRIO, não uma placa posta em cima dele.
//
// Duas vezes o usuário viu a mesma coisa na tela: primeiro "uns quadrados na
// arena marcados com bordas", depois "a arena está um redondo". As duas são o
// mesmo defeito — o tabuleiro trazia chão próprio. Achatar a casa neutra até
// o nível da terra escondeu a borda; o prato de terra escondeu o degrau. Nada
// disso tirava a placa de cima do cenário.
//
// Agora a casa sem regra NÃO É DESENHADA: o que se vê ali é o chão da mata.
// Só o que carrega regra ganha laje, e aí o relevo tem serventia, porque
// água, dano, bônus e bloqueio precisam ser achados de relance.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArenaNeutralCellIsTheGroundTest,
	"BattleSquare.Battle.Arena.NeutralCellIsTheGroundItself",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArenaNeutralCellIsTheGroundTest::RunTest(const FString& Parameters)
{
	UWorld* World = ArenaCena::CriarMundoDaGeometria();
	ABattleArena* Arena = World->SpawnActor<ABattleArena>();

	FBattleState& Estado = Arena->GetMutableCurrentState();
	Estado.CellLayout[Estado.CellIndex(0, 0)] = static_cast<uint8>(ECellProperty::Water);

	Arena->DispatchBeginPlay();

	const TArray<TObjectPtr<UStaticMeshComponent>>& Lajes = Arena->GetCellTileMeshes();
	const int32 Colunas = Arena->GetActiveGridColumns();

	const UStaticMeshComponent* ComRegra = Lajes[CellLayoutIndex(0, 0, Colunas)];
	const UStaticMeshComponent* SemRegra = Lajes[CellLayoutIndex(1, 1, Colunas)];

	if (TestNotNull(TEXT("A casa de água existe"), ComRegra)
		&& TestNotNull(TEXT("A casa neutra existe"), SemRegra))
	{
		TestFalse(TEXT("A casa SEM REGRA não é desenhada — ela é o chão da mata"),
			SemRegra->IsVisible());
		TestTrue(TEXT("E a casa COM REGRA é desenhada, senão a água some"),
			ComRegra->IsVisible());
	}

	ArenaCena::DestruirMundoDaGeometria(World);
	return true;
}

// A batalha acontece SOBRE O CHÃO DA MATA, não sobre um prato de terra.
//
// "A arena está um redondo, não precisa disso, deixe como se fosse o cenário
// real, estamos lutando em parte dele" — 2026-08-29, olhando a tela. A arena
// desenhava um disco de terra próprio porque a mata ficava BoardElevation
// abaixo dela, e alguma coisa tinha que tapar o degrau. Encostar o topo do
// chão da mata na superfície da casa neutra desfaz o degrau e a necessidade.
//
// O teste mede o ENCOSTE. Enquanto ele valer, não há degrau para tapar, e o
// prato não pode voltar por falta de chão.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArenaBoardSitsOnTheForestFloorTest,
	"BattleSquare.Battle.Arena.BoardSitsOnTheForestFloor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArenaBoardSitsOnTheForestFloorTest::RunTest(const FString& Parameters)
{
	UWorld* World = ArenaCena::CriarMundoDaGeometria();
	ABattleArena* Arena = World->SpawnActor<ABattleArena>();
	Arena->DispatchBeginPlay();

	AForestBackdrop* Mata = Arena->GetForestBackdrop();
	if (!TestNotNull(TEXT("A mata existe"), Mata))
	{
		ArenaCena::DestruirMundoDaGeometria(World);
		return false;
	}

	// Onde o pet pisa numa casa sem regra — a mesma altura que a arena usa
	// para posicionar o pet, e por isso a única referência honesta aqui.
	const float SuperficieDaCasa =
		Arena->GetCellWorldLocation(/*Column=*/1, /*Row=*/1).Z;
	const float TopoDoChaoDaMata =
		Mata->GetActorLocation().Z + AForestBackdrop::GroundTopLocalZ();

	TestEqual(TEXT("O topo do chão da mata É a superfície da casa"),
		TopoDoChaoDaMata, SuperficieDaCasa, 0.01f);

	ArenaCena::DestruirMundoDaGeometria(World);
	return true;
}

// A casa BLOQUEADA não pode ser só um piso um pouco mais alto.
//
// Era o que ela era: quem olhava via chão, e "não dá para andar aqui" ficava
// por conta de quem já sabia da regra. Um tronco ou uma pedra ocupando a casa
// diz isso sozinho — e é sobre ele que destruir e escalar vão se apoiar.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArenaBlockedCellCarriesAnObstacleTest,
	"BattleSquare.Battle.Arena.BlockedCellCarriesAnObstacle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArenaBlockedCellCarriesAnObstacleTest::RunTest(const FString& Parameters)
{
	UWorld* World = ArenaCena::CriarMundoDaGeometria();
	ABattleArena* Arena = World->SpawnActor<ABattleArena>();

	FBattleState& Estado = Arena->GetMutableCurrentState();
	for (uint8& Casa : Estado.CellLayout)
	{
		Casa = static_cast<uint8>(ECellProperty::None);
	}
	Estado.CellLayout[Estado.CellIndex(2, 2)] = static_cast<uint8>(ECellProperty::Blocked);
	Estado.CellLayout[Estado.CellIndex(0, 0)] = static_cast<uint8>(ECellProperty::Water);

	Arena->RefreshTileVisuals();

	const TArray<TObjectPtr<UStaticMeshComponent>>& Obstaculos = Arena->GetCellObstacleMeshes();
	TestEqual(TEXT("ha um obstaculo por casa, criado no construtor"),
		Obstaculos.Num(), Arena->GetCellTileMeshes().Num());

	const int32 NaPedra = Estado.CellIndex(2, 2);
	const int32 NaAgua = Estado.CellIndex(0, 0);
	const int32 NoPlano = Estado.CellIndex(1, 1);

	if (!TestTrue(TEXT("os indices das casas existem"),
		Obstaculos.IsValidIndex(NaPedra) && Obstaculos.IsValidIndex(NaAgua) && Obstaculos.IsValidIndex(NoPlano)))
	{
		ArenaCena::DestruirMundoDaGeometria(World);
		return false;
	}

	TestTrue(TEXT("a casa bloqueada MOSTRA o obstaculo"), Obstaculos[NaPedra]->IsVisible());
	TestFalse(TEXT("a agua nao ganha obstaculo"), Obstaculos[NaAgua]->IsVisible());
	TestFalse(TEXT("a casa sem regra nao ganha obstaculo"), Obstaculos[NoPlano]->IsVisible());

	// O mesmo defeito de sempre: componente visível sem asset atribuído passa
	// em toda lógica e não existe na tela.
	UStaticMesh* Malha = Obstaculos[NaPedra]->GetStaticMesh();
	if (!TestNotNull(TEXT("o obstaculo tem malha atribuida"), Malha))
	{
		ArenaCena::DestruirMundoDaGeometria(World);
		return false;
	}

	// E ele precisa TER volume. A medida sai da CAIXA DO COMPONENTE, não de
	// uma conta refeita aqui: refazer a conta é escrever a regra duas vezes,
	// e as duas cópias concordam até a primeira edição.
	const FBoxSphereBounds Caixa =
		Obstaculos[NaPedra]->CalcBounds(Obstaculos[NaPedra]->GetRelativeTransform());

	TestTrue(TEXT("ele sobe acima da laje, nao e' um piso"),
		Caixa.BoxExtent.Z * 2.0f > Arena->CellSize * 0.5f);
	TestTrue(TEXT("e nao invade a casa vizinha"),
		FMath::Max(Caixa.BoxExtent.X, Caixa.BoxExtent.Y) * 2.0f <= Arena->CellSize);

	// Ele ASSENTA no topo da casa bloqueada: nem flutuando sobre a laje, nem
	// com meio corpo dentro dela — o defeito que os pets já tiveram.
	TestEqual(TEXT("o pe do obstaculo encosta na superficie da casa bloqueada"),
		static_cast<float>(Caixa.Origin.Z - Caixa.BoxExtent.Z),
		ABattleArena::GetCellSurfaceHeight(static_cast<uint8>(ECellProperty::Blocked)),
		1.0f);

	ArenaCena::DestruirMundoDaGeometria(World);
	return true;
}

// O que ocupa a casa sai das COORDENADAS dela: o mesmo campo dá sempre a
// mesma cena. Sorteio com relógio faria a arena mudar de cara a cada abertura
// e tornaria impossível reproduzir o que o usuário viu na tela.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArenaObstacleIsDeterministicPerCellTest,
	"BattleSquare.Battle.Arena.ObstacleIsDeterministicPerCell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArenaObstacleIsDeterministicPerCellTest::RunTest(const FString& Parameters)
{
	UWorld* World = ArenaCena::CriarMundoDaGeometria();

	const auto CenaDaCasaBloqueada = [](ABattleArena* Arena)
	{
		FBattleState& Estado = Arena->GetMutableCurrentState();
		Estado.CellLayout[Estado.CellIndex(2, 2)] = static_cast<uint8>(ECellProperty::Blocked);
		Arena->RefreshTileVisuals();

		UStaticMeshComponent* Obstaculo = Arena->GetCellObstacleMeshes()[Estado.CellIndex(2, 2)];
		return TPair<UStaticMesh*, FRotator>(Obstaculo->GetStaticMesh(), Obstaculo->GetRelativeRotation());
	};

	ABattleArena* Primeira = World->SpawnActor<ABattleArena>();
	ABattleArena* Segunda = World->SpawnActor<ABattleArena>();

	const TPair<UStaticMesh*, FRotator> Uma = CenaDaCasaBloqueada(Primeira);
	const TPair<UStaticMesh*, FRotator> Outra = CenaDaCasaBloqueada(Segunda);

	TestEqual(TEXT("a mesma casa recebe a mesma materia"), Uma.Key, Outra.Key);
	TestTrue(TEXT("e o mesmo giro"), Uma.Value.Equals(Outra.Value, 0.01f));

	ArenaCena::DestruirMundoDaGeometria(World);
	return true;
}

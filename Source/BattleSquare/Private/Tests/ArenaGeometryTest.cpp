// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattleArena.h"
#include "Battle/BattleTypes.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Misc/AutomationTest.h"
#include "UObject/SoftObjectPtr.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"

namespace ArenaCena
{
	UWorld* CriarMundo()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
		FWorldContext& Contexto = GEngine->CreateNewWorldContext(EWorldType::Game);
		Contexto.SetCurrentWorld(World);
		World->InitializeActorsForPlay(FURL());
		return World;
	}

	void DestruirMundo(UWorld* World)
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

	TestTrue(TEXT("Tem chão"), Padrao->GetArenaFloorMesh() != nullptr);
	TestTrue(TEXT("E a malha do chão está ATRIBUÍDA"),
		Padrao->GetArenaFloorMesh() && Padrao->GetArenaFloorMesh()->GetStaticMesh() != nullptr);

	const TArray<TObjectPtr<UStaticMeshComponent>>& Moldura = Padrao->GetArenaFrameMeshes();
	TestEqual(TEXT("Moldura fechada nos quatro lados"), Moldura.Num(), 4);
	for (const UStaticMeshComponent* Viga : Moldura)
	{
		TestTrue(TEXT("Viga existe e tem malha ATRIBUÍDA"),
			Viga != nullptr && Viga->GetStaticMesh() != nullptr);
	}

	const TArray<TObjectPtr<UStaticMeshComponent>>& Lajes = Padrao->GetCellTileMeshes();
	TestEqual(TEXT("Uma laje por casa da grade 3x3"), Lajes.Num(), 9);
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
		{ TEXT("chão"), &Padrao->FloorMaterial },
		{ TEXT("moldura"), &Padrao->FrameMaterial },
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
	UWorld* World = ArenaCena::CriarMundo();
	ABattleArena* Arena = World->SpawnActor<ABattleArena>();

	FBattleState& Estado = Arena->GetMutableCurrentState();
	Estado.CellLayout[CellLayoutIndex(0, 0)] = static_cast<uint8>(ECellProperty::Water);
	Estado.CellLayout[CellLayoutIndex(2, 2)] = static_cast<uint8>(ECellProperty::Blocked);

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
	TestEqual(TEXT("E a consulta por casa concorda com ela"),
		Arena->GetCellSurfaceHeightAt(2, 2),
		ABattleArena::GetCellSurfaceHeight(static_cast<uint8>(ECellProperty::Blocked)));

	ArenaCena::DestruirMundo(World);
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
	UWorld* World = ArenaCena::CriarMundo();

	UMaterialInterface* MaterialDoLugar = LoadObject<UMaterialInterface>(
		nullptr, TEXT("/Engine/EngineMaterials/WorldGridMaterial.WorldGridMaterial"));
	if (!TestNotNull(TEXT("Material de referência carregou"), MaterialDoLugar))
	{
		ArenaCena::DestruirMundo(World);
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

	ArenaCena::DestruirMundo(World);
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
	UWorld* World = ArenaCena::CriarMundo();
	ABattleArena* Arena = World->SpawnActor<ABattleArena>();

	TestFalse(TEXT("Sem chão embaixo, não há o que adotar"),
		Arena->AdoptAmbienceFromWorldLocation(FVector(0.0f, 0.0f, 500000.0f)));
	TestNull(TEXT("E nada foi emprestado"), Arena->GetAdoptedFloorMaterial());

	ArenaCena::DestruirMundo(World);
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

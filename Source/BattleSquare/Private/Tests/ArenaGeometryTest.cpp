// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattleArena.h"
#include "Battle/BattleTypes.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Misc/AutomationTest.h"
#include "UObject/SoftObjectPtr.h"

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

	// O pet fica em Z=0 (GetCellWorldLocation): laje acima disso o enterraria.
	TestTrue(TEXT("Laje de andar fica ABAIXO do pé do pet"), Neutra < 0.0f);

	return true;
}

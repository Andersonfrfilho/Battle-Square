// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "World/VillageLayout.h"

/**
 * A MALHA DE CADA PRÉDIO DA VILA (adocao-de-arte, AR8).
 *
 * O prédio deixa de ser cubo colorido e passa a vestir um modelo de verdade —
 * pela mesma via de DADO que `ScenaryPalette::MeshPathForRole` abriu: override
 * em `[/Script/BattleSquare.Art]`, e primitiva de fallback quando não há chave.
 *
 * POR QUE AQUI E NÃO EM `ScenaryPalette` (invariante 18): a paleta é o lugar
 * único do visual DE CENÁRIO, indexado por `EScenaryRole`. Prédio não é papel de
 * cenário — ele já tem sua própria fonte de cor (`AVillage::BuildingColorInBiome`
 * + `BiomeTint`), que nunca passou pela paleta. Pôr o prédio lá exigiria a
 * paleta conhecer `EVillageBuilding`, e `Environment/` passaria a depender de
 * `World/` — dependência circular, já que `Village.cpp` inclui a paleta. A
 * fonte continua ÚNICA: uma chave por prédio, na MESMA seção de config, lida por
 * um lugar só.
 */
namespace VillageBuildingArt
{
	/** A chave estável de config de um prédio (`Mesh_Village_<Predio>`). */
	BATTLESQUARE_API const TCHAR* BuildingConfigKey(EVillageBuilding Building);

	/**
	 * O caminho da malha deste prédio, ou vazio quando não há override.
	 *
	 * Vazio é resposta VÁLIDA e é o caminho normal sem pacote adotado: quem
	 * chama cai no cubo de sempre (invariante 20 — o jogo fecha verde SEM o
	 * pacote). Nunca devolve caminho inventado.
	 */
	BATTLESQUARE_API FString MeshPathFor(EVillageBuilding Building);

	/**
	 * A escala que faz uma malha de caixa medida `MeshBounds` caber na pegada
	 * que o traçado reservou (`FootprintUnits` × `HeightUnits`).
	 *
	 * Existe porque a conta antiga dividia por 100 — o lado do cubo da engine.
	 * Malha autorada tem caixa PRÓPRIA, e dividir por 100 a deformaria; é o
	 * mesmo motivo pelo qual `ForestBackdrop` mede a caixa da árvore em vez de
	 * fixar altura por espécie.
	 *
	 * Eixo com medida não-positiva (malha degenerada) cai em escala 1 naquele
	 * eixo, nunca em zero: escala zero é malha atribuída e INVISÍVEL, o defeito
	 * que o teste da vila já cobra.
	 */
	BATTLESQUARE_API FVector ScaleToFootprint(
		const FVector& MeshBounds, const FVector2D& FootprintUnits, float HeightUnits);
}

// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * A MALHA DO PET, POR CATÁLOGO (adocao-de-arte, AR6).
 *
 * O `APetView` já sabia vestir malha esqueletal (`CharacterMesh`, e a silhueta
 * procedural de reserva quando não acha). O que faltava era ONDE procurar: ele
 * montava `/Game/Pets/Characters/SK_<Tipo>` a partir do TIPO do pet (`Fogo`,
 * `Agua`) — e os modelos adotados moram em `/Game/Quaternius/Monsters/SK_Dragon`,
 * indexados por CATÁLOGO, não por tipo. Ele nunca acharia.
 *
 * Esta é a ponte, e ela segue o mesmo padrão de dado das AR5 e AR8: override em
 * `[/Script/BattleSquare.Art]`, com a chave `Mesh_Pet_<nome>`.
 *
 * POR QUE PELO NOME E NÃO PELO `catalogId`: o id do catálogo é um **UUID gerado
 * pelo banco** (`uuid().defaultRandom()`) — muda a cada seed e difere entre
 * máquinas. Uma config indexada por ele valeria só numa base, num dia. O NOME é
 * único nos 115 pets do seed (medido), estável, e é o que se lê na config sem
 * precisar consultar o banco para saber de quem se está falando.
 *
 * POR QUE NÃO UM BLUEPRINT POR PET (o que a caixa AR6 dizia originalmente):
 * seriam 100+ assets para dizer o que uma linha de config diz, contra o padrão
 * que AR5/AR8 já firmaram (a malha vem de DADO), e dependeria de
 * `BlueprintTools.create` — a ferramenta que trava o editor quando a fábrica
 * pede parâmetro. Menos asset, mesma capacidade, e nenhuma janela modal.
 */
namespace PetModelArt
{
	/** A chave de config de um pet do catálogo (`Mesh_Pet_<catalogId>`). */
	BATTLESQUARE_API FString ConfigKeyFor(const FString& PetName);

	/**
	 * O caminho da malha esqueletal deste pet, ou vazio quando não há override.
	 *
	 * Vazio é resposta VÁLIDA e é o caminho normal de quem ainda não tem modelo
	 * atribuído: o `APetView` cai na silhueta procedural, que é VISÍVEL — a
	 * ausência aparece na tela, não vira pet invisível.
	 *
	 * Nome com caractere fora do permitido devolve vazio: ele vem do espelho
	 * assinado, e chave montada com texto de fora é travessia esperando
	 * acontecer — a mesma peneira que `CharacterMeshPathForType` já aplicava.
	 * Acento passa (os pets se chamam `Faísca`, `Vigília`): o que não passa é
	 * separador de caminho e ponto.
	 */
	BATTLESQUARE_API FString MeshPathFor(const FString& PetName);
}

// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Meta/PetCollectionSaveGame.h"

/**
 * O VEREDITO de uma venda — porque a recusa diz o motivo.
 *
 * "Nada aconteceu" é indistinguível de defeito, e aqui há dois motivos
 * diferentes para nada acontecer.
 */
enum class EPetSaleVerdict : uint8
{
	Sold,

	/** Não existe pet com este id na coleção. */
	NotOwned,

	/**
	 * É o pet ATIVO — o que anda com o jogador e entra em batalha.
	 *
	 * Vender o companheiro deixaria o jogador sem pet nenhum no mundo, e é a
	 * metade reversível: permitir depois é acrescentar; ter permitido e
	 * proibir depois seria tirar dinheiro que alguém já contou.
	 */
	ActivePet,

	/**
	 * É ROUBADO (crime-e-recompensa, CR6): pet marcado como roubado não se
	 * vende nem se troca — a marca segue o pet inclusive para quem comprou de
	 * boa-fé, e o Mercado é justamente onde o roubo tentaria virar dinheiro
	 * limpo. Vender roubado no Mercado comum apagaria o crime; só o
	 * mercado-negro (K3) o aceita, e é feature própria.
	 */
	Stolen
};

/**
 * A venda de um pet da coleção — PURA, e só a remoção.
 *
 * Quem paga é a carteira, e quanto é a tabela de `SettlementEconomy`
 * (invariante 15). Aqui só se garante a outra metade do contrapeso da task:
 * vender REMOVE — vender sem remover duplicaria pet e dinheiro ao mesmo tempo,
 * o mesmo formato de defeito que I3 evitou para item equipado.
 */
class BATTLESQUARE_API FPetSaleRules
{
public:
	/**
	 * Remove UMA instância com este `CatalogId`, se houver e não for a ativa.
	 *
	 * Pelo id de CATÁLOGO, nunca por índice ou tipo: dois pets do mesmo tipo
	 * com ids diferentes são capturas independentes, e vender um não pode
	 * levar o outro.
	 */
	static EPetSaleVerdict TrySell(TArray<FOwnedPetInstance>& Collection,
		const FString& CatalogId, const FString& ActiveCatalogId,
		const TSet<FString>& StolenCatalogIds);
};

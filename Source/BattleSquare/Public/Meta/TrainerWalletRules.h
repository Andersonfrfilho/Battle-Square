// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Meta/PetCollectionSaveGame.h"

/**
 * A CARTEIRA do treinador — o que entra, o que sai, e o chão.
 *
 * Pura sobre o perfil, como `FTrainerSpecialtyRules`: não lê save, não conhece
 * ator, não conhece tela. Quem cobra e quem paga é a tabela de
 * `SettlementEconomy` (invariante 15) — aqui só se garante que o dinheiro não
 * nasce nem some por engano.
 *
 * Ela existe porque a economia inteira já estava calculada e testada SEM
 * carteira nenhuma: `SettlementEconomy` era tabela com teste, não preço na
 * tela. O próprio header dela escreveu a acusação — "prédio com porta que não
 * abre é promessa quebrada".
 */
class BATTLESQUARE_API FTrainerWalletRules
{
public:
	/**
	 * A bolsa inicial, UMA vez por treinador.
	 *
	 * Devolve true se concedeu. A marca `bWalletGranted` é o que separa "novo"
	 * de "quebrado": zerar a carteira gastando NÃO devolve a bolsa — senão
	 * falir viraria renda, e o chão da economia é a cura de graça em casa
	 * (paga em caminhada), nunca dinheiro que renasce.
	 */
	static bool GrantStartingMoneyOnce(FTrainerProfile& Profile, int32 StartingMoney);

	/**
	 * Gasta, se houver. Devolve false — sem efeito — quando não há saldo ou
	 * quando o valor não é positivo.
	 *
	 * Não existe saldo negativo, e é decisão: dívida é mecânica própria, com
	 * dono próprio, não um `int` que passou do zero sem ninguém pedir.
	 */
	static bool TrySpend(FTrainerProfile& Profile, int32 Amount);

	/** Recebe. Valor não positivo é ignorado — pagamento de zero não é pagamento. */
	static void Earn(FTrainerProfile& Profile, int32 Amount);
};

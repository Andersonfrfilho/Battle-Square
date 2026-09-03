// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/PetDataLoader.h"
#include "Meta/PetCollectionSaveGame.h"

/**
 * O QUADRO DE LIÇÕES da Escola (decisão 64): o que o seu pet alcança, o que
 * ainda não, e onde treinar para alcançar.
 *
 * PURO, no formato de `FWorldStatusReadout`: recebe o registro e a instância,
 * devolve linhas. Não lê save, não carrega espelho, não desenha — e por isso
 * tem teste sem abrir o editor.
 *
 * Ele existe porque o golpe trancado só aparecia DENTRO da batalha — o botão
 * inerte com o cadeado. Descobrir o requisito no meio da luta é descobrir
 * tarde: a Escola é onde se lê ANTES, com calma, e com o dedo apontando o
 * campo do pátio que destrava.
 */
// Adiantada AQUI, no escopo global: dentro do namespace, `class F...&` no
// parâmetro declararia uma `SchoolTeachings::FTypeEffectivenessTable` nova —
// incompleta para sempre, e o erro sai longe, no `.cpp`.
class FTypeEffectivenessTable;

namespace SchoolTeachings
{
	/**
	 * Uma linha por golpe do registro. As TRANCADAS dizem o requisito e onde
	 * treinar; as alcançadas dizem só que se alcançou — lição não é lista de
	 * conquista.
	 */
	BATTLESQUARE_API TArray<FString> BuildLessonLines(
		const FLoadedPetRecord& Record, const FOwnedPetInstance& Owned);

	/**
	 * A ESTRATÉGIA do tipo: bom contra, ruim contra, e de quem apanhar.
	 *
	 * Lida da MESMA tabela que a batalha usa (`FTypeEffectivenessTable`) —
	 * uma lição escrita à mão ensinaria um número e a batalha cobraria outro,
	 * que é exatamente o defeito que a tabela veio matar ("existia, era
	 * testada, e nunca chegava ao jogo").
	 *
	 * `AllTypes` são os tipos que EXISTEM (escola/elemento do catálogo): a
	 * lição compara contra o mundo real, nunca contra uma lista própria.
	 */
	BATTLESQUARE_API TArray<FString> BuildStrategyLines(const FString& PetType,
		const FTypeEffectivenessTable& Table, const TArray<FString>& AllTypes);

	/**
	 * Os avisos de TERRENO — e cada frase tem um teste de BattleSim por trás.
	 *
	 * A escola só ensina o que a batalha cobra: dano de casa que voar escapa
	 * (`LeavingTheGround.AvoidsCellDamage`), camuflar que não escapa (mesmo
	 * teste), submergir que exige água funda, gelo que nega e lama que é
	 * aposta. Frase sem teste correspondente não entra.
	 */
	BATTLESQUARE_API TArray<FString> BuildTerrainLines();
}

// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Balance/PetBiologyCatalog.h"
#include "GameplayTagContainer.h"
#include "Battle/BattleState.h"
#include "Data/PetDataLoader.h"
#include "Balance/TypeEffectivenessTable.h"
#include "BattleDataTranslator.generated.h"

// T18 (tasks.md): traduz um pet carregado (FLoadedPetRecord — id uuid,
// type string) para o formato do núcleo (FPetState — só inteiros/enums,
// AD-004). "type" NUNCA entra em FPetState — vira FGameplayTag aqui,
// numa struct de apresentação que só existe em BattleSquare, exatamente
// como GameplayTags nunca entra em BattleSim (AD-012).

// Dados de pet que só interessam à apresentação — nome, tipo/tag. Nunca
// atravessa para o BattleSim.
USTRUCT()
struct FPetPresentationInfo
{
	GENERATED_BODY()

	UPROPERTY()
	uint8 PetId = 0;

	UPROPERTY()
	FString Name;

	UPROPERTY()
	FGameplayTag TypeTag;

	// T4 (colecao-e-captura): tipo em texto puro — TypeTag já existe para
	// GAS/apresentação (FGameplayTag), mas FOwnedPetInstance precisa de
	// FString direto, sem reconstruir a partir do nome da tag.
	UPROPERTY()
	FString Type;

	/**
	 * A BIOLOGIA do pet, como o cadastro a trouxe.
	 *
	 * Viaja na apresentação, e não no estado do núcleo, porque o núcleo já
	 * recebe o EFEITO dela (resistência e firmeza já compostas) e nunca precisa
	 * saber que existe uma pele. Quem precisa do nome é a coleção e a tela.
	 */
	FPetBiology Biology;


	// T4 (colecao-e-captura, DP-colecao-03): id do registro de catálogo
	// (FLoadedPetRecord::Id) preservado até aqui — é o que permite
	// ABattleArena saber, quando a batalha termina, qual registro de
	// catálogo capturar. Nunca atravessa para BattleSim (mesma regra do
	// resto desta struct).
	UPROPERTY()
	FString CatalogId;

	/**
	 * Efetividade do tipo DESTE pet contra o do oponente, em porcentagem.
	 *
	 * Guardado na MONTAGEM, não recalculado na apresentação: a fórmula de dano
	 * vive no núcleo, e reescrevê-la num widget é o que `audit_no_recalculation.sh`
	 * proíbe — cliente e servidor podem divergir sem nada acusar.
	 *
	 * Existe porque a efetividade passou a se aplicar e continuava INVISÍVEL:
	 * o jogador tomava mais dano sem ter como saber por quê, e regra que só se
	 * descobre perdendo não ensina nada.
	 *
	 * 100 = neutro, e é o padrão para quem monta sem tabela.
	 */
	UPROPERTY()
	int32 EffectivenessPercent = 100;

	/**
	 * Nomes dos golpes, na ordem do slot (até quatro).
	 *
	 * Só o nome atravessa: o poder vive no dado assinado e é a MONTAGEM quem o
	 * usa. Levar poder até a tela convidaria alguém a recalcular dano ali, que
	 * é o que `audit_no_recalculation.sh` proíbe.
	 */
	UPROPERTY()
	TArray<FString> MoveNames;

	/**
	 * Requisito de atributo de cada golpe, na MESMA ordem de MoveNames.
	 *
	 * Vem do dado assinado. "none" com 0 é golpe sem requisito — o que todo
	 * golpe cadastrado antes desta feature continua sendo.
	 */
	UPROPERTY()
	TArray<FString> MoveRequiresAttribute;

	UPROPERTY()
	TArray<int32> MoveRequiresValue;

	/**
	 * O pet ALCANÇA o requisito de cada golpe?
	 *
	 * Começa tudo verdadeiro e só é apertado por quem conhece os atributos do
	 * pet (FPetMoveRequirements, contra a coleção). Nasce permissivo de
	 * propósito: pet que não está na coleção — o oponente selvagem, por
	 * exemplo — não tem atributo nenhum, e trancá-lo por isso o deixaria sem
	 * golpe nenhum para usar.
	 *
	 * É por ÍNDICE, nunca por filtragem da lista: o índice do golpe é o que
	 * viaja no commit (DP-golpe-04), e remover um item deslocaria todos os
	 * seguintes — o jogador escolheria um golpe e o servidor resolveria outro.
	 */
	UPROPERTY()
	TArray<bool> MoveUnlocked;
};

class BATTLESQUARE_API FBattleDataTranslator
{
public:
	// PetId é responsabilidade de quem monta a partida (DP-06 do design:
	// mapeamento uuid->PetId é local à batalha, sequencial, estável
	// durante toda ela) — esta função só traduz UM registro já com o
	// PetId, Side e posição inicial decididos pelo chamador.
	static void TranslatePet(
		const FLoadedPetRecord& Source,
		uint8 PetId,
		uint8 Side,
		uint8 Column,
		uint8 Row,
		FPetState& OutBattleState,
		FPetPresentationInfo& OutPresentation);

	/**
	 * O mesmo, com os ITENS que aquele pet está vestindo.
	 *
	 * Sobrecarga, e não um parâmetro a mais em toda chamada: os itens moram no
	 * SAVE, e o tradutor não conhece save — quem sabe os dois é a montagem da
	 * partida. Quem monta sem coleção (o teste, a arena de treino) continua
	 * chamando a de cima e recebendo o pet sem item, que é o comportamento de
	 * sempre.
	 *
	 * Os ids vêm do catálogo de itens; id desconhecido soma zero e não derruba
	 * — um erro de digitação no cadastro não pode tirar o pet da partida.
	 */
	static void TranslatePetWithItems(
		const FLoadedPetRecord& Source,
		const TArray<FString>& EquippedItemIds,
		uint8 PetId,
		uint8 Side,
		uint8 Column,
		uint8 Row,
		FPetState& OutBattleState,
		FPetPresentationInfo& OutPresentation);

	// T3 (escala-pets-skills, design.md — DP-escala-01): traduz os DOIS
	// lados de uma partida ao mesmo tempo, pré-multiplicando o Attack de
	// cada lado pela efetividade do tipo dele contra o tipo do oponente.
	// Chama a mesma lógica de TranslatePet internamente para os campos
	// que não mudam (Defense/Speed/MaxHealth/Health nunca são alterados
	// por tipo — só Attack). O núcleo (BattleSim) nunca sabe que tipo
	// existe: o Attack que ele recebe já chega efetivo.
	/**
	 * COMPÕE a resistência a um fluido: o traço dá a BASE, biologia e item SOMAM.
	 *
	 * Pura e pública porque é a regra, e a regra precisa de prova própria — o
	 * item ainda não existe como sistema (não há cadastro, save nem equipar),
	 * e sem esta função o ponto onde ele somaria seria uma descoberta no dia em
	 * que alguém o criasse, em vez de uma linha.
	 *
	 * A BIOLOGIA pode ser NEGATIVA, e é isso que faz a escolha dela ter custo:
	 * pelo encharca, e uma biologia que só somasse bônus seria uma lista de
	 * vantagens em vez de um eixo. O ITEM continua preso em não-negativo — item
	 * mal cadastrado não pode ABRIR uma fraqueza que o pet não tem.
	 *
	 * Presa entre -100 e 100: acima de 100 o dano viraria negativo, e curar
	 * quem pisa na lava é o oposto do que a regra promete.
	 */
	static int32 ComposeFluidResist(int32 FromTrait, int32 FromBiology, int32 FromItem);

	static void TranslateMatchup(
		const FLoadedPetRecord& LeftSource,
		const FLoadedPetRecord& RightSource,
		const FTypeEffectivenessTable& EffectivenessTable,
		uint8 LeftPetId,
		uint8 RightPetId,
		FPetState& OutLeftState,
		FPetPresentationInfo& OutLeftPresentation,
		FPetState& OutRightState,
		FPetPresentationInfo& OutRightPresentation);

	/**
	 * O mesmo, com os itens que CADA lado está vestindo.
	 *
	 * Sobrecarga pela mesma razão de `TranslatePetWithItems`: os itens moram no
	 * save, e só quem monta a partida conhece os dois. Quem monta sem coleção
	 * continua chamando a de cima e recebendo pets sem item — o comportamento
	 * de sempre.
	 */
	static void TranslateMatchupWithItems(
		const FLoadedPetRecord& LeftSource,
		const TArray<FString>& LeftItemIds,
		const FLoadedPetRecord& RightSource,
		const TArray<FString>& RightItemIds,
		const FTypeEffectivenessTable& EffectivenessTable,
		uint8 LeftPetId,
		uint8 RightPetId,
		FPetState& OutLeftState,
		FPetPresentationInfo& OutLeftPresentation,
		FPetState& OutRightState,
		FPetPresentationInfo& OutRightPresentation);
};


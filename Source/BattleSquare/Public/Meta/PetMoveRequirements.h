// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Meta/PetCollectionSaveGame.h"

struct FPetPresentationInfo;

/**
 * O pet alcança o que o golpe exige?
 *
 * Função pura sobre o requisito assinado e os atributos do pet: não lê save,
 * não lê mundo, e por isso a regra tem UM lugar e um teste — em vez de uma
 * cópia na tela, outra na fila de ações e uma terceira no servidor, que é como
 * se produz um golpe que aparece trancado e mesmo assim executa.
 *
 * Nomes de atributo vêm do dado assinado como texto ("musculature",
 * "flight", …), e não como enum, pelo mesmo motivo de `terrainEffect`: o
 * backend guarda o dado, o núcleo do jogo guarda o significado.
 */
class BATTLESQUARE_API FPetMoveRequirements
{
public:
	/**
	 * Atributo desconhecido NUNCA tranca.
	 *
	 * É a escolha segura na direção certa: um requisito escrito errado no
	 * backend — ou um atributo que esta versão do jogo ainda não conhece —
	 * deixaria o golpe inalcançável para sempre, e o jogador não teria como
	 * saber por quê. Destrancar é reversível; trancar em silêncio, não.
	 */
	static bool IsMet(const FString& RequiredAttribute, int32 RequiredValue,
		const FOwnedPetInstance& Instance);

	/**
	 * Marca em MoveUnlocked o que este pet alcança.
	 *
	 * Instância nula é pet FORA da coleção — oponente selvagem, pet ainda não
	 * capturado. Ele não tem atributo nenhum, e tudo fica destrancado: gatear
	 * por atributos que não existem o deixaria sem golpe.
	 */
	static void ApplyToPresentation(FPetPresentationInfo& Presentation,
		const FOwnedPetInstance* Instance);

	/**
	 * Nome do atributo como o JOGADOR lê ("flight" → "Voo").
	 *
	 * Mora aqui, e não em quem desenha, porque três telas precisam dele — o
	 * golpe trancado, o ganho de atributo e o anúncio de desbloqueio. Três
	 * cópias concordariam até a primeira edição, e o sintoma seria o mesmo
	 * atributo com dois nomes na mesma partida.
	 *
	 * Nome desconhecido volta COMO VEIO. Inventar um rótulo bonito para um
	 * atributo que o jogo não conhece esconderia o erro de cadastro atrás de
	 * uma palavra plausível.
	 */
	static FText GetAttributeLabel(const FString& Attribute);

	/**
	 * OS CINCO ATRIBUTOS, na grafia do dado assinado — a lista canônica.
	 *
	 * Ela morava como array literal dentro de quem planta os campos de treino,
	 * e o pátio da Escola precisou da mesma lista: seria a quarta tabela
	 * escondida que este projeto acha em dois dias (uso do solo, material da
	 * ponte, tipo de assentamento). Duas cópias concordam até alguém criar o
	 * sexto atributo — e aí um lugar ganha campo e o outro não.
	 */
	static const TArray<FString>& AllAttributeNames();

	/** "exige Voo 12", ou vazio quando o golpe não exige nada. */
	static FText DescribeRequirement(const FString& Attribute, int32 RequiredValue);
};

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
};

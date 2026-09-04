// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "CharacterAppearance.generated.h"

/**
 * A APARÊNCIA DO PERSONAGEM (crime-e-recompensa, decisão 23) — a identidade
 * VISUAL do treinador, como dado.
 *
 * Existe porque a marca de procurado dura "até o jogador mudar de aparência a
 * ponto de não ser identificado com o pet", e "quem viu o cartaz reconhece".
 * Sem a aparência como dado, "mudar de aparência" não tem o que medir, e
 * "reconhecer" não tem o que comparar.
 *
 * São TRAÇOS discretos, não uma malha: o cartaz de procurado descreve um
 * fulano ("cabelo escuro, casaco vermelho"), e é a descrição que se compara —
 * não os pixels. Traço por traço é o que permite "mudou o casaco mas não o
 * rosto" ser uma pergunta com resposta.
 */
USTRUCT()
struct BATTLESQUARE_API FCharacterAppearance
{
	GENERATED_BODY()

	/** O corte de cabelo — mudável no barbeiro (decisão 23). */
	UPROPERTY()
	int32 HairStyle = 0;

	UPROPERTY()
	int32 HairColor = 0;

	/** A roupa — a mais fácil de trocar, e a que menos disfarça. */
	UPROPERTY()
	int32 Outfit = 0;

	/**
	 * O ROSTO — o traço que NÃO se troca à toa. Roupa e cabelo mudam num
	 * espelho; o rosto é o que ancora "é a mesma pessoa" quando todo o resto
	 * mudou. Sem um traço estável, disfarçar-se seria trivial e a marca não
	 * duraria nada.
	 */
	UPROPERTY()
	int32 Face = 0;
};

/**
 * QUANTO duas aparências se parecem, e QUANDO isso conta como reconhecimento.
 *
 * Puro: recebe o que o cartaz mostrava e o que se vê agora, devolve se é a
 * mesma pessoa aos olhos de quem olha. A regra da decisão 23 vive aqui, num
 * lugar só — a polícia (CR9), o NPC que denuncia e o jogador que reconhece
 * leem a MESMA conta.
 */
namespace CharacterRecognition
{
	/**
	 * Quantos traços batem entre o cartaz e quem se vê (0 a 4).
	 */
	BATTLESQUARE_API int32 MatchingTraits(
		const FCharacterAppearance& OnPoster, const FCharacterAppearance& SeenNow);

	/**
	 * É reconhecido como o procurado do cartaz?
	 *
	 * O ROSTO é necessário: mudar roupa e cabelo NÃO basta se o rosto é o
	 * mesmo — e é isso que impede o disfarce barato. Mas rosto igual sozinho
	 * também não condena: precisa de mais um traço, senão dois desconhecidos
	 * de rosto parecido seriam a mesma pessoa. Rosto + ao menos um outro.
	 */
	BATTLESQUARE_API bool IsRecognized(
		const FCharacterAppearance& OnPoster, const FCharacterAppearance& SeenNow);
}

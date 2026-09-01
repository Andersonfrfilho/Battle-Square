// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * OS DEUSES, e o que cada um governa.
 *
 * A lista é curta pelo mesmo motivo da lista de biomas: cada entrada aqui
 * precisa de um LUGAR no mundo para existir. Deus sem templo é nome num enum, e
 * nome num enum não é ninguém.
 *
 * E o domínio não é sabor: é ele que decide onde o templo fica. Templo do deus
 * do monte fica no monte; da água, na cachoeira; do fogo, na rocha queimada.
 * Assim o mapa se lê sem legenda — quem vê um templo sabe do que ele é pelo
 * lugar onde está.
 *
 * Valores novos vão para o FIM: eles viajam em save.
 */
UENUM()
enum class EDeity : uint8
{
	/**
	 * MÃE NATUREZA: o equilíbrio do mundo vivo.
	 *
	 * Ela é a primeira porque já existia antes deste arquivo — é a força que a
	 * spec chama para reequilibrar o que os jogadores esgotam. O templo dela
	 * fica no BOSQUE mais fechado: onde a mata está mais viva.
	 */
	MaeNatureza,

	/** O deus do MONTE: a pedra que não se move. Templo na saia da montanha. */
	Pedra,

	/** A deusa da ÁGUA que corre. Templo na cachoeira. */
	Corrente,

	/** O deus do FOGO de baixo. Templo na beira da rocha queimada. */
	Braseiro,

	/**
	 * O deus do FUNDO: o que mora onde não bate sol.
	 *
	 * Templo na caverna — e é o único cujo templo não se vê de fora, o que é o
	 * ponto dele.
	 */
	Abismo
};

namespace Pantheon
{
	/** Quantos deuses existem. */
	BATTLESQUARE_API int32 Count();

	/** O nome curto, para o mapa e para o registro. Não é texto de jogador. */
	BATTLESQUARE_API const TCHAR* DebugName(EDeity Which);
}

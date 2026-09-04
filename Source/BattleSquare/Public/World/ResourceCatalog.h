// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * OS RECURSOS DO MUNDO (mundo-vivo, MV7 — decisão 68). Fonte ÚNICA: adicionar
 * recurso é adicionar uma linha AQUI, nunca uma tabela nova ao lado (L-032).
 *
 * Cada recurso vem de onde o mundo já o tem — a fonte não é inventada, é o
 * bioma/peça que o assado já gera.
 */
enum class EWorldResource : uint8
{
	Madeira,   // árvore / bosque
	Pedra,     // rocha / montanha
	Fruta,     // pomar / árvore
	Flor,      // clareira / campo
	Fibra,     // planta / pântano
	Cogumelo,  // caverna / pântano (o lado Fantasma)
	Minerio,   // vulcão / caverna
	Cristal,   // caverna
	Mel,       // bosque
	Peixe,     // rio / água
	Argila,    // margem / pântano
	Sal,       // praia
	Agua,      // poço / nascente
	Gelo,      // geleira

	Count UMETA(Hidden)
};

/** A FERRAMENTA que a colheita pode exigir (decisão 36/68-b). */
enum class EGatherTool : uint8
{
	Nenhuma,   // mão vazia — só o fácil
	Machado,   // madeira
	Picareta,  // pedra, minério, cristal
	Vara,      // peixe
	Balde,     // água
};

namespace ResourceCatalog
{
	/** O nome do recurso para a tela (LOCTEXT). */
	BATTLESQUARE_API FText NameOf(EWorldResource Resource);

	/** A ferramenta que este recurso EXIGE — Nenhuma quando a mão basta. */
	BATTLESQUARE_API EGatherTool RequiredTool(EWorldResource Resource);
}

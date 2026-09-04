// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "World/ResourceCatalog.h"

/**
 * A COLHEITA (mundo-vivo, MV7 — decisões 36/37/68). Quanto se colhe de um
 * recurso depende da FERRAMENTA (68-b) e de o PET AJUDAR (68-c) — regra pura,
 * testável sem mundo.
 */
namespace ResourceGathering
{
	/** O que o jogador traz para a colheita. */
	struct BATTLESQUARE_API FGatherContext
	{
		/** A ferramenta em mãos. */
		EGatherTool Tool = EGatherTool::Nenhuma;

		/** Tem um pet por perto ajudando? */
		bool bPetHelps = false;
	};

	/**
	 * Quanto se colhe de `Resource`, dado o contexto.
	 *
	 * Sem a ferramenta EXIGIDA, rende ZERO — o recurso aparece mas não vem
	 * (68-b). Com a ferramenta (ou quando a mão basta), rende a base; o pet por
	 * perto AUMENTA o rendimento (68-c), nunca é requisito. Nunca negativo.
	 */
	BATTLESQUARE_API int32 Yield(EWorldResource Resource, const FGatherContext& Context);
}

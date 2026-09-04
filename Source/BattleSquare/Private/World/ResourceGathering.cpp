// Copyright 2026 Anderson. All Rights Reserved.

#include "World/ResourceGathering.h"

namespace ResourceGatheringConst
{
	constexpr int32 RendimentoBase = 2;
	constexpr int32 BonusDoPet = 1; // 68-c: o pet é braço, soma ao braço do jogador.
}

int32 ResourceGathering::Yield(EWorldResource Resource, const FGatherContext& Context)
{
	const EGatherTool Exigida = ResourceCatalog::RequiredTool(Resource);

	// 68-b: sem a ferramenta exigida, o recurso aparece mas NÃO rende. Mão vazia
	// (Exigida == Nenhuma) sempre passa neste portão.
	if (Exigida != EGatherTool::Nenhuma && Context.Tool != Exigida)
	{
		return 0;
	}

	// 68-c: o pet por perto AUMENTA o rendimento — nunca é requisito. Sem pet,
	// ainda se colhe, só menos.
	return ResourceGatheringConst::RendimentoBase
		+ (Context.bPetHelps ? ResourceGatheringConst::BonusDoPet : 0);
}

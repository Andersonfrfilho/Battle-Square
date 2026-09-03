// Copyright 2026 Anderson. All Rights Reserved.

#include "World/IslandChart.h"

IslandChart::FUseCount IslandChart::CountOf(
	TArrayView<const FBakedGroundUse> Patches, EGroundUse Use)
{
	FUseCount Contagem;

	for (const FBakedGroundUse& Mancha : Patches)
	{
		if (Mancha.Use != Use)
		{
			continue;
		}

		// A POSIÇÃO NÃO É LIDA AQUI, e é o ponto todo desta função: ela conta
		// e não sabe onde. Uma contagem que olhasse a posição seria uma
		// contagem que pode vazá-la na próxima edição.
		++(Mancha.bHidden ? Contagem.Hidden : Contagem.Shown);
	}

	return Contagem;
}

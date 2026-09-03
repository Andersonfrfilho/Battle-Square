// Copyright 2026 Anderson. All Rights Reserved.

#include "World/RespawnRules.h"

#include "World/VillageLayout.h"

namespace
{
	/**
	 * A folga entre a parede do Centro e os pés de quem acorda.
	 *
	 * Acordar DENTRO do prédio seria nascer emparedado — o prédio bloqueia, e
	 * é o que faz a vila ser um lugar. Menor que a calçada da porta (160), de
	 * propósito: acordar já DENTRO da faixa da porta faz o painel anunciar
	 * "entrou: Centro de Recuperacao" sozinho, e é o anúncio que conta onde se
	 * está sem precisar de texto novo.
	 */
	constexpr float FolgaDoAcordar = 120.0f;
}

bool RespawnRules::NearestRecoveryCenter(const FVector2D& CaiuEm, FWakeSpot& Onde)
{
	bool bAchou = false;
	float MaisPerto = TNumericLimits<float>::Max();

	for (const FSettlementPlacement& Vila : RegionLayout::Plan())
	{
		const TArray<FVillagePlacement> Pecas = VillageLayout::PlanFor(Vila.Kind);
		if (!VillageLayout::HasBuilding(Pecas, EVillageBuilding::CentroDeRecuperacao))
		{
			continue;
		}

		const float Ate = FVector2D::Distance(CaiuEm, Vila.CenterUnits);
		if (Ate >= MaisPerto)
		{
			continue;
		}

		const FVillagePlacement* Centro = Pecas.FindByPredicate(
			[](const FVillagePlacement& Peca)
			{
				return Peca.Building == EVillageBuilding::CentroDeRecuperacao;
			});
		if (!Centro)
		{
			continue;
		}

		// NA PORTA, virado para a praça. A praça é o centro da vila, então o
		// rumo "de volta ao meio" existe sempre — exceto quando o prédio ESTÁ
		// no meio, e aí qualquer lado serve.
		const FVector2D ParaAPraca = Centro->OffsetUnits.IsNearlyZero()
			? FVector2D(1.0f, 0.0f)
			: -Centro->OffsetUnits.GetSafeNormal();

		const float Afastamento =
			FMath::Max(Centro->HalfExtentUnits.X, Centro->HalfExtentUnits.Y)
				+ FolgaDoAcordar;

		MaisPerto = Ate;
		Onde.Kind = Vila.Kind;
		Onde.VillageCenterUnits = Vila.CenterUnits;
		Onde.WakeUnits = Vila.CenterUnits + Centro->OffsetUnits + ParaAPraca * Afastamento;
		bAchou = true;
	}

	return bAchou;
}

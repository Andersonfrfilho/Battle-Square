// Copyright 2026 Anderson. All Rights Reserved.

#include "World/SchoolyardLayout.h"

#include "Meta/PetMoveRequirements.h"
#include "World/VillageLayout.h"

namespace
{
	/**
	 * O campo do pátio é MIÚDO de propósito: 120 contra os 400 do campo da
	 * mata. O pátio ensina e especializa; a clareira grande da mata continua
	 * sendo o lugar com espaço — e continua sendo destino.
	 */
	constexpr float RaioDoCampoDoPatio = 120.0f;

	/** Entre um campo e o vizinho, de centro a centro. */
	constexpr float PassoEntreCampos = 300.0f;
}

TArray<SchoolyardLayout::FSchoolyardField> SchoolyardLayout::PlanFor(ESettlementKind Kind)
{
	TArray<FSchoolyardField> Campos;

	const TArray<FVillagePlacement> Pecas = VillageLayout::PlanFor(Kind);
	const FVillagePlacement* Escola = Pecas.FindByPredicate(
		[](const FVillagePlacement& Peca)
		{
			return Peca.Building == EVillageBuilding::Escola;
		});

	if (!Escola)
	{
		return Campos;
	}

	// ATRÁS da escola, na faixa da clareira: entre o lote (onde os prédios
	// acabam) e a mata (onde as árvores começam). É o único chão da vila em
	// que nada mais pode nascer — e "atrás" é a direção da praça para a
	// escola, prolongada, então a conta sobrevive se a escola mudar de lado.
	const FVector2D ParaTras = Escola->OffsetUnits.IsNearlyZero()
		? FVector2D(1.0f, 0.0f)
		: Escola->OffsetUnits.GetSafeNormal();

	const float Lote = VillageLayout::PlotHalfExtentUnitsFor(Kind);
	const float Clareira = VillageLayout::ClearingHalfExtentUnitsFor(Kind);
	const FVector2D MeioDaFaixa = ParaTras * ((Lote + Clareira) * 0.5f);

	const FVector2D AoLado(-ParaTras.Y, ParaTras.X);

	const TArray<FString>& Atributos = FPetMoveRequirements::AllAttributeNames();
	for (int32 Indice = 0; Indice < Atributos.Num(); ++Indice)
	{
		FSchoolyardField Campo;
		Campo.Attribute = Atributos[Indice];
		Campo.RadiusUnits = RaioDoCampoDoPatio;

		// Em fileira, centrada na faixa: o índice do meio fica no eixo da
		// escola, os outros abrem para os lados.
		const float Afastamento = PassoEntreCampos
			* (static_cast<float>(Indice) - static_cast<float>(Atributos.Num() - 1) * 0.5f);
		Campo.OffsetUnits = MeioDaFaixa + AoLado * Afastamento;

		Campos.Add(Campo);
	}

	return Campos;
}

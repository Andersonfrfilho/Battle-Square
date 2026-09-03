// Copyright 2026 Anderson. All Rights Reserved.

#include "Meta/PetMoveRequirements.h"
#include "Misc/AutomationTest.h"
#include "World/SchoolyardLayout.h"
#include "World/VillageLayout.h"

/**
 * O PÁTIO DA ESCOLA — decisão 63, e a prova é geométrica.
 *
 * A CI4 morreu uma vez por geometria não medida (a Escola a vinte mil unidades
 * do campo). O pátio nasce com a medição no teste: cabe na clareira, não
 * invade prédio, e existe SÓ onde há Escola.
 */

namespace PatioDaEscolaTeste
{
	// Nome próprio (L-042): helper homônimo em outro arquivo de teste vira
	// sobrecarga ambígua quando o unity build junta os dois.
	bool CampoDoPatioInvadePredio(const SchoolyardLayout::FSchoolyardField& Campo,
		const FVillagePlacement& Peca)
	{
		// Ponto do retângulo mais perto do centro do círculo — o teste padrão
		// de círculo contra caixa, sem inventar folga.
		const FVector2D MaisPerto(
			FMath::Clamp(Campo.OffsetUnits.X,
				Peca.OffsetUnits.X - Peca.HalfExtentUnits.X,
				Peca.OffsetUnits.X + Peca.HalfExtentUnits.X),
			FMath::Clamp(Campo.OffsetUnits.Y,
				Peca.OffsetUnits.Y - Peca.HalfExtentUnits.Y,
				Peca.OffsetUnits.Y + Peca.HalfExtentUnits.Y));

		return FVector2D::Distance(MaisPerto, Campo.OffsetUnits) < Campo.RadiusUnits;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSchoolyardExistsOnlyWithASchoolTest,
	"BattleSquare.World.Patio.SoOndeHaEscola",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSchoolyardExistsOnlyWithASchoolTest::RunTest(const FString&)
{
	const ESettlementKind Todos[] = {
		ESettlementKind::VilaInicial, ESettlementKind::VilaDaAcademia,
		ESettlementKind::VilaDoMercado, ESettlementKind::CidadeGrande,
		ESettlementKind::PostoDeFronteira,
	};

	bool bAlgumTemPatio = false;
	for (ESettlementKind Tipo : Todos)
	{
		const bool bTemEscola = VillageLayout::HasBuilding(
			VillageLayout::PlanFor(Tipo), EVillageBuilding::Escola);
		const TArray<SchoolyardLayout::FSchoolyardField> Campos =
			SchoolyardLayout::PlanFor(Tipo);

		// O pátio é DA ESCOLA: vila sem escola com campos soltos seria a
		// academia de graça que o traçado nega de propósito.
		TestEqual(TEXT("patio se e so se ha Escola"), Campos.Num() > 0, bTemEscola);
		bAlgumTemPatio |= Campos.Num() > 0;

		if (Campos.Num() == 0)
		{
			continue;
		}

		// TODOS os atributos, cada um UMA vez: um pátio sem o campo de voo
		// especializaria em quatro dos cinco, e ninguém saberia por quê.
		TSet<FString> Atributos;
		for (const SchoolyardLayout::FSchoolyardField& Campo : Campos)
		{
			Atributos.Add(Campo.Attribute);
		}

		TestEqual(TEXT("um campo por atributo canonico"),
			Atributos.Num(), FPetMoveRequirements::AllAttributeNames().Num());
	}

	TestTrue(TEXT("ha pelo menos um patio na regiao"), bAlgumTemPatio);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSchoolyardFitsAndInvadesNothingTest,
	"BattleSquare.World.Patio.CabeNaClareiraENaoInvade",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSchoolyardFitsAndInvadesNothingTest::RunTest(const FString&)
{
	// A geometria que matou a CI4 original, agora afirmada: os campos ficam na
	// FAIXA DA CLAREIRA — fora do lote (onde os prédios acabam) e dentro da
	// clareira (onde a mata começa).
	const ESettlementKind Tipos[] = {
		ESettlementKind::VilaInicial, ESettlementKind::VilaDaAcademia,
		ESettlementKind::VilaDoMercado, ESettlementKind::CidadeGrande,
		ESettlementKind::PostoDeFronteira,
	};

	for (ESettlementKind Tipo : Tipos)
	{
		const float Lote = VillageLayout::PlotHalfExtentUnitsFor(Tipo);
		const float Clareira = VillageLayout::ClearingHalfExtentUnitsFor(Tipo);
		const TArray<FVillagePlacement> Pecas = VillageLayout::PlanFor(Tipo);

		for (const SchoolyardLayout::FSchoolyardField& Campo : SchoolyardLayout::PlanFor(Tipo))
		{
			// Dentro da clareira, com o CÍRCULO inteiro — a quina de um campo
			// na mata é árvore nascendo dentro do treino.
			TestTrue(TEXT("o campo inteiro cabe na clareira"),
				FMath::Abs(Campo.OffsetUnits.X) + Campo.RadiusUnits <= Clareira
					&& FMath::Abs(Campo.OffsetUnits.Y) + Campo.RadiusUnits <= Clareira);

			// Fora do lote: o pátio mora na faixa, não na praça.
			TestTrue(TEXT("o centro do campo fica fora do lote dos predios"),
				FMath::Abs(Campo.OffsetUnits.X) > Lote
					|| FMath::Abs(Campo.OffsetUnits.Y) > Lote);

			// E nenhum prédio é invadido — o teste que a CI4 original nunca
			// teve, e que teria dito na hora que a geometria não fechava.
			for (const FVillagePlacement& Peca : Pecas)
			{
				TestFalse(TEXT("campo do patio nao invade predio"),
					PatioDaEscolaTeste::CampoDoPatioInvadePredio(Campo, Peca));
			}
		}
	}

	return true;
}

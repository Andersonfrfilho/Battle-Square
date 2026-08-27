// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattleTypes.h"
#include "Misc/AutomationTest.h"

// GetDirectionTowards é o INVERSO de GetDirectionDelta, e a única defesa
// contra alguém escrever uma segunda tabela de direções — o que já produziu
// um defeito real aqui ("Baixo" andando para a direita na tela).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDirectionTowardsRoundTripsTest,
	"BattleSim.Types.DirectionTowards.RoundTrips",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDirectionTowardsRoundTripsTest::RunTest(const FString& Parameters)
{
	for (uint8 Index = static_cast<uint8>(EBattleDirection::Cima);
		Index <= static_cast<uint8>(EBattleDirection::BaixoDireita); ++Index)
	{
		const EBattleDirection Original = static_cast<EBattleDirection>(Index);
		int8 DeltaColumn = 0;
		int8 DeltaRow = 0;
		GetDirectionDelta(Original, DeltaColumn, DeltaRow);

		TestEqual(TEXT("Ida e volta preserva a direção"),
			static_cast<uint8>(GetDirectionTowards(DeltaColumn, DeltaRow)), Index);
	}

	// Alvo longe: só o SINAL decide, porque todo passo é de uma casa.
	TestEqual(TEXT("Distância maior que uma casa vira o passo certo"),
		static_cast<uint8>(GetDirectionTowards(2, -2)),
		static_cast<uint8>(EBattleDirection::CimaDireita));

	// Alvo na própria casa não tem direção — e não pode virar uma qualquer.
	TestEqual(TEXT("Mesma casa não tem direção"),
		static_cast<uint8>(GetDirectionTowards(0, 0)),
		static_cast<uint8>(EBattleDirection::Nenhuma));

	return true;
}

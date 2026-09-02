// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattleArenaConstants.h"
#include "Battle/BattleEvent.h"
#include "Battle/BattlePhases.h"
#include "Battle/BattleState.h"
#include "Battle/FluidRegistry.h"
#include "Misc/AutomationTest.h"

/**
 * A CORRENTE CARREGA — a água deixa de ser estado e vira força.
 *
 * Ela entra pelo cano do movimento que já existe, e no lugar que dá sentido à
 * ordem: DEPOIS do passo voluntário e ANTES do dano de casa. Quem a água leva
 * para dentro da lava se queima na casa onde PAROU, não na de onde saiu.
 */

namespace ProvaDaCorrente
{
	constexpr int32 Forte = BattleArenaConstants::CurrentCarriesAbovePerMille + 100;
	constexpr int32 Manso = BattleArenaConstants::CurrentCarriesAbovePerMille - 10;

	FBattleState CampoComRio(int32 ForcaPorMil)
	{
		FBattleState Estado;
		Estado.GridColumns = 3;
		Estado.GridRows = 3;
		Estado.CellLayout.SetNumZeroed(9);
		Estado.Random.State = 5;
		Estado.ApplyDefaultTerrainRequirements();

		// Um rio correndo para a DIREITA na linha do meio.
		for (int32 Coluna = 0; Coluna < 3; ++Coluna)
		{
			Estado.CellLayout[Estado.CellIndex(Coluna, 1)] =
				static_cast<uint8>(ECellProperty::Water);
			Estado.SetFlowAt(Coluna, 1, EBattleDirection::Direita, ForcaPorMil);
		}

		FPetState Nadador;
		Nadador.PetId = 1; Nadador.Side = 0; Nadador.Column = 0; Nadador.Row = 1;
		Nadador.Health = 100; Nadador.MaxHealth = 100; Nadador.Speed = 30;
		Estado.Pets.Add(Nadador);

		return Estado;
	}

	void UmSlotSemAgir(FBattleState& Estado)
	{
		const FBattleAction Nada;
		TArray<FBattleEvent> Traco;
		BattlePhases::ApplyMovement(Estado,
		BattlePhases::DuelSlotActions(Estado, Nada, Nada), 0, Traco);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCurrentCarriesDownstreamTest,
	"BattleSim.CurrentCarries.Downstream",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCurrentCarriesDownstreamTest::RunTest(const FString& Parameters)
{
	// O ACEITE: o pet desce o rio SOZINHO, sem ter pedido para andar.
	FBattleState Estado = ProvaDaCorrente::CampoComRio(ProvaDaCorrente::Forte);
	ProvaDaCorrente::UmSlotSemAgir(Estado);

	TestEqual(TEXT("a corrente levou uma casa rio abaixo"),
		static_cast<int32>(Estado.Pets[0].Column), 1);
	TestEqual(TEXT("e nao mudou de linha"),
		static_cast<int32>(Estado.Pets[0].Row), 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCurrentCarriesOnlyWhereItRunsHardTest,
	"BattleSim.CurrentCarries.OnlyWhereItRunsHard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCurrentCarriesOnlyWhereItRunsHardTest::RunTest(const FString& Parameters)
{
	// O CONTRAPESO. Sem ele, uma regra que carregasse em qualquer água passaria
	// no aceite — e atravessar um fio manso deixaria de ser escolha para virar
	// sorte, com o jogador sendo arrastado por uma poça.
	FBattleState Manso = ProvaDaCorrente::CampoComRio(ProvaDaCorrente::Manso);
	ProvaDaCorrente::UmSlotSemAgir(Manso);

	TestEqual(TEXT("agua mansa nao carrega"),
		static_cast<int32>(Manso.Pets[0].Column), 0);

	// E água PARADA também não — nem com o pet dentro dela.
	FBattleState Parada = ProvaDaCorrente::CampoComRio(0);
	ProvaDaCorrente::UmSlotSemAgir(Parada);

	TestEqual(TEXT("agua parada nao carrega"),
		static_cast<int32>(Parada.Pets[0].Column), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCurrentCarriesFlyingEscapesButDivingDoesNotTest,
	"BattleSim.CurrentCarries.FlyingEscapesButDivingDoesNot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCurrentCarriesFlyingEscapesButDivingDoesNotTest::RunTest(const FString& Parameters)
{
	// QUEM VOA ESCAPA: ele não encosta na água.
	FBattleState Voando = ProvaDaCorrente::CampoComRio(ProvaDaCorrente::Forte);
	Voando.Pets[0].PostureFlags |= static_cast<uint16>(EBattlePostureFlags::Flying);
	ProvaDaCorrente::UmSlotSemAgir(Voando);

	TestEqual(TEXT("quem voa nao e carregado"),
		static_cast<int32>(Voando.Pets[0].Column), 0);

	// QUEM SUBMERGE, NÃO — e esta é uma diferença DELIBERADA em relação ao dano
	// de casa, que deixa os dois escaparem. O dano é do CHÃO, e quem não pisa
	// nele não o sente; a corrente é a ÁGUA, e estar submerso é estar mais
	// dentro dela. Tratar submergir como fuga faria mergulhar num rio deixar de
	// ter preço.
	FBattleState Submerso = ProvaDaCorrente::CampoComRio(ProvaDaCorrente::Forte);
	Submerso.Pets[0].PostureFlags |=
		static_cast<uint16>(EBattlePostureFlags::Underground);
	ProvaDaCorrente::UmSlotSemAgir(Submerso);

	TestEqual(TEXT("quem submerge E carregado"),
		static_cast<int32>(Submerso.Pets[0].Column), 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCurrentCarriesStopsAtWhatIsInTheWayTest,
	"BattleSim.CurrentCarries.StopsAtWhatIsInTheWay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCurrentCarriesStopsAtWhatIsInTheWayTest::RunTest(const FString& Parameters)
{
	// CASA OCUPADA: a corrente empurra contra e ele FICA. Levar um pet para
	// cima de outro poria duas criaturas na mesma casa por um caminho que não
	// é a trombada — e a trombada tem regras próprias que este empurrão não
	// sabe aplicar.
	FBattleState ComVizinho = ProvaDaCorrente::CampoComRio(ProvaDaCorrente::Forte);

	FPetState Parado;
	Parado.PetId = 2; Parado.Side = 1; Parado.Column = 1; Parado.Row = 1;
	Parado.Health = 100; Parado.MaxHealth = 100;
	ComVizinho.Pets.Add(Parado);

	ProvaDaCorrente::UmSlotSemAgir(ComVizinho);

	TestEqual(TEXT("a corrente nao empilha um pet sobre o outro"),
		static_cast<int32>(ComVizinho.Pets[0].Column), 0);

	// CASA FECHADA: mesma coisa.
	FBattleState ComPedra = ProvaDaCorrente::CampoComRio(ProvaDaCorrente::Forte);
	ComPedra.CellLayout[ComPedra.CellIndex(1, 1)] =
		static_cast<uint8>(ECellProperty::Blocked);
	ProvaDaCorrente::UmSlotSemAgir(ComPedra);

	TestEqual(TEXT("a corrente nao empurra para dentro de pedra"),
		static_cast<int32>(ComPedra.Pets[0].Column), 0);

	// BORDA DA GRADE: o rio corre para fora, e ele fica.
	FBattleState NaBorda = ProvaDaCorrente::CampoComRio(ProvaDaCorrente::Forte);
	NaBorda.Pets[0].Column = 2;
	ProvaDaCorrente::UmSlotSemAgir(NaBorda);

	TestEqual(TEXT("a corrente nao leva ninguem para fora do tabuleiro"),
		static_cast<int32>(NaBorda.Pets[0].Column), 2);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCurrentCarriesBeforeTheCellChargesTest,
	"BattleSim.CurrentCarries.BeforeTheCellCharges",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCurrentCarriesBeforeTheCellChargesTest::RunTest(const FString& Parameters)
{
	// A ORDEM É A REGRA: quem a água leva para dentro da lava se queima na casa
	// onde PAROU, não na de onde saiu. Se o dano viesse antes do empurrão, a
	// corrente seria um jeito grátis de atravessar terreno perigoso.
	FBattleState Estado = ProvaDaCorrente::CampoComRio(ProvaDaCorrente::Forte);
	Estado.SetFluidAt(1, 1, EFluidKind::Lava);

	ProvaDaCorrente::UmSlotSemAgir(Estado);

	TestEqual(TEXT("a corrente o levou para a casa de lava"),
		static_cast<int32>(Estado.Pets[0].Column), 1);
	TestEqual(TEXT("e ele se queimou NELA"),
		Estado.Pets[0].PendingDamage,
		FluidRegistry::DamagePerSlot(EFluidKind::Lava));

	return true;
}

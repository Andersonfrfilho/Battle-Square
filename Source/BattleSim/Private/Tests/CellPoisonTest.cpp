// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattleArenaConstants.h"
#include "Battle/BattleEvent.h"
#include "Battle/BattlePhases.h"
#include "Battle/BattleState.h"
#include "Battle/FluidRegistry.h"
#include "Misc/AutomationTest.h"

/**
 * VENENO É DA CASA, e não do fluido.
 *
 * A decisão se responde sozinha: veneno é de uma poça ESPECÍFICA. Pô-lo no
 * registro faria toda água doce da ilha ser venenosa — e faria a tabela do
 * fluido responder por uma coisa que muda de lugar para lugar.
 *
 * Dos sete atributos que a caminhada pediu, seis já existiam e já estavam do
 * lado certo (fundura e corrente na casa; densidade, condução e substância no
 * fluido). Este era o único que faltava.
 */

namespace ProvaDoVeneno
{
	constexpr int32 Dose = 7;

	FBattleState CampoComPocaVenenosa()
	{
		FBattleState Estado;
		Estado.GridColumns = 3;
		Estado.GridRows = 3;
		Estado.CellLayout.SetNumZeroed(9);
		Estado.Random.State = 13;
		Estado.ApplyDefaultTerrainRequirements();

		Estado.CellLayout[Estado.CellIndex(1, 1)] =
			static_cast<uint8>(ECellProperty::ShallowWater);
		Estado.SetPoisonAt(1, 1, Dose);

		FPetState Ele;
		Ele.PetId = 1; Ele.Side = 0; Ele.Column = 1; Ele.Row = 1;
		Ele.Health = 100; Ele.MaxHealth = 100; Ele.Speed = 30;

		FPetState Outro;
		Outro.PetId = 2; Outro.Side = 1; Outro.Column = 0; Outro.Row = 0;
		Outro.Health = 100; Outro.MaxHealth = 100;

		Estado.Pets.Add(Ele);
		Estado.Pets.Add(Outro);
		return Estado;
	}

	void PassarUmSlot(FBattleState& Estado)
	{
		const FBattleAction Nada;
		TArray<FBattleEvent> Traco;
		BattlePhases::ApplyMovement(Estado, Nada, Nada, 0, Traco);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCellPoisonHurtsWhoStandsInItTest,
	"BattleSim.CellPoison.HurtsWhoStandsInIt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCellPoisonHurtsWhoStandsInItTest::RunTest(const FString& Parameters)
{
	FBattleState Estado = ProvaDoVeneno::CampoComPocaVenenosa();
	ProvaDoVeneno::PassarUmSlot(Estado);

	TestEqual(TEXT("quem esta na poca venenosa se envenena"),
		Estado.Pets[0].PendingDamage, ProvaDoVeneno::Dose);

	// O CONTRAPESO: a casa ao lado é limpa. Sem ele, uma regra que envenenasse
	// o tabuleiro inteiro passaria, e a poça deixaria de ser um LUGAR.
	TestEqual(TEXT("quem esta fora dela nao se envenena"),
		Estado.Pets[1].PendingDamage, 0);

	// E nada é aplicado ainda (BTL-07): a vida só cai em F5.
	TestEqual(TEXT("a vida ainda nao caiu"), Estado.Pets[0].Health, 100);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCellPoisonIsOfThePlaceNotTheWaterTest,
	"BattleSim.CellPoison.IsOfThePlaceNotTheWater",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCellPoisonIsOfThePlaceNotTheWaterTest::RunTest(const FString& Parameters)
{
	// O PRINCÍPIO, e ele é o que mantém três regras coerentes em vez de três
	// exceções: o que é do LUGAR alcança quem toca o lugar — e voar ou
	// submergir tiram o pet dali. O que é da ÁGUA alcança quem está dentro
	// dela, e submergir é estar MAIS dentro.
	//
	// Veneno de poça é do lugar, como a brasa. A correnteza é da água.
	FBattleState Voando = ProvaDoVeneno::CampoComPocaVenenosa();
	Voando.Pets[0].PostureFlags |= static_cast<uint16>(EBattlePostureFlags::Flying);
	ProvaDoVeneno::PassarUmSlot(Voando);
	TestEqual(TEXT("quem voa nao se envenena"), Voando.Pets[0].PendingDamage, 0);

	FBattleState Submerso = ProvaDoVeneno::CampoComPocaVenenosa();
	Submerso.Pets[0].PostureFlags |=
		static_cast<uint16>(EBattlePostureFlags::Underground);
	ProvaDoVeneno::PassarUmSlot(Submerso);
	TestEqual(TEXT("quem submerge tambem escapa do veneno do LUGAR"),
		Submerso.Pets[0].PendingDamage, 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCellPoisonIsNotAFluidPropertyTest,
	"BattleSim.CellPoison.IsNotAFluidProperty",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCellPoisonIsNotAFluidPropertyTest::RunTest(const FString& Parameters)
{
	// A PROVA DA DECISÃO: duas casas do MESMO fluido, uma envenenada e outra
	// não. Se veneno fosse do registro, isto seria impossível de expressar — e
	// toda água doce da ilha seria venenosa junto.
	FBattleState Estado = ProvaDoVeneno::CampoComPocaVenenosa();
	Estado.CellLayout[Estado.CellIndex(2, 1)] =
		static_cast<uint8>(ECellProperty::ShallowWater);

	TestEqual(TEXT("as duas casas sao do mesmo fluido"),
		static_cast<int32>(Estado.FluidAt(1, 1)),
		static_cast<int32>(Estado.FluidAt(2, 1)));

	TestTrue(TEXT("e so uma esta envenenada"),
		Estado.PoisonAt(1, 1) > 0 && Estado.PoisonAt(2, 1) == 0);

	// E o veneno SOMA com o que o fluido cobra, pelo mesmo acumulador: uma
	// poça de lava envenenada custa as duas coisas, e não a maior delas.
	FBattleState Dobrado = ProvaDoVeneno::CampoComPocaVenenosa();
	Dobrado.CellLayout[Dobrado.CellIndex(1, 1)] =
		static_cast<uint8>(ECellProperty::Water);
	Dobrado.SetFluidAt(1, 1, EFluidKind::Lava);
	ProvaDoVeneno::PassarUmSlot(Dobrado);

	TestEqual(TEXT("veneno e lava somam no mesmo acumulador"),
		Dobrado.Pets[0].PendingDamage,
		ProvaDoVeneno::Dose + FluidRegistry::DamagePerSlot(EFluidKind::Lava));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCellPoisonEntersTheHashTest,
	"BattleSim.CellPoison.EntersTheHash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCellPoisonEntersTheHashTest::RunTest(const FString& Parameters)
{
	FBattleState Limpa;
	Limpa.GridColumns = 3;
	Limpa.GridRows = 3;
	Limpa.CellLayout.SetNumZeroed(9);

	FBattleState Envenenada = Limpa;
	Envenenada.SetPoisonAt(1, 1, 7);

	TestNotEqual(TEXT("o veneno muda a assinatura"),
		Limpa.ComputeHash(), Envenenada.ComputeHash());

	// Lista VAZIA e lista de ZEROS são a mesma casa limpa — somar o array cru
	// daria assinaturas diferentes para estados idênticos.
	FBattleState Materializada = Limpa;
	Materializada.CellPoison.SetNumZeroed(9);

	TestEqual(TEXT("lista vazia e lista de zeros dao a MESMA assinatura"),
		Materializada.ComputeHash(), Limpa.ComputeHash());

	return true;
}

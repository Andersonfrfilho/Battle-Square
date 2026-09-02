// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattleArenaConstants.h"
#include "Battle/BattleEvent.h"
#include "Battle/BattlePhases.h"
#include "Battle/BattleState.h"
#include "Battle/FluidRegistry.h"
#include "Misc/AutomationTest.h"

/**
 * FRAQUEZA e FIRMEZA — as duas metades da P3 que tinham onde nascer.
 *
 * *"pets que têm fraqueza a certos elementos e outros que são imunes"* — a
 * imunidade já cabia (100%); a FRAQUEZA não, porque o número era sem sinal.
 *
 * E a FIRMEZA fecha o buraco que a C2 declarou por extenso: lá, quem resistia à
 * corrente era só o LUGAR, porque não havia número por criatura.
 */

namespace ProvaDaFraqueza
{
	FBattleState NaLava(int32 Resistencia)
	{
		FBattleState Estado;
		Estado.GridColumns = 3;
		Estado.GridRows = 3;
		Estado.CellLayout.SetNumZeroed(9);
		Estado.Random.State = 3;
		Estado.ApplyDefaultTerrainRequirements();

		Estado.CellLayout[Estado.CellIndex(1, 1)] =
			static_cast<uint8>(ECellProperty::Water);
		Estado.SetFluidAt(1, 1, EFluidKind::Lava);

		FPetState Ele;
		Ele.PetId = 1; Ele.Side = 0; Ele.Column = 1; Ele.Row = 1;
		Ele.Health = 200; Ele.MaxHealth = 200; Ele.Speed = 30;
		Ele.SetResistPercentFor(EFluidKind::Lava, Resistencia);
		Estado.Pets.Add(Ele);

		return Estado;
	}

	FBattleState NoRio(int32 ForcaPorMil, int32 FirmezaPorMil)
	{
		FBattleState Estado;
		Estado.GridColumns = 5;
		Estado.GridRows = 3;
		Estado.CellLayout.SetNumZeroed(15);
		Estado.Random.State = 5;
		Estado.ApplyDefaultTerrainRequirements();

		for (int32 Coluna = 0; Coluna < 5; ++Coluna)
		{
			Estado.CellLayout[Estado.CellIndex(Coluna, 1)] =
				static_cast<uint8>(ECellProperty::Water);
			Estado.SetFlowAt(Coluna, 1, EBattleDirection::Direita, ForcaPorMil);
		}

		FPetState Ele;
		Ele.PetId = 1; Ele.Side = 0; Ele.Column = 1; Ele.Row = 1;
		Ele.Health = 200; Ele.MaxHealth = 200; Ele.Speed = 30;
		Ele.FootingPerMille = FirmezaPorMil;
		Estado.Pets.Add(Ele);

		return Estado;
	}

	void PassarUmSlot(FBattleState& Estado)
	{
		const FBattleAction Nada;
		TArray<FBattleEvent> Traco;
		BattlePhases::ApplyMovement(Estado, Nada, Nada, 0, Traco);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaknessMakesItHurtMoreTest,
	"BattleSim.Weakness.MakesItHurtMore",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWeaknessMakesItHurtMoreTest::RunTest(const FString& Parameters)
{
	const int32 Bruto = FluidRegistry::DamagePerSlot(EFluidKind::Lava);

	// FRAQUEZA é resistência NEGATIVA, e sem sinal ela não cabia no número.
	FBattleState Fraco = ProvaDaFraqueza::NaLava(-100);
	ProvaDaFraqueza::PassarUmSlot(Fraco);
	TestEqual(TEXT("quem e fraco leva o dobro"),
		Fraco.Pets[0].PendingDamage, Bruto * 2);

	// AS TRÊS FAIXAS, e é a existência das três que prova que o eixo tem
	// sentido: sem a do meio, "fraco" e "imune" seriam dois booleanos.
	FBattleState Normal = ProvaDaFraqueza::NaLava(0);
	ProvaDaFraqueza::PassarUmSlot(Normal);
	TestEqual(TEXT("quem nao resiste leva o normal"),
		Normal.Pets[0].PendingDamage, Bruto);

	FBattleState Imune = ProvaDaFraqueza::NaLava(100);
	ProvaDaFraqueza::PassarUmSlot(Imune);
	TestEqual(TEXT("quem e imune nao leva nada"), Imune.Pets[0].PendingDamage, 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaknessHasAFloorTest,
	"BattleSim.Weakness.HasAFloor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWeaknessHasAFloorTest::RunTest(const FString& Parameters)
{
	// PISO DE -100. Sem ele, o dobro viraria triplo sem nada segurando, e um
	// descuido de cadastro (um -800 digitado) viraria morte instantânea.
	FBattleState Absurdo = ProvaDaFraqueza::NaLava(-800);
	ProvaDaFraqueza::PassarUmSlot(Absurdo);

	TestEqual(TEXT("a fraqueza para no dobro"),
		Absurdo.Pets[0].PendingDamage,
		FluidRegistry::DamagePerSlot(EFluidKind::Lava) * 2);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFootingResistsTheCurrentTest,
	"BattleSim.Footing.ResistsTheCurrent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFootingResistsTheCurrentTest::RunTest(const FString& Parameters)
{
	constexpr int32 Corrente = 200;

	// O BURACO DA C2, FECHADO: na mesma corrente, quem tem firmeza fica e quem
	// não tem é levado. Antes disto, só o LUGAR resistia.
	FBattleState Firme = ProvaDaFraqueza::NoRio(Corrente, /*Firmeza=*/250);
	ProvaDaFraqueza::PassarUmSlot(Firme);
	TestEqual(TEXT("quem tem firmeza fica"),
		static_cast<int32>(Firme.Pets[0].Column), 1);

	FBattleState Solto = ProvaDaFraqueza::NoRio(Corrente, /*Firmeza=*/0);
	ProvaDaFraqueza::PassarUmSlot(Solto);
	TestEqual(TEXT("quem nao tem e levado"),
		static_cast<int32>(Solto.Pets[0].Column), 2);

	// A ESCALA É A MESMA, e é o que torna a comparação direta: firmeza igual à
	// força ainda segura; um por mil abaixo, não. Escalas diferentes exigiriam
	// conversão, e conversão é onde o sentido se perde.
	FBattleState NoLimite = ProvaDaFraqueza::NoRio(Corrente, Corrente);
	ProvaDaFraqueza::PassarUmSlot(NoLimite);
	TestEqual(TEXT("firmeza igual a forca aguenta"),
		static_cast<int32>(NoLimite.Pets[0].Column), 1);

	FBattleState UmAbaixo = ProvaDaFraqueza::NoRio(Corrente, Corrente - 1);
	ProvaDaFraqueza::PassarUmSlot(UmAbaixo);
	TestEqual(TEXT("um por mil abaixo, nao"),
		static_cast<int32>(UmAbaixo.Pets[0].Column), 2);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFootingAlsoLetsYouWalkUpstreamTest,
	"BattleSim.Footing.AlsoLetsYouWalkUpstream",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFootingAlsoLetsYouWalkUpstreamTest::RunTest(const FString& Parameters)
{
	constexpr int32 Corrente = 200;

	// AS DUAS REGRAS TÊM DE CONCORDAR sobre quem a corrente domina. Quem ela
	// não consegue carregar também não consegue barrar — senão haveria um pet
	// que fica parado mas não pode andar contra, o que não descreve nada.
	FBattleState Firme = ProvaDaFraqueza::NoRio(Corrente, /*Firmeza=*/250);

	FBattleAction Subir;
	Subir.Type = EActionType::Mover;
	Subir.Direction = EBattleDirection::Esquerda;

	const FBattleAction Nada;
	TArray<FBattleEvent> Traco;
	BattlePhases::ApplyMovement(Firme, Subir, Nada, 0, Traco);

	TestEqual(TEXT("quem tem firmeza sobe a correnteza"),
		static_cast<int32>(Firme.Pets[0].Column), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFootingEntersTheHashTest,
	"BattleSim.Footing.EntersTheHash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFootingEntersTheHashTest::RunTest(const FString& Parameters)
{
	const FBattleState Firme = ProvaDaFraqueza::NoRio(200, 250);
	const FBattleState Solto = ProvaDaFraqueza::NoRio(200, 0);

	TestNotEqual(TEXT("a firmeza muda a assinatura"),
		Firme.ComputeHash(), Solto.ComputeHash());

	const FBattleState Fraco = ProvaDaFraqueza::NaLava(-50);
	const FBattleState Normal = ProvaDaFraqueza::NaLava(0);

	TestNotEqual(TEXT("a fraqueza muda a assinatura"),
		Fraco.ComputeHash(), Normal.ComputeHash());

	return true;
}

// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattleArenaConstants.h"
#include "Battle/BattleEvent.h"
#include "Battle/BattlePhases.h"
#include "Battle/BattleState.h"
#include "Battle/FluidRegistry.h"
#include "Misc/AutomationTest.h"

/**
 * RESISTÊNCIA A FLUIDO — o que um item de proteção compra.
 *
 * O núcleo carrega o NÚMERO e não sabe de onde ele vem: item, traço da
 * espécie, bênção de templo. É a mesma fronteira que faz o Attack chegar aqui
 * já pré-multiplicado pela efetividade de tipo, sem o núcleo nunca aprender
 * que tipo existe.
 */

namespace ProvaDaResistencia
{
	FBattleState NaLava(int32 ResistenciaPorCento)
	{
		FBattleState Estado;
		Estado.Random.State = 3;
		Estado.ApplyDefaultTerrainRequirements();

		FPetState Ele;
		Ele.PetId = 1; Ele.Side = 0; Ele.Column = 1; Ele.Row = 1;
		Ele.Health = 90; Ele.MaxHealth = 90; Ele.Speed = 40;
		Ele.SetResistPercentFor(EFluidKind::Lava, ResistenciaPorCento);

		FPetState Outro;
		Outro.PetId = 2; Outro.Side = 1; Outro.Column = 2; Outro.Row = 2;
		Outro.Health = 90; Outro.MaxHealth = 90;

		Estado.Pets.Add(Ele);
		Estado.Pets.Add(Outro);

		Estado.CellLayout[Estado.CellIndex(1, 1)] =
			static_cast<uint8>(ECellProperty::Water);
		Estado.SetFluidAt(1, 1, EFluidKind::Lava);
		return Estado;
	}

	void PassarUmSlot(FBattleState& Estado)
	{
		const FBattleAction Nada;
		TArray<FBattleEvent> Traco;
		BattlePhases::ApplyMovement(Estado, Nada, Nada, 0, Traco);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFluidResistanceTheItemCancelsTheBurnTest,
	"BattleSim.FluidResistance.TheItemCancelsTheBurn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFluidResistanceTheItemCancelsTheBurnTest::RunTest(const FString& Parameters)
{
	// O ACEITE DA F5, nas duas direções — e a segunda metade é o contrapeso
	// sem o qual "não vem" passaria numa regra que zerasse todo dano.
	FBattleState Protegido = ProvaDaResistencia::NaLava(100);
	ProvaDaResistencia::PassarUmSlot(Protegido);
	TestEqual(TEXT("com o item, o dano da lava nao vem"),
		Protegido.Pets[0].PendingDamage, 0);

	FBattleState Desprotegido = ProvaDaResistencia::NaLava(0);
	ProvaDaResistencia::PassarUmSlot(Desprotegido);
	TestEqual(TEXT("sem o item, vem inteiro"),
		Desprotegido.Pets[0].PendingDamage,
		FluidRegistry::DamagePerSlot(EFluidKind::Lava));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFluidResistanceIsAPercentNotASwitchTest,
	"BattleSim.FluidResistance.IsAPercentNotASwitch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFluidResistanceIsAPercentNotASwitchTest::RunTest(const FString& Parameters)
{
	// PORCENTAGEM, e não booleano de imunidade. O jogo já fala em porcentagem
	// em todo lugar — efetividade de tipo, escorregão, atraso — e um booleano
	// fecharia a porta para "resiste um pouco" sem ganhar nada em troca.
	//
	// Meia resistência é meio dano, e a conta é inteira: 12 vira 6.
	FBattleState Meio = ProvaDaResistencia::NaLava(50);
	ProvaDaResistencia::PassarUmSlot(Meio);

	const int32 Bruto = FluidRegistry::DamagePerSlot(EFluidKind::Lava);
	TestEqual(TEXT("meia resistencia, meio dano"),
		Meio.Pets[0].PendingDamage, (Bruto * 50) / 100);

	// E ela fica entre 0 e 100: acima de 100 o dano viraria negativo, e curar
	// quem pisa na lava é o oposto do que a regra promete.
	FBattleState Absurdo = ProvaDaResistencia::NaLava(300);
	ProvaDaResistencia::PassarUmSlot(Absurdo);
	TestEqual(TEXT("resistencia acima de 100 nao cura"),
		Absurdo.Pets[0].PendingDamage, 0);

	FBattleState Negativa = ProvaDaResistencia::NaLava(-50);
	ProvaDaResistencia::PassarUmSlot(Negativa);
	TestEqual(TEXT("resistencia negativa nao aumenta o dano"),
		Negativa.Pets[0].PendingDamage, Bruto);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFluidResistanceProtectsOnlyItsOwnFluidTest,
	"BattleSim.FluidResistance.ProtectsOnlyItsOwnFluid",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFluidResistanceProtectsOnlyItsOwnFluidTest::RunTest(const FString& Parameters)
{
	// A bota de lava não é armadura geral. Resistir a um fluido não pode
	// proteger de outro — senão o primeiro item do jogo seria o último.
	FBattleState Estado = ProvaDaResistencia::NaLava(100);
	TestEqual(TEXT("resiste a lava"),
		Estado.Pets[0].ResistPercentFor(EFluidKind::Lava), 100);
	TestEqual(TEXT("e nao resiste a agua salgada"),
		Estado.Pets[0].ResistPercentFor(EFluidKind::AguaSalgada), 0);

	// E NÃO PROTEGE DA BRASA: a casa de dano não é fluido, e resistir a lava
	// não pode fazer ninguém pisar em brasa de graça. Este é o caso que só
	// aparece quando os dois se somam na mesma casa.
	Estado.CellLayout[Estado.CellIndex(1, 1)] =
		static_cast<uint8>(ECellProperty::Damage);
	ProvaDaResistencia::PassarUmSlot(Estado);

	TestEqual(TEXT("a resistencia a lava nao protege da brasa"),
		Estado.Pets[0].PendingDamage, BattleArenaConstants::CellDamageAmount);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFluidResistanceEntersTheHashTest,
	"BattleSim.FluidResistance.EntersTheHash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFluidResistanceEntersTheHashTest::RunTest(const FString& Parameters)
{
	// Dois pets iguais, um com a proteção e outro sem, são dois estados
	// diferentes. Sem isto no hash, a diferença só apareceria no slot em que
	// alguém pisa na lava — tarde demais para o hash servir de alarme.
	const FBattleState Com = ProvaDaResistencia::NaLava(100);
	const FBattleState Sem = ProvaDaResistencia::NaLava(0);

	TestNotEqual(TEXT("a resistencia muda a assinatura"),
		Com.ComputeHash(), Sem.ComputeHash());

	// E a mesma resistência dá a mesma assinatura: um hash que variasse
	// sozinho acusaria dessincronia a cada carga.
	const FBattleState Igual = ProvaDaResistencia::NaLava(100);
	TestEqual(TEXT("a mesma resistencia da a mesma assinatura"),
		Igual.ComputeHash(), Com.ComputeHash());

	return true;
}

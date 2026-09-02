// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattleArenaConstants.h"
#include "Battle/BattleEvent.h"
#include "Battle/BattlePhases.h"
#include "Battle/BattleState.h"
#include "Battle/FluidRegistry.h"
#include "Misc/AutomationTest.h"

/**
 * O DANO DE ESTAR DENTRO — a lava deixando de ser cenário colorido.
 *
 * O registro já sabia que a lava custa 12, e nada acontecia: o número estava
 * guardado e ninguém cobrava. É L-041 na escala do fluido — regra completa,
 * testada, e sem efeito nenhum no jogo.
 *
 * O dano entra pelo MESMO cano do dano de casa que já existia: mesmo
 * acumulador, mesma guarda de quem está fora do chão, mesmo evento em F5.
 */

namespace ProvaDoDanoDoFluido
{
	FBattleState CampoCom(EFluidKind Fluido, uint8 Terreno)
	{
		FBattleState Estado;
		Estado.Random.State = 7;
		Estado.ApplyDefaultTerrainRequirements();

		FPetState Ele;
		Ele.PetId = 1; Ele.Side = 0; Ele.Column = 1; Ele.Row = 1;
		Ele.Health = 90; Ele.MaxHealth = 90; Ele.Speed = 40;

		FPetState Outro;
		Outro.PetId = 2; Outro.Side = 1; Outro.Column = 2; Outro.Row = 2;
		Outro.Health = 90; Outro.MaxHealth = 90;

		Estado.Pets.Add(Ele);
		Estado.Pets.Add(Outro);

		Estado.CellLayout[Estado.CellIndex(1, 1)] = Terreno;
		Estado.SetFluidAt(1, 1, Fluido);
		return Estado;
	}

	/** Roda a fase que cobra o terreno, sem ninguém agir. */
	void PassarUmSlot(FBattleState& Estado)
	{
		const FBattleAction Nada;
		TArray<FBattleEvent> Traco;
		BattlePhases::ApplyMovement(Estado, Nada, Nada, 0, Traco);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFluidDamageLavaBurnsAndWaterDoesNotTest,
	"BattleSim.FluidDamage.LavaBurnsAndWaterDoesNot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFluidDamageLavaBurnsAndWaterDoesNotTest::RunTest(const FString& Parameters)
{
	// O ACEITE DA F4, nas duas direções.
	FBattleState NaLava = ProvaDoDanoDoFluido::CampoCom(
		EFluidKind::Lava, static_cast<uint8>(ECellProperty::Water));
	ProvaDoDanoDoFluido::PassarUmSlot(NaLava);

	TestEqual(TEXT("quem esta na lava se queima"),
		NaLava.Pets[0].PendingDamage,
		FluidRegistry::DamagePerSlot(EFluidKind::Lava));

	// E MOLHAR NÃO MACHUCA. Sem esta metade, um dano posto por engano em todos
	// os fluidos passaria — e a ilha inteira viraria armadilha.
	FBattleState NaAgua = ProvaDoDanoDoFluido::CampoCom(
		EFluidKind::AguaDoce, static_cast<uint8>(ECellProperty::Water));
	ProvaDoDanoDoFluido::PassarUmSlot(NaAgua);

	TestEqual(TEXT("quem esta na agua nao se machuca"),
		NaAgua.Pets[0].PendingDamage, 0);

	// E quem está LONGE do fluido não é alcançado: o pet do outro lado do
	// tabuleiro não pode se queimar na lava alheia.
	TestEqual(TEXT("quem esta fora nao se queima"),
		NaLava.Pets[1].PendingDamage, 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFluidDamageFlyingEscapesTheLavaTest,
	"BattleSim.FluidDamage.FlyingEscapesTheLava",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFluidDamageFlyingEscapesTheLavaTest::RunTest(const FString& Parameters)
{
	// A casa só alcança quem está PISANDO nela — e isso já era regra do dano
	// de casa (`Flying`: "imune a físico e ao dano de casa"). O fluido entra
	// pela mesma porta, então herda a regra em vez de inventar outra.
	FBattleState Voando = ProvaDoDanoDoFluido::CampoCom(
		EFluidKind::Lava, static_cast<uint8>(ECellProperty::Water));
	Voando.Pets[0].PostureFlags |= static_cast<uint16>(EBattlePostureFlags::Flying);

	ProvaDoDanoDoFluido::PassarUmSlot(Voando);
	TestEqual(TEXT("quem voa nao se queima na lava"),
		Voando.Pets[0].PendingDamage, 0);

	// CAMUFLAR NÃO SALVA: quem se esconde continua em pé no mesmo lugar. É a
	// distinção que separa os três esconderijos, e ela tinha de valer aqui
	// também — senão camuflar viraria imunidade a terreno de graça.
	FBattleState Camuflado = ProvaDoDanoDoFluido::CampoCom(
		EFluidKind::Lava, static_cast<uint8>(ECellProperty::Water));
	Camuflado.Pets[0].PostureFlags |=
		static_cast<uint16>(EBattlePostureFlags::Camouflaged);

	ProvaDoDanoDoFluido::PassarUmSlot(Camuflado);
	TestTrue(TEXT("quem se camufla continua se queimando"),
		Camuflado.Pets[0].PendingDamage > 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFluidDamageGoesThroughTheSamePipeTest,
	"BattleSim.FluidDamage.GoesThroughTheSamePipe",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFluidDamageGoesThroughTheSamePipeTest::RunTest(const FString& Parameters)
{
	// MESMO CANO, e é o que impede um segundo caminho de dano com as próprias
	// regras de morte e a própria narração.
	//
	// Uma casa que é BRASA e tem LAVA cobra as duas coisas, somadas no mesmo
	// acumulador — e não uma delas ganhando da outra.
	FBattleState Dobrado = ProvaDoDanoDoFluido::CampoCom(
		EFluidKind::Lava, static_cast<uint8>(ECellProperty::Damage));
	ProvaDoDanoDoFluido::PassarUmSlot(Dobrado);

	TestEqual(TEXT("brasa e lava somam no mesmo acumulador"),
		Dobrado.Pets[0].PendingDamage,
		BattleArenaConstants::CellDamageAmount
			+ FluidRegistry::DamagePerSlot(EFluidKind::Lava));

	// E NADA É APLICADO NA FASE DE MOVIMENTO (BTL-07): a vida só cai em F5,
	// depois de todo o dano do slot ter sido somado. Aplicar aqui faria um pet
	// morrer antes de poder revidar.
	TestEqual(TEXT("a vida ainda nao caiu na fase de movimento"),
		Dobrado.Pets[0].Health, 90);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFluidDamageKillsThroughTheNormalDeathPathTest,
	"BattleSim.FluidDamage.KillsThroughTheNormalDeathPath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFluidDamageKillsThroughTheNormalDeathPathTest::RunTest(const FString& Parameters)
{
	// A lava MATA, e mata pelo caminho normal — com `DanoAplicado` e
	// `PetMorreu` narrados por F5, como qualquer outra morte. Uma morte que
	// não emite evento é uma morte que a tela não conta.
	FBattleState Quase = ProvaDoDanoDoFluido::CampoCom(
		EFluidKind::Lava, static_cast<uint8>(ECellProperty::Water));
	Quase.Pets[0].Health = 1;

	ProvaDoDanoDoFluido::PassarUmSlot(Quase);

	TArray<FBattleEvent> Traco;
	BattlePhases::ApplyResolution(Quase, 0, Traco);

	TestFalse(TEXT("a lava matou"), Quase.Pets[0].IsAlive());

	bool bNarrouDano = false;
	bool bNarrouMorte = false;
	for (const FBattleEvent& Evento : Traco)
	{
		if (Evento.Type == EBattleEventType::DanoAplicado && Evento.ActorId == 1)
		{
			bNarrouDano = true;
		}
		if (Evento.Type == EBattleEventType::PetMorreu && Evento.ActorId == 1)
		{
			bNarrouMorte = true;
		}
	}

	TestTrue(TEXT("o dano da lava foi narrado"), bNarrouDano);
	TestTrue(TEXT("a morte na lava foi narrada"), bNarrouMorte);

	return true;
}

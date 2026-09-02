// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattlePhases.h"
#include "Battle/BattleState.h"
#include "Battle/BattleEvent.h"
#include "Battle/BattleTypes.h"
#include "Battle/FluidRegistry.h"
#include "Misc/AutomationTest.h"

// Namespace NOMEADO: o anônimo não isola em unity build, onde os arquivos de
// teste viram uma unidade de tradução só e dois helpers homônimos colidem
// (L-042).
namespace FogoEFocoTeste
{
	FBattleState MontarDuelo()
	{
		FBattleState State;

		FPetState Atacante;
		Atacante.PetId = 1; Atacante.Side = 0;
		Atacante.Column = 1; Atacante.Row = 1;
		Atacante.Health = 500; Atacante.MaxHealth = 500;
		Atacante.Attack = 100; Atacante.Defense = 10; Atacante.Speed = 5;
		Atacante.MovePowers[0] = 100;

		FPetState Alvo = Atacante;
		Alvo.PetId = 2; Alvo.Side = 1; Alvo.Column = 2;
		Alvo.MovePowers[0] = 0;

		State.Pets.Add(Atacante);
		State.Pets.Add(Alvo);
		return State;
	}

	uint8 CasaDoAlvo(const FBattleState& State)
	{
		for (const FPetState& Pet : State.Pets)
		{
			if (Pet.Side == 1)
			{
				return State.CellLayout[State.CellIndex(Pet.Column, Pet.Row)];
			}
		}
		return 0;
	}

	bool TemEvento(const TArray<FBattleEvent>& Trace, EBattleEventType Tipo)
	{
		for (const FBattleEvent& Evento : Trace)
		{
			if (Evento.Type == Tipo) { return true; }
		}
		return false;
	}
}

// ---------------------------------------------------------------------------
// P6 — A ÁGUA APAGA O FOGO
//
// Antes disto, o fogo apenas NÃO CONDUZIA: a ausência de uma regra, não uma
// regra. Brasa acesa dentro d'água é a casa contando duas coisas incompatíveis
// ao mesmo tempo, e o jogador que aprendeu "molhado conduz" não tem como
// adivinhar que molhado também deveria apagar.
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWaterQuenchesFireTest,
	"BattleSim.Moves.Fire.WaterQuenchesIt",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FWaterQuenchesFireTest::RunTest(const FString& Parameters)
{
	FBattleState State = FogoEFocoTeste::MontarDuelo();
	State.Pets[0].MoveTerrainEffects[0] = static_cast<uint8>(ECellProperty::Damage);

	// A casa do alvo é ÁGUA de verdade — fundura E substância, porque a regra
	// pergunta ao registro de fluidos, não à fundura.
	State.CellLayout[State.CellIndex(2, 1)] = static_cast<uint8>(ECellProperty::Water);

	TArray<FBattleEvent> Trace;
	FBattleAction Aguardar;
	BattlePhases::ApplyCombat(State, MakeMoveAction(EActionType::Atacar, 0), Aguardar, 0, Trace);

	TestEqual(TEXT("a casa continua agua — a brasa nao pegou"),
		static_cast<int32>(FogoEFocoTeste::CasaDoAlvo(State)),
		static_cast<int32>(ECellProperty::Water));

	// E O JOGADOR FICA SABENDO. Sem evento, ele vê o acerto e não vê a brasa,
	// e a única conclusão disponível é que o golpe falhou.
	TestTrue(TEXT("e o trace DIZ que a agua apagou"),
		FogoEFocoTeste::TemEvento(Trace, EBattleEventType::FogoApagou));

	// O golpe ainda ACERTOU: apagar o fogo não é errar o golpe.
	TestTrue(TEXT("o dano aconteceu assim mesmo"), State.Pets[1].PendingDamage > 0);

	return true;
}

// CONTRAPESO 1 — em terra seca a brasa PEGA. Sem este teste, um bug que
// impedisse todo depósito de fogo passaria pelo de cima cantando vitória.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFireStillBurnsOnDryGroundTest,
	"BattleSim.Moves.Fire.StillBurnsOnDryGround",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FFireStillBurnsOnDryGroundTest::RunTest(const FString& Parameters)
{
	FBattleState State = FogoEFocoTeste::MontarDuelo();
	State.Pets[0].MoveTerrainEffects[0] = static_cast<uint8>(ECellProperty::Damage);

	TArray<FBattleEvent> Trace;
	FBattleAction Aguardar;
	BattlePhases::ApplyCombat(State, MakeMoveAction(EActionType::Atacar, 0), Aguardar, 0, Trace);

	TestEqual(TEXT("no chao seco a casa vira brasa"),
		static_cast<int32>(FogoEFocoTeste::CasaDoAlvo(State)),
		static_cast<int32>(ECellProperty::Damage));
	TestTrue(TEXT("e nada foi apagado"),
		!FogoEFocoTeste::TemEvento(Trace, EBattleEventType::FogoApagou));

	return true;
}

// CONTRAPESO 2 — A LAVA NÃO APAGA, e é por isso que a pergunta é ao registro
// de fluidos e não à fundura: a lava é funda como a água e faz o oposto dela.
// Perguntar "esta casa é água funda?" acertaria os dois testes acima e erraria
// este — e erraria exatamente na casa mais perigosa do campo.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLavaDoesNotQuenchFireTest,
	"BattleSim.Moves.Fire.LavaDoesNotQuenchIt",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLavaDoesNotQuenchFireTest::RunTest(const FString& Parameters)
{
	FBattleState State = FogoEFocoTeste::MontarDuelo();
	State.Pets[0].MoveTerrainEffects[0] = static_cast<uint8>(ECellProperty::Damage);

	State.CellLayout[State.CellIndex(2, 1)] = static_cast<uint8>(ECellProperty::Water);
	State.SetFluidAt(2, 1, EFluidKind::Lava);

	TArray<FBattleEvent> Trace;
	FBattleAction Aguardar;
	BattlePhases::ApplyCombat(State, MakeMoveAction(EActionType::Atacar, 0), Aguardar, 0, Trace);

	TestTrue(TEXT("lava nao apaga fogo"),
		!FogoEFocoTeste::TemEvento(Trace, EBattleEventType::FogoApagou));

	return true;
}

// CONTRAPESO 3 — só o FOGO é apagado. Um golpe de gelo na água continua
// congelando; a água apaga a brasa, não todo depósito que existe.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWaterOnlyQuenchesFireTest,
	"BattleSim.Moves.Fire.WaterOnlyQuenchesFire",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FWaterOnlyQuenchesFireTest::RunTest(const FString& Parameters)
{
	FBattleState State = FogoEFocoTeste::MontarDuelo();
	State.Pets[0].MoveTerrainEffects[0] = static_cast<uint8>(ECellProperty::Ice);
	State.CellLayout[State.CellIndex(2, 1)] = static_cast<uint8>(ECellProperty::Water);

	TArray<FBattleEvent> Trace;
	FBattleAction Aguardar;
	BattlePhases::ApplyCombat(State, MakeMoveAction(EActionType::Atacar, 0), Aguardar, 0, Trace);

	TestEqual(TEXT("o gelo congela a agua normalmente"),
		static_cast<int32>(FogoEFocoTeste::CasaDoAlvo(State)),
		static_cast<int32>(ECellProperty::Ice));

	return true;
}

// ---------------------------------------------------------------------------
// P5 — GOLPE QUE SE DESLIGA SOB DANO
//
// A checagem lê `PendingDamage`, e não a vida: dentro do turno a vida ainda
// não caiu (BTL-07 aplica tudo de uma vez em F5), e perguntar a ela
// responderia sempre "não" — a regra nasceria morta, verde em todo teste.
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDamageBreaksFocusTest,
	"BattleSim.Moves.Focus.DamageBreaksIt",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDamageBreaksFocusTest::RunTest(const FString& Parameters)
{
	FBattleState State = FogoEFocoTeste::MontarDuelo();
	State.Pets[0].MoveNeedsFocus[0] = 1;
	State.Pets[0].MoveTerrainEffects[0] = static_cast<uint8>(ECellProperty::Damage);
	State.Pets[0].PendingDamage = 25;

	TArray<FBattleEvent> Trace;
	FBattleAction Aguardar;
	BattlePhases::ApplyCombat(State, MakeMoveAction(EActionType::Atacar, 0), Aguardar, 0, Trace);

	TestTrue(TEXT("o trace diz que a concentracao quebrou"),
		FogoEFocoTeste::TemEvento(Trace, EBattleEventType::ConcentracaoQuebrada));

	// NADA do golpe acontece — nem dano, nem terreno. Golpe pela metade seria
	// a regra punindo e recompensando no mesmo slot.
	TestEqual(TEXT("nenhum dano saiu"), State.Pets[1].PendingDamage, 0);
	TestEqual(TEXT("e a casa do alvo nao mudou"),
		static_cast<int32>(FogoEFocoTeste::CasaDoAlvo(State)),
		static_cast<int32>(ECellProperty::None));

	return true;
}

// CONTRAPESO 1 — INTEIRO, o golpe sai. Sem ele, um bug que desligasse o golpe
// de foco sempre passaria no teste de cima.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUnharmedPetKeepsFocusTest,
	"BattleSim.Moves.Focus.UnharmedPetKeepsIt",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FUnharmedPetKeepsFocusTest::RunTest(const FString& Parameters)
{
	FBattleState State = FogoEFocoTeste::MontarDuelo();
	State.Pets[0].MoveNeedsFocus[0] = 1;

	TArray<FBattleEvent> Trace;
	FBattleAction Aguardar;
	BattlePhases::ApplyCombat(State, MakeMoveAction(EActionType::Atacar, 0), Aguardar, 0, Trace);

	TestTrue(TEXT("sem dano, o golpe de foco sai inteiro"),
		State.Pets[1].PendingDamage > 0);
	TestTrue(TEXT("e nada quebrou"),
		!FogoEFocoTeste::TemEvento(Trace, EBattleEventType::ConcentracaoQuebrada));

	return true;
}

// CONTRAPESO 2 — GOLPE COMUM APANHANDO CONTINUA SAINDO. Este é o que protege
// todo pet já cadastrado: a regra nova não pode desligar retroativamente um
// golpe que ninguém marcou, e o dono só descobriria isso perdendo.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOrdinaryMoveIgnoresDamageTest,
	"BattleSim.Moves.Focus.OrdinaryMoveIgnoresDamage",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FOrdinaryMoveIgnoresDamageTest::RunTest(const FString& Parameters)
{
	FBattleState State = FogoEFocoTeste::MontarDuelo();
	State.Pets[0].PendingDamage = 999;

	TArray<FBattleEvent> Trace;
	FBattleAction Aguardar;
	BattlePhases::ApplyCombat(State, MakeMoveAction(EActionType::Atacar, 0), Aguardar, 0, Trace);

	TestTrue(TEXT("golpe sem foco sai mesmo apanhando muito"),
		State.Pets[1].PendingDamage > 0);
	TestTrue(TEXT("e nada quebrou"),
		!FogoEFocoTeste::TemEvento(Trace, EBattleEventType::ConcentracaoQuebrada));

	return true;
}

// O FOCO ENTRA NO HASH. Dois estados que jogam diferente e têm o mesmo hash
// são uma dessincronia que só aparece na partida em rede, e depois de a
// jogada já ter divergido.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFocusChangesTheHashTest,
	"BattleSim.Moves.Focus.ChangesTheHash",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FFocusChangesTheHashTest::RunTest(const FString& Parameters)
{
	FBattleState Sem = FogoEFocoTeste::MontarDuelo();
	FBattleState Com = FogoEFocoTeste::MontarDuelo();
	Com.Pets[0].MoveNeedsFocus[0] = 1;

	TestNotEqual(TEXT("exigir foco muda o hash"),
		Sem.ComputeHash(), Com.ComputeHash());

	return true;
}

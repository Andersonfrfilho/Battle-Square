// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattlePhases.h"
#include "Battle/BattleState.h"
#include "Battle/BattleEvent.h"
#include "Battle/BattleTypes.h"
#include "Battle/BattleArenaConstants.h"
#include "Misc/AutomationTest.h"

// Namespace NOMEADO (L-042).
namespace CampoInteiroTeste
{
	FBattleState MontarArena(uint8 Colunas, uint8 Linhas, int32 Poder)
	{
		FBattleState State;
		State.GridColumns = Colunas;
		State.GridRows = Linhas;
		State.CellLayout.SetNumZeroed(Colunas * Linhas);

		FPetState Atacante;
		Atacante.PetId = 1; Atacante.Side = 0;
		// AO LADO do alvo. A primeira versão os pôs nas duas pontas, e o golpe
		// simplesmente não alcançava: os cinco testes reprovaram código certo,
		// e a mensagem ("0 casas viraram água") não dava nenhuma pista de que
		// a causa era alcance. Medir antes de culpar o código.
		Atacante.Column = Colunas - 2; Atacante.Row = Linhas / 2;
		Atacante.Health = 500; Atacante.MaxHealth = 500;
		Atacante.Attack = 100; Atacante.Defense = 10; Atacante.Speed = 5;
		Atacante.MovePowers[0] = Poder;
		Atacante.MoveTerrainEffects[0] = static_cast<uint8>(ECellProperty::Water);

		FPetState Alvo = Atacante;
		Alvo.PetId = 2; Alvo.Side = 1; Alvo.Column = Colunas - 1;
		Alvo.MovePowers[0] = 0;
		Alvo.MoveTerrainEffects[0] = 0;

		State.Pets.Add(Atacante);
		State.Pets.Add(Alvo);
		return State;
	}

	int32 ContarCasas(const FBattleState& State, ECellProperty Qual)
	{
		int32 Quantas = 0;
		for (const uint8 Casa : State.CellLayout)
		{
			if (Casa == static_cast<uint8>(Qual)) { ++Quantas; }
		}
		return Quantas;
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
// P4 — EFEITO DE CAMPO INTEIRO
//
// > "dependendo do tamanho do campo, alguns ataques como eletricidade
// >  conseguiriam eletrizar todo o campo ou congelar"
//
// O limiar compara PODER com TAMANHO. Uma bandeira no cadastro ("este golpe é
// de área") não teria essa propriedade: bastaria marcá-la num golpe fraco.
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FStrongMoveTakesTheWholeFieldTest,
	"BattleSim.Moves.Field.StrongMoveTakesIt",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FStrongMoveTakesTheWholeFieldTest::RunTest(const FString& Parameters)
{
	// Poder EXATAMENTE no limiar da 3x3, calculado pela mesma conta que a
	// regra usa. Escrever "180" à mão faria o teste guardar uma segunda cópia
	// do limiar, e as duas concordariam até alguém mexer na constante.
	FBattleState State = CampoInteiroTeste::MontarArena(3, 3, 0);
	State.Pets[0].MovePowers[0] = State.PowerToTakeTheField();

	TArray<FBattleEvent> Trace;
	FBattleAction Aguardar;
	BattlePhases::ApplyCombat(State, MakeMoveAction(EActionType::Atacar, 0), Aguardar, 0, Trace);

	TestEqual(TEXT("as nove casas viraram agua"),
		CampoInteiroTeste::ContarCasas(State, ECellProperty::Water), 9);
	TestTrue(TEXT("e o trace anuncia o campo, nao nove mudancas de casa"),
		CampoInteiroTeste::TemEvento(Trace, EBattleEventType::CampoInteiroMudou));
	TestTrue(TEXT("sem nove linhas de TerrenoMudou enterrando o feed"),
		!CampoInteiroTeste::TemEvento(Trace, EBattleEventType::TerrenoMudou));

	return true;
}

// CONTRAPESO 1 — ABAIXO DO LIMIAR, uma casa só. Sem este teste, um bug que
// fizesse TODO golpe tomar o campo passaria no de cima cantando vitória.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWeakMoveTakesOneCellTest,
	"BattleSim.Moves.Field.WeakMoveTakesOneCell",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FWeakMoveTakesOneCellTest::RunTest(const FString& Parameters)
{
	FBattleState State = CampoInteiroTeste::MontarArena(3, 3, 0);
	State.Pets[0].MovePowers[0] = State.PowerToTakeTheField() - 1;

	TArray<FBattleEvent> Trace;
	FBattleAction Aguardar;
	BattlePhases::ApplyCombat(State, MakeMoveAction(EActionType::Atacar, 0), Aguardar, 0, Trace);

	TestEqual(TEXT("um poder abaixo do limiar alaga UMA casa"),
		CampoInteiroTeste::ContarCasas(State, ECellProperty::Water), 1);
	TestTrue(TEXT("e nao anuncia campo nenhum"),
		!CampoInteiroTeste::TemEvento(Trace, EBattleEventType::CampoInteiroMudou));

	return true;
}

// ESTE É O ACEITE DA TASK: é o TAMANHO que decide.
//
// O MESMO golpe, com o MESMO poder, toma a arena pequena e não toma a grande.
// Sem esta comparação, "campo inteiro" seria uma propriedade do golpe — e a
// frase do pedido ("dependendo do tamanho do campo") não teria sido atendida.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFieldSizeDecidesTest,
	"BattleSim.Moves.Field.SizeDecides",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FFieldSizeDecidesTest::RunTest(const FString& Parameters)
{
	FBattleState Pequena = CampoInteiroTeste::MontarArena(3, 3, 0);
	const int32 Poder = Pequena.PowerToTakeTheField();
	Pequena.Pets[0].MovePowers[0] = Poder;

	FBattleState Grande = CampoInteiroTeste::MontarArena(5, 5, Poder);

	TArray<FBattleEvent> TracePequena;
	TArray<FBattleEvent> TraceGrande;
	FBattleAction Aguardar;
	BattlePhases::ApplyCombat(Pequena, MakeMoveAction(EActionType::Atacar, 0), Aguardar, 0, TracePequena);
	BattlePhases::ApplyCombat(Grande, MakeMoveAction(EActionType::Atacar, 0), Aguardar, 0, TraceGrande);

	AddInfo(FString::Printf(TEXT("poder %d — 3x3 exige %d, 5x5 exige %d"),
		Poder, Pequena.PowerToTakeTheField(), Grande.PowerToTakeTheField()));

	TestTrue(TEXT("o mesmo golpe TOMA a arena pequena"),
		CampoInteiroTeste::TemEvento(TracePequena, EBattleEventType::CampoInteiroMudou));
	TestTrue(TEXT("e NAO toma a grande"),
		!CampoInteiroTeste::TemEvento(TraceGrande, EBattleEventType::CampoInteiroMudou));

	return true;
}

// CONTRAPESO 2 — a casa BLOQUEADA continua de pé. O golpe de campo passa pelo
// mesmo bloco de depósito justamente para herdar isto: derrubar obstáculo é
// decisão do movimento, onde o pet gasta o slot e todo mundo pode reagir.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWholeFieldSparesObstaclesTest,
	"BattleSim.Moves.Field.SparesObstacles",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FWholeFieldSparesObstaclesTest::RunTest(const FString& Parameters)
{
	FBattleState State = CampoInteiroTeste::MontarArena(3, 3, 0);
	State.Pets[0].MovePowers[0] = State.PowerToTakeTheField();
	State.CellLayout[State.CellIndex(1, 0)] = static_cast<uint8>(ECellProperty::Blocked);

	TArray<FBattleEvent> Trace;
	FBattleAction Aguardar;
	BattlePhases::ApplyCombat(State, MakeMoveAction(EActionType::Atacar, 0), Aguardar, 0, Trace);

	TestEqual(TEXT("o obstaculo continua de pe"),
		static_cast<int32>(State.CellLayout[State.CellIndex(1, 0)]),
		static_cast<int32>(ECellProperty::Blocked));
	TestEqual(TEXT("e as outras oito viraram agua"),
		CampoInteiroTeste::ContarCasas(State, ECellProperty::Water), 8);

	return true;
}

// CONTRAPESO 3 — a ÁGUA APAGA O FOGO TAMBÉM AQUI. Um golpe de campo inteiro
// que acendesse brasa dentro d'água desfaria a regra da P6 só por ser grande.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWholeFieldFireStillQuenchedTest,
	"BattleSim.Moves.Field.FireStillQuenchedByWater",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FWholeFieldFireStillQuenchedTest::RunTest(const FString& Parameters)
{
	FBattleState State = CampoInteiroTeste::MontarArena(3, 3, 0);
	State.Pets[0].MovePowers[0] = State.PowerToTakeTheField();
	State.Pets[0].MoveTerrainEffects[0] = static_cast<uint8>(ECellProperty::Damage);

	// Uma casa molhada no meio do campo seco.
	State.CellLayout[State.CellIndex(1, 1)] = static_cast<uint8>(ECellProperty::Water);

	TArray<FBattleEvent> Trace;
	FBattleAction Aguardar;
	BattlePhases::ApplyCombat(State, MakeMoveAction(EActionType::Atacar, 0), Aguardar, 0, Trace);

	TestEqual(TEXT("a casa molhada continua agua — a brasa nao pegou nela"),
		static_cast<int32>(State.CellLayout[State.CellIndex(1, 1)]),
		static_cast<int32>(ECellProperty::Water));
	TestEqual(TEXT("e as outras oito pegaram fogo"),
		CampoInteiroTeste::ContarCasas(State, ECellProperty::Damage), 8);

	return true;
}

// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattlePhases.h"
#include "Battle/BattleState.h"
#include "Battle/BattleEvent.h"
#include "Misc/AutomationTest.h"

namespace TerraTeste
{
	FBattleState Duelo()
	{
		FBattleState Estado;
		Estado.Random.State = 11;

		FPetState Cavador;
		Cavador.PetId = 1; Cavador.Side = 0; Cavador.Column = 0; Cavador.Row = 1;
		Cavador.Health = 90; Cavador.MaxHealth = 90; Cavador.Attack = 30;

		FPetState Outro;
		Outro.PetId = 2; Outro.Side = 1; Outro.Column = 2; Outro.Row = 1;
		Outro.Health = 90; Outro.MaxHealth = 90; Outro.Defense = 10;

		Estado.Pets.Add(Cavador);
		Estado.Pets.Add(Outro);
		return Estado;
	}

	FBattleAction Cavar(EBattleDirection Direcao)
	{
		FBattleAction Acao;
		Acao.Type = EActionType::Escavar;
		Acao.Direction = Direcao;
		return Acao;
	}

	int32 Conta(const TArray<FBattleEvent>& Traco, EBattleEventType Tipo)
	{
		int32 Total = 0;
		for (const FBattleEvent& Evento : Traco)
		{
			if (Evento.Type == Tipo) { ++Total; }
		}
		return Total;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEscavarErgueBarreiraTest,
	"BattleSim.Moves.Terra.EscavarErgueBarreira",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEscavarErgueBarreiraTest::RunTest(const FString&)
{
	// NADA no jogo cria obstáculo — eles só são derrubados. Esta é a única
	// ação que muda o tabuleiro a FAVOR de quem a usa, e é o que dá identidade
	// ao elemento Terra, que era o único sem skill nenhuma.
	FBattleState Estado = TerraTeste::Duelo();
	FBattleAction Nada;
	TArray<FBattleEvent> Traco;

	BattlePhases::ApplyPostures(Estado,
		BattlePhases::DuelSlotActions(Estado, TerraTeste::Cavar(EBattleDirection::Direita), Nada), 0, Traco);

	const int32 Frente = Estado.CellIndex(1, 1);
	TestEqual(TEXT("A casa à frente virou barreira"), Estado.CellLayout[Frente],
		static_cast<uint8>(ECellProperty::Blocked));
	TestEqual(TEXT("E foi anunciado"),
		TerraTeste::Conta(Traco, EBattleEventType::TerrenoMudou), 1);

	// Quem cavou NÃO fica marcado: erguer terra muda o tabuleiro, não a
	// postura. Vestir bandeira aqui daria ao cavador uma proteção que ninguém
	// pediu, e que o teste da postura de outro pet herdaria sem querer.
	TestEqual(TEXT("Quem cavou não muda de postura"),
		Estado.Pets[0].PostureFlags, static_cast<uint16>(0));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEscavarNaoEnterraNinguemTest,
	"BattleSim.Moves.Terra.EscavarNaoEnterraNinguem",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEscavarNaoEnterraNinguemTest::RunTest(const FString&)
{
	// Levantar terra em cima de alguém seria enterrá-lo — e enterrar é outra
	// jogada, com outro nome, outro custo e outro evento. Sem esta recusa,
	// escavar viraria um golpe que remove o oponente do tabuleiro de graça.
	FBattleState Estado = TerraTeste::Duelo();
	Estado.Pets[1].Column = 1; Estado.Pets[1].Row = 1;

	FBattleAction Nada;
	TArray<FBattleEvent> Traco;
	BattlePhases::ApplyPostures(Estado,
		BattlePhases::DuelSlotActions(Estado, TerraTeste::Cavar(EBattleDirection::Direita), Nada), 0, Traco);

	TestEqual(TEXT("A casa ocupada não vira barreira"),
		Estado.CellLayout[Estado.CellIndex(1, 1)],
		static_cast<uint8>(ECellProperty::None));
	TestEqual(TEXT("E a recusa é ALTA, com evento"),
		TerraTeste::Conta(Traco, EBattleEventType::PosturaFalhou), 1);

	// Fora da grade também recusa — e sem isto a conta de índice sairia da
	// lista, que é falha bem pior que a jogada não acontecer.
	FBattleState NaBorda = TerraTeste::Duelo();
	TArray<FBattleEvent> Outro;
	BattlePhases::ApplyPostures(NaBorda,
		BattlePhases::DuelSlotActions(NaBorda, TerraTeste::Cavar(EBattleDirection::Esquerda), Nada), 0, Outro);
	TestEqual(TEXT("Fora da grade recusa"),
		TerraTeste::Conta(Outro, EBattleEventType::PosturaFalhou), 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSecarApagaAAguaTest,
	"BattleSim.Moves.Secar.ApagaAAgua",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSecarApagaAAguaTest::RunTest(const FString&)
{
	// Faltava a RESPOSTA: o terreno só ficava mais molhado ou congelava, e
	// nada o secava de propósito. Quem alagou o campo tinha vantagem sem
	// contra-jogo, e a cadeia natural leva slots demais para servir de réplica.
	for (const ECellProperty Molhado : { ECellProperty::Water,
		ECellProperty::ShallowWater, ECellProperty::Mud, ECellProperty::Ice })
	{
		FBattleState Estado = TerraTeste::Duelo();
		Estado.Pets[1].Column = 1; Estado.Pets[1].Row = 1;
		Estado.Pets[0].MovePowers[0] = 40;
		Estado.Pets[0].MoveTerrainEffects[0] = static_cast<uint8>(ECellProperty::Dries);

		const int32 Casa = Estado.CellIndex(1, 1);
		Estado.SetTemporaryTerrain(1, 1, static_cast<uint8>(Molhado), /*Slots=*/3);

		FBattleAction Golpe = MakeMoveAction(EActionType::Atacar, 0);
		Golpe.Direction = EBattleDirection::Direita;

		FBattleAction Nada;
		TArray<FBattleEvent> Traco;
		BattlePhases::ApplyCombat(Estado,
		BattlePhases::DuelSlotActions(Estado, Golpe, Nada), 0, Traco);

		TestEqual(TEXT("A casa secou"), Estado.CellLayout[Casa],
			static_cast<uint8>(ECellProperty::None));

		// E o PRAZO some junto: um cronômetro sobrevivente devolveria a água
		// dois slots depois, e o jogador veria a poça ressuscitar sozinha.
		TestEqual(TEXT("E o prazo do terreno some junto"),
			static_cast<int32>(Estado.CellCountdown[Casa]), 0);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSecarNaoEOMesmoQueNadaTest,
	"BattleSim.Moves.Secar.NaoEOMesmoQueNada",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSecarNaoEOMesmoQueNadaTest::RunTest(const FString&)
{
	// `None` no golpe significa "não mexe na casa", e é o valor de todo golpe
	// comum. Se secar fosse `None`, ela não existiria — e se `None` secasse,
	// TODO golpe do jogo passaria a limpar o terreno ao acertar.
	FBattleState Estado = TerraTeste::Duelo();
	Estado.Pets[1].Column = 1; Estado.Pets[1].Row = 1;
	Estado.Pets[0].MovePowers[0] = 40;
	Estado.Pets[0].MoveTerrainEffects[0] = static_cast<uint8>(ECellProperty::None);

	const int32 Casa = Estado.CellIndex(1, 1);
	Estado.CellLayout[Casa] = static_cast<uint8>(ECellProperty::Water);

	FBattleAction Golpe = MakeMoveAction(EActionType::Atacar, 0);
	Golpe.Direction = EBattleDirection::Direita;

	FBattleAction Nada;
	TArray<FBattleEvent> Traco;
	BattlePhases::ApplyCombat(Estado,
		BattlePhases::DuelSlotActions(Estado, Golpe, Nada), 0, Traco);

	TestEqual(TEXT("Golpe comum NÃO seca a casa"), Estado.CellLayout[Casa],
		static_cast<uint8>(ECellProperty::Water));

	return true;
}

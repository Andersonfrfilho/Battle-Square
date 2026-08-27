// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattleArena.h"
#include "Misc/AutomationTest.h"
#include "World/WorldEncounterFlow.h"

// A batalha do mundo precisa AVISAR que começou.
//
// Sem este aviso ela abria sem botões de ação: o jogador caminhava, encontrava
// um oponente, a câmera ia para a arena — e ele ficava olhando uma luta que não
// podia jogar. Pior que não ter a transição, porque parece defeito do jogo e
// não falta de recurso.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWorldEncounterFlowAnnouncesBattleStartTest,
	"BattleSquare.World.EncounterFlow.AnnouncesBattleStart",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldEncounterFlowAnnouncesBattleStartTest::RunTest(const FString& Parameters)
{
	UWorldEncounterFlow* Flow = NewObject<UWorldEncounterFlow>();

	int32 Avisos = 0;
	ABattleArena* Recebida = nullptr;
	Flow->OnWorldBattleStarted.AddLambda([&Avisos, &Recebida](ABattleArena* Arena)
	{
		++Avisos;
		Recebida = Arena;
	});

	// Encontro inválido não pode virar aviso: montar a tela para uma batalha
	// que não começou deixaria botões órfãos por cima do mundo.
	Flow->HandleEncounterTriggered(nullptr);

	TestEqual(TEXT("Encontro inválido não anuncia batalha"), Avisos, 0);
	TestNull(TEXT("E não entrega arena nenhuma"), Recebida);

	return true;
}

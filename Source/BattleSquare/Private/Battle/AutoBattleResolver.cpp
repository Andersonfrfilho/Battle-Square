// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/AutoBattleResolver.h"

#include "Battle/BattleOutcome.h"
#include "Battle/BattleRandom.h"
#include "Battle/BattleResolver.h"
#include "Battle/DumbOpponentAI.h"

namespace
{
	/**
	 * O teto de turnos. Dois bots aleatórios podem dançar sem se matar; o
	 * teto transforma a dança em EMPATE declarado — e empate na defesa
	 * mantém o posto, que é o desenho conservador: ninguém perde título para
	 * um relógio.
	 */
	constexpr int32 TetoDeTurnos = 60;
}

uint8 AutoBattleResolver::ResolveBotVsBot(FBattleState State, uint64 BotSeed)
{
	// O acaso da IA é PRÓPRIO e semeado por fora: o do estado pertence à
	// resolução (dano, esquiva), e usar o mesmo para as escolhas faria a
	// escolha do bot mexer no sorteio do dano — dois jogos no mesmo dado.
	FBattleRandom BotRandom;
	BotRandom.State = BotSeed != 0 ? BotSeed : 1;

	for (int32 Turno = 0; Turno < TetoDeTurnos; ++Turno)
	{
		const FTurnCommit Esquerda =
			FDumbOpponentAI::GenerateRandomValidCommit(State, /*Side=*/0, BotRandom);
		const FTurnCommit Direita =
			FDumbOpponentAI::GenerateRandomValidCommit(State, /*Side=*/1, BotRandom);

		FBattleResolveResult Resultado = FBattleResolver::ResolveTurn(
			State, FBattleResolver::DuelCommits(State, Esquerda, Direita));

		// O DESFECHO É DE QUEM CHAMA, por design do núcleo — e a armadilha
		// está escrita na arena: "nem aqui nem o coordinator chamavam isto,
		// então BatalhaEncerrada nunca disparava em produção". Este resolvedor
		// caiu NO MESMO buraco na primeira versão: a sonda mostrou vida em
		// -11 com a batalha seguindo, e todo duelo morria no teto como
		// empate. Avaliar depois de CADA turno é o contrato.
		BattleOutcome::EvaluateOutcome(Resultado.NextState, Resultado.Trace);

		for (const FBattleEvent& Evento : Resultado.Trace)
		{
			if (Evento.Type == EBattleEventType::BatalhaEncerrada)
			{
				// O MESMO campo que a tela lê (`Value` = lado vencedor,
				// 0xFF empate) — uma segunda leitura divergiria na primeira
				// edição, como toda segunda leitura.
				return static_cast<uint8>(Evento.Value);
			}
		}

		State = Resultado.NextState;
	}

	return 0xFF;
}

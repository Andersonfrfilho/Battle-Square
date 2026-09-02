// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattleResolver.h"
#include "Battle/BattlePhases.h"

namespace
{
	void EmitTurnBoundary(TArray<FBattleEvent>& OutTrace, EBattleEventType Type, uint8 SlotIndex, uint8 Phase)
	{
		FBattleEvent Event;
		Event.Type = Type;
		Event.SlotIndex = SlotIndex;
		Event.Phase = Phase;
		Event.ActorId = BattleEventNoActor;
		Event.TargetId = BattleEventNoActor;
		OutTrace.Add(Event);
	}
}

// PRECEDÊNCIA — a nota antiga estava certa PARA V1, e a CP4 a tornou obsoleta.
//
// Ela dizia que as quatro fases resolvem os dois lados simetricamente (postura
// não compete, movimento coleta todas as intenções antes de aplicar qualquer
// uma, combate acumula dano em vez de aplicar em sequência) e que, com UM pet
// por lado, não existia ponto de decisão onde a ordem mudasse o resultado.
// Isso continua verdade para um pet por lado.
//
// COM DOIS ALIADOS, EXISTE — e foi MEDIDO em 02/09/2026, não deduzido: a lama é
// a única casa cujo efeito é sorteado, o sorteio vem de um gerador COM ESTADO,
// e quem pede um número primeiro recebe outro número. Os mesmos commits em
// ordem invertida davam hash 1440089706 contra 1488336816.
//
// Em rede, os dois clientes recebem os commits na ordem em que cada jogador
// confirmou — que não é a mesma. A partida divergiria sem nada acusar, e o
// culpado aparente seria a latência ou o próprio sorteio.
//
// A ORDENAÇÃO POR `PetId` em `ResolveTurn` é o conserto, e ela não é um
// critério novo: é o MESMO que `ComputeHash` usa para ordenar pets e que a
// escolha de alvo no combate usa para desempatar. Desempate por VELOCIDADE
// seria um segundo critério, que precisaria concordar com esses dois para
// sempre — e continua sem ponto de decisão que o justifique.
//
// Três testes de ordem passaram ANTES deste defeito aparecer, e nenhum o
// tocava: dois invertiam `State.Pets` (que a coleta de intenções nem itera) e
// o terceiro invertia os commits mas sem lama, então ninguém sorteava nada.
// Ver `BattleSim.Resolver.Ordering` e L-047 em STATE.md.
TArray<FTurnCommit> FBattleResolver::DuelCommits(
	const FBattleState& InState,
	const FTurnCommit& LeftCommit,
	const FTurnCommit& RightCommit)
{
	TArray<FTurnCommit> Commits;

	// O pet VIVO de cada lado, que é a quem o commit daquele lado pertencia
	// no contrato de v1. Lado sem pet vivo não gera commit: um commit
	// endereçado a ninguém seria uma ação que o resolvedor teria de descartar
	// mais tarde, e descartar em silêncio é como uma jogada some.
	for (uint8 Lado = 0; Lado < 2; ++Lado)
	{
		const FPetState* Pet = InState.FindAlivePetOnSideConst(Lado);
		if (!Pet)
		{
			continue;
		}

		FTurnCommit Commit = (Lado == 0) ? LeftCommit : RightCommit;
		Commit.PetId = Pet->PetId;
		Commits.Add(Commit);
	}

	return Commits;
}

FBattleResolveResult FBattleResolver::ResolveTurn(
	const FBattleState& InState,
	TArrayView<const FTurnCommit> Commits)
{
	// Pureza: NextState é uma CÓPIA de InState. InState nunca é mutado —
	// é o que torna ResolveTurn chamável repetidamente com o mesmo
	// argumento e sempre produzir o mesmo resultado (BTL-16).
	FBattleResolveResult Result;
	Result.NextState = InState;

	FBattleState& State = Result.NextState;
	TArray<FBattleEvent>& Trace = Result.Trace;

	EmitTurnBoundary(Trace, EBattleEventType::TurnoIniciado, 0, /*Phase=*/1);

	for (uint8 SlotIndex = 0; SlotIndex < FTurnCommit::ActionsPerTurn; ++SlotIndex)
	{
		// Pet morto: as fases de T5 a T8 já ignoram pets sem IsAlive() em
		// cada busca (FindAlivePetOnSide, CollectIntent, FindLivingOpponentAtCell)
		// — "ações restantes descartadas" é garantido por construção,
		// sem código extra necessário aqui.
		// A AÇÃO DE CADA LADO, achada pelo DONO do commit.
		//
		// As fases ainda recebem "a ação da esquerda e a da direita" — quem
		// muda isso é a CP4. O que a CP3 troca é o ENDEREÇAMENTO: a ação
		// chega aqui porque alguém a endereçou àquele pet, e não porque ela
		// estava na variável chamada `LeftCommit`.
		//
		// Commit sem dono reconhecível é IGNORADO. Inventar um dono faria uma
		// ação chegar a quem ninguém escolheu, e o jogador veria o pet dele
		// fazer algo que ele não pediu.
		// AS AÇÕES DESTE SLOT, uma por pet endereçado.
		//
		// Montada aqui, e não dentro de cada fase: as três fases precisam da
		// MESMA lista, e montá-la três vezes daria três chances de as três
		// discordarem sobre quem age neste slot.
		//
		// Commit sem dono reconhecível não entra. Inventar um dono faria uma
		// ação chegar a quem ninguém escolheu.
		TArray<FSlotAction> AcoesDoSlot;
		AcoesDoSlot.Reserve(Commits.Num());

		for (const FTurnCommit& Commit : Commits)
		{
			if (!State.FindPetByIdConst(Commit.PetId))
			{
				continue;
			}

			FSlotAction Acao;
			Acao.PetId = Commit.PetId;
			Acao.Action = Commit.Actions[SlotIndex];
			AcoesDoSlot.Add(Acao);
		}

		// ORDENADAS POR `PetId`, e isto NÃO é zelo: é conserto de um defeito
		// medido.
		//
		// A lama sorteia, e o sorteio vem de um gerador COM ESTADO — quem pede
		// um número primeiro recebe um número diferente de quem pede depois.
		// Sem esta ordenação, dois aliados na lama escorregam conforme a ordem
		// em que os commits chegaram, e essa ordem não é a mesma nos dois
		// clientes: medido em 02/09/2026, os mesmos commits invertidos deram
		// hash 1440089706 contra 1488336816.
		//
		// O sintoma seria uma partida em rede que diverge sem nada acusar, e o
		// culpado aparente seria a latência ou o próprio sorteio.
		//
		// `PetId`, e não velocidade: é o mesmo desempate que `ComputeHash` já
		// usa e que a escolha de alvo no combate já usa. Um segundo critério
		// de precedência precisaria concordar com esses dois para sempre.
		AcoesDoSlot.Sort([](const FSlotAction& A, const FSlotAction& B)
		{
			return A.PetId < B.PetId;
		});

		// F1 — Declaração. Nada muda; apenas marca o início do slot.
		EmitTurnBoundary(Trace, EBattleEventType::SlotIniciado, SlotIndex, /*Phase=*/1);

		// F2 → F3 → F4 → F5, sempre nesta ordem (design.md).
		BattlePhases::ApplyPostures(State, AcoesDoSlot, SlotIndex, Trace);
		BattlePhases::ApplyMovement(State, AcoesDoSlot, SlotIndex, Trace);
		BattlePhases::ApplyCombat(State, AcoesDoSlot, SlotIndex, Trace);
		BattlePhases::ApplyResolution(State, SlotIndex, Trace);
	}

	State.TurnNumber += 1;

	EmitTurnBoundary(Trace, EBattleEventType::TurnoEncerrado, FTurnCommit::ActionsPerTurn - 1, /*Phase=*/5);

	return Result;
}

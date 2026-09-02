// Copyright 2026 Anderson. All Rights Reserved.

#include "Net/BattleNetTypes.h"

namespace
{
	// Maior valor válido de cada enum — ver BattleTypes.h (BattleSim).
	// EActionType: Aguardar=0 .. Esquivar=5. EBattleDirection: Nenhuma=0
	// .. BaixoDireita=8. Duplicado aqui de propósito: esta é a fronteira
	// de validação de rede, e ela precisa continuar correta mesmo que o
	// enum ganhe valores novos sem ninguém lembrar de atualizar este
	// arquivo — por isso o teste (T2) cobre o valor máximo válido E o
	// primeiro inválido, não só um caso qualquer.
	constexpr uint8 MaxValidActionType = static_cast<uint8>(EActionType::Esquivar);
	constexpr uint8 MaxValidDirection = static_cast<uint8>(EBattleDirection::BaixoDireita);

	bool IsValidAction(const FBattleAction& Action)
	{
		return static_cast<uint8>(Action.Type) <= MaxValidActionType
			&& static_cast<uint8>(Action.Direction) <= MaxValidDirection;
	}
}

bool ValidateNetTurnCommit(const FNetTurnCommit& Commit)
{
	return IsValidAction(Commit.ActionA)
		&& IsValidAction(Commit.ActionB)
		&& IsValidAction(Commit.ActionC);
}

FTurnCommit ToTurnCommit(const FNetTurnCommit& NetCommit)
{
	// O DONO atravessa a conversão. Perdê-lo aqui faria o commit chegar ao
	// núcleo sem endereço, e o resolvedor o ignoraria em silêncio — a ação do
	// jogador sumiria entre o fio e a regra.
	FTurnCommit Result;
	Result.PetId = NetCommit.PetId;
	Result.Actions[0] = NetCommit.ActionA;
	Result.Actions[1] = NetCommit.ActionB;
	Result.Actions[2] = NetCommit.ActionC;
	return Result;
}

FNetTurnCommit ToNetTurnCommit(const FTurnCommit& Commit)
{
	FNetTurnCommit Result;
	Result.PetId = Commit.PetId;
	Result.ActionA = Commit.Actions[0];
	Result.ActionB = Commit.Actions[1];
	Result.ActionC = Commit.Actions[2];
	return Result;
}

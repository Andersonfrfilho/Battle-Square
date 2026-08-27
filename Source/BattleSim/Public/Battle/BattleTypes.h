// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BattleTypes.generated.h"

// Tipo de uma ação de combate. Ver AD-009: ação é o par (Tipo, Direção),
// não uma lista plana de 12 ações — sem direção, ataque numa grade 3x3
// atinge o tabuleiro inteiro a partir do centro e a posição vira decorativa.
UENUM()
enum class EActionType : uint8
{
	Aguardar = 0,
	Mover,
	Atacar,
	Magia,
	Defender,
	Esquivar,

	// DP-ia-04: esconder-se é AÇÃO, não estado ligado por fora.
	Camuflar,
	Voar,
	Submergir
};

// As 8 direções da grade. Defender e Aguardar ignoram a direção.
UENUM()
enum class EBattleDirection : uint8
{
	Nenhuma = 0,
	Cima,
	Baixo,
	Esquerda,
	Direita,
	CimaEsquerda,
	CimaDireita,
	BaixoEsquerda,
	BaixoDireita
};

// Uma ação enfileirada: par (Tipo, Direção). Fixado em 2 bytes por
// T1 (BattleSim.tasks.md) — é a base do custo de rede do commit.
USTRUCT()
struct FBattleAction
{
	GENERATED_BODY()

	UPROPERTY()
	EActionType Type = EActionType::Aguardar;

	UPROPERTY()
	EBattleDirection Direction = EBattleDirection::Nenhuma;
};

// As 3 ações que um jogador compromete por turno, às cegas (AD-005).
USTRUCT()
struct FTurnCommit
{
	GENERATED_BODY()

	static constexpr int32 ActionsPerTurn = 3;

	UPROPERTY()
	FBattleAction Actions[ActionsPerTurn];
};

static_assert(sizeof(FBattleAction) == 2, "FBattleAction deve ocupar 2 bytes — ver design.md, custo de rede do commit.");

// Empacota coluna e linha (0..2 cada) num único uint8: 4 bits por eixo.
// Usado no trace de eventos (FromCell/ToCell) para manter o struct plano.
FORCEINLINE uint8 PackCell(uint8 Column, uint8 Row)
{
	return static_cast<uint8>((Column & 0x0F) | ((Row & 0x0F) << 4));
}

FORCEINLINE void UnpackCell(uint8 PackedCell, uint8& OutColumn, uint8& OutRow)
{
	OutColumn = PackedCell & 0x0F;
	OutRow = (PackedCell >> 4) & 0x0F;
}

// Deslocamento de uma direção na grade. Nenhuma (Defender/Aguardar) e
// qualquer entrada fora do enum mapeiam para (0,0) — ausência de
// movimento, não erro. Usado por F3 (movimento) e F4 (alcance de ataque).
FORCEINLINE void GetDirectionDelta(EBattleDirection Direction, int8& OutDeltaColumn, int8& OutDeltaRow)
{
	switch (Direction)
	{
		case EBattleDirection::Cima:          OutDeltaColumn =  0; OutDeltaRow = -1; break;
		case EBattleDirection::Baixo:         OutDeltaColumn =  0; OutDeltaRow =  1; break;
		case EBattleDirection::Esquerda:      OutDeltaColumn = -1; OutDeltaRow =  0; break;
		case EBattleDirection::Direita:       OutDeltaColumn =  1; OutDeltaRow =  0; break;
		case EBattleDirection::CimaEsquerda:  OutDeltaColumn = -1; OutDeltaRow = -1; break;
		case EBattleDirection::CimaDireita:   OutDeltaColumn =  1; OutDeltaRow = -1; break;
		case EBattleDirection::BaixoEsquerda: OutDeltaColumn = -1; OutDeltaRow =  1; break;
		case EBattleDirection::BaixoDireita:  OutDeltaColumn =  1; OutDeltaRow =  1; break;
		default:                              OutDeltaColumn =  0; OutDeltaRow =  0; break;
	}
}

// Inverso de GetDirectionDelta: o passo que aproxima de um alvo.
//
// Mora AQUI, coladinho na tabela que inverte, e não na IA que precisa dele:
// uma segunda cópia da relação direção<->deslocamento já produziu um defeito
// neste projeto, e cópias concordam até a primeira edição. Só o SINAL importa
// — a grade é 3x3 e todo passo é de uma casa.
FORCEINLINE EBattleDirection GetDirectionTowards(int32 DeltaColumn, int32 DeltaRow)
{
	const int32 StepColumn = FMath::Clamp(DeltaColumn, -1, 1);
	const int32 StepRow = FMath::Clamp(DeltaRow, -1, 1);

	if (StepColumn == 0 && StepRow == 0)
	{
		return EBattleDirection::Nenhuma;
	}

	// Nenhuma (0) fica de fora de propósito: ela é ausência de direção, e já
	// foi devolvida acima quando o alvo está na própria casa.
	for (uint8 Index = static_cast<uint8>(EBattleDirection::Cima);
		Index <= static_cast<uint8>(EBattleDirection::BaixoDireita); ++Index)
	{
		const EBattleDirection Candidate = static_cast<EBattleDirection>(Index);
		int8 CandidateColumn = 0;
		int8 CandidateRow = 0;
		GetDirectionDelta(Candidate, CandidateColumn, CandidateRow);

		if (CandidateColumn == StepColumn && CandidateRow == StepRow)
		{
			return Candidate;
		}
	}

	return EBattleDirection::Nenhuma;
}

FORCEINLINE bool IsInsideGrid(int32 Column, int32 Row, int32 GridSize = 3)
{
	return Column >= 0 && Column < GridSize && Row >= 0 && Row < GridSize;
}

// Arenas Variadas (design.md, DP-arena-01): propriedade de uma casa da
// grade. None = comportamento neutro, idêntico ao de antes desta
// feature — é o valor padrão de toda casa não configurada.
UENUM()
enum class ECellProperty : uint8
{
	None = 0,
	Blocked,
	Damage,
	Buff,

	// Submergir exige ÁGUA. Acrescentado ao FIM do enum de propósito: os
	// valores existentes vão para o layout da arena e para o hash do estado, e
	// inserir no meio reinterpretaria toda arena já escrita.
	Water
};

// Tamanho fixo da grade (3x3 = 9 casas) — ver spec.md, Out of Scope:
// tamanho variável de arena é reformulação de UI/câmera, fora daqui.
inline constexpr int32 BattleGridCellCount = 9;

FORCEINLINE int32 CellLayoutIndex(int32 Column, int32 Row)
{
	return Row * 3 + Column;
}

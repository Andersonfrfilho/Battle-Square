// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Battle/BattleRandom.h"
#include "BattleState.generated.h"

// Postura assumida num slot — bitmask, zerada ao fim de F5 (BTL-12).
UENUM(meta = (Bitflags))
enum class EBattlePostureFlags : uint8
{
	None      = 0,
	Defending = 1 << 0,
	Dodging   = 1 << 1,
};
ENUM_CLASS_FLAGS(EBattlePostureFlags)

// Estado de um pet dentro da batalha. Apenas inteiros e enums — ver AD-004:
// um float aqui quebra o determinismo em silêncio.
USTRUCT()
struct FPetState
{
	GENERATED_BODY()

	// Estável entre turnos: critério final de desempate (BTL-17), nunca a
	// ordem de iteração de um contêiner.
	UPROPERTY()
	uint8 PetId = 0;

	// 0 = esquerda, 1 = direita.
	UPROPERTY()
	uint8 Side = 0;

	UPROPERTY()
	uint8 Column = 0;

	UPROPERTY()
	uint8 Row = 0;

	UPROPERTY()
	int32 Health = 0;

	// Separado de Health — resolve o "HP: X/X" do protótipo antigo, que
	// nunca guardava o teto separadamente.
	UPROPERTY()
	int32 MaxHealth = 0;

	UPROPERTY()
	int32 Attack = 0;

	UPROPERTY()
	int32 Defense = 0;

	UPROPERTY()
	int32 Speed = 0;

	UPROPERTY()
	uint8 PostureFlags = 0; // EBattlePostureFlags empacotado

	// Acumulador de dano de F4 (Combate) — NUNCA aplicado na própria fase
	// (design.md, BTL-07). F5 (Encerramento) aplica tudo de uma vez e
	// zera este campo. É o que garante que dois pets que se matam no
	// mesmo slot morrem os dois: nenhum "morre primeiro".
	UPROPERTY()
	int32 PendingDamage = 0;

	bool IsAlive() const { return Health > 0; }
};

// Tudo que descreve uma batalha em andamento. Serializável, comparável,
// hasheável — ver design.md: é o que atravessa a fronteira do núcleo como
// dado, junto com o trace de eventos.
USTRUCT()
struct FBattleState
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FPetState> Pets;

	// O gerador de aleatoriedade da batalha vive AQUI DENTRO — ver AD-004 e
	// BattleRandom.h. É o que permite que reconexão e replay reproduzam a
	// mesma sequência: o estado do PRNG viaja junto com o resto do estado.
	UPROPERTY()
	FBattleRandom Random;

	UPROPERTY()
	int32 TurnNumber = 0;

	UPROPERTY()
	bool bBattleEnded = false;

	// Lado vencedor (0 ou 1), ou BattleNoWinnerSide se empate/batalha em
	// andamento. Só tem significado quando bBattleEnded é true — ver
	// BattleOutcome.h (T10).
	UPROPERTY()
	uint8 WinningSide = 0xFF;

	// Hash do estado, para detecção de dessincronia entre cliente e
	// servidor (ver design.md, Tratamento de Erro). Não depende de ordem
	// de contêiner: itera Pets ordenado por PetId antes de combinar.
	uint64 ComputeHash() const;
};

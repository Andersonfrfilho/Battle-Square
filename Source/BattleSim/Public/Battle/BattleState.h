// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Battle/BattleRandom.h"
#include "Battle/BattleTypes.h"
#include "BattleState.generated.h"

// Postura assumida num slot — bitmask, zerada ao fim de F5 (BTL-12).
UENUM(meta = (Bitflags))
enum class EBattlePostureFlags : uint8
{
	None        = 0,
	Defending   = 1 << 0,
	Dodging     = 1 << 1,

	// DP-ia-04. Três esconderijos com trocas DISTINTAS entre si — se as
	// trocas coincidissem, seriam três nomes para "esquivar melhor".
	Camouflaged = 1 << 2,  // imune a físico E magia; custa a ação seguinte
	Flying      = 1 << 3,  // imune a físico e ao dano de casa; magia acerta MAIS
	Underground = 1 << 4,  // imune a tudo; custa mover E atacar na ação seguinte

	// Cobrança do slot seguinte. Vive em PostureFlags, e não num campo novo,
	// porque campo novo entraria no hash do estado e invalidaria os snapshots
	// de determinismo de cenários que nem usam estas ações.
	Revealing   = 1 << 5,  // saindo da camuflagem
	Emerging    = 1 << 6,  // saindo do subsolo
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

	/**
	 * Poder de cada golpe, em PORCENTAGEM de dano (100 = neutro).
	 *
	 * Está no estado do núcleo, e não numa tabela consultada de fora, porque a
	 * resolução precisa dele: o commit carrega só o ÍNDICE do golpe
	 * (DP-golpe-04), e um índice sem poder não resolve nada. Consultar de fora
	 * durante a batalha quebraria a fronteira que mantém o núcleo verificável.
	 *
	 * ZERO significa "pet sem golpe cadastrado", e o combate cai no
	 * multiplicador padrão — o comportamento de antes dos golpes. Tratar zero
	 * como poder faria esse pet bater sem dano nenhum.
	 */
	UPROPERTY()
	int32 MovePowers[4] = { 0, 0, 0, 0 };

	/**
	 * O que cada golpe DEIXA na casa que acertou, como ECellProperty.
	 *
	 * `None` significa "não muda nada" — que é o mesmo valor de uma casa
	 * neutra, e isso é proposital: um golpe sem efeito não pode ser
	 * confundido com um golpe que neutraliza a casa.
	 */
	UPROPERTY()
	uint8 MoveTerrainEffects[4] = { 0, 0, 0, 0 };

	uint8 GetMoveTerrainEffect(uint8 MoveIndex) const
	{
		return MoveIndex < 4 ? MoveTerrainEffects[MoveIndex] : 0;
	}

	/** Poder do golpe naquele índice, ou 0 fora da faixa. */
	int32 GetMovePower(uint8 MoveIndex) const
	{
		return MoveIndex < 4 ? MovePowers[MoveIndex] : 0;
	}

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

	FBattleState()
	{
		// Arena neutra por padrão — toda casa None, comportamento
		// idêntico ao de antes de Arenas Variadas (design.md, zero
		// regressão). Índice = Row*3+Column (CellLayoutIndex).
		CellLayout.Init(static_cast<uint8>(ECellProperty::None), BattleGridCellCount);
	}

	UPROPERTY()
	TArray<FPetState> Pets;

	// Arenas Variadas (design.md, DP-arena-01): propriedade de cada casa
	// da grade, ECellProperty empacotado. Viaja DENTRO do estado — mesma
	// razão de FBattleRandom estar aqui (AD-004): precisa sobreviver a
	// serialização, replicação e reconexão junto com o resto.
	UPROPERTY()
	TArray<uint8> CellLayout;

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

	/**
	 * v1 é 1v1 (spec: mais de um pet por lado é M3) — o primeiro pet vivo do
	 * lado é o único pet do lado. A busca por Side, em vez de índice fixo, é o
	 * que deixa as fases prontas para N pets sem mudar assinatura.
	 *
	 * Mora aqui porque duas fases precisavam dela e cada uma tinha a sua
	 * cópia: cópias concordam até a primeira edição, e o compilador só
	 * reclamou quando o unity build juntou os dois arquivos.
	 */
	FPetState* FindAlivePetOnSide(uint8 Side);
};

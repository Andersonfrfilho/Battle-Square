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
	 * Acaso do combate, já em PORCENTAGEM pronta — não em atributo.
	 *
	 * O núcleo recebe "esquiva 12%" e "varia 8% para cada lado", nunca
	 * "reflexo 50" e "agressividade 30". A conversão de atributo para chance
	 * mora na MONTAGEM (FPetAttributeProgression::ApplyToBattleState), junto
	 * de MovePowers e pelo mesmo motivo: o núcleo resolve, não interpreta —
	 * e ajustar o equilíbrio não pode exigir tocar no que precisa continuar
	 * determinístico.
	 *
	 * ZERO nos dois é ausência de acaso, e é o padrão de propósito: um
	 * FPetState montado à mão resolve exatamente como antes desta feature.
	 * O padrão de ±20% do jogo é aplicado por quem traduz o pet, não por
	 * quem resolve o turno — senão toda batalha construída em teste teria
	 * dano imprevisível, e a fórmula de dano perderia os testes que a
	 * protegem.
	 *
	 * O núcleo ainda RECORTA os dois nos tetos (DP-atr-07): a amarra que
	 * impede o número de decidir mais que a decisão não pode morar num lugar
	 * que a montagem consiga passar por cima.
	 */
	UPROPERTY()
	int32 ReflexDodgePercent = 0;

	UPROPERTY()
	int32 DamageVariancePercent = 0;

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

	/**
	 * O que cada golpe faz com ATRIBUTO, e em quem.
	 *
	 * O SINAL diz o alvo: positivo sobe o SEU atributo, negativo derruba o
	 * DELE. Um campo em vez de dois, e a leitura sai natural — "+30 de ataque"
	 * é ficar mais forte, "−30 de ataque" é enfraquecer o outro. Não existe
	 * subir o atributo do oponente nem baixar o próprio, então a bijeção é
	 * completa e não esconde nenhum caso.
	 */
	UPROPERTY()
	uint8 MoveEffectStats[4] = { 0, 0, 0, 0 };

	UPROPERTY()
	int32 MoveEffectPercents[4] = { 0, 0, 0, 0 };

	/**
	 * O efeito ATIVO neste pet. UM de cada vez, e o novo substitui o antigo.
	 *
	 * Empilhar seria dominante: três magias de ataque no mesmo turno dobrariam
	 * o dano, e a escola psíquica passaria a vencer por repetição em vez de
	 * por escolha. Substituir mantém a decisão viva — vale a pena trocar o
	 * bônus que já está de pé?
	 */
	UPROPERTY()
	uint8 ActiveEffectStat = 0;

	UPROPERTY()
	int32 ActiveEffectPercent = 0;

	UPROPERTY()
	uint8 ActiveEffectSlotsRemaining = 0;

	uint8 GetMoveEffectStat(uint8 MoveIndex) const
	{
		return MoveIndex < 4 ? MoveEffectStats[MoveIndex] : 0;
	}

	int32 GetMoveEffectPercent(uint8 MoveIndex) const
	{
		return MoveIndex < 4 ? MoveEffectPercents[MoveIndex] : 0;
	}

	/** Atributo já com o efeito ativo somado. Nunca abaixo de 1. */
	int32 GetEffectiveStat(EBattleStat Which) const
	{
		const int32 Base =
			Which == EBattleStat::Ataque ? Attack :
			Which == EBattleStat::Defesa ? Defense :
			Which == EBattleStat::Velocidade ? Speed : 0;

		if (ActiveEffectSlotsRemaining == 0
			|| static_cast<EBattleStat>(ActiveEffectStat) != Which)
		{
			return Base;
		}

		// Piso de 1: um atributo zerado por magia faria o pet parar de existir
		// como adversário, e perder assim não ensina nada a quem perdeu.
		return FMath::Max(1, Base + (Base * ActiveEffectPercent) / 100);
	}

	int32 GetEffectiveAttack() const { return GetEffectiveStat(EBattleStat::Ataque); }
	int32 GetEffectiveDefense() const { return GetEffectiveStat(EBattleStat::Defesa); }
	int32 GetEffectiveSpeed() const { return GetEffectiveStat(EBattleStat::Velocidade); }

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
		// regressão). Índice = Row*GridColumns+Column (CellLayoutIndex).
		CellLayout.Init(static_cast<uint8>(ECellProperty::None), BattleGridDefaultCellCount);

		// As regras de terreno do JOGO nascem com o estado.
		//
		// A primeira versão pedia que a montagem as aplicasse, e dois testes
		// caíram na hora: estado montado à mão ficava sem regra, e submergir
		// passava a funcionar em terra seca. Regra que depende de alguém
		// lembrar de chamar é regra que some no caminho que ninguém revisou —
		// o mesmo modo de falhar que este projeto já pagou quatro vezes com
		// recurso testado e nunca alcançado.
		//
		// Quem quiser outra regra sobrescreve depois, que é como um teste
		// monta um caso sem reconstruir a tabela inteira.
		ApplyDefaultTerrainRequirements();
	}

	UPROPERTY()
	TArray<FPetState> Pets;

	// Arenas Variadas (design.md, DP-arena-01): propriedade de cada casa
	// da grade, ECellProperty empacotado. Viaja DENTRO do estado — mesma
	// razão de FBattleRandom estar aqui (AD-004): precisa sobreviver a
	// serialização, replicação e reconexão junto com o resto.
	UPROPERTY()
	TArray<uint8> CellLayout;

	/**
	 * Dimensões da grade. Moram no ESTADO, e não numa constante global,
	 * porque a resolução depende delas: mesma semente e mesmas ações num
	 * campo 3x3 e num 4x6 são duas batalhas diferentes, e um tamanho lido
	 * de fora deixaria essa diferença invisível para o hash — que é
	 * exatamente a assinatura que existe para detectar divergência.
	 *
	 * Não precisam ser iguais: 3x2 e 4x6 são grades legítimas.
	 */
	/**
	 * Que TERRENO cada ação de postura exige, e com que nível.
	 *
	 * Indexado por EActionType. Zero é "não exige nada", e é o que toda ação
	 * sem requisito continua sendo.
	 *
	 * Isto morava num `if` dentro do núcleo — "submergir exige água" — e era
	 * por isso que uma skill nova custava uma edição de BattlePhasePosture.
	 * Como DADO, `escavar` (que quer pedra) e um poder que exija água FUNDA
	 * passam a ser uma linha de configuração, e não uma exceção a mais.
	 *
	 * O NÍVEL é comparação, não igualdade: exigir "água ao menos 2" deixa a
	 * poça de fora sem precisar listar cada terreno que não serve.
	 */
	UPROPERTY()
	uint8 SkillTerrainRequirement[16] = { 0 };

	UPROPERTY()
	uint8 SkillTerrainLevel[16] = { 0 };

	/**
	 * O terreno da casa satisfaz o que aquela ação exige?
	 *
	 * Ação sem requisito passa sempre. Com requisito, a casa precisa ser
	 * daquele terreno E ter nível ao menos igual — a comparação é o que deixa
	 * "água ao menos funda" excluir a poça sem listar terreno por terreno.
	 */
	/**
	 * Declara o que uma ação exige do terreno.
	 *
	 * Chamado pela MONTAGEM. Se ninguém chamar, nenhuma ação exige nada — e é
	 * por isso que o padrão precisa ser posto em algum lugar visível, e não
	 * ficar dependendo de quem monta lembrar.
	 */
	void RequireTerrainForSkill(EActionType Action, ECellProperty Terrain)
	{
		const int32 Indice = static_cast<int32>(Action);
		if (Indice >= 0 && Indice < 16)
		{
			SkillTerrainRequirement[Indice] = static_cast<uint8>(Terrain);
		}
	}

	/**
	 * Os requisitos que o jogo tem hoje.
	 *
	 * Mora aqui, e não em cada montagem, porque "submergir exige água funda" é
	 * regra do jogo e não de uma partida. Toda batalha nasce com ela; quem
	 * quiser outra sobrescreve depois — o que é como um teste monta um caso
	 * sem precisar reconstruir a tabela inteira.
	 */
	void ApplyDefaultTerrainRequirements()
	{
		RequireTerrainForSkill(EActionType::Submergir, ECellProperty::Water);
	}

	bool TerrainAllowsSkill(EActionType Action, uint8 CellProperty) const
	{
		const int32 Indice = static_cast<int32>(Action);
		if (Indice < 0 || Indice >= 16)
		{
			return true;
		}

		const uint8 Exigido = SkillTerrainRequirement[Indice];
		if (Exigido == static_cast<uint8>(ECellProperty::None))
		{
			return true;
		}

		// ÁGUA é o caso com nível: a poça e o fundo são o mesmo terreno com
		// funduras diferentes, e exigir água é exigir ao menos aquela fundura.
		if (IsAnyWater(Exigido))
		{
			return IsAnyWater(CellProperty)
				&& WaterDepthOf(CellProperty) >= WaterDepthOf(Exigido);
		}

		return CellProperty == Exigido;
	}

	UPROPERTY()
	uint8 GridColumns = static_cast<uint8>(BattleGridDefaultColumns);

	UPROPERTY()
	uint8 GridRows = static_cast<uint8>(BattleGridDefaultRows);

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

	/** Índice da casa no layout, com as dimensões DESTE estado. */
	int32 CellIndex(int32 Column, int32 Row) const
	{
		return CellLayoutIndex(Column, Row, static_cast<int32>(GridColumns));
	}

	bool IsInside(int32 Column, int32 Row) const
	{
		return IsInsideGrid(Column, Row,
			static_cast<int32>(GridColumns), static_cast<int32>(GridRows));
	}

	/**
	 * Troca as dimensões e redimensiona o layout, deixando toda casa nova
	 * neutra. Recorta para a faixa válida em vez de recusar: uma grade
	 * pedida errada num arquivo de config não deve derrubar a batalha.
	 */
	BATTLESIM_API void ResizeGrid(int32 Columns, int32 Rows);

	/**
	 * Põe um duelo 1v1 nas bordas OPOSTAS da grade atual: lado 0 na primeira
	 * coluna, lado 1 na última, ambos na linha do meio.
	 *
	 * Mora aqui porque a posição inicial depende do tamanho da grade, e a
	 * grade mora aqui. Deixar a montagem escolher a casa levaria a duas
	 * respostas para a mesma pergunta assim que o campo deixasse de ser 3x3
	 * — que é exatamente o que aconteceu com as posições fixas (1,1)/(2,1).
	 */
	BATTLESIM_API void PlaceDuelistsAtStartingCells();

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

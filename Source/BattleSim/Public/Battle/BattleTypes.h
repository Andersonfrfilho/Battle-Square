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
	Submergir,

	/**
	 * ATRAVESSAR: o incorpóreo passa pelo que tem corpo.
	 *
	 * A skill do elemento Fantasma. Onde os outros esbarram na casa bloqueada
	 * — ou gastam um slot derrubando o tronco — ele passa. Acrescentada ao FIM
	 * do enum de propósito: os valores existentes viajam no commit de todo
	 * turno (DP-golpe-04) e no hash do traço, e inserir no meio
	 * reinterpretaria toda partida já gravada.
	 */
	Atravessar,

	/**
	 * ILUMINAR: a luz desfaz o que se esconde.
	 *
	 * A skill do elemento Luz, e a resposta ao fantasma. Sem ela, um pet que
	 * fica invisível e que o físico não acerta não teria contra-jogo — e
	 * "forte contra" viraria só um número na tabela.
	 */
	Iluminar,

	/**
	 * ESCAVAR: o pet de terra levanta uma barreira de terra à frente.
	 *
	 * A skill do elemento Terra, que era o único sem nenhuma — fogo voa, água
	 * submerge, planta camufla, fantasma atravessa, luz ilumina, e a terra
	 * ficava sem identidade própria.
	 *
	 * Ela é o oposto de tudo que existe: NADA no jogo cria obstáculo hoje,
	 * eles só são derrubados. Um pet que ergue cobertura muda o tabuleiro a
	 * favor em vez de contra, e é a única ação que faz isso.
	 *
	 * Ao FIM do enum, pelo mesmo motivo dos anteriores: os valores viajam no
	 * commit de todo turno e no hash do traço.
	 */
	Escavar
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

// Empacota coluna e linha (0..14 cada) num único uint8: 4 bits por eixo.
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

/**
 * Que atributo uma magia de efeito mexe.
 *
 * `Nenhum` é o golpe SEM efeito, e é o valor zero de propósito: um golpe
 * cadastrado antes desta feature não mexe em atributo nenhum, e é assim que
 * ele sempre se comportou.
 */
UENUM()
enum class EBattleStat : uint8
{
	Nenhum = 0,
	Ataque,
	Defesa,
	Velocidade
};

/**
 * Por quantos SLOTS um efeito de atributo dura.
 *
 * Três é o turno inteiro: quem gasta o primeiro slot numa magia de efeito
 * colhe nos dois seguintes. Um slot só faria a magia não valer o turno; mais
 * que um turno faria o primeiro a agir vencer por acúmulo.
 */
inline constexpr uint8 BattleStatEffectSlots = 3;

/** Teto do efeito, para os dois lados. */
inline constexpr int32 BattleStatEffectMaxPercent = 60;

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

// DP-golpe-04: o SEGUNDO BYTE de FBattleAction significa coisas diferentes
// conforme o tipo — direção para Mover, ÍNDICE DO GOLPE (0–3) para Atacar e
// Magia.
//
// Ele já era 2 bytes, e esse tamanho é a base do custo de rede do commit
// (BattleSim.tasks.md, T1). Um campo novo cresceria o commit de todo turno de
// toda partida para carregar informação que cabe num byte já reservado.
//
// As duas funções abaixo existem para que ninguém precise fazer o cast à mão:
// `static_cast<uint8>(Action.Direction)` espalhado pelo código seria a receita
// para alguém tratar índice como direção.
constexpr uint8 BattleMovesPerPet = 4;

FORCEINLINE uint8 GetMoveIndexFromAction(const FBattleAction& Action)
{
	const uint8 Bruto = static_cast<uint8>(Action.Direction);
	return Bruto < BattleMovesPerPet ? Bruto : 0;
}

FORCEINLINE FBattleAction MakeMoveAction(EActionType Type, uint8 MoveIndex)
{
	FBattleAction Action;
	Action.Type = Type;
	Action.Direction = static_cast<EBattleDirection>(
		MoveIndex < BattleMovesPerPet ? MoveIndex : 0);
	return Action;
}

FORCEINLINE bool IsInsideGrid(int32 Column, int32 Row, int32 GridColumns, int32 GridRows)
{
	return Column >= 0 && Column < GridColumns && Row >= 0 && Row < GridRows;
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
	Water,

	/**
	 * A POÇA: molha o pé, dá para andar, e NÃO serve para o que precisa de
	 * fundura.
	 *
	 * `Water` continua sendo a FUNDA, e isso é escolha de migração: é onde
	 * submergir já funciona hoje, então toda arena escrita, todo golpe com
	 * `terrainEffect: water` e todo snapshot de determinismo mantêm o
	 * comportamento exato. A novidade é a rasa.
	 */
	ShallowWater,

	/**
	 * Água CONGELADA. Terreno temporário: derrete sozinho e volta a ser água.
	 *
	 * Quanto ele dura é o "nível de congelamento" — não existe nível separado
	 * da duração, porque duas casas de gelo que duram o mesmo e diferem num
	 * número abstrato seriam indistinguíveis para quem joga.
	 */
	Ice,

	/**
	 * LAMA: terra encharcada. O terreno INCERTO do jogo.
	 *
	 * Andar nela dá em uma de três coisas — escorregar, atravessar firme, ou
	 * atravessar devagar. É o que a separa do gelo: o gelo nega com CERTEZA, e
	 * é por isso que congelar vale um slot; a lama é aposta, e é por isso que
	 * ela é o que SOBRA da água em vez de ser um golpe que alguém escolhe.
	 *
	 * Ao FIM do enum, pelo mesmo motivo dos anteriores.
	 */
	Mud,

	/**
	 * SECAR não é terreno — é EFEITO.
	 *
	 * Ela nunca fica numa casa: um golpe que a deposita apaga a água, a lama e
	 * o gelo dali, e a casa volta a ser chão seco. Vive neste enum porque o
	 * golpe carrega um `ECellProperty` só, e um campo novo em `FBattleAction`
	 * cresceria o commit de toda partida para dizer o que cabe aqui.
	 *
	 * Existe porque faltava: o terreno só ficava MAIS molhado ou congelava, e
	 * nada o secava de propósito. A cadeia seca com o tempo, e ninguém podia
	 * forçar — então quem alagou o campo tinha vantagem sem resposta.
	 */
	Dries
};

/**
 * Quão MOLHADA está esta casa: 3 rio, 2 poça, 1 lama, 0 seco.
 *
 * Serve à secagem, que precisa comparar dois candidatos e ficar com o mais
 * molhado: gelo derretendo sobre chão seco deixa poça, mas gelo derretendo
 * sobre um RIO tem de devolver o rio. Sem a comparação, a cadeia de secagem
 * transformaria todo rio congelado numa poça — o gelo viraria a maneira de
 * secar o campo, que é o oposto do que ele é.
 *
 * O gelo fica de fora: ele não é uma casa molhada, é uma casa DURA.
 */
FORCEINLINE int32 WetnessOf(uint8 CellProperty)
{
	switch (static_cast<ECellProperty>(CellProperty))
	{
		case ECellProperty::Water:        return 3;
		case ECellProperty::ShallowWater: return 2;
		case ECellProperty::Mud:          return 1;
		default:                          return 0;
	}
}

/** Dos dois, o mais molhado. */
FORCEINLINE uint8 WetterOf(uint8 A, uint8 B)
{
	return WetnessOf(A) >= WetnessOf(B) ? A : B;
}

/**
 * A lama dá em uma de três coisas, e a proporção é BALANÇO, não física.
 *
 * Um terço para cada é o que mantém a aposta legível: o jogador aprende
 * depressa que a lama é risco, sem precisar de tabela. Escorregar mais raro
 * que atravessar faria a lama parecer chão comum na maioria das vezes, e a
 * decisão de entrar nela deixaria de existir.
 */
inline constexpr int32 MudSlipChancePercent = 33;
inline constexpr int32 MudSlowChancePercent = 33;

/**
 * A LAMA precisa de MUITA ÁGUA e de CLIMA ÚMIDO. Sem as duas, a poça seca.
 *
 * Isto é o que impede o tabuleiro inteiro de virar lama numa partida longa: a
 * poça solta — a que sobrou de um gelo derretendo em terra seca — evapora e
 * acabou. Lama é o chão encharcado de quem tem água por perto, não o destino
 * de toda gota que cai.
 *
 * A "muita água" é medida do PRÓPRIO tabuleiro, e não declarada à parte: uma
 * arena com rio produz lama, uma clareira seca não, e ninguém precisa manter
 * um segundo número em sincronia com o mapa que está ali.
 */
inline constexpr int32 MudMinHumidity = 60;
inline constexpr int32 MudWetCellsNumerator = 1;
inline constexpr int32 MudWetCellsDenominator = 5;
inline constexpr int32 MudMinWetCells = 2;

/**
 * Umidade padrão: a MATA, que é onde a arena e o mundo aberto se passam.
 *
 * Acima do limiar de propósito. Um padrão seco deixaria a lama existir só
 * depois de alguém lembrar de configurar o clima — e recurso testado que
 * ninguém alcança é o defeito que este projeto já registrou mais de uma vez.
 */
inline constexpr uint8 BattleDefaultHumidity = 70;

/** Quanto a lama tira da velocidade de quem atravessa devagar. */
inline constexpr int32 MudSlowPercent = 50;

/** Esta casa é água, de qualquer fundura? */
FORCEINLINE bool IsAnyWater(uint8 CellProperty)
{
	const ECellProperty Regra = static_cast<ECellProperty>(CellProperty);
	return Regra == ECellProperty::Water || Regra == ECellProperty::ShallowWater;
}

/**
 * Quanta fundura esta casa tem: 0 seco, 1 poça, 2 fundo.
 *
 * Número, e não enum, porque a pergunta que os golpes fazem é de COMPARAÇÃO —
 * "este poder exige ao menos 2" — e comparar níveis é o que faz um requisito
 * de terreno crescer sem virar uma lista de casos.
 */
FORCEINLINE int32 WaterDepthOf(uint8 CellProperty)
{
	switch (static_cast<ECellProperty>(CellProperty))
	{
		case ECellProperty::ShallowWater: return 1;
		case ECellProperty::Water:        return 2;
		default:                          return 0;
	}
}

/**
 * O que um golpe DEIXA na casa que acertou: o terreno, e por quanto tempo.
 *
 * Um parâmetro em vez de dois soltos, porque são uma coisa só — e porque o
 * caminho do combate já passa nove valores por baixo, e o décimo posicional
 * seria o que alguém acabaria trocando de lugar com o vizinho.
 *
 * `Slots == 0` é permanente: o alagamento de sempre.
 */
struct FTerrainDeposit
{
	uint8 Terrain = 0;
	uint8 Slots = 0;
};

// Grade PADRÃO. Não é mais o único tamanho possível: o estado carrega as
// suas próprias dimensões (FBattleState::GridColumns/GridRows), e estes
// valores são só o que uma arena nasce sendo quando ninguém escolheu.
inline constexpr int32 BattleGridDefaultColumns = 3;
inline constexpr int32 BattleGridDefaultRows = 3;
inline constexpr int32 BattleGridDefaultCellCount =
	BattleGridDefaultColumns * BattleGridDefaultRows;

// TETO DE 15 POR EIXO, e ele não é arbitrário: PackCell guarda coluna e
// linha em 4 bits cada dentro de um uint8, e o trace de eventos depende
// desse empacotamento. Grade 16x16 não estouraria a memória — estouraria
// o evento, calada, trocando a coluna 16 pela 0.
inline constexpr int32 BattleGridMinSide = 1;
inline constexpr int32 BattleGridMaxSide = 15;

// Índice da casa no layout linear. O número de COLUNAS é obrigatório e não
// tem padrão de propósito: um padrão silencioso aqui reintroduziria o 3
// fixo em qualquer chamada que esquecesse de passá-lo, e o erro apareceria
// como casa trocada, não como falha de compilação.
FORCEINLINE int32 CellLayoutIndex(int32 Column, int32 Row, int32 GridColumns)
{
	return Row * GridColumns + Column;
}

// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Battle/BattleRandom.h"
#include "Battle/BattleTypes.h"
#include "BattleState.generated.h"

/**
 * O que o pet É por natureza — vem do ELEMENTO, e não de uma escolha do turno.
 */
UENUM()
enum class EPetTrait : uint8
{
	Nenhum = 0,

	/**
	 * INCORPÓREO: não encosta no chão, nunca.
	 *
	 * O fantasma. Casa de dano não o alcança, gelo não o faz escorregar e lama
	 * não o atola — não porque ele resista a isso, mas porque nada disso toca
	 * nele. É a diferença entre uma imunidade concedida e uma que decorre do
	 * que a criatura é.
	 */
	Incorporeo = 1 << 0
};


// Postura assumida num slot — bitmask, zerada ao fim de F5 (BTL-12).
UENUM(meta = (Bitflags))
enum class EBattlePostureFlags : uint16
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

	Slowed      = 1 << 7,

	/**
	 * ATRAVESSANDO: o incorpóreo passa pelo que tem corpo, neste slot.
	 *
	 * O byte ANTERIOR acabava aqui — `Slowed` era o oitavo bit, e eu tinha
	 * escrito que o próximo obrigaria a alargar. Obrigou. `PostureFlags` virou
	 * uint16, e isso muda o hash de todo estado: é mudança deliberada, não
	 * descuido, e a alternativa era espremer duas ideias no mesmo bit.
	 */
	Phasing     = 1 << 8,
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
	 * Por quantos SLOTS o terreno que o golpe deixa sobrevive.
	 *
	 * ZERO é PARA SEMPRE, e isso não é um acidente de valor padrão: água e
	 * casa de dano nunca expiraram, e todo golpe já assinado tem zero aqui.
	 * Fosse zero "some na hora", a fatia do gelo apagaria em silêncio o efeito
	 * de todo golpe de terreno que existe.
	 *
	 * É também o "nível de congelamento" que o gelo tem — não existe nível
	 * separado da duração (DP-gelo-01). Duas casas de gelo que derretem no
	 * mesmo instante e diferem num número abstrato seriam indistinguíveis
	 * para quem joga, e o número seria decoração.
	 */
	UPROPERTY()
	uint8 MoveTerrainDurations[4] = { 0, 0, 0, 0 };

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

		// A LAMA entra ANTES da magia e sobrevive a ela: são causas
		// diferentes, e uma não pode apagar a outra. Guardar o atraso no
		// mesmo par (ActiveEffectStat, Percent) faria a lama zerar a magia
		// que o jogador tinha acabado de lançar — e ele veria o efeito dele
		// sumir sem nada explicar.
		const bool bNaLama = Which == EBattleStat::Velocidade
			&& (PostureFlags & static_cast<uint16>(EBattlePostureFlags::Slowed)) != 0;
		const int32 ComLama = bNaLama
			? FMath::Max(1, Base - (Base * MudSlowPercent) / 100)
			: Base;

		if (ActiveEffectSlotsRemaining == 0
			|| static_cast<EBattleStat>(ActiveEffectStat) != Which)
		{
			return ComLama;
		}

		// Piso de 1: um atributo zerado por magia faria o pet parar de existir
		// como adversário, e perder assim não ensina nada a quem perdeu.
		return FMath::Max(1, ComLama + (ComLama * ActiveEffectPercent) / 100);
	}

	int32 GetEffectiveAttack() const { return GetEffectiveStat(EBattleStat::Ataque); }
	int32 GetEffectiveDefense() const { return GetEffectiveStat(EBattleStat::Defesa); }
	int32 GetEffectiveSpeed() const { return GetEffectiveStat(EBattleStat::Velocidade); }

	uint8 GetMoveTerrainEffect(uint8 MoveIndex) const
	{
		return MoveIndex < 4 ? MoveTerrainEffects[MoveIndex] : 0;
	}

	uint8 GetMoveTerrainDuration(uint8 MoveIndex) const
	{
		return MoveIndex < 4 ? MoveTerrainDurations[MoveIndex] : 0;
	}

	/** Poder do golpe naquele índice, ou 0 fora da faixa. */
	int32 GetMovePower(uint8 MoveIndex) const
	{
		return MoveIndex < 4 ? MovePowers[MoveIndex] : 0;
	}

	UPROPERTY()
	uint16 PostureFlags = 0; // EBattlePostureFlags empacotado

	/**
	 * O que este pet É, e não o que ele escolheu fazer.
	 *
	 * Postura é decisão do turno e some no fim do slot; TRAÇO é do bicho, vem
	 * do elemento dele e vale sempre. Guardar "incorpóreo" em PostureFlags
	 * faria o fantasma virar corpo toda vez que o slot terminasse.
	 */
	UPROPERTY()
	uint8 Traits = 0;

	bool HasTrait(EPetTrait Trait) const
	{
		return (Traits & static_cast<uint8>(Trait)) != 0;
	}

	/**
	 * Este pet está fora do chão AGORA?
	 *
	 * Um lugar só para a pergunta, porque três fases a fazem — o dano da
	 * casa, o escorregão e o atolo — e três cópias concordariam até a
	 * primeira edição. O fantasma entrou justamente por aqui: ele é a
	 * primeira criatura que está fora do chão sem ter feito nada.
	 */
	bool IsOffTheGround() const
	{
		return HasTrait(EPetTrait::Incorporeo)
			|| (PostureFlags & static_cast<uint16>(EBattlePostureFlags::Flying)) != 0
			|| (PostureFlags & static_cast<uint16>(EBattlePostureFlags::Underground)) != 0;
	}

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
		CellCountdown.Init(0, BattleGridDefaultCellCount);
		CellRevertsTo.Init(static_cast<uint8>(ECellProperty::None), BattleGridDefaultCellCount);

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
	 * TERRENO TEMPORÁRIO: quantos slots faltam, e no que a casa volta a ser.
	 *
	 * Duas listas paralelas a `CellLayout`, e não um campo só, porque terreno
	 * que passa são três informações: o que a casa é agora, quanto falta, e o
	 * que havia embaixo. Sem a terceira, congelar uma POÇA e esperar derreter
	 * devolveria água FUNDA — o jogador ganharia fundura de graça, e o gelo
	 * viraria a maneira mais barata de alagar o campo.
	 *
	 * Zero em `CellCountdown` é permanente, que é o que toda casa é hoje.
	 *
	 * Ninguém mexe nestes arrays diretamente: quem põe terreno que passa usa
	 * `SetTemporaryTerrain`, e quem faz o tempo correr é a fase de
	 * encerramento. Duas listas editadas à mão em lugares diferentes
	 * concordariam até a primeira edição.
	 */
	UPROPERTY()
	TArray<uint8> CellCountdown;

	UPROPERTY()
	TArray<uint8> CellRevertsTo;

	/**
	 * Põe terreno na casa, com prazo.
	 *
	 * `Slots == 0` é terreno PERMANENTE — o comportamento de água e casa de
	 * dano, que nunca voltaram atrás.
	 *
	 * Congelar sobre gelo não empilha nem reinicia sozinho: o prazo novo vale,
	 * e o que a casa volta a ser continua sendo o de baixo. Fosse o de cima, a
	 * segunda camada faria a casa derreter para GELO, e ela nunca mais seria
	 * água.
	 */
	void SetTemporaryTerrain(int32 Column, int32 Row, uint8 Terrain, uint8 Slots)
	{
		const int32 Indice = CellIndex(Column, Row);
		if (!CellLayout.IsValidIndex(Indice))
		{
			return;
		}

		if (Slots > 0)
		{
			const bool bJaEraTemporario =
				CellCountdown.IsValidIndex(Indice) && CellCountdown[Indice] > 0;

			if (CellRevertsTo.IsValidIndex(Indice) && !bJaEraTemporario)
			{
				CellRevertsTo[Indice] = CellLayout[Indice];
			}
			if (CellCountdown.IsValidIndex(Indice))
			{
				CellCountdown[Indice] = Slots;
			}
		}
		else
		{
			// Terreno permanente APAGA o prazo que houvesse: alagar de vez uma
			// casa congelada não pode deixar para trás um cronômetro que a
			// devolveria ao que ela era antes do gelo.
			if (CellCountdown.IsValidIndex(Indice))
			{
				CellCountdown[Indice] = 0;
			}
			if (CellRevertsTo.IsValidIndex(Indice))
			{
				CellRevertsTo[Indice] = static_cast<uint8>(ECellProperty::None);
			}
		}

		CellLayout[Indice] = Terrain;
	}

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
	 * O que o CHÃO faz com quem tenta sair dele, em porcentagem.
	 *
	 * Escorregar é movimento PERDIDO — a ação foi gasta e o pet não saiu do
	 * lugar. Atrasar é sair devagar: anda, mas age mais devagar no resto do
	 * slot. O que sobra dos dois é atravessar firme.
	 *
	 * Não é o mesmo que casa bloqueada: ali o DESTINO tem corpo, e existe
	 * derrubar e subir. Aqui é o chão de baixo que trai.
	 *
	 * Porcentagem, e não um booleano, porque é o que faz o GELO e a LAMA serem
	 * o mesmo mecanismo com números diferentes: o gelo nega com certeza (100),
	 * a lama é aposta (33/33). Dois códigos para isso produziriam duas regras
	 * que concordam até a primeira edição, que é a duplicação por que este
	 * projeto já pagou três vezes.
	 */
	UPROPERTY()
	uint8 TerrainSlipPercent[16] = { 0 };

	UPROPERTY()
	uint8 TerrainSlowPercent[16] = { 0 };

	/**
	 * A CADEIA DE SECAGEM: no que cada terreno se transforma, e em quantos
	 * slots. Indexado por ECellProperty.
	 *
	 * Gelo → poça → lama → chão seco. Escrita como dado, e não como uma
	 * escada de `if` no encerramento do slot, porque é exatamente a forma que
	 * o resto desta feature tomou — e porque a cadeia é a coisa mais provável
	 * de alguém querer ajustar por balanço.
	 *
	 * Delay ZERO é "não seca sozinho", e é o que todo terreno é por padrão: o
	 * rio não vira poça, a brasa não apaga, a casa de bônus não gasta.
	 */
	UPROPERTY()
	uint8 TerrainDriesTo[16] = { 0 };

	UPROPERTY()
	uint8 TerrainDryDelay[16] = { 0 };

	/**
	 * Umidade do lugar, 0 a 100. Quem a decide é a MONTAGEM, a partir do
	 * clima do cenário.
	 *
	 * Entra como número, e não como o `EScenaryClimate` que já existe, porque
	 * aquele mora no BattleSquare e faz conta com float — e float aqui quebra
	 * o determinismo em silêncio (AD-004). O núcleo guarda o efeito; a camada
	 * de fora guarda o significado.
	 */
	UPROPERTY()
	uint8 Humidity = BattleDefaultHumidity;

	/** Quantas casas do tabuleiro têm água, de qualquer fundura. */
	int32 CountWetCells() const
	{
		int32 Molhadas = 0;
		for (uint8 Casa : CellLayout)
		{
			if (IsAnyWater(Casa))
			{
				++Molhadas;
			}
		}
		return Molhadas;
	}

	/**
	 * Este lugar produz LAMA, ou a poça simplesmente seca?
	 *
	 * Precisa das duas coisas: clima úmido e água em quantidade. Uma poça
	 * solta em plena mata evapora — lama é chão encharcado, e uma gota não
	 * encharca nada.
	 */
	bool WouldFormMud() const
	{
		if (Humidity < MudMinHumidity)
		{
			return false;
		}

		const int32 Casas = CellLayout.Num();
		const int32 Exigido = FMath::Max(MudMinWetCells,
			(Casas * MudWetCellsNumerator) / MudWetCellsDenominator);
		return CountWetCells() >= Exigido;
	}

	/**
	 * No que este terreno se transforma ao secar.
	 *
	 * A tabela manda, MENOS quando ela aponta para lama num lugar que não
	 * produz lama — aí a casa seca de vez. Um lugar só para essa pergunta
	 * porque a alternativa é a fase de encerramento consultar clima e contar
	 * água por conta própria, e regra que mora onde é usada vira duas cópias
	 * na segunda vez que alguém precisar dela.
	 */
	uint8 NextTerrainWhenDrying(uint8 Current) const
	{
		if (Current >= 16)
		{
			return static_cast<uint8>(ECellProperty::None);
		}

		const uint8 Alvo = TerrainDriesTo[Current];
		if (Alvo == static_cast<uint8>(ECellProperty::Mud) && !WouldFormMud())
		{
			return static_cast<uint8>(ECellProperty::None);
		}
		return Alvo;
	}

	/** Este chão faz ALGUMA coisa com quem sai dele? */
	bool TerrainAffectsDeparture(uint8 CellProperty) const
	{
		return CellProperty < 16
			&& (TerrainSlipPercent[CellProperty] > 0
				|| TerrainSlowPercent[CellProperty] > 0);
	}

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
	 * Os padrões do jogo já vêm no construtor; isto é para SOBRESCREVER — uma
	 * arena com regra própria, ou um teste que quer um caso sem reconstruir a
	 * tabela inteira.
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

		// No GELO se escorrega SEMPRE: o movimento é gasto e o pet fica onde
		// estava. É o que faz congelar valer o slot de quem congelou — a
		// recompensa é o que o OUTRO deixa de fazer, não o dano. Certeza, e
		// não sorteio, porque quem gastou uma ação para congelar precisa
		// saber o que comprou.
		TerrainSlipPercent[static_cast<int32>(ECellProperty::Ice)] = 100;

		// A LAMA é aposta: um terço escorrega, um terço atrasa, um terço
		// atravessa firme. É o que a separa do gelo e o que a torna digna de
		// ser o resto da água em vez de um golpe que alguém escolhe.
		TerrainSlipPercent[static_cast<int32>(ECellProperty::Mud)] = MudSlipChancePercent;
		TerrainSlowPercent[static_cast<int32>(ECellProperty::Mud)] = MudSlowChancePercent;

		// A cadeia. O gelo derrete em POÇA, e não no que havia embaixo: gelo é
		// água congelada, e derreter deixa água. Quem garante que um RIO
		// congelado volte a ser rio é a comparação por WetterOf lá na
		// secagem, não uma exceção aqui.
		TerrainDriesTo[static_cast<int32>(ECellProperty::Ice)] =
			static_cast<uint8>(ECellProperty::ShallowWater);

		TerrainDriesTo[static_cast<int32>(ECellProperty::ShallowWater)] =
			static_cast<uint8>(ECellProperty::Mud);
		TerrainDryDelay[static_cast<int32>(ECellProperty::ShallowWater)] = 2;

		TerrainDriesTo[static_cast<int32>(ECellProperty::Mud)] =
			static_cast<uint8>(ECellProperty::None);
		TerrainDryDelay[static_cast<int32>(ECellProperty::Mud)] = 2;
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

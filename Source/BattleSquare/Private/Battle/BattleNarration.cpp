// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattleNarration.h"

#include "Battle/BattleEvent.h"
#include "Battle/BattleState.h"

#define LOCTEXT_NAMESPACE "BattleNarration"

namespace
{
	/** Nome do atributo como o jogador lê. */
	FText DescribeStat(uint8 Stat)
	{
		switch (static_cast<EBattleStat>(Stat))
		{
			case EBattleStat::Ataque:     return LOCTEXT("StatAtaque", "ataque");
			case EBattleStat::Defesa:     return LOCTEXT("StatDefesa", "defesa");
			case EBattleStat::Velocidade: return LOCTEXT("StatVelocidade", "velocidade");
			default:                      return LOCTEXT("StatNenhum", "atributo");
		}
	}

	/** O terreno como o jogador lê. */
	FText DescribeTerrain(uint8 Terrain)
	{
		switch (static_cast<ECellProperty>(Terrain))
		{
			case ECellProperty::Water:        return LOCTEXT("TerrenoAguaFunda", "água funda");
			case ECellProperty::ShallowWater: return LOCTEXT("TerrenoPoca", "poça");
			case ECellProperty::Ice:          return LOCTEXT("TerrenoGelo", "gelo");
			case ECellProperty::Mud:          return LOCTEXT("TerrenoLama", "lama");
			case ECellProperty::Damage:       return LOCTEXT("TerrenoBrasa", "brasa");
			case ECellProperty::Buff:         return LOCTEXT("TerrenoBonus", "terreno de bônus");
			case ECellProperty::Blocked:      return LOCTEXT("TerrenoObstaculo", "obstáculo");
			default:                          return LOCTEXT("TerrenoSeco", "chão seco");
		}
	}
}

namespace
{
	TArray<FBattleNarrationFeed::FLine> GNarrationLines;

	/** Nome ausente não pode quebrar a frase nem derrubar a partida: o feed é
	 *  a última coisa do jogo que tem direito de falhar. */
	FText SafeName(const FString& Name, const FText& Fallback)
	{
		return Name.IsEmpty() ? Fallback : FText::FromString(Name);
	}
}

FText FBattleNarration::Describe(const FBattleEvent& Event, const FString& ActorName, const FString& TargetName)
{
	// Argumentos NOMEADOS, não posicionais: em alemão o objeto vem antes do
	// verbo, e {0}/{1} obrigariam o tradutor a reordenar o que não é dele.
	FFormatNamedArguments Args;
	Args.Add(TEXT("Actor"), SafeName(ActorName, LOCTEXT("PetSemNome", "O pet")));
	Args.Add(TEXT("Target"), SafeName(TargetName, LOCTEXT("AdversarioSemNome", "o adversário")));
	Args.Add(TEXT("Value"), FText::AsNumber(Event.Value));

	switch (Event.Type)
	{
	case EBattleEventType::AtaqueAcertou:
		// O NÚMERO, e não só o "acertou": a variação de dano é sorteada
		// (BattlePhaseCombat), e sem o valor na tela dois golpes iguais
		// parecem idênticos enquanto tiram vidas diferentes.
		return FText::Format(LOCTEXT("AtaqueAcertou", "{Actor} atacou {Target} e acertou - {Value} de dano"), Args);

	case EBattleEventType::AtaqueErrou:
		return FText::Format(LOCTEXT("AtaqueErrou", "{Actor} atacou {Target} e errou"), Args);

	case EBattleEventType::Esquivou:
		return FText::Format(LOCTEXT("Esquivou", "{Actor} esquivou"), Args);

	// DP-atr-08: todo sorteio que muda o resultado é NARRADO. Dano que some
	// sem explicação parece defeito, e este projeto já gastou rodadas com o
	// jogador achando que a regra estava quebrada quando ela funcionava.
	//
	// Frase DIFERENTE da esquiva declarada de propósito: uma foi decisão do
	// jogador, a outra foi o atributo agindo sozinho, e confundir as duas
	// ensinaria a lição errada sobre o que a escolha dele fez.
	// DP-atr-08 outra vez: sorteio ou não, o que MUDA o resultado é narrado.
	// Um bônus que aparece sem frase faz o jogador achar que o dano saiu
	// errado, e este projeto já gastou rodadas com regra funcionando e
	// parecendo quebrada.
	case EBattleEventType::AtributoAlterado:
	{
		FFormatNamedArguments ArgsAtributo = Args;
		ArgsAtributo.Add(TEXT("Atributo"), DescribeStat(Event.Detail));
		ArgsAtributo.Add(TEXT("Quanto"), FText::AsNumber(FMath::Abs(Event.Value)));

		// Frases DIFERENTES para subir e descer. "alterou o ataque em 30" faz
		// o jogador parar para descobrir de que lado a coisa foi — e ele está
		// no meio de um turno.
		return Event.Value > 0
			? FText::Format(
				LOCTEXT("AtributoSubiu", "{Actor} concentrou-se: {Atributo} +{Quanto}%"), ArgsAtributo)
			: FText::Format(
				LOCTEXT("AtributoDesceu", "{Actor} enfraqueceu {Target}: {Atributo} −{Quanto}%"), ArgsAtributo);
	}

	case EBattleEventType::AtributoVoltouAoNormal:
	{
		FFormatNamedArguments ArgsFim = Args;
		ArgsFim.Add(TEXT("Atributo"), DescribeStat(Event.Detail));
		return FText::Format(
			LOCTEXT("AtributoNormalizou", "{Actor}: {Atributo} voltou ao normal"), ArgsFim);
	}

	// O TABULEIRO mudando é jogada, e até agora mudava calado: a laje trocava
	// de cor e nenhuma linha dizia por quê. Quem não estava olhando para
	// aquela casa no instante exato não tinha como saber que ela mudou.
	case EBattleEventType::TerrenoMudou:
	{
		FFormatNamedArguments ArgsTerreno = Args;
		ArgsTerreno.Add(TEXT("Terreno"), DescribeTerrain(static_cast<uint8>(Event.Value)));

		// Terreno COM PRAZO diz o prazo. "Congelou a casa" sem mais nada faria
		// o jogador planejar em cima de algo que vai embora, e descobrir isso
		// tarde demais é o mesmo que a regra não existir para ele.
		if (Event.Detail > 0)
		{
			ArgsTerreno.Add(TEXT("Slots"), FText::AsNumber(static_cast<int32>(Event.Detail)));
			return FText::Format(LOCTEXT("TerrenoMudouComPrazo",
				"{Actor} cobriu a casa de {Terreno} — dura {Slots} ações"), ArgsTerreno);
		}

		return FText::Format(LOCTEXT("TerrenoMudou",
			"{Actor} mudou a casa para {Terreno}"), ArgsTerreno);
	}

	case EBattleEventType::TerrenoDerreteu:
	{
		FFormatNamedArguments ArgsFim = Args;
		ArgsFim.Add(TEXT("Terreno"), DescribeTerrain(static_cast<uint8>(Event.Value)));

		// SEM ator, e a frase reflete isso: derreter não tem autor, e pôr um
		// nome ali faria o jogador procurar o que o outro tinha feito.
		return FText::Format(LOCTEXT("TerrenoDerreteu",
			"o gelo derreteu — a casa voltou a ser {Terreno}"), ArgsFim);
	}

	case EBattleEventType::Escorregou:
	{
		// Diz a CONSEQUÊNCIA, não a física: "escorregou" sozinho poderia ser
		// dano, tontura ou casa trocada. O que o jogador precisa saber é que
		// a ação foi gasta e ele está onde estava.
		//
		// E diz ONDE, porque o gelo e a lama pedem leituras diferentes: no
		// gelo escorregar era certo e ele podia ter previsto; na lama foi
		// aposta perdida. Uma frase só para os dois ensinaria a lição errada
		// sobre o que a escolha dele valeu.
		FFormatNamedArguments ArgsChao = Args;
		ArgsChao.Add(TEXT("Terreno"), DescribeTerrain(static_cast<uint8>(Event.Detail)));
		return FText::Format(LOCTEXT("Escorregou",
			"{Actor} escorregou na {Terreno} — perdeu o movimento"), ArgsChao);
	}

	case EBattleEventType::AtravessouDevagar:
	{
		FFormatNamedArguments ArgsChao = Args;
		ArgsChao.Add(TEXT("Terreno"), DescribeTerrain(static_cast<uint8>(Event.Detail)));
		// Diz o que ele PERDE, não que "ficou lento": velocidade sozinha é
		// número, e o jogador precisa ligar isto ao turno que vem pela frente.
		return FText::Format(LOCTEXT("AtravessouDevagar",
			"{Actor} atolou na {Terreno} — atravessou, mas devagar"), ArgsChao);
	}

	case EBattleEventType::Revelado:
		// Diz o que MUDOU, não o que brilhou: "iluminou" sozinho seria efeito
		// visual, e o jogador precisa entender que o golpe passou a acertar.
		return FText::Format(LOCTEXT("Revelado",
			"{Actor} iluminou {Target} — agora ele pode ser atingido"), Args);

	case EBattleEventType::EsquivouPorReflexo:
		return FText::Format(LOCTEXT("EsquivouPorReflexo", "{Actor} desviou por reflexo"), Args);

	case EBattleEventType::ObstaculoDerrubado:
		// Diz que a passagem ABRIU, e não só que algo caiu: é a consequência
		// que muda a jogada seguinte, de quem derrubou e de quem assistiu.
		return FText::Format(LOCTEXT("ObstaculoDerrubado",
			"{Actor} derrubou o obstáculo — a passagem abriu"), Args);

	case EBattleEventType::SubiuNoObstaculo:
		// Diz o que a altura VALE. "Subiu no tronco" sozinho seria uma
		// mudança de casa como outra qualquer, e o jogador não ligaria o
		// golpe mais forte do slot seguinte a esta linha.
		return FText::Format(LOCTEXT("SubiuNoObstaculo",
			"{Actor} escalou o obstáculo — ataca de cima e alcança quem voa"), Args);

	case EBattleEventType::Defendeu:
		return FText::Format(LOCTEXT("Defendeu", "{Actor} defendeu o golpe"), Args);

	case EBattleEventType::DanoAplicado:
		return FText::Format(LOCTEXT("DanoAplicado", "{Target} perdeu {Value} de vida"), Args);

	case EBattleEventType::PetMorreu:
		return FText::Format(LOCTEXT("PetMorreu", "{Target} foi derrotado"), Args);

	case EBattleEventType::PosturaFalhou:
		// Diz o MOTIVO. "Falhou" sozinho faria o jogador concluir que a skill
		// está quebrada, quando o que faltava era a casa certa.
		if (static_cast<EBattlePostureFlags>(Event.Value) == EBattlePostureFlags::Underground)
		{
			return FText::Format(LOCTEXT("SubmergirSemAgua",
				"{Actor} tentou mergulhar, mas não há água nesta casa"), Args);
		}
		return FText::Format(LOCTEXT("PosturaFalhou", "{Actor} não conseguiu assumir a postura"), Args);

	case EBattleEventType::EncontroNoMesmoPonto:
		return FText::Format(LOCTEXT("EncontroNoMesmoPonto",
			"{Actor} e {Target} foram para a mesma casa e trombaram"), Args);

	case EBattleEventType::MovimentoBloqueado:
		// Dizer só "bloqueado" deixa o jogador achando que foi bug. A borda é
		// a explicação, e é ela que ensina a não repetir a escolha.
		return FText::Format(LOCTEXT("MovimentoBloqueado", "{Actor} tentou andar e esbarrou no limite da arena"), Args);

	case EBattleEventType::PosturaAssumida:
		switch (static_cast<EBattlePostureFlags>(Event.Value))
		{
		case EBattlePostureFlags::Defending:
			return FText::Format(LOCTEXT("PosturaDefender", "{Actor} se defendeu"), Args);
		case EBattlePostureFlags::Dodging:
			return FText::Format(LOCTEXT("PosturaEsquivar", "{Actor} ficou evasivo"), Args);
		case EBattlePostureFlags::Camouflaged:
			return FText::Format(LOCTEXT("PosturaCamuflar", "{Actor} se camuflou e sumiu de vista"), Args);
		case EBattlePostureFlags::Flying:
			return FText::Format(LOCTEXT("PosturaVoar", "{Actor} alçou voo — fora do alcance físico, exposto à magia"), Args);
		case EBattlePostureFlags::Underground:
			return FText::Format(LOCTEXT("PosturaSubmergir", "{Actor} mergulhou no solo"), Args);
		default:
			return FText::Format(LOCTEXT("PosturaAssumida", "{Actor} assumiu postura"), Args);
		}

	case EBattleEventType::Moveu:
		return FText::Format(LOCTEXT("Moveu", "{Actor} se moveu"), Args);

	default:
		// Contabilidade de slot, turno e fim de batalha: quem anuncia isso é a
		// arena, com o contexto de quem venceu, que o evento sozinho não tem.
		return FText::GetEmpty();
	}
}

void FBattleNarrationFeed::Push(const FText& Text, const FColor& Color)
{
	if (Text.IsEmpty())
	{
		return;
	}

	GNarrationLines.Add(FLine{ Text, Color });

	while (GNarrationLines.Num() > MaxLines)
	{
		GNarrationLines.RemoveAt(0);
	}
}

void FBattleNarrationFeed::Clear()
{
	GNarrationLines.Reset();
}

const TArray<FBattleNarrationFeed::FLine>& FBattleNarrationFeed::GetLines()
{
	return GNarrationLines;
}

#undef LOCTEXT_NAMESPACE

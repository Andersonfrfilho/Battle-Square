// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattleEvent.h"
#include "Battle/BattleState.h"
#include "Meta/PetAttributeProgression.h"
#include "Misc/AutomationTest.h"

namespace
{
	FBattleEvent AttrEvent(EBattleEventType Tipo, uint8 Ator, int32 Valor = 0,
		uint8 Slot = 0, uint8 Alvo = BattleEventNoActor)
	{
		FBattleEvent Evento;
		Evento.Type = Tipo;
		Evento.ActorId = Ator;
		Evento.TargetId = Alvo;
		Evento.Value = Valor;
		Evento.SlotIndex = Slot;
		return Evento;
	}
}

// Musculatura vem do DANO CAUSADO, não do ataque desferido.
//
// DP-atr-03: atacar o vazio não fortalece ninguém. Se contasse o golpe, o
// jogador subiria musculatura batendo em direção nenhuma contra um bot parado —
// e o atributo mediria paciência, não força.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMusculatureComesFromDamageDealtTest,
	"BattleSquare.Meta.AttributeProgression.MusculatureComesFromDamageDealt",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMusculatureComesFromDamageDealtTest::RunTest(const FString& Parameters)
{
	const TArray<FBattleEvent> ComDano = {
		AttrEvent(EBattleEventType::AtaqueAcertou, /*Ator=*/1, /*Valor=*/100),
	};
	const TArray<FBattleEvent> SoErros = {
		AttrEvent(EBattleEventType::AtaqueErrou, /*Ator=*/1),
		AttrEvent(EBattleEventType::AtaqueErrou, /*Ator=*/1),
		AttrEvent(EBattleEventType::AtaqueErrou, /*Ator=*/1),
	};

	TestTrue(TEXT("Dano causado dá musculatura"),
		FPetAttributeProgression::ComputeGains(ComDano, 1).Musculature > 0);
	TestEqual(TEXT("Errar não dá musculatura nenhuma"),
		FPetAttributeProgression::ComputeGains(SoErros, 1).Musculature, 0);

	// Dano do OUTRO pet não conta: o ator do evento é quem bateu.
	TestEqual(TEXT("Dano alheio não fortalece"),
		FPetAttributeProgression::ComputeGains(ComDano, /*PetId=*/2).Musculature, 0);

	return true;
}

// Personalidade INCLINA: atacar puxa para agressivo, postura puxa para
// cauteloso. Ela não sobe — é eixo, e é isso que permite golpe exigir um LADO.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPersonalityLeansBothWaysTest,
	"BattleSquare.Meta.AttributeProgression.PersonalityLeansBothWays",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPersonalityLeansBothWaysTest::RunTest(const FString& Parameters)
{
	const TArray<FBattleEvent> Agressivo = {
		AttrEvent(EBattleEventType::AtaqueAcertou, 1, 50),
		AttrEvent(EBattleEventType::AtaqueAcertou, 1, 50),
	};
	const TArray<FBattleEvent> Cauteloso = {
		AttrEvent(EBattleEventType::PosturaAssumida, 1,
			static_cast<int32>(EBattlePostureFlags::Defending)),
		AttrEvent(EBattleEventType::PosturaAssumida, 1,
			static_cast<int32>(EBattlePostureFlags::Dodging)),
	};

	TestTrue(TEXT("Atacar inclina para agressivo"),
		FPetAttributeProgression::ComputeGains(Agressivo, 1).Personality > 0);
	TestTrue(TEXT("Assumir postura inclina para cauteloso"),
		FPetAttributeProgression::ComputeGains(Cauteloso, 1).Personality < 0);

	return true;
}

// Proficiência de skill só conta a postura que PROTEGEU.
//
// Camuflar contra um oponente que não ataca não ensina nada, e contar isso
// faria o desbloqueio recompensar moagem em vez de jogo.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSkillProficiencyNeedsEffectiveUseTest,
	"BattleSquare.Meta.AttributeProgression.SkillProficiencyNeedsEffectiveUse",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSkillProficiencyNeedsEffectiveUseTest::RunTest(const FString& Parameters)
{
	// Camuflou e NÃO tomou dano: contou.
	const TArray<FBattleEvent> Protegeu = {
		AttrEvent(EBattleEventType::PosturaAssumida, 1,
			static_cast<int32>(EBattlePostureFlags::Camouflaged), /*Slot=*/0),
	};

	// Camuflou e tomou dano no MESMO slot: não protegeu, não conta.
	const TArray<FBattleEvent> NaoProtegeu = {
		AttrEvent(EBattleEventType::PosturaAssumida, 1,
			static_cast<int32>(EBattlePostureFlags::Camouflaged), /*Slot=*/0),
		AttrEvent(EBattleEventType::DanoAplicado, /*Ator=*/2, /*Valor=*/30,
			/*Slot=*/0, /*Alvo=*/1),
	};

	TestEqual(TEXT("Camuflagem que protegeu conta"),
		FPetAttributeProgression::ComputeGains(Protegeu, 1)
			.SkillProficiency[FPetAttributeProgression::Camouflage], 1);

	TestEqual(TEXT("Camuflagem que não protegeu NÃO conta"),
		FPetAttributeProgression::ComputeGains(NaoProtegeu, 1)
			.SkillProficiency[FPetAttributeProgression::Camouflage], 0);

	return true;
}

// Cada skill sobe SOZINHA: voar muito não destrava camuflagem. É o que torna o
// caminho do jogador legível no pet dele.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEachSkillGrowsOnItsOwnTest,
	"BattleSquare.Meta.AttributeProgression.EachSkillGrowsOnItsOwn",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEachSkillGrowsOnItsOwnTest::RunTest(const FString& Parameters)
{
	const TArray<FBattleEvent> SoVoou = {
		AttrEvent(EBattleEventType::PosturaAssumida, 1,
			static_cast<int32>(EBattlePostureFlags::Flying), /*Slot=*/0),
		AttrEvent(EBattleEventType::PosturaAssumida, 1,
			static_cast<int32>(EBattlePostureFlags::Flying), /*Slot=*/1),
	};

	const FPetAttributeGains Ganhos = FPetAttributeProgression::ComputeGains(SoVoou, 1);

	TestEqual(TEXT("Voo subiu duas vezes"),
		Ganhos.SkillProficiency[FPetAttributeProgression::Flight], 2);
	TestEqual(TEXT("Camuflagem continua zerada"),
		Ganhos.SkillProficiency[FPetAttributeProgression::Camouflage], 0);
	TestEqual(TEXT("Subsolo também"),
		Ganhos.SkillProficiency[FPetAttributeProgression::Underground], 0);

	return true;
}

// Aplicar SOMA ao que já existe, nunca substitui: o pet carrega o que treinou
// entre batalhas, e sobrescrever apagaria a progressão inteira a cada luta.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FApplyAccumulatesTest,
	"BattleSquare.Meta.AttributeProgression.ApplyAccumulates",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FApplyAccumulatesTest::RunTest(const FString& Parameters)
{
	FOwnedPetInstance Pet;
	Pet.Musculature = 10;
	Pet.Personality = -3;
	Pet.SkillProficiency[FPetAttributeProgression::Flight] = 4;

	FPetAttributeGains Ganhos;
	Ganhos.Musculature = 5;
	Ganhos.Personality = 2;
	Ganhos.SkillProficiency[FPetAttributeProgression::Flight] = 1;

	FPetAttributeProgression::Apply(Pet, Ganhos);

	TestEqual(TEXT("Musculatura somou"), Pet.Musculature, 15);
	TestEqual(TEXT("Personalidade somou, e continua negativa"), Pet.Personality, -1);
	TestEqual(TEXT("Voo somou"),
		Pet.SkillProficiency[FPetAttributeProgression::Flight], 5);

	return true;
}

// O traço chega TURNO A TURNO. Sem somar, o que valeria seria só o último
// turno — e uma batalha de dez turnos daria o atributo de um.
//
// O modo de falhar é silencioso e plausível: o pet ganha ALGUMA coisa, então
// nada parece quebrado; só o total é que está errado, e ninguém confere um
// total que nunca viu.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGainsAccumulateAcrossTurnsTest,
	"BattleSquare.Meta.AttributeProgression.GainsAccumulateAcrossTurns",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FGainsAccumulateAcrossTurnsTest::RunTest(const FString& Parameters)
{
	const TArray<FBattleEvent> PrimeiroTurno = {
		AttrEvent(EBattleEventType::AtaqueAcertou, /*Ator=*/1, /*Valor=*/30),
	};
	const TArray<FBattleEvent> SegundoTurno = {
		AttrEvent(EBattleEventType::AtaqueAcertou, /*Ator=*/1, /*Valor=*/50),
	};

	FPetAttributeGains Total;
	Total.Add(FPetAttributeProgression::ComputeGains(PrimeiroTurno, /*PetId=*/1));
	Total.Add(FPetAttributeProgression::ComputeGains(SegundoTurno, /*PetId=*/1));

	// 30/10 no primeiro turno e 50/10 no segundo: 3 + 5.
	TestEqual(TEXT("Musculatura soma os dois turnos"), Total.Musculature, 8);
	TestEqual(TEXT("Personalidade soma os dois ataques"), Total.Personality, 2);
	TestFalse(TEXT("Total com ganho não é vazio"), Total.IsEmpty());

	// Somar turno vazio não mexe em nada — turno em que o pet só andou não
	// pode zerar o que ele já tinha conquistado nos anteriores.
	FPetAttributeGains Intocado = Total;
	Intocado.Add(FPetAttributeProgression::ComputeGains({}, /*PetId=*/1));
	TestEqual(TEXT("Turno sem evento não muda a musculatura"),
		Intocado.Musculature, Total.Musculature);
	TestEqual(TEXT("Turno sem evento não muda a personalidade"),
		Intocado.Personality, Total.Personality);

	return true;
}

// A soma parcial de dano NÃO se perde no arredondamento entre turnos.
//
// 9 de dano por turno, cinco turnos: somando os ganhos dá 0, porque cada
// turno arredonda 9/10 para zero. É a diferença entre "soma os ganhos" e
// "soma o dano" — e este teste registra qual das duas está valendo.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPartialDamageRoundsPerTurnTest,
	"BattleSquare.Meta.AttributeProgression.PartialDamageRoundsPerTurn",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPartialDamageRoundsPerTurnTest::RunTest(const FString& Parameters)
{
	FPetAttributeGains Total;
	for (int32 Turno = 0; Turno < 5; ++Turno)
	{
		Total.Add(FPetAttributeProgression::ComputeGains(
			{ AttrEvent(EBattleEventType::AtaqueAcertou, /*Ator=*/1, /*Valor=*/9) },
			/*PetId=*/1));
	}

	TestEqual(TEXT("Cada turno arredonda para baixo por conta própria"),
		Total.Musculature, 0);
	TestEqual(TEXT("A personalidade, essa, conta os cinco ataques"),
		Total.Personality, 5);

	return true;
}

// A PERSONALIDADE é um eixo, e ele se parte na tradução: o lado agressivo vira
// constância de golpe, o cauteloso soma ao reflexo. Um pet no meio não ganha
// nem uma coisa nem outra — é o que faz o extremo valer.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPersonalityAxisSplitsIntoTwoNumbersTest,
	"BattleSquare.Meta.AttributeProgression.PersonalityAxisSplitsIntoTwoNumbers",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPersonalityAxisSplitsIntoTwoNumbersTest::RunTest(const FString& Parameters)
{
	FOwnedPetInstance Agressivo;
	Agressivo.Personality = 20;

	FPetState EstadoAgressivo;
	FPetAttributeProgression::ApplyToBattleState(Agressivo, EstadoAgressivo);
	TestEqual(TEXT("O agressivo bate mais constante — faixa estreitada de 20 para 10"),
		EstadoAgressivo.DamageVariancePercent, 10);
	TestEqual(TEXT("E não ganha esquiva nenhuma com isso"),
		EstadoAgressivo.ReflexDodgePercent, 0);

	FOwnedPetInstance Cauteloso;
	Cauteloso.Personality = -20;

	FPetState EstadoCauteloso;
	FPetAttributeProgression::ApplyToBattleState(Cauteloso, EstadoCauteloso);
	TestEqual(TEXT("Cautela 20 vira 5% de esquiva por reflexo"),
		EstadoCauteloso.ReflexDodgePercent, 5);
	TestEqual(TEXT("E ele bate na faixa CHEIA — cautela não dá constância"),
		EstadoCauteloso.DamageVariancePercent, 20);

	FOwnedPetInstance NoMeio;
	FPetState EstadoDoMeio;
	FPetAttributeProgression::ApplyToBattleState(NoMeio, EstadoDoMeio);
	TestEqual(TEXT("No meio do eixo, sem esquiva"), EstadoDoMeio.ReflexDodgePercent, 0);
	TestEqual(TEXT("No meio do eixo, faixa cheia"), EstadoDoMeio.DamageVariancePercent, 20);

	// O TETO da esquiva (DP-atr-07) é a amarra que impede atributo alto de
	// virar imunidade. Sem ele, cautela 400 daria 100% de esquiva.
	FOwnedPetInstance Extremo;
	Extremo.Personality = -400;

	FPetState EstadoExtremo;
	FPetAttributeProgression::ApplyToBattleState(Extremo, EstadoExtremo);
	TestEqual(TEXT("A esquiva por reflexo tem teto"), EstadoExtremo.ReflexDodgePercent, 25);

	// E o PISO da variação: dano perfeitamente previsível apagaria o acaso
	// que a fatia inteira existe para introduzir.
	FOwnedPetInstance MuitoAgressivo;
	MuitoAgressivo.Personality = 400;

	FPetState EstadoMuitoAgressivo;
	FPetAttributeProgression::ApplyToBattleState(MuitoAgressivo, EstadoMuitoAgressivo);
	TestEqual(TEXT("A variação tem piso — nunca chega a zero"),
		EstadoMuitoAgressivo.DamageVariancePercent, 5);

	return true;
}

// As TRÊS proficiências somam para o reflexo, em vez de valer a maior.
//
// Quem treinou um pouco de cada se defende melhor que quem só sabe voar — e é
// isso que dá razão para variar as posturas em vez de repetir a favorita.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEverySkillFeedsReflexesTest,
	"BattleSquare.Meta.AttributeProgression.EverySkillFeedsReflexes",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEverySkillFeedsReflexesTest::RunTest(const FString& Parameters)
{
	FOwnedPetInstance Variado;
	Variado.SkillProficiency[FPetAttributeProgression::Camouflage] = 5;
	Variado.SkillProficiency[FPetAttributeProgression::Flight] = 5;
	Variado.SkillProficiency[FPetAttributeProgression::Underground] = 6;

	// 5+5+6 = 16 de reflexo, e 16/4 = 4% de esquiva.
	FPetState EstadoVariado;
	FPetAttributeProgression::ApplyToBattleState(Variado, EstadoVariado);
	TestEqual(TEXT("As três somam: 16 de reflexo vira 4%"),
		EstadoVariado.ReflexDodgePercent, 4);

	FOwnedPetInstance Especialista;
	Especialista.SkillProficiency[FPetAttributeProgression::Flight] = 8;

	FPetState EstadoEspecialista;
	FPetAttributeProgression::ApplyToBattleState(Especialista, EstadoEspecialista);
	TestTrue(TEXT("Especialista de 8 fica ATRÁS do variado de 16"),
		EstadoEspecialista.ReflexDodgePercent < EstadoVariado.ReflexDodgePercent);

	// A esquiva é GROSSEIRA de propósito: um ponto a cada 4 de reflexo. Dois
	// pets separados por um único ponto esquivam igual, e isso é desejável —
	// diferença mínima de treino não deveria decidir partida. Está escrito
	// aqui porque a primeira versão deste teste comparou 8 contra 9, viu os
	// dois em 2% e pareceu defeito.
	FOwnedPetInstance QuaseIgual;
	QuaseIgual.SkillProficiency[FPetAttributeProgression::Flight] = 9;

	FPetState EstadoQuaseIgual;
	FPetAttributeProgression::ApplyToBattleState(QuaseIgual, EstadoQuaseIgual);
	TestEqual(TEXT("8 e 9 de reflexo dão a MESMA esquiva — a escala é grossa"),
		EstadoQuaseIgual.ReflexDodgePercent, EstadoEspecialista.ReflexDodgePercent);

	// Cautela SOMA à prática — os dois caminhos levam ao mesmo reflexo.
	FOwnedPetInstance CautelosoEPraticante;
	CautelosoEPraticante.Personality = -5;
	CautelosoEPraticante.SkillProficiency[FPetAttributeProgression::Camouflage] = 5;

	// 5 de prática + 5 de cautela = 10 de reflexo, e 10/4 = 2%.
	FPetState EstadoSomado;
	FPetAttributeProgression::ApplyToBattleState(CautelosoEPraticante, EstadoSomado);
	TestEqual(TEXT("Cautela e prática somam"), EstadoSomado.ReflexDodgePercent, 2);

	return true;
}

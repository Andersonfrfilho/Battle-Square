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

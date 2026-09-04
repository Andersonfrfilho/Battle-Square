// Copyright 2026 Anderson. All Rights Reserved.

#include "Meta/PoliceEscalation.h"
#include "Meta/PoliceEncounter.h"
#include "Meta/ArrestPenalty.h"
#include "Misc/AutomationTest.h"

/**
 * CR9 — a polícia patrulha, reconhece, prende, e ESCALA com quem a vence.
 * O caso negativo (jogador limpo passa direto) é o centro do teste, como o
 * contrapeso do aceite exige.
 */

namespace PoliciaTeste
{
	FCharacterAppearance DoBandido()
	{
		FCharacterAppearance A;
		A.HairStyle = 3; A.HairColor = 2; A.Outfit = 5; A.Face = 77;
		return A;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPoliceEscalatesWithDefeatsTest,
	"BattleSquare.Meta.Policia.EscalaComAsDerrotas",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPoliceEscalatesWithDefeatsTest::RunTest(const FString&)
{
	using namespace PoliceEscalation;

	const int32 SuspectLevel = 10;

	// O PRIMEIRO destacamento: dupla de RONDA, no nível do suspeito, degrau 0.
	FPoliceForce SemDerrota;
	SemDerrota.CopsDefeatedBySuspect = 0;
	FReinforcement Primeiro = NextReinforcement(SemDerrota, SuspectLevel);
	TestEqual(TEXT("o primeiro destacamento e par a par"), Primeiro.Tier, 0);
	TestEqual(TEXT("o primeiro pet vem no nivel do suspeito"),
		Primeiro.PetLevel, SuspectLevel);
	TestTrue(TEXT("o primeiro tipo e a ronda"),
		Primeiro.Type == EPoliceForceType::Ronda);
	TestEqual(TEXT("a policia anda SEMPRE em dois"),
		Primeiro.OfficerCount, 2);

	// A ESCADA DE TIPOS (decisão 28-c): cada derrota sobe uma patente, na ordem.
	const EPoliceForceType Escada[] = {
		EPoliceForceType::Ronda,
		EPoliceForceType::Ostensiva,
		EPoliceForceType::Investigadora,
		EPoliceForceType::Elite,
		EPoliceForceType::Federal,
	};
	int32 NivelAnterior = -1;
	for (int32 Derrotas = 0; Derrotas < 5; ++Derrotas)
	{
		FPoliceForce Force;
		Force.CopsDefeatedBySuspect = Derrotas;
		FReinforcement Reforco = NextReinforcement(Force, SuspectLevel);
		TestTrue(TEXT("a patente sobe na ordem da escada"),
			Reforco.Type == Escada[Derrotas]);
		TestTrue(TEXT("cada derrota deixa o proximo ESTRITAMENTE mais forte"),
			Reforco.PetLevel > NivelAnterior);
		TestEqual(TEXT("todo destacamento e uma dupla"), Reforco.OfficerCount, 2);
		NivelAnterior = Reforco.PetLevel;
	}

	// O TOPO: a escada TRAVA na federal, mas o nivel dos pets segue subindo —
	// federal nunca vira briga facil.
	FPoliceForce Muitas; Muitas.CopsDefeatedBySuspect = 9;
	FReinforcement AlemDoTopo = NextReinforcement(Muitas, SuspectLevel);
	TestTrue(TEXT("acima do topo a patente TRAVA na federal"),
		AlemDoTopo.Type == EPoliceForceType::Federal);
	TestTrue(TEXT("mas o nivel dos pets continua subindo alem do topo"),
		AlemDoTopo.PetLevel > NextReinforcement(
			[]{ FPoliceForce F; F.CopsDefeatedBySuspect = 4; return F; }(),
			SuspectLevel).PetLevel);

	// DETERMINÍSTICO: mesmo histórico, mesma polícia — sem relógio no meio.
	FPoliceForce Tres; Tres.CopsDefeatedBySuspect = 3;
	TestTrue(TEXT("mesmo historico produz a mesma patente"),
		NextReinforcement(Tres, SuspectLevel).Type ==
		NextReinforcement(Tres, SuspectLevel).Type);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPoliceReactsAndIgnoresTest,
	"BattleSquare.Meta.Policia.PrendeQuemBateEIgnoraOResto",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPoliceReactsAndIgnoresTest::RunTest(const FString&)
{
	using namespace PoliceEncounter;

	// O CASO POSITIVO: dentro do alcance, aparência do cartaz E o mesmo pet.
	FEncounter Bate;
	Bate.bWithinPatrolRange = true;
	Bate.bPetMatchesWanted = true;
	Bate.PosterAppearance = PoliciaTeste::DoBandido();
	Bate.SuspectAppearance = PoliciaTeste::DoBandido();
	TestTrue(TEXT("o procurado reconhecido e preso"),
		Decide(Bate) == EPoliceAction::Arrest);

	// O CONTRAPESO — o coração do teste: um jogador LIMPO (aparência diferente)
	// passa direto, mesmo dentro do alcance. Sem isto, "reage" e "sempre ativo"
	// seriam indistinguiveis.
	FEncounter Limpo = Bate;
	Limpo.SuspectAppearance.Face = 1;   // outro rosto = não é o do cartaz
	Limpo.SuspectAppearance.HairStyle = 9;
	TestTrue(TEXT("jogador limpo passa direto pela ronda"),
		Decide(Limpo) == EPoliceAction::Ignore);

	// UMA coincidência só NÃO basta: mesmo rosto do cartaz, mas OUTRO pet.
	// A decisão 26 exige as duas — é o que torna o erro raro.
	FEncounter SoAparencia = Bate;
	SoAparencia.bPetMatchesWanted = false;
	TestTrue(TEXT("mesma aparencia mas outro pet nao prende"),
		Decide(SoAparencia) == EPoliceAction::Ignore);

	// FORA DO ALCANCE: nem o procurado perfeito é tocado.
	FEncounter Longe = Bate;
	Longe.bWithinPatrolRange = false;
	TestTrue(TEXT("fora do alcance a policia ignora"),
		Decide(Longe) == EPoliceAction::Ignore);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArrestAlwaysRepaysVictimTest,
	"BattleSquare.Meta.Policia.APrisaoRessarceSempre",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArrestAlwaysRepaysVictimTest::RunTest(const FString&)
{
	using namespace ArrestPenalty;

	// COM saldo: paga a multa, cumpre o tempo-base, ressarce o roubado.
	FArrestInput ComSaldo;
	ComSaldo.SuspectBalance = 500;
	ComSaldo.FineAmount = 200;
	ComSaldo.BaseLockTurns = 10;
	ComSaldo.VictimLoss = 150;
	FArrestResult R1 = Apply(ComSaldo);
	TestTrue(TEXT("confisca o pet"), R1.bPetConfiscated);
	TestEqual(TEXT("tempo-base quando paga"), R1.LockTurns, 10);
	TestEqual(TEXT("pagou a multa cheia"), R1.FinePaid, 200);
	TestEqual(TEXT("o roubado recebe o que perdeu"), R1.ToVictim, 150);
	TestFalse(TEXT("o governo nao precisou cobrir"), R1.bGovernmentCovered);

	// SEM saldo (decisão 28): DOBRA o tempo, não paga — e ainda assim o roubado
	// recebe tudo, porque o GOVERNO cobre. A garantia que amarra 27 e 28.
	FArrestInput SemSaldo;
	SemSaldo.SuspectBalance = 0;
	SemSaldo.FineAmount = 200;
	SemSaldo.BaseLockTurns = 10;
	SemSaldo.VictimLoss = 150;
	FArrestResult R2 = Apply(SemSaldo);
	TestEqual(TEXT("sem saldo o tempo DOBRA"), R2.LockTurns, 20);
	TestEqual(TEXT("nada pago quando nao ha saldo"), R2.FinePaid, 0);
	TestEqual(TEXT("o roubado recebe o que perdeu MESMO ASSIM"),
		R2.ToVictim, 150);
	TestTrue(TEXT("o governo cobriu a diferenca"), R2.bGovernmentCovered);

	return true;
}

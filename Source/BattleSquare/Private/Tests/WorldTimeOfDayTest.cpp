// Copyright 2026 Anderson. All Rights Reserved.

#include "Environment/WorldTimeOfDay.h"

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** As quatro fases, para varrer sem repetir a lista em cada teste. */
	const EDayPhase FasesDoDiaDoRelogio[] = {
		EDayPhase::Dawn, EDayPhase::Day, EDayPhase::Dusk, EDayPhase::Night
	};

	const EPetActivity AtividadesDoRelogio[] = {
		EPetActivity::Diurnal, EPetActivity::Crepuscular, EPetActivity::Nocturnal
	};

	/** A fase em que cada atividade deveria mandar. */
	EDayPhase FasePreferidaDoRelogio(EPetActivity Atividade)
	{
		switch (Atividade)
		{
		case EPetActivity::Diurnal:     return EDayPhase::Day;
		case EPetActivity::Crepuscular: return EDayPhase::Dawn;
		case EPetActivity::Nocturnal:   return EDayPhase::Night;
		}
		return EDayPhase::Day;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWorldTimeOfDayHourWrapsAroundTheDay,
	"BattleSquare.Environment.WorldTimeOfDay.HoraDaVoltaAoComecar",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldTimeOfDayHourWrapsAroundTheDay::RunTest(const FString& Parameters)
{
	const float Ciclo = WorldTimeOfDay::DefaultSecondsPerDay;

	TestEqual(TEXT("comeca a meia-noite"), WorldTimeOfDay::HourAt(0.0f), 0.0f);
	TestEqual(TEXT("metade do ciclo e meio-dia"), WorldTimeOfDay::HourAt(Ciclo * 0.5f), 12.0f, 0.01f);

	// Um ciclo inteiro devolve a mesma hora: sem isto, uma sessao longa
	// acordaria na hora 73 e o sol nunca mais desceria.
	TestEqual(TEXT("um dia depois volta ao inicio"), WorldTimeOfDay::HourAt(Ciclo), 0.0f, 0.01f);
	TestEqual(TEXT("tres dias depois tambem"), WorldTimeOfDay::HourAt(Ciclo * 3.25f), 6.0f, 0.01f);

	// Tempo negativo acontece quando alguem quer comecar a partida ja de
	// noite: a hora precisa cair dentro do dia, nao virar numero negativo.
	const float HoraAntes = WorldTimeOfDay::HourAt(-Ciclo * 0.25f);
	TestTrue(TEXT("tempo negativo cai dentro do dia"), HoraAntes >= 0.0f && HoraAntes < 24.0f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWorldTimeOfDayPhaseAgreesWithTheSun,
	"BattleSquare.Environment.WorldTimeOfDay.FaseConcordaComOSol",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldTimeOfDayPhaseAgreesWithTheSun::RunTest(const FString& Parameters)
{
	// O defeito que este teste existe para impedir e o mais confuso de todos:
	// o painel dizendo "noite" com o sol a pino, porque a tabela de fases e a
	// conta do sol seguiram caminhos separados (L-032).
	TestEqual(TEXT("meio-dia e dia"), WorldTimeOfDay::PhaseAtHour(12.0f), EDayPhase::Day);
	TestEqual(TEXT("meia-noite e noite"), WorldTimeOfDay::PhaseAtHour(0.0f), EDayPhase::Night);
	TestEqual(TEXT("seis da manha e amanhecer"), WorldTimeOfDay::PhaseAtHour(6.0f), EDayPhase::Dawn);
	TestEqual(TEXT("dezoito e entardecer"), WorldTimeOfDay::PhaseAtHour(18.0f), EDayPhase::Dusk);

	// Varredura de hora em hora: sempre que a fase disser Dia, o sol tem de
	// estar acima da linha; sempre que disser Noite, no maximo raspando.
	for (int32 Hora = 0; Hora < 24; ++Hora)
	{
		const float Elevacao = WorldTimeOfDay::SunElevationDegrees(static_cast<float>(Hora));
		const EDayPhase Fase = WorldTimeOfDay::PhaseAtHour(static_cast<float>(Hora));

		if (Fase == EDayPhase::Day)
		{
			TestTrue(FString::Printf(TEXT("hora %d e dia e o sol esta em cima"), Hora), Elevacao > 0.0f);
		}
		if (Fase == EDayPhase::Night)
		{
			TestTrue(FString::Printf(TEXT("hora %d e noite e o sol nao esta a pino"), Hora), Elevacao < 30.0f);
		}
	}

	// O sol cruza o horizonte as 6 e as 18, e isso sai da conta, nao de um if.
	TestEqual(TEXT("as 6 o sol esta no horizonte"), WorldTimeOfDay::SunElevationDegrees(6.0f), 0.0f, 0.01f);
	TestEqual(TEXT("ao meio-dia o sol esta a pino"), WorldTimeOfDay::SunElevationDegrees(12.0f), 90.0f, 0.01f);
	TestEqual(TEXT("as 18 o sol esta no horizonte"), WorldTimeOfDay::SunElevationDegrees(18.0f), 0.0f, 0.01f);
	TestTrue(TEXT("a meia-noite o sol esta embaixo"), WorldTimeOfDay::SunElevationDegrees(0.0f) < -80.0f);

	// A inclinacao da luz e o oposto da elevacao. Errar o sinal ilumina a cena
	// de baixo para cima, e o defeito parece ser do material.
	for (int32 Hora = 0; Hora < 24; ++Hora)
	{
		const float H = static_cast<float>(Hora);
		TestEqual(
			FString::Printf(TEXT("hora %d: inclinacao e o oposto da elevacao"), Hora),
			WorldTimeOfDay::SunPitchDegrees(H),
			-WorldTimeOfDay::SunElevationDegrees(H),
			0.001f);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWorldTimeOfDaySunCrossesTheSkyFromSideToSide,
	"BattleSquare.Environment.WorldTimeOfDay.SolAtravessaODeUmLadoAoOutro",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldTimeOfDaySunCrossesTheSkyFromSideToSide::RunTest(const FString& Parameters)
{
	// Nasce no leste, cruza o sul, se poe no oeste.
	TestEqual(TEXT("as 6 nasce no leste"), WorldTimeOfDay::SunAzimuthDegrees(6.0f), 90.0f, 0.01f);
	TestEqual(TEXT("ao meio-dia cruza o sul"), WorldTimeOfDay::SunAzimuthDegrees(12.0f), 180.0f, 0.01f);
	TestEqual(TEXT("as 18 se poe no oeste"), WorldTimeOfDay::SunAzimuthDegrees(18.0f), 270.0f, 0.01f);

	// O lado do amanhecer nao pode ser o lado do entardecer: se fosse, a
	// sombra da montanha cairia duas vezes no mesmo lugar.
	const float LadoDoAmanhecer = WorldTimeOfDay::SunAzimuthDegrees(6.0f);
	const float LadoDoEntardecer = WorldTimeOfDay::SunAzimuthDegrees(18.0f);
	TestTrue(TEXT("amanhecer e entardecer vem de lados opostos"),
		FMath::Abs(LadoDoEntardecer - LadoDoAmanhecer) > 90.0f);

	// Toda hora tem lado dentro da volta, sem angulo negativo escapando.
	for (int32 Passo = 0; Passo < 48; ++Passo)
	{
		const float Hora = Passo * 0.5f;
		const float Lado = WorldTimeOfDay::SunAzimuthDegrees(Hora);
		TestTrue(FString::Printf(TEXT("lado valido na hora %.1f"), Hora), Lado >= 0.0f && Lado < 360.0f);
	}

	// A rotacao inteira e feita das duas contas, nao de uma terceira.
	for (int32 Hora = 0; Hora < 24; ++Hora)
	{
		const float H = static_cast<float>(Hora);
		// `FRotator` guarda `double` na UE5; o cast diz qual sobrecarga de
		// `TestEqual` vale, em vez de deixar o compilador escolher (e ele nao
		// escolhe: para de compilar).
		const FRotator Giro = WorldTimeOfDay::SunRotation(H);
		TestEqual(FString::Printf(TEXT("hora %d: inclinacao bate"), Hora),
			static_cast<float>(Giro.Pitch), WorldTimeOfDay::SunPitchDegrees(H), 0.001f);
		TestEqual(FString::Printf(TEXT("hora %d: lado bate"), Hora),
			static_cast<float>(Giro.Yaw), WorldTimeOfDay::SunAzimuthDegrees(H), 0.001f);
		TestEqual(FString::Printf(TEXT("hora %d: sem rolagem"), Hora),
			static_cast<float>(Giro.Roll), 0.0f, 0.001f);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWorldTimeOfDayNightIsDarkAndNoonIsBright,
	"BattleSquare.Environment.WorldTimeOfDay.NoiteEscuraMeioDiaClaro",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldTimeOfDayNightIsDarkAndNoonIsBright::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("a meia-noite e noite"), WorldTimeOfDay::IsNight(0.0f));
	TestFalse(TEXT("ao meio-dia nao e noite"), WorldTimeOfDay::IsNight(12.0f));

	TestEqual(TEXT("brilho maximo ao meio-dia"), WorldTimeOfDay::SunBrightness(12.0f), 1.0f, 0.01f);

	const float BrilhoDaNoite = WorldTimeOfDay::SunBrightness(0.0f);
	TestTrue(TEXT("a noite e escura"), BrilhoDaNoite < 0.1f);

	// Nunca zero: noite cega nao deixa ninguem andar, e o jogador conclui que
	// o jogo travou em vez de que anoiteceu.
	TestTrue(TEXT("a noite ainda deixa o luar"), BrilhoDaNoite > 0.0f);

	// O brilho so cresce da meia-noite ao meio-dia. Um vale no meio seria uma
	// piscada de escuridao a plena luz.
	float Anterior = WorldTimeOfDay::SunBrightness(0.0f);
	for (int32 Passo = 1; Passo <= 24; ++Passo)
	{
		const float Hora = Passo * 0.5f;
		const float Atual = WorldTimeOfDay::SunBrightness(Hora);
		TestTrue(
			FString::Printf(TEXT("brilho nao cai antes do meio-dia (hora %.1f)"), Hora),
			Atual >= Anterior - 0.001f);
		Anterior = Atual;
	}

	// Toda hora produz cor valida e sem componente negativa.
	for (int32 Hora = 0; Hora < 24; ++Hora)
	{
		const FLinearColor Cor = WorldTimeOfDay::SunColor(static_cast<float>(Hora));
		TestTrue(
			FString::Printf(TEXT("hora %d tem cor valida"), Hora),
			Cor.R >= 0.0f && Cor.G >= 0.0f && Cor.B >= 0.0f);
	}

	// A luz da noite e fria e a do entardecer e quente: e a cor, nao o relogio
	// na tela, que conta a hora para quem esta jogando.
	const FLinearColor CorDaNoite = WorldTimeOfDay::SunColor(0.0f);
	TestTrue(TEXT("a noite puxa para o azul"), CorDaNoite.B > CorDaNoite.R);

	const FLinearColor CorDoEntardecer = WorldTimeOfDay::SunColor(18.2f);
	TestTrue(TEXT("o entardecer puxa para o laranja"), CorDoEntardecer.R > CorDoEntardecer.B);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWorldTimeOfDayEachActivityPeaksInItsOwnHour,
	"BattleSquare.Environment.WorldTimeOfDay.CadaBichoNaSuaHora",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldTimeOfDayEachActivityPeaksInItsOwnHour::RunTest(const FString& Parameters)
{
	for (const EPetActivity Atividade : AtividadesDoRelogio)
	{
		const EDayPhase Preferida = FasePreferidaDoRelogio(Atividade);
		const int32 NoAuge = WorldTimeOfDay::EncounterWeightPercent(Atividade, Preferida);

		for (const EDayPhase Fase : FasesDoDiaDoRelogio)
		{
			const int32 Peso = WorldTimeOfDay::EncounterWeightPercent(Atividade, Fase);

			// Nenhum peso e zero: bicho impossivel fora da hora dele vira
			// parede, e quem joga de noite nunca completaria a colecao diurna.
			TestTrue(TEXT("nenhum peso e zero"), Peso > 0);
			TestTrue(TEXT("nenhum peso passa de cem"), Peso <= 100);
			TestTrue(TEXT("nenhuma fase supera a hora preferida"), Peso <= NoAuge);
		}

		TestEqual(TEXT("na hora dele o peso e cheio"), NoAuge, 100);
	}

	// O noturno tem de ser mais raro de dia do que o diurno, senao a atividade
	// e enfeite de ficha e o ciclo do dia nao muda nada no que se encontra.
	TestTrue(
		TEXT("de dia o diurno aparece mais que o noturno"),
		WorldTimeOfDay::EncounterWeightPercent(EPetActivity::Diurnal, EDayPhase::Day) >
		WorldTimeOfDay::EncounterWeightPercent(EPetActivity::Nocturnal, EDayPhase::Day));

	TestTrue(
		TEXT("de noite o noturno aparece mais que o diurno"),
		WorldTimeOfDay::EncounterWeightPercent(EPetActivity::Nocturnal, EDayPhase::Night) >
		WorldTimeOfDay::EncounterWeightPercent(EPetActivity::Diurnal, EDayPhase::Night));

	// O tardio tem os dois crepusculos como auge, nao so um.
	TestEqual(
		TEXT("o tardio manda tambem no entardecer"),
		WorldTimeOfDay::EncounterWeightPercent(EPetActivity::Crepuscular, EDayPhase::Dusk),
		100);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWorldTimeOfDayActivityNameFallsBackToDiurnal,
	"BattleSquare.Environment.WorldTimeOfDay.NomeDesconhecidoViraDiurno",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldTimeOfDayActivityNameFallsBackToDiurnal::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Diurnal"), WorldTimeOfDay::ActivityFromName(TEXT("Diurnal")), EPetActivity::Diurnal);
	TestEqual(TEXT("Crepuscular"), WorldTimeOfDay::ActivityFromName(TEXT("Crepuscular")), EPetActivity::Crepuscular);
	TestEqual(TEXT("Nocturnal"), WorldTimeOfDay::ActivityFromName(TEXT("Nocturnal")), EPetActivity::Nocturnal);

	// Erro de digitacao na ficha some com o bicho da manha, nao do jogo.
	TestEqual(TEXT("vazio"), WorldTimeOfDay::ActivityFromName(NAME_None), EPetActivity::Diurnal);
	TestEqual(TEXT("besteira"), WorldTimeOfDay::ActivityFromName(TEXT("Nocturnl")), EPetActivity::Diurnal);

	for (const EDayPhase Fase : FasesDoDiaDoRelogio)
	{
		TestTrue(
			TEXT("toda fase tem nome para o painel"),
			FCString::Strlen(WorldTimeOfDay::PhaseDebugName(Fase)) > 0);
	}

	return true;
}

#endif

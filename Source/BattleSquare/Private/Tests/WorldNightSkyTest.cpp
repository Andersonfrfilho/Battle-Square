// Copyright 2026 Anderson. All Rights Reserved.

#include "Environment/WorldNightSky.h"

#include "Environment/ScenaryClimate.h"
#include "Environment/WorldTimeOfDay.h"
#include "Environment/WorldWeather.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Uma semente de mundo estável, para o céu dos testes ser sempre o mesmo. */
	constexpr uint32 SementeDoCeuNoturno = 20260831u;

	/** Quantos dias varrer quando o teste quer medir frequência, e não um caso. */
	constexpr int32 DiasVarridosDoCeuNoturno = 150;

	/** De quinze em quinze minutos: eclipse dura horas, e passo de uma hora já pula alguns. */
	constexpr float PassoEmHorasDoCeuNoturno = 0.25f;

	/** A hora do dia dentro de um total de horas corridas. */
	float HoraDoDiaNoCeuNoturno(float HorasCorridas)
	{
		return FMath::Fmod(HorasCorridas, WorldTimeOfDay::HoursPerDay);
	}

	/** Quantos passos a varredura tem. */
	int32 PassosDaVarreduraDoCeuNoturno()
	{
		return FMath::RoundToInt(DiasVarridosDoCeuNoturno * WorldTimeOfDay::HoursPerDay / PassoEmHorasDoCeuNoturno);
	}

	const EMoonPhase FasesDoCeuNoturno[] = {
		EMoonPhase::New, EMoonPhase::WaxingCrescent, EMoonPhase::FirstQuarter, EMoonPhase::WaxingGibbous,
		EMoonPhase::Full, EMoonPhase::WaningGibbous, EMoonPhase::LastQuarter, EMoonPhase::WaningCrescent
	};

	const ESkyEclipse EclipsesDoCeuNoturno[] = {
		ESkyEclipse::None, ESkyEclipse::Lunar, ESkyEclipse::Solar
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWorldNightSkyPhaseGoesAroundTheMonth,
	"BattleSquare.Environment.WorldNightSky.AFaseFechaOMes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldNightSkyPhaseGoesAroundTheMonth::RunTest(const FString& Parameters)
{
	const float MesEmHoras = WorldNightSky::SynodicMonthDays * WorldTimeOfDay::HoursPerDay;

	TestEqual(TEXT("no primeiro dia a lua esta cheia"),
		WorldNightSky::MoonPhaseFraction(0.0f), WorldNightSky::PhaseAtDayZero, 0.001f);

	TestEqual(TEXT("um mes depois a fase e a mesma"),
		WorldNightSky::MoonPhaseFraction(MesEmHoras), WorldNightSky::MoonPhaseFraction(0.0f), 0.001f);

	TestEqual(TEXT("meio mes depois a lua esta nova"),
		WorldNightSky::MoonPhaseFraction(MesEmHoras * 0.5f), 0.0f, 0.001f);

	// A fase passa por TODAS as oito, e não só pelas quatro famosas.
	TSet<uint8> Vistas;
	for (int32 Passo = 0; Passo < 400; ++Passo)
	{
		const float Horas = Passo * MesEmHoras / 400.0f;
		Vistas.Add(static_cast<uint8>(WorldNightSky::PhaseOf(WorldNightSky::MoonPhaseFraction(Horas))));
	}
	TestEqual(TEXT("o mes passa pelas oito fases"), Vistas.Num(), static_cast<int32>(UE_ARRAY_COUNT(FasesDoCeuNoturno)));

	TestEqual(TEXT("fracao 0 e lua nova"),
		static_cast<int32>(WorldNightSky::PhaseOf(0.0f)), static_cast<int32>(EMoonPhase::New));
	TestEqual(TEXT("fracao 0,5 e lua cheia"),
		static_cast<int32>(WorldNightSky::PhaseOf(0.5f)), static_cast<int32>(EMoonPhase::Full));
	TestEqual(TEXT("fracao 1 volta a ser lua nova"),
		static_cast<int32>(WorldNightSky::PhaseOf(1.0f)), static_cast<int32>(EMoonPhase::New));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWorldNightSkyFullMoonOwnsTheMidnight,
	"BattleSquare.Environment.WorldNightSky.ACheiaMandaNaMeiaNoite",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldNightSkyFullMoonOwnsTheMidnight::RunTest(const FString& Parameters)
{
	// A lua cheia é a que está do lado oposto ao sol: ela sobe quando ele se
	// põe, e isso sai do ATRASO, não de uma tabela de horários.
	TestEqual(TEXT("a lua cheia atrasa meio dia"),
		WorldNightSky::MoonRiseLagHours(0.5f), WorldTimeOfDay::HoursPerDay * 0.5f, 0.01f);
	TestEqual(TEXT("a lua nova nao atrasa nada"),
		WorldNightSky::MoonRiseLagHours(0.0f), 0.0f, 0.01f);

	TestTrue(TEXT("a lua cheia esta no ceu a meia-noite"), WorldNightSky::IsMoonUp(0.0f, 0.5f));
	TestFalse(TEXT("a lua cheia nao esta no ceu ao meio-dia"), WorldNightSky::IsMoonUp(12.0f, 0.5f));

	TestTrue(TEXT("a lua nova esta no ceu ao meio-dia"), WorldNightSky::IsMoonUp(12.0f, 0.0f));
	TestFalse(TEXT("a lua nova nao esta no ceu a meia-noite"), WorldNightSky::IsMoonUp(0.0f, 0.0f));

	TestTrue(TEXT("a meia-noite a cheia esta no alto"),
		WorldNightSky::MoonElevationDegrees(0.0f, 0.5f) > 80.0f);

	// Quanto da lua está aceso é o mesmo desenho, do escuro ao cheio.
	TestEqual(TEXT("a lua nova nao tem luz"), WorldNightSky::MoonLitFraction(0.0f), 0.0f, 0.001f);
	TestEqual(TEXT("a lua cheia tem toda a luz"), WorldNightSky::MoonLitFraction(0.5f), 1.0f, 0.001f);
	TestEqual(TEXT("o quarto tem metade"), WorldNightSky::MoonLitFraction(0.25f), 0.5f, 0.001f);

	// Brilho é luz acesa VEZES altura: lua abaixo do horizonte não ilumina.
	TestEqual(TEXT("lua fora do ceu nao ilumina"),
		WorldNightSky::MoonBrightness(12.0f, 0.5f), 0.0f, 0.001f);
	TestTrue(TEXT("a cheia no alto ilumina quase tudo"),
		WorldNightSky::MoonBrightness(0.0f, 0.5f) > 0.95f);
	TestTrue(TEXT("a noite de cheia e mais clara que a de crescente fina"),
		WorldNightSky::MoonBrightness(0.0f, 0.5f) > WorldNightSky::MoonBrightness(21.0f, 0.125f));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWorldNightSkyEclipseIsRareAndArrivesAnyway,
	"BattleSquare.Environment.WorldNightSky.OEclipseERaroMasVem",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldNightSkyEclipseIsRareAndArrivesAnyway::RunTest(const FString& Parameters)
{
	int32 Lunares = 0;
	int32 Solares = 0;
	int32 Passos = 0;
	float MaiorProfundidadeLunar = 0.0f;
	bool bViuRampa = false;

	for (int32 Passo = 0; Passo < PassosDaVarreduraDoCeuNoturno(); ++Passo)
	{
		const float Horas = Passo * PassoEmHorasDoCeuNoturno;
		const float Hora = HoraDoDiaNoCeuNoturno(Horas);
		const ESkyEclipse Qual = WorldNightSky::EclipseAt(Hora, Horas);
		const float Profundidade = WorldNightSky::EclipseDepth(Hora, Horas);
		++Passos;

		if (Qual == ESkyEclipse::Lunar)
		{
			++Lunares;
			MaiorProfundidadeLunar = FMath::Max(MaiorProfundidadeLunar, Profundidade);
		}
		if (Qual == ESkyEclipse::Solar)
		{
			++Solares;
		}
		if (Qual != ESkyEclipse::None && Profundidade > 0.01f && Profundidade < 0.3f)
		{
			bViuRampa = true;
		}
		if (Qual == ESkyEclipse::None)
		{
			TestEqual(TEXT("sem eclipse a profundidade e zero"), Profundidade, 0.0f, 0.0001f);
		}
	}

	// Os dois EXISTEM. Um céu que só sabe fazer eclipse lunar entrega metade
	// do que foi pedido, e passaria calado num teste que só olha "aconteceu".
	TestTrue(TEXT("o eclipse lunar acontece"), Lunares > 0);
	TestTrue(TEXT("o eclipse solar acontece"), Solares > 0);

	// E são RAROS: a raridade é a razão de existirem.
	TestTrue(TEXT("o eclipse ocupa menos de 5 por cento do tempo"),
		(Lunares + Solares) < Passos / 20);

	// Alguma vez ele é FUNDO — eclipse que nunca passa de raspão não vira lua
	// vermelha nenhuma na tela.
	TestTrue(TEXT("algum eclipse lunar e fundo"), MaiorProfundidadeLunar > 0.7f);

	// E entra e sai por rampa, em vez de aparecer inteiro de uma vez.
	TestTrue(TEXT("o eclipse entra por rampa"), bViuRampa);

	// Nada de eclipse na primeira noite: ele nasceria como rotina.
	for (int32 Passo = 0; Passo < 96; ++Passo)
	{
		const float Horas = Passo * PassoEmHorasDoCeuNoturno;
		TestEqual(TEXT("a primeira noite nao tem eclipse"),
			static_cast<int32>(WorldNightSky::EclipseAt(HoraDoDiaNoCeuNoturno(Horas), Horas)),
			static_cast<int32>(ESkyEclipse::None));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWorldNightSkyBloodMoonIsTheEclipseItself,
	"BattleSquare.Environment.WorldNightSky.ALuaVermelhaEOEclipse",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldNightSkyBloodMoonIsTheEclipseItself::RunTest(const FString& Parameters)
{
	bool bViuVermelha = false;
	bool bDivergiu = false;
	bool bViuCorMudada = false;

	for (int32 Passo = 0; Passo < PassosDaVarreduraDoCeuNoturno(); ++Passo)
	{
		const float Horas = Passo * PassoEmHorasDoCeuNoturno;
		const float Hora = HoraDoDiaNoCeuNoturno(Horas);
		const bool bEclipsada = WorldNightSky::EclipseAt(Hora, Horas) == ESkyEclipse::Lunar;
		const bool bVermelha = WorldNightSky::IsBloodMoon(Hora, Horas);

		if (bEclipsada != bVermelha)
		{
			bDivergiu = true;
		}
		if (bVermelha)
		{
			bViuVermelha = true;
			const FLinearColor Cor = WorldNightSky::MoonColor(Hora, Horas);
			if (Cor.R > Cor.B)
			{
				bViuCorMudada = true;
			}
		}
		else
		{
			// Fora do eclipse a lua é pérola: mais azulada que vermelha.
			const FLinearColor Cor = WorldNightSky::MoonColor(Hora, Horas);
			TestTrue(TEXT("fora do eclipse a lua nao e vermelha"), Cor.B >= Cor.R);
		}
	}

	TestTrue(TEXT("a lua vermelha acontece"), bViuVermelha);
	TestFalse(TEXT("lua vermelha e eclipse lunar nunca divergem"), bDivergiu);
	TestTrue(TEXT("no fundo do eclipse a lua fica avermelhada"), bViuCorMudada);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWorldNightSkySolarEclipseDarkensTheNoon,
	"BattleSquare.Environment.WorldNightSky.OSolarEscureceOMeioDia",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldNightSkySolarEclipseDarkensTheNoon::RunTest(const FString& Parameters)
{
	float MaiorCobertura = 0.0f;
	bool bCoberturaForaDoSolar = false;

	for (int32 Passo = 0; Passo < PassosDaVarreduraDoCeuNoturno(); ++Passo)
	{
		const float Horas = Passo * PassoEmHorasDoCeuNoturno;
		const float Hora = HoraDoDiaNoCeuNoturno(Horas);
		const float Cobertura = WorldNightSky::SolarEclipseCoverage(Hora, Horas);
		const bool bSolar = WorldNightSky::EclipseAt(Hora, Horas) == ESkyEclipse::Solar;

		if (Cobertura > 0.0f && !bSolar)
		{
			bCoberturaForaDoSolar = true;
		}
		if (bSolar)
		{
			MaiorCobertura = FMath::Max(MaiorCobertura, Cobertura);

			// Eclipse solar é de DIA, e o motivo é que ele TAPA o sol: sem sol
			// no céu não há o que tapar. Deduzir isso de a lua nova subir junto
			// com o sol quase funciona, e o quase deixava o eclipse cair antes
			// do amanhecer.
			TestTrue(TEXT("o eclipse solar acontece de dia"),
				WorldTimeOfDay::SunElevationDegrees(Hora) > 0.0f);
		}
	}

	TestFalse(TEXT("so o eclipse solar cobre o sol"), bCoberturaForaDoSolar);
	TestTrue(TEXT("o sol chega a ser coberto de verdade"), MaiorCobertura > 0.3f);
	TestTrue(TEXT("a cobertura nunca passa de um"), MaiorCobertura <= 1.0f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWorldNightSkyStarsWaitForTheDarkAndTheCloudsHideThem,
	"BattleSquare.Environment.WorldNightSky.AEstrelaEsperaOEscuro",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldNightSkyStarsWaitForTheDarkAndTheCloudsHideThem::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("ao meio-dia nao ha estrela"),
		WorldNightSky::StarBrightness(12.0f, EWeather::Clear), 0.0f, 0.001f);
	TestEqual(TEXT("com o sol no horizonte ainda nao ha estrela"),
		WorldNightSky::StarBrightness(18.0f, EWeather::Clear), 0.0f, 0.001f);

	TestTrue(TEXT("a meia-noite limpa o ceu esta cheio de estrela"),
		WorldNightSky::StarBrightness(0.0f, EWeather::Clear) > 0.9f);

	// A nuvem tapa: é a mesma cobertura que o clima já declara, não uma
	// segunda tabela de opacidade.
	TestTrue(TEXT("nublado apaga parte das estrelas"),
		WorldNightSky::StarBrightness(0.0f, EWeather::Cloudy) < WorldNightSky::StarBrightness(0.0f, EWeather::Clear));
	TestEqual(TEXT("na tempestade nao se ve estrela"),
		WorldNightSky::StarBrightness(0.0f, EWeather::Storm), 0.0f, 0.001f);

	// O crepúsculo é uma rampa, e não um interruptor numa hora cheia. Varre-se
	// o anoitecer em passo fino em vez de cravar uma hora: a rampa dura o que
	// o sol demora para descer os 18 graus, e quem mexer na curva do sol move a
	// janela junto — cravar 20h aqui foi exatamente o que falhou, porque às 20h
	// o céu já está preto.
	const float CeuDeMeiaNoite = WorldNightSky::StarBrightness(0.0f, EWeather::Clear);
	bool bViuMeiaEstrela = false;
	for (int32 Passo = 0; Passo <= 240; ++Passo)
	{
		const float Hora = 12.0f + Passo * 0.05f;
		const float Brilho = WorldNightSky::StarBrightness(Hora, EWeather::Clear);
		if (Brilho > 0.0f && Brilho < CeuDeMeiaNoite)
		{
			bViuMeiaEstrela = true;
			break;
		}
	}
	TestTrue(TEXT("as estrelas aparecem aos poucos"), bViuMeiaEstrela);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWorldNightSkyCometComesBackOnItsOwnTime,
	"BattleSquare.Environment.WorldNightSky.OCometaVoltaNaHoraDele",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldNightSkyCometComesBackOnItsOwnTime::RunTest(const FString& Parameters)
{
	const int32 Periodos = 4;
	const int32 PassosPorDia = FMath::RoundToInt(WorldTimeOfDay::HoursPerDay / PassoEmHorasDoCeuNoturno);
	const int32 TotalDePassos = FMath::RoundToInt(Periodos * WorldNightSky::CometPeriodDays) * PassosPorDia;

	TArray<int32> PassagensVistas;
	int32 PassosVisiveis = 0;

	for (int32 Passo = 0; Passo < TotalDePassos; ++Passo)
	{
		const float Horas = Passo * PassoEmHorasDoCeuNoturno;
		if (!WorldNightSky::CometVisible(SementeDoCeuNoturno, Horas))
		{
			continue;
		}
		++PassosVisiveis;
		const int32 Periodo = FMath::FloorToInt(WorldNightSky::ElapsedDays(Horas) / WorldNightSky::CometPeriodDays);
		PassagensVistas.AddUnique(Periodo);
	}

	TestTrue(TEXT("o cometa aparece em todos os periodos varridos"), PassagensVistas.Num() >= Periodos - 1);

	// Ele fica por perto DOIS dias, não um instante nem um mês.
	const float DiasVisiveisPorPassagem = PassosVisiveis / static_cast<float>(PassosPorDia * PassagensVistas.Num());
	TestEqual(TEXT("cada passagem dura os dias declarados"),
		DiasVisiveisPorPassagem, WorldNightSky::CometVisibleDays, 0.2f);

	// A mesma semente responde a mesma coisa: o céu é reconstruível.
	for (int32 Passo = 0; Passo < 400; ++Passo)
	{
		const float Horas = Passo * WorldTimeOfDay::HoursPerDay * 0.5f;
		TestEqual(TEXT("a mesma semente da o mesmo cometa"),
			WorldNightSky::CometVisible(SementeDoCeuNoturno, Horas),
			WorldNightSky::CometVisible(SementeDoCeuNoturno, Horas));
	}

	// E sementes diferentes trazem o cometa em dias diferentes: se o dia saísse
	// do calendário, ele seria horário de ônibus.
	bool bAlgumaDivergiu = false;
	for (int32 Passo = 0; Passo < PassosPorDia * 41; ++Passo)
	{
		const float Horas = Passo * PassoEmHorasDoCeuNoturno;
		if (WorldNightSky::CometVisible(SementeDoCeuNoturno, Horas)
			!= WorldNightSky::CometVisible(SementeDoCeuNoturno + 7u, Horas))
		{
			bAlgumaDivergiu = true;
			break;
		}
	}
	TestTrue(TEXT("a semente decide o dia da passagem"), bAlgumaDivergiu);

	// Onde ele aparece é estável dentro da passagem e alto o bastante para ser visto.
	const float PrimeiraPassagemEmHoras = WorldNightSky::CometPeriodDays * 0.5f * WorldTimeOfDay::HoursPerDay;
	const float Azimute = WorldNightSky::CometAzimuthDegrees(SementeDoCeuNoturno, PrimeiraPassagemEmHoras);
	const float Elevacao = WorldNightSky::CometElevationDegrees(SementeDoCeuNoturno, PrimeiraPassagemEmHoras);
	TestTrue(TEXT("o azimute do cometa cabe na volta"), Azimute >= 0.0f && Azimute <= 360.0f);
	TestTrue(TEXT("o cometa fica acima do horizonte"), Elevacao > 0.0f && Elevacao < 90.0f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWorldNightSkyAuroraBelongsToTheColdAlone,
	"BattleSquare.Environment.WorldNightSky.AAuroraEDoFrio",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldNightSkyAuroraBelongsToTheColdAlone::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("na geleira a aurora acende de noite"),
		WorldNightSky::AuroraStrength(EScenaryClimate::Cold, 0.0f) > 0.9f);
	TestEqual(TEXT("na geleira nao ha aurora ao meio-dia"),
		WorldNightSky::AuroraStrength(EScenaryClimate::Cold, 12.0f), 0.0f, 0.001f);

	const EScenaryClimate Mornos[] = {
		EScenaryClimate::Temperate, EScenaryClimate::Mild, EScenaryClimate::Desert, EScenaryClimate::Humid
	};
	for (const EScenaryClimate Clima : Mornos)
	{
		TestEqual(TEXT("fora do frio nao ha aurora"),
			WorldNightSky::AuroraStrength(Clima, 0.0f), 0.0f, 0.001f);
	}

	// Sem sorteio: quem atravessa a ilha até a geleira à noite VÊ. Uma moeda
	// por noite mandaria a pessoa andar horas para nada.
	for (int32 Noite = 0; Noite < 30; ++Noite)
	{
		TestTrue(TEXT("toda noite de geleira tem aurora"),
			WorldNightSky::AuroraStrength(EScenaryClimate::Cold, 0.0f) > 0.0f);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWorldNightSkyGivesEveryNameItsOwnCase,
	"BattleSquare.Environment.WorldNightSky.CadaNomeTemSeuCaso",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldNightSkyGivesEveryNameItsOwnCase::RunTest(const FString& Parameters)
{
	TSet<FString> NomesDeFase;
	for (const EMoonPhase Fase : FasesDoCeuNoturno)
	{
		NomesDeFase.Add(WorldNightSky::PhaseDebugName(Fase));
	}
	TestEqual(TEXT("cada fase tem nome proprio"), NomesDeFase.Num(), static_cast<int32>(UE_ARRAY_COUNT(FasesDoCeuNoturno)));

	TSet<FString> NomesDeEclipse;
	for (const ESkyEclipse Eclipse : EclipsesDoCeuNoturno)
	{
		NomesDeEclipse.Add(WorldNightSky::EclipseDebugName(Eclipse));
	}
	TestEqual(TEXT("cada eclipse tem nome proprio"), NomesDeEclipse.Num(), static_cast<int32>(UE_ARRAY_COUNT(EclipsesDoCeuNoturno)));

	return true;
}

#endif

// Copyright 2026 Anderson. All Rights Reserved.

#include "Environment/WorldNightSky.h"

#include "Battle/DeterministicSpread.h"
#include "Environment/WorldTimeOfDay.h"

namespace
{
	/** A parte depois da virgula, sempre em [0,1) mesmo para valor negativo. */
	float FracaoDeVolta(float Valor)
	{
		const float Sobra = FMath::Fmod(Valor, 1.0f);
		return Sobra < 0.0f ? Sobra + 1.0f : Sobra;
	}

	/** Distancia entre duas fracoes de volta, pelo caminho mais curto. */
	float DistanciaNaVolta(float Primeira, float Segunda)
	{
		const float Direta = FMath::Abs(Primeira - Segunda);
		return FMath::Min(Direta, 1.0f - Direta);
	}

	/**
	 * Quao perto a fase esta de um alvo, de 0 (fora da janela) a 1 (em cima).
	 *
	 * E rampa, nao interruptor: um eclipse que comeca fundo no primeiro
	 * instante nao parece um eclipse, parece a lua trocada de cor.
	 */
	float ProximidadeDaFase(float Fracao, float Alvo)
	{
		const float Distancia = DistanciaNaVolta(Fracao, Alvo);
		if (Distancia >= WorldNightSky::EclipsePhaseWindow)
		{
			return 0.0f;
		}
		return 1.0f - Distancia / WorldNightSky::EclipsePhaseWindow;
	}

	/** A semente do cometa, separada da do clima para as duas nao andarem juntas. */
	uint32 SementeDoCometa(uint32 SementeDoMundo)
	{
		return BattleSpread::Scatter(SementeDoMundo ^ BattleSpread::SeedFromText(TEXT("cometa")));
	}

	/** Qual passagem do cometa e esta: a conta que as tres funcoes dele partilham. */
	int32 PassagemDoCometa(float ElapsedHours)
	{
		const float Dias = WorldNightSky::ElapsedDays(ElapsedHours);
		return FMath::FloorToInt(Dias / WorldNightSky::CometPeriodDays);
	}

	/** Em que dia do periodo esta passagem aparece. */
	float DiaDeEntradaDoCometa(uint32 SementeDoMundo, int32 Passagem)
	{
		const float Folga = WorldNightSky::CometPeriodDays - WorldNightSky::CometVisibleDays;
		return BattleSpread::Fraction(SementeDoCometa(SementeDoMundo), Passagem) * Folga;
	}

	const FLinearColor CorDePerola(0.86f, 0.88f, 0.95f);
	const FLinearColor CorDeSangue(0.55f, 0.11f, 0.06f);
}

namespace WorldNightSky
{
	float ElapsedDays(float ElapsedHours)
	{
		return ElapsedHours / WorldTimeOfDay::HoursPerDay;
	}

	float MoonPhaseFraction(float ElapsedHours)
	{
		return FracaoDeVolta(PhaseAtDayZero + ElapsedDays(ElapsedHours) / SynodicMonthDays);
	}

	EMoonPhase PhaseOf(float PhaseFraction)
	{
		const int32 Gavetas = 8;
		const int32 Gaveta = FMath::RoundToInt(FracaoDeVolta(PhaseFraction) * Gavetas) % Gavetas;
		return static_cast<EMoonPhase>(Gaveta);
	}

	float MoonLitFraction(float PhaseFraction)
	{
		return (1.0f - FMath::Cos(2.0f * UE_PI * FracaoDeVolta(PhaseFraction))) * 0.5f;
	}

	float MoonRiseLagHours(float PhaseFraction)
	{
		return FracaoDeVolta(PhaseFraction) * WorldTimeOfDay::HoursPerDay;
	}

	/**
	 * A lua sobe pelo mesmo caminho do sol, com atraso.
	 *
	 * Nao existe segunda conta de como um corpo cruza o ceu: se existisse,
	 * mudar a inclinacao do sol deixaria a lua torta, e ninguem lembraria de
	 * mexer nos dois lugares (L-032).
	 */
	static float HoraAparenteDaLua(float Hour, float PhaseFraction)
	{
		return FracaoDeVolta((Hour - MoonRiseLagHours(PhaseFraction)) / WorldTimeOfDay::HoursPerDay) * WorldTimeOfDay::HoursPerDay;
	}

	float MoonElevationDegrees(float Hour, float PhaseFraction)
	{
		return WorldTimeOfDay::SunElevationDegrees(HoraAparenteDaLua(Hour, PhaseFraction));
	}

	float MoonAzimuthDegrees(float Hour, float PhaseFraction)
	{
		return WorldTimeOfDay::SunAzimuthDegrees(HoraAparenteDaLua(Hour, PhaseFraction));
	}

	FRotator MoonRotation(float Hour, float PhaseFraction)
	{
		return WorldTimeOfDay::SunRotation(HoraAparenteDaLua(Hour, PhaseFraction));
	}

	bool IsMoonUp(float Hour, float PhaseFraction)
	{
		return MoonElevationDegrees(Hour, PhaseFraction) > 0.0f;
	}

	float MoonBrightness(float Hour, float PhaseFraction)
	{
		const float Elevacao = MoonElevationDegrees(Hour, PhaseFraction);
		if (Elevacao <= 0.0f)
		{
			return 0.0f;
		}
		const float Altura = FMath::Sin(FMath::DegreesToRadians(Elevacao));
		return MoonLitFraction(PhaseFraction) * FMath::Clamp(Altura, 0.0f, 1.0f);
	}

	float NodeProximity(float ElapsedHours)
	{
		const float MeiaVolta = DraconicMonthDays * 0.5f;
		const float Onde = FracaoDeVolta(NodeHalfCycleAtDayZero + ElapsedDays(ElapsedHours) / MeiaVolta);
		const float Distancia = FMath::Min(Onde, 1.0f - Onde);
		if (Distancia >= EclipseNodeWindow)
		{
			return 0.0f;
		}
		return 1.0f - Distancia / EclipseNodeWindow;
	}

	ESkyEclipse EclipseAt(float Hour, float ElapsedHours)
	{
		if (NodeProximity(ElapsedHours) <= 0.0f)
		{
			return ESkyEclipse::None;
		}

		const float Fase = MoonPhaseFraction(ElapsedHours);

		// Cada eclipse pede no ceu o corpo que ele APAGA: o lunar e a lua
		// escurecendo, o solar e o sol sendo tapado. Uma condicao unica para os
		// dois quase funciona -- na lua nova ela sobe com o sol -- e o "quase"
		// custa uma hora: a fase aceita pela janela vai de 0,965 a 0,035, e a
		// lua chega a subir 50 minutos antes do sol. Ai o eclipse solar caia
		// ANTES do amanhecer, tapando um sol que ainda nao havia.
		if (ProximidadeDaFase(Fase, 0.5f) > 0.0f)
		{
			return IsMoonUp(Hour, Fase) ? ESkyEclipse::Lunar : ESkyEclipse::None;
		}
		if (ProximidadeDaFase(Fase, 0.0f) > 0.0f)
		{
			const bool bSolNoCeu = WorldTimeOfDay::SunElevationDegrees(Hour) > 0.0f;
			return bSolNoCeu ? ESkyEclipse::Solar : ESkyEclipse::None;
		}
		return ESkyEclipse::None;
	}

	float EclipseDepth(float Hour, float ElapsedHours)
	{
		const ESkyEclipse Qual = EclipseAt(Hour, ElapsedHours);
		if (Qual == ESkyEclipse::None)
		{
			return 0.0f;
		}

		const float Fase = MoonPhaseFraction(ElapsedHours);
		const float Alvo = (Qual == ESkyEclipse::Lunar) ? 0.5f : 0.0f;
		return NodeProximity(ElapsedHours) * ProximidadeDaFase(Fase, Alvo);
	}

	bool IsBloodMoon(float Hour, float ElapsedHours)
	{
		return EclipseAt(Hour, ElapsedHours) == ESkyEclipse::Lunar;
	}

	FLinearColor MoonColor(float Hour, float ElapsedHours)
	{
		if (!IsBloodMoon(Hour, ElapsedHours))
		{
			return CorDePerola;
		}
		return FMath::Lerp(CorDePerola, CorDeSangue, EclipseDepth(Hour, ElapsedHours));
	}

	float SolarEclipseCoverage(float Hour, float ElapsedHours)
	{
		if (EclipseAt(Hour, ElapsedHours) != ESkyEclipse::Solar)
		{
			return 0.0f;
		}
		return EclipseDepth(Hour, ElapsedHours);
	}

	float SkyDarkness(float Hour)
	{
		const float Elevacao = WorldTimeOfDay::SunElevationDegrees(Hour);
		const float Faixa = FirstStarSunElevationDegrees - FullStarSunElevationDegrees;
		return FMath::Clamp((FirstStarSunElevationDegrees - Elevacao) / Faixa, 0.0f, 1.0f);
	}

	float StarBrightness(float Hour, EWeather Weather)
	{
		return SkyDarkness(Hour) * (1.0f - WorldWeather::CloudCover(Weather));
	}

	bool CometVisible(uint32 Seed, float ElapsedHours)
	{
		const int32 Passagem = PassagemDoCometa(ElapsedHours);
		const float NoPeriodo = ElapsedDays(ElapsedHours) - Passagem * CometPeriodDays;
		const float Entrada = DiaDeEntradaDoCometa(Seed, Passagem);
		return NoPeriodo >= Entrada && NoPeriodo < Entrada + CometVisibleDays;
	}

	float CometAzimuthDegrees(uint32 Seed, float ElapsedHours)
	{
		const int32 Passagem = PassagemDoCometa(ElapsedHours);
		return BattleSpread::Between(0.0f, 360.0f, BattleSpread::Fraction(SementeDoCometa(Seed), Passagem * 2 + 1));
	}

	float CometElevationDegrees(uint32 Seed, float ElapsedHours)
	{
		const int32 Passagem = PassagemDoCometa(ElapsedHours);
		return BattleSpread::Between(20.0f, 70.0f, BattleSpread::Fraction(SementeDoCometa(Seed), Passagem * 2 + 2));
	}

	float AuroraStrength(EScenaryClimate Climate, float Hour)
	{
		if (Climate != EScenaryClimate::Cold)
		{
			return 0.0f;
		}
		return SkyDarkness(Hour);
	}

	const TCHAR* PhaseDebugName(EMoonPhase Phase)
	{
		switch (Phase)
		{
		case EMoonPhase::New:             return TEXT("nova");
		case EMoonPhase::WaxingCrescent:  return TEXT("crescente fina");
		case EMoonPhase::FirstQuarter:    return TEXT("quarto crescente");
		case EMoonPhase::WaxingGibbous:   return TEXT("crescente gibosa");
		case EMoonPhase::Full:            return TEXT("cheia");
		case EMoonPhase::WaningGibbous:   return TEXT("minguante gibosa");
		case EMoonPhase::LastQuarter:     return TEXT("quarto minguante");
		case EMoonPhase::WaningCrescent:  return TEXT("minguante fina");
		}
		return TEXT("nova");
	}

	const TCHAR* EclipseDebugName(ESkyEclipse Eclipse)
	{
		switch (Eclipse)
		{
		case ESkyEclipse::None:  return TEXT("nenhum");
		case ESkyEclipse::Lunar: return TEXT("ECLIPSE LUNAR (lua vermelha)");
		case ESkyEclipse::Solar: return TEXT("ECLIPSE SOLAR");
		}
		return TEXT("nenhum");
	}
}

// Copyright 2026 Anderson. All Rights Reserved.

#include "Environment/WorldWeather.h"

#include "Battle/DeterministicSpread.h"
#include "Environment/WorldTimeOfDay.h"

namespace TempoDoMundo
{
	/** Acima desta altura de sol, céu limpo é "ensolarado". */
	constexpr float SolAltoGraus = 20.0f;

	/** Quanto do sol passa por cada céu. */
	constexpr float PassaLimpo = 1.0f;
	constexpr float PassaNublado = 0.8f;
	constexpr float PassaEncoberto = 0.5f;
	constexpr float PassaChuva = 0.4f;

	/** Quanto do céu está tomado. */
	constexpr float CoberturaLimpo = 0.05f;
	constexpr float CoberturaNublado = 0.4f;
	constexpr float CoberturaEncoberto = 0.9f;
	constexpr float CoberturaChuva = 1.0f;

	/**
	 * O quanto cada céu mexe na umidade do lugar.
	 *
	 * Chuva sobe muito porque é ela que precisa virar lama na arena; céu limpo
	 * DESCE, porque um dia de sol seca o chão e um deserto sob sol deve secar
	 * mais que a média dele.
	 */
	constexpr int32 UmidadeDaChuva = 35;
	constexpr int32 UmidadeDoEncoberto = 10;
	constexpr int32 UmidadeDoNublado = 5;
	constexpr int32 UmidadeDoLimpo = -5;

	constexpr int32 UmidadeMinima = 0;
	constexpr int32 UmidadeMaxima = 100;

	/**
	 * Quantos por cento de cada céu, por clima. Somam 100 em cada linha.
	 *
	 * A tabela é a resposta a "existe deserto?": no deserto chove 3% do tempo
	 * e o céu está limpo em três de cada quatro blocos. Não é um `if` dizendo
	 * "deserto não chove" — é raro, e a chuva rara no deserto é justamente o
	 * que faz ela valer alguma coisa quando cai.
	 */
	struct FPesosDoCeu
	{
		int32 Chuva = 0;
		int32 Encoberto = 0;
		int32 Nublado = 0;
	};

	FPesosDoCeu PesosDoClima(EScenaryClimate Climate)
	{
		switch (Climate)
		{
		case EScenaryClimate::Desert:
			return FPesosDoCeu{3, 5, 17};
		case EScenaryClimate::Cold:
			return FPesosDoCeu{30, 25, 25};
		case EScenaryClimate::Mild:
			return FPesosDoCeu{15, 10, 20};
		case EScenaryClimate::Temperate:
		default:
			return FPesosDoCeu{25, 15, 25};
		}
	}
}

namespace WorldWeather
{
	int32 RainChancePercent(EScenaryClimate Climate)
	{
		return TempoDoMundo::PesosDoClima(Climate).Chuva;
	}

	EWeather WeatherAt(uint32 Seed, EScenaryClimate Climate, float ElapsedHours)
	{
		// O sorteio é por BLOCO de horas, não por hora: dentro do bloco o
		// índice é o mesmo, e o céu não muda. É o que faz a chuva durar.
		const int32 Bloco = FMath::FloorToInt(ElapsedHours / HoursPerSpell);
		const int32 Sorteio = BattleSpread::Below(Seed, Bloco, 100);

		const TempoDoMundo::FPesosDoCeu Pesos = TempoDoMundo::PesosDoClima(Climate);
		if (Sorteio < Pesos.Chuva)
		{
			return EWeather::Rain;
		}
		if (Sorteio < Pesos.Chuva + Pesos.Encoberto)
		{
			return EWeather::Overcast;
		}
		if (Sorteio < Pesos.Chuva + Pesos.Encoberto + Pesos.Nublado)
		{
			return EWeather::Cloudy;
		}
		return EWeather::Clear;
	}

	float CloudCover(EWeather Weather)
	{
		using namespace TempoDoMundo;
		switch (Weather)
		{
		case EWeather::Rain:
			return CoberturaChuva;
		case EWeather::Overcast:
			return CoberturaEncoberto;
		case EWeather::Cloudy:
			return CoberturaNublado;
		case EWeather::Clear:
		default:
			return CoberturaLimpo;
		}
	}

	float SunDimming(EWeather Weather)
	{
		using namespace TempoDoMundo;
		switch (Weather)
		{
		case EWeather::Rain:
			return PassaChuva;
		case EWeather::Overcast:
			return PassaEncoberto;
		case EWeather::Cloudy:
			return PassaNublado;
		case EWeather::Clear:
		default:
			return PassaLimpo;
		}
	}

	int32 HumidityPercent(EScenaryClimate Climate, EWeather Weather)
	{
		using namespace TempoDoMundo;
		// A base é a do LUGAR, e vem de quem já a conhece. Trazer uma segunda
		// tabela aqui faria o deserto ter duas umidades (L-032).
		const int32 Base = ScenaryClimate::HumidityPercent(Climate);

		int32 Ajuste = UmidadeDoLimpo;
		switch (Weather)
		{
		case EWeather::Rain:
			Ajuste = UmidadeDaChuva;
			break;
		case EWeather::Overcast:
			Ajuste = UmidadeDoEncoberto;
			break;
		case EWeather::Cloudy:
			Ajuste = UmidadeDoNublado;
			break;
		case EWeather::Clear:
		default:
			break;
		}

		return FMath::Clamp(Base + Ajuste, UmidadeMinima, UmidadeMaxima);
	}

	bool IsRaining(EWeather Weather)
	{
		return Weather == EWeather::Rain;
	}

	bool IsSunny(EWeather Weather, float Hour)
	{
		// Quem sabe a altura do sol é o relógio. Repetir a conta aqui daria um
		// "ensolarado" que discorda do pôr do sol que a tela mostra.
		return Weather == EWeather::Clear
			&& WorldTimeOfDay::SunElevationDegrees(Hour) >= TempoDoMundo::SolAltoGraus;
	}

	const TCHAR* WeatherDebugName(EWeather Weather)
	{
		switch (Weather)
		{
		case EWeather::Rain:
			return TEXT("chuva");
		case EWeather::Overcast:
			return TEXT("encoberto");
		case EWeather::Cloudy:
			return TEXT("nublado");
		case EWeather::Clear:
		default:
			return TEXT("limpo");
		}
	}
}

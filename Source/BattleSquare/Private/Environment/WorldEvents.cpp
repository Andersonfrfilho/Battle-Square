// Copyright 2026 Anderson. All Rights Reserved.

#include "Environment/WorldEvents.h"

#include "Battle/DeterministicSpread.h"
#include "Environment/IslandFeatureLayout.h"
#include "Environment/IslandGeography.h"

namespace EventosDoMundo
{
	/** Temperos, para cada sorteio ter fluxo próprio a partir da mesma semente. */
	constexpr uint32 TemperoDoTremor = 0x51ED2701u;
	constexpr uint32 TemperoDaMagnitude = 0xC0FFEE11u;
	constexpr uint32 TemperoDoFuracao = 0x1B873593u;

	/**
	 * A rampa de um evento dentro da janela dele: 0 nas bordas, 1 no meio.
	 *
	 * Rampa e não interruptor. Evento que liga em força cheia e desliga do
	 * mesmo jeito pisca na tela e não deixa ninguém reagir; com a rampa, o
	 * vento AUMENTA, e quem está na praia tem tempo de sair.
	 */
	float RampaDaJanela(float FracaoDaJanela)
	{
		if (FracaoDaJanela <= 0.0f || FracaoDaJanela >= 1.0f)
		{
			return 0.0f;
		}
		return FMath::Sin(FracaoDaJanela * UE_PI);
	}

	/** Em que período de `Duracao` horas cai esta hora. */
	int32 PeriodoDe(float ElapsedHours, float Duracao)
	{
		return FMath::FloorToInt(ElapsedHours / Duracao);
	}

	/** Quantas horas já correram DENTRO do período atual. */
	float DentroDoPeriodo(float ElapsedHours, float Duracao)
	{
		return ElapsedHours - PeriodoDe(ElapsedHours, Duracao) * Duracao;
	}

	/**
	 * A hora, dentro do período, em que o tremor começa.
	 *
	 * A folga desconta a onda inteira, e não só o tremor: assim toda a corrente
	 * — tremor, espera, onda — termina antes do período virar, e o tsunami
	 * nunca precisa olhar para o período anterior. Descontar só o tremor faria
	 * a onda de um período vazar para o seguinte, onde já há outro tremor.
	 */
	float EntradaDoTremor(uint32 Seed, int32 Periodo)
	{
		const float Folga = WorldEvents::EarthquakePeriodHours
			- WorldEvents::EarthquakeSpanHours
			- WorldEvents::TsunamiDelayHours
			- WorldEvents::TsunamiSpanHours;
		return BattleSpread::Fraction(Seed ^ TemperoDoTremor, Periodo) * Folga;
	}

	/** A hora, dentro do período, em que o furacão entra. */
	float EntradaDoFuracao(uint32 Seed, int32 Periodo)
	{
		const float Folga = WorldEvents::HurricanePeriodHours - WorldEvents::HurricaneSpanHours;
		return BattleSpread::Fraction(Seed ^ TemperoDoFuracao, Periodo) * Folga;
	}

	/** A magnitude sorteada para o tremor deste período. */
	float MagnitudeDoPeriodo(uint32 Seed, int32 Periodo)
	{
		return BattleSpread::Between(
			WorldEvents::MinMagnitude,
			WorldEvents::MaxMagnitude,
			BattleSpread::Fraction(Seed ^ TemperoDaMagnitude, Periodo));
	}

	/** Diferença de ângulo entre dois rumos, sempre de 0 a 180. */
	float DistanciaAngular(float PrimeiroGraus, float SegundoGraus)
	{
		const float Bruta = FMath::Fmod(FMath::Fmod(PrimeiroGraus - SegundoGraus, 360.0f) + 360.0f, 360.0f);
		return Bruta > 180.0f ? 360.0f - Bruta : Bruta;
	}

	/** Onde começa a faixa que o mar alcança: a praia e tudo para fora dela. */
	float RaioDaBeirada()
	{
		return IslandGeography::LandRadiusUnits() - IslandGeography::BeachWidthUnits();
	}
}

namespace WorldEvents
{
	FVector2D FaultCenterUnits()
	{
		// O vulcão é sempre o mesmo enquanto o mundo roda, e `Plan()` monta um
		// array a cada chamada. Guardar o resultado numa estática evita remontar
		// o plano da ilha a cada quadro do painel sem abrir um segundo lugar
		// dizendo onde o vulcão está.
		static const FVector2D Falha = []()
		{
			for (const IslandFeatureLayout::FFeaturePlacement& Peca : IslandFeatureLayout::Plan())
			{
				if (Peca.Feature == IslandFeatureLayout::EIslandFeature::Volcano)
				{
					return Peca.CenterUnits();
				}
			}
			return FVector2D::ZeroVector;
		}();
		return Falha;
	}

	float EarthquakeMagnitude(uint32 Seed, float ElapsedHours)
	{
		const int32 Periodo = EventosDoMundo::PeriodoDe(ElapsedHours, EarthquakePeriodHours);
		const float Entrada = EventosDoMundo::EntradaDoTremor(Seed, Periodo);
		const float Agora = EventosDoMundo::DentroDoPeriodo(ElapsedHours, EarthquakePeriodHours);

		if (Agora < Entrada || Agora >= Entrada + EarthquakeSpanHours)
		{
			return 0.0f;
		}

		const float NaJanela = (Agora - Entrada) / EarthquakeSpanHours;
		return EventosDoMundo::MagnitudeDoPeriodo(Seed, Periodo) * EventosDoMundo::RampaDaJanela(NaJanela);
	}

	FVector2D EarthquakeEpicenterUnits(uint32 Seed, float ElapsedHours)
	{
		const int32 Periodo = EventosDoMundo::PeriodoDe(ElapsedHours, EarthquakePeriodHours);
		const uint32 Fluxo = Seed ^ EventosDoMundo::TemperoDoTremor;

		const float Rumo = BattleSpread::Between(0.0f, 360.0f,
			BattleSpread::Fraction(Fluxo, Periodo * 2 + 1));
		const float Longe = BattleSpread::Between(0.0f, FaultSpreadUnits,
			BattleSpread::Fraction(Fluxo, Periodo * 2 + 2));

		const float Radianos = FMath::DegreesToRadians(Rumo);
		return FaultCenterUnits() + FVector2D(FMath::Cos(Radianos), FMath::Sin(Radianos)) * Longe;
	}

	float EarthquakeShaking(uint32 Seed, const FVector2D& PositionUnits, float ElapsedHours)
	{
		const float Magnitude = EarthquakeMagnitude(Seed, ElapsedHours);
		if (Magnitude <= 0.0f)
		{
			return 0.0f;
		}

		const float Longe = static_cast<float>(
			FVector2D::Distance(PositionUnits, EarthquakeEpicenterUnits(Seed, ElapsedHours)));
		const float Perto = 1.0f - FMath::Clamp(Longe / EarthquakeReachUnits, 0.0f, 1.0f);
		return Magnitude * Perto;
	}

	float HurricaneEyeAngleDegrees(uint32 Seed, float ElapsedHours)
	{
		const int32 Periodo = EventosDoMundo::PeriodoDe(ElapsedHours, HurricanePeriodHours);
		return BattleSpread::Between(0.0f, 360.0f,
			BattleSpread::Fraction(Seed ^ EventosDoMundo::TemperoDoFuracao, Periodo * 2 + 1));
	}

	float HurricaneStrength(uint32 Seed, const FVector2D& PositionUnits, float ElapsedHours)
	{
		const int32 Periodo = EventosDoMundo::PeriodoDe(ElapsedHours, HurricanePeriodHours);
		const float Entrada = EventosDoMundo::EntradaDoFuracao(Seed, Periodo);
		const float Agora = EventosDoMundo::DentroDoPeriodo(ElapsedHours, HurricanePeriodHours);

		if (Agora < Entrada || Agora >= Entrada + HurricaneSpanHours)
		{
			return 0.0f;
		}

		// Terra adentro o furacão não existe, e a passagem para o mar é suave:
		// na beira da praia ele já se sente de leve, e em cima da água bate
		// inteiro. Um corte seco daria vento máximo de um lado de uma linha
		// invisível e nada do outro.
		const float Raio = static_cast<float>(PositionUnits.Size());
		const float Beirada = EventosDoMundo::RaioDaBeirada();
		const float Largura = IslandGeography::BeachWidthUnits();
		const float Molhado = FMath::Clamp((Raio - Beirada) / Largura, 0.0f, 1.0f);
		if (Molhado <= 0.0f)
		{
			return 0.0f;
		}

		const float Rumo = static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(PositionUnits.Y, PositionUnits.X)));
		const float Desvio = EventosDoMundo::DistanciaAngular(Rumo, HurricaneEyeAngleDegrees(Seed, ElapsedHours));
		const float NoSetor = 1.0f - FMath::Clamp(Desvio / HurricaneHalfWidthDegrees, 0.0f, 1.0f);
		if (NoSetor <= 0.0f)
		{
			return 0.0f;
		}

		const float NaJanela = (Agora - Entrada) / HurricaneSpanHours;
		return EventosDoMundo::RampaDaJanela(NaJanela) * NoSetor * Molhado;
	}

	bool TsunamiRises(uint32 Seed, float ElapsedHours)
	{
		const int32 Periodo = EventosDoMundo::PeriodoDe(ElapsedHours, EarthquakePeriodHours);
		return EventosDoMundo::MagnitudeDoPeriodo(Seed, Periodo) >= TsunamiMinMagnitude;
	}

	float TsunamiRiseUnits(uint32 Seed, float ElapsedHours)
	{
		if (!TsunamiRises(Seed, ElapsedHours))
		{
			return 0.0f;
		}

		const int32 Periodo = EventosDoMundo::PeriodoDe(ElapsedHours, EarthquakePeriodHours);
		const float Tremor = EventosDoMundo::EntradaDoTremor(Seed, Periodo);
		const float Entrada = Tremor + EarthquakeSpanHours + TsunamiDelayHours;
		const float Agora = EventosDoMundo::DentroDoPeriodo(ElapsedHours, EarthquakePeriodHours);

		if (Agora < Entrada || Agora >= Entrada + TsunamiSpanHours)
		{
			return 0.0f;
		}

		const float NaJanela = (Agora - Entrada) / TsunamiSpanHours;
		const float Magnitude = EventosDoMundo::MagnitudeDoPeriodo(Seed, Periodo);
		return TsunamiMaxRiseUnits * Magnitude * EventosDoMundo::RampaDaJanela(NaJanela);
	}

	bool TsunamiReaches(const FVector2D& PositionUnits)
	{
		return static_cast<float>(PositionUnits.Size()) >= EventosDoMundo::RaioDaBeirada();
	}

	EWorldEvent EventAt(uint32 Seed, const FVector2D& PositionUnits, float ElapsedHours)
	{
		// A ordem é a da severidade do enum, de cima para baixo: onde dois se
		// encontram, o que se vê é o mais forte. Perguntar na ordem inversa
		// esconderia o tsunami atrás do furacão pelo qual ele passa.
		if (TsunamiReaches(PositionUnits) && TsunamiRiseUnits(Seed, ElapsedHours) > 0.0f)
		{
			return EWorldEvent::Tsunami;
		}
		if (HurricaneStrength(Seed, PositionUnits, ElapsedHours) > 0.0f)
		{
			return EWorldEvent::Hurricane;
		}
		if (EarthquakeShaking(Seed, PositionUnits, ElapsedHours) > 0.0f)
		{
			return EWorldEvent::Earthquake;
		}
		return EWorldEvent::None;
	}

	float EventStrength(uint32 Seed, const FVector2D& PositionUnits, float ElapsedHours)
	{
		switch (EventAt(Seed, PositionUnits, ElapsedHours))
		{
		case EWorldEvent::Tsunami:
			return FMath::Clamp(TsunamiRiseUnits(Seed, ElapsedHours) / TsunamiMaxRiseUnits, 0.0f, 1.0f);
		case EWorldEvent::Hurricane:
			return HurricaneStrength(Seed, PositionUnits, ElapsedHours);
		case EWorldEvent::Earthquake:
			return EarthquakeShaking(Seed, PositionUnits, ElapsedHours);
		case EWorldEvent::None:
		default:
			return 0.0f;
		}
	}

	const TCHAR* EventDebugName(EWorldEvent Event)
	{
		switch (Event)
		{
		case EWorldEvent::Tsunami:
			return TEXT("TSUNAMI");
		case EWorldEvent::Hurricane:
			return TEXT("FURACAO");
		case EWorldEvent::Earthquake:
			return TEXT("TERREMOTO");
		case EWorldEvent::None:
		default:
			return TEXT("calmo");
		}
	}
}

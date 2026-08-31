// Copyright 2026 Anderson. All Rights Reserved.

#include "Environment/WorldTimeOfDay.h"

#include "Battle/DeterministicSpread.h"

namespace
{
	/**
	 * Onde o crepúsculo acaba e a noite fecha, em graus abaixo do horizonte.
	 *
	 * Doze graus é o crepúsculo civil — o ponto em que se deixa de enxergar
	 * sem luz artificial. Não é número escolhido para o jogo ficar bonito, é
	 * o mesmo motivo do lapse rate da serra: o modelo real já traz a curva
	 * certa de graça.
	 */
	constexpr float FimDoCrepusculoGraus = -12.0f;

	/** Acima disto o sol já não pinta nada de laranja. */
	constexpr float SolAltoGraus = 25.0f;

	/**
	 * Como as espécies se repartem entre as três horas, em porcentagem.
	 *
	 * Metade diurna de propósito: o jogo abre às sete da manhã, e um mundo em
	 * que dois de cada três bichos só saem à noite pareceria vazio no primeiro
	 * minuto de quem instalou. O outro metade dividido em dois dá à noite e ao
	 * entardecer moradores próprios — que é o que faz esperar anoitecer valer
	 * a pena.
	 *
	 * Somam 100 com os noturnos, que ficam com o resto: escrever os três
	 * deixaria a soma poder não fechar.
	 */
	constexpr int32 RelogioPercentualDeDiurnos = 50;
	constexpr int32 RelogioPercentualDeCrepusculares = 25;

	/** O que o luar deixa. Não é zero: noite cega não deixa ninguém andar. */
	constexpr float BrilhoDaNoite = 0.02f;

	/** O que o sol deixa quando está exatamente no horizonte. */
	constexpr float BrilhoNoHorizonte = 0.2f;

	const FLinearColor CorDoLuar(0.35f, 0.45f, 0.75f);
	const FLinearColor CorDoCrepusculo(1.0f, 0.45f, 0.18f);
	const FLinearColor CorDoSolAlto(1.0f, 0.96f, 0.90f);
}

namespace WorldTimeOfDay
{
	float HourAt(float ElapsedSeconds, float SecondsPerDay)
	{
		if (SecondsPerDay <= 0.0f)
		{
			return 0.0f;
		}

		// `Fmod` de negativo devolve negativo, e um jogo que começa com o
		// relógio atrasado nasceria na hora -3. O segundo `Fmod` normaliza.
		const float Fracao = FMath::Fmod(FMath::Fmod(ElapsedSeconds, SecondsPerDay) + SecondsPerDay, SecondsPerDay);
		return Fracao / SecondsPerDay * HoursPerDay;
	}

	EDayPhase PhaseAtHour(float Hour)
	{
		const float Normalizada = FMath::Fmod(FMath::Fmod(Hour, HoursPerDay) + HoursPerDay, HoursPerDay);

		if (Normalizada < DawnStartHour)
		{
			return EDayPhase::Night;
		}
		if (Normalizada < DayStartHour)
		{
			return EDayPhase::Dawn;
		}
		if (Normalizada < DuskStartHour)
		{
			return EDayPhase::Day;
		}
		if (Normalizada < NightStartHour)
		{
			return EDayPhase::Dusk;
		}
		return EDayPhase::Night;
	}

	float SunElevationDegrees(float Hour)
	{
		// Meia-noite no fundo, meio-dia no alto, e o sol cruzando o horizonte
		// às 6 e às 18 por consequência — não por um `if` que declara isso.
		const float Radianos = 2.0f * PI * (Hour - 6.0f) / HoursPerDay;
		return 90.0f * FMath::Sin(Radianos);
	}

	float SunPitchDegrees(float Hour)
	{
		return -SunElevationDegrees(Hour);
	}

	float SunAzimuthDegrees(float Hour)
	{
		// Uma volta inteira por dia, ancorada no leste às 6. O sol nasce num
		// lado e se põe no outro por consequência da volta, não por um `if`
		// que troca o lado no meio do caminho.
		const float Bruto = 90.0f + (Hour - 6.0f) * (360.0f / HoursPerDay);
		return FMath::Fmod(FMath::Fmod(Bruto, 360.0f) + 360.0f, 360.0f);
	}

	FRotator SunRotation(float Hour)
	{
		return FRotator(SunPitchDegrees(Hour), SunAzimuthDegrees(Hour), 0.0f);
	}

	bool IsNight(float Hour)
	{
		return SunElevationDegrees(Hour) <= FimDoCrepusculoGraus;
	}

	float SunBrightness(float Hour)
	{
		const float Elevacao = SunElevationDegrees(Hour);

		if (Elevacao <= FimDoCrepusculoGraus)
		{
			return BrilhoDaNoite;
		}

		if (Elevacao < 0.0f)
		{
			const float Fatia = (Elevacao - FimDoCrepusculoGraus) / -FimDoCrepusculoGraus;
			return FMath::Lerp(BrilhoDaNoite, BrilhoNoHorizonte, Fatia);
		}

		const float Fatia = FMath::Sin(FMath::DegreesToRadians(Elevacao));
		return FMath::Lerp(BrilhoNoHorizonte, 1.0f, Fatia);
	}

	FLinearColor SunColor(float Hour)
	{
		const float Elevacao = SunElevationDegrees(Hour);

		if (Elevacao <= FimDoCrepusculoGraus)
		{
			return CorDoLuar;
		}

		if (Elevacao < 0.0f)
		{
			const float Fatia = (Elevacao - FimDoCrepusculoGraus) / -FimDoCrepusculoGraus;
			return FMath::Lerp(CorDoLuar, CorDoCrepusculo, Fatia);
		}

		if (Elevacao >= SolAltoGraus)
		{
			return CorDoSolAlto;
		}

		return FMath::Lerp(CorDoCrepusculo, CorDoSolAlto, Elevacao / SolAltoGraus);
	}

	int32 EncounterWeightPercent(EPetActivity Activity, EDayPhase Phase)
	{
		switch (Activity)
		{
		case EPetActivity::Diurnal:
			switch (Phase)
			{
			case EDayPhase::Day:   return 100;
			case EDayPhase::Dawn:  return 40;
			case EDayPhase::Dusk:  return 40;
			case EDayPhase::Night: return 5;
			}
			break;

		case EPetActivity::Crepuscular:
			switch (Phase)
			{
			case EDayPhase::Dawn:  return 100;
			case EDayPhase::Dusk:  return 100;
			case EDayPhase::Day:   return 25;
			case EDayPhase::Night: return 25;
			}
			break;

		case EPetActivity::Nocturnal:
			switch (Phase)
			{
			case EDayPhase::Night: return 100;
			case EDayPhase::Dawn:  return 20;
			case EDayPhase::Dusk:  return 20;
			case EDayPhase::Day:   return 5;
			}
			break;
		}

		return 0;
	}

	EPetActivity ActivityFromName(FName Name)
	{
		if (Name == TEXT("Crepuscular"))
		{
			return EPetActivity::Crepuscular;
		}
		if (Name == TEXT("Nocturnal"))
		{
			return EPetActivity::Nocturnal;
		}

		// Nome desconhecido cai em diurno, que é o horário em que qualquer um
		// está jogando: um erro de digitação na ficha some com o bicho da
		// manhã, não do jogo inteiro.
		return EPetActivity::Diurnal;
	}

	EPetActivity ActivityForSpecies(const FString& CatalogId)
	{
		if (CatalogId.IsEmpty())
		{
			return EPetActivity::Diurnal;
		}

		const uint32 Semente = BattleSpread::SeedFromText(CatalogId + TEXT("|atividade"));
		const int32 Sorteado = BattleSpread::Below(Semente, /*Indice=*/0, 100);

		if (Sorteado < RelogioPercentualDeDiurnos)
		{
			return EPetActivity::Diurnal;
		}
		if (Sorteado < RelogioPercentualDeDiurnos + RelogioPercentualDeCrepusculares)
		{
			return EPetActivity::Crepuscular;
		}
		return EPetActivity::Nocturnal;
	}

	int32 PickSpeciesForPhase(
		const TArray<FString>& CatalogIds, EDayPhase Phase, FRandomStream& Stream)
	{
		if (CatalogIds.IsEmpty())
		{
			return INDEX_NONE;
		}

		int32 Total = 0;
		for (const FString& Id : CatalogIds)
		{
			Total += EncounterWeightPercent(ActivityForSpecies(Id), Phase);
		}

		// Roleta: cada espécie ocupa uma fatia do tamanho do peso dela. Sortear
		// a espécie e depois aceitar ou recusar pelo peso daria o mesmo
		// resultado só depois de um número indeterminado de tentativas — e a
		// reposição de população não tem quantas tentativas quiser.
		int32 Restante = Stream.RandRange(0, Total - 1);
		for (int32 Indice = 0; Indice < CatalogIds.Num(); ++Indice)
		{
			Restante -= EncounterWeightPercent(ActivityForSpecies(CatalogIds[Indice]), Phase);
			if (Restante < 0)
			{
				return Indice;
			}
		}

		return CatalogIds.Num() - 1;
	}

	const TCHAR* PhaseDebugName(EDayPhase Phase)
	{
		switch (Phase)
		{
		case EDayPhase::Dawn:  return TEXT("amanhecer");
		case EDayPhase::Day:   return TEXT("dia");
		case EDayPhase::Dusk:  return TEXT("entardecer");
		case EDayPhase::Night: return TEXT("noite");
		}
		return TEXT("?");
	}
}

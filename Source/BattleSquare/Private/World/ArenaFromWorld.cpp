// Copyright 2026 Anderson. All Rights Reserved.

#include "World/ArenaFromWorld.h"

namespace
{
	/**
	 * Prioridade entre duas coisas que caíram na MESMA casa.
	 *
	 * Uma casa tem um terreno só, e o mundo não pede licença: um tronco na
	 * beira do rio é comum. Sem uma ordem declarada, quem vence é a ordem em
	 * que os atores foram varridos — e essa ordem não é estável entre
	 * execuções, então a mesma partida abriria arenas diferentes.
	 *
	 * O CORPO vence a água porque é o que o jogador vê: a pedra está ali, e
	 * uma casa desenhada como rio onde há uma pedra em pé seria a tela
	 * mentindo sobre a regra.
	 */
	int32 PesoDoTerreno(uint8 Terreno)
	{
		switch (static_cast<ECellProperty>(Terreno))
		{
			case ECellProperty::Blocked:      return 5;
			case ECellProperty::Water:        return 4;
			case ECellProperty::Mud:          return 3;
			case ECellProperty::ShallowWater: return 2;
			case ECellProperty::Buff:         return 1;
			default:                          return 0;
		}
	}

	/**
	 * A enchente: casa SECA encostada em água vira poça rasa.
	 *
	 * Uma passada só, e lida de uma cópia. Sem a cópia, a poça que acabou de
	 * nascer molharia a vizinha dela na mesma varredura, e a água atravessaria
	 * o tabuleiro inteiro numa chamada — o que não é enchente, é dilúvio, e
	 * ainda dependeria da ordem em que as casas foram visitadas.
	 *
	 * Nasce RASA de propósito: alagamento é água por cima do chão, não um lago
	 * fundo. Quem submerge continua precisando do leito de verdade, e quando a
	 * água baixar é a regra de secagem que já existe (`WouldFormMud`) que
	 * decide se aquilo virou lama — não uma segunda cópia dela aqui.
	 *
	 * Pedra e clareira de treino ficam de fora: a pedra continua em pé dentro
	 * d'água, e apagar a casa de bônus faria a tempestade DESTRUIR terreno em
	 * vez de cobri-lo.
	 */
	void EspalharAlagamento(TArray<uint8>& Layout, int32 Columns, int32 Rows)
	{
		const TArray<uint8> Antes = Layout;

		auto EhAgua = [&Antes](int32 Indice)
		{
			if (!Antes.IsValidIndex(Indice))
			{
				return false;
			}
			const ECellProperty Casa = static_cast<ECellProperty>(Antes[Indice]);
			return Casa == ECellProperty::Water || Casa == ECellProperty::ShallowWater;
		};

		for (int32 Linha = 0; Linha < Rows; ++Linha)
		{
			for (int32 Coluna = 0; Coluna < Columns; ++Coluna)
			{
				const int32 Indice = CellLayoutIndex(Coluna, Linha, Columns);
				if (static_cast<ECellProperty>(Antes[Indice]) != ECellProperty::None)
				{
					continue;
				}

				const bool bEncostaNaAgua =
					(Coluna > 0           && EhAgua(CellLayoutIndex(Coluna - 1, Linha, Columns)))
					|| (Coluna < Columns - 1 && EhAgua(CellLayoutIndex(Coluna + 1, Linha, Columns)))
					|| (Linha > 0            && EhAgua(CellLayoutIndex(Coluna, Linha - 1, Columns)))
					|| (Linha < Rows - 1     && EhAgua(CellLayoutIndex(Coluna, Linha + 1, Columns)));

				if (bEncostaNaAgua)
				{
					Layout[Indice] = static_cast<uint8>(ECellProperty::ShallowWater);
				}
			}
		}
	}

	uint8 TerrenoDaAmostra(EWorldFeatureKind Kind, int32 HumidityPercent)
	{
		switch (Kind)
		{
			case EWorldFeatureKind::Solid:
				return static_cast<uint8>(ECellProperty::Blocked);

			case EWorldFeatureKind::DeepWater:
				return static_cast<uint8>(ECellProperty::Water);

			case EWorldFeatureKind::Shore:
				// A MESMA regra que decide se uma poça vira lama ao secar,
				// aplicada ao lugar em vez de ao tempo: margem de rio em mata
				// úmida é lamaçal, e no deserto é areia molhada. Sem isto, a
				// umidade valeria para a água que o jogador cria e não para a
				// que já estava lá — duas físicas no mesmo campo.
				return HumidityPercent >= MudMinHumidity
					? static_cast<uint8>(ECellProperty::Mud)
					: static_cast<uint8>(ECellProperty::ShallowWater);

			case EWorldFeatureKind::TrainingGround:
				return static_cast<uint8>(ECellProperty::Buff);
		}

		return static_cast<uint8>(ECellProperty::None);
	}
}

TArray<uint8> FArenaFromWorld::Build(const FArenaFromWorldParams& Params,
	TArray<uint8>* OutFluids)
{
	if (Params.Features.IsEmpty()
		|| Params.Columns <= 0 || Params.Rows <= 0
		|| Params.CellSize <= 0.0f)
	{
		return TArray<uint8>();
	}

	TArray<uint8> Layout;
	Layout.Init(static_cast<uint8>(ECellProperty::None), Params.Columns * Params.Rows);

	if (OutFluids)
	{
		OutFluids->Init(0, Params.Columns * Params.Rows);
	}

	// O canto da grade, para converter posição de mundo em casa. A grade fica
	// CENTRADA no ponto do encontro: a batalha acontece onde os dois se
	// encontraram, e não num quadrado ao lado dele.
	const float LarguraTotal = Params.Columns * Params.CellSize;
	const float AlturaTotal = Params.Rows * Params.CellSize;
	const float CantoX = Params.EncounterLocation.X - LarguraTotal * 0.5f;
	const float CantoY = Params.EncounterLocation.Y - AlturaTotal * 0.5f;

	for (const FWorldFeatureSample& Amostra : Params.Features)
	{
		const int32 Coluna = FMath::FloorToInt(
			(Amostra.Location.X - CantoX) / Params.CellSize);
		const int32 Linha = FMath::FloorToInt(
			(Amostra.Location.Y - CantoY) / Params.CellSize);

		// Fora da grade é o caso NORMAL, não erro: a coleta varre um raio
		// generoso em volta do encontro, e a maior parte do que ela acha fica
		// de fora. Recusar a montagem por isso seria não ter arena nenhuma
		// perto de uma mata.
		if (!IsInsideGrid(Coluna, Linha, Params.Columns, Params.Rows))
		{
			continue;
		}

		const int32 Indice = CellLayoutIndex(Coluna, Linha, Params.Columns);
		const uint8 Novo = TerrenoDaAmostra(Amostra.Kind, Params.HumidityPercent);

		if (PesoDoTerreno(Novo) > PesoDoTerreno(Layout[Indice]))
		{
			Layout[Indice] = Novo;

			// O FLUIDO SAI DO MESMO DESEMPATE que a propriedade, e é por isso
			// que ele não tem função própria: uma segunda passada escolhendo
			// "qual amostra vence" concordaria com esta até a primeira edição,
			// e aí a casa teria a fundura de uma amostra e a substância de
			// outra — sem nada acusar.
			if (OutFluids)
			{
				(*OutFluids)[Indice] = Amostra.Fluid;
			}
		}
	}

	// Depois das amostras e ANTES de desobstruir as casas iniciais: a água
	// espalha a partir do que o mundo pôs ali, e um duelista pode nascer com o
	// pé na água — o que não pode é nascer dentro de pedra.
	if (Params.bFlooded)
	{
		EspalharAlagamento(Layout, Params.Columns, Params.Rows);
	}

	ClearStartingCells(Layout, Params.Columns, Params.Rows);
	return Layout;
}

void FArenaFromWorld::ClearStartingCells(TArray<uint8>& Layout, int32 Columns, int32 Rows)
{
	if (Columns <= 0 || Rows <= 0 || Layout.Num() != Columns * Rows)
	{
		return;
	}

	// A MESMA conta de FBattleState::PlaceDuelistsAtStartingCells: primeira e
	// última coluna, linha do meio arredondando para baixo. Escrita aqui de
	// novo é o risco conhecido — duas cópias concordam até a primeira edição
	// (L-032/L-033) — e por isso há teste que compara as duas.
	const int32 LinhaDoMeio = (Rows - 1) / 2;

	for (const int32 Coluna : { 0, Columns - 1 })
	{
		const int32 Indice = CellLayoutIndex(Coluna, LinhaDoMeio, Columns);
		if (Layout.IsValidIndex(Indice)
			&& Layout[Indice] == static_cast<uint8>(ECellProperty::Blocked))
		{
			Layout[Indice] = static_cast<uint8>(ECellProperty::None);
		}
	}
}

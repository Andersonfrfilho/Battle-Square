// Copyright 2026 Anderson. All Rights Reserved.

#include "Environment/CaveLabyrinth.h"

#include "Battle/DeterministicSpread.h"

namespace
{
	using namespace CaveLabyrinth;

	struct FLado
	{
		int32 PassoColuna;
		int32 PassoLinha;
		uint8 Parede;
		uint8 ParedeOposta;
	};

	/** Os quatro lados, e o par que precisa cair JUNTO para a passagem existir. */
	constexpr FLado LadosDoLabirinto[4] = {
		{  0,  1, WallNorth, WallSouth },
		{  1,  0, WallEast,  WallWest  },
		{  0, -1, WallSouth, WallNorth },
		{ -1,  0, WallWest,  WallEast  }
	};

	/** Quantas saídas uma casa tem. */
	int32 SaidasDe(const FCaveGrid& Planta, int32 Coluna, int32 Linha)
	{
		int32 Abertas = 0;
		for (const FLado& Lado : LadosDoLabirinto)
		{
			if (!Planta.HasWall(Coluna, Linha, Lado.Parede))
			{
				++Abertas;
			}
		}
		return Abertas;
	}

	/** Distâncias em passos a partir de uma casa. −1 onde não se chega. */
	TArray<int32> DistanciasDe(const FCaveGrid& Planta, int32 Coluna, int32 Linha)
	{
		TArray<int32> Distancia;
		if (!Planta.IsValid() || !Planta.Contains(Coluna, Linha))
		{
			return Distancia;
		}

		Distancia.Init(-1, Planta.CellCount());
		Distancia[Planta.Index(Coluna, Linha)] = 0;

		TArray<int32> Fila;
		Fila.Reserve(Planta.CellCount());
		Fila.Add(Planta.Index(Coluna, Linha));

		for (int32 Frente = 0; Frente < Fila.Num(); ++Frente)
		{
			const int32 Casa = Fila[Frente];
			const int32 C = Casa % Planta.Columns;
			const int32 L = Casa / Planta.Columns;

			for (const FLado& Lado : LadosDoLabirinto)
			{
				if (Planta.HasWall(C, L, Lado.Parede))
				{
					continue;
				}

				const int32 ProximaColuna = C + Lado.PassoColuna;
				const int32 ProximaLinha = L + Lado.PassoLinha;
				if (!Planta.Contains(ProximaColuna, ProximaLinha))
				{
					continue;
				}

				const int32 Vizinho = Planta.Index(ProximaColuna, ProximaLinha);
				if (Distancia[Vizinho] >= 0)
				{
					continue;
				}

				Distancia[Vizinho] = Distancia[Casa] + 1;
				Fila.Add(Vizinho);
			}
		}

		return Distancia;
	}
}

CaveLabyrinth::FCaveGrid CaveLabyrinth::Carve(int32 Columns, int32 Rows, uint32 Seed)
{
	FCaveGrid Planta;
	if (Columns < 1 || Rows < 1)
	{
		return Planta;
	}

	Planta.Columns = Columns;
	Planta.Rows = Rows;
	Planta.Walls.Init(AllWalls, Columns * Rows);

	TArray<bool> Visitada;
	Visitada.Init(false, Columns * Rows);

	TArray<int32> Pilha;
	Pilha.Reserve(Columns * Rows);
	Pilha.Add(0);
	Visitada[0] = true;

	// Um fluxo por decisão, e o contador nunca reinicia: dois sorteios com o
	// mesmo índice sairiam iguais, e um labirinto que sempre vira para o mesmo
	// lado é um corredor com curvas.
	int32 Decisao = 0;

	while (Pilha.Num() > 0)
	{
		const int32 Casa = Pilha.Last();
		const int32 Coluna = Casa % Columns;
		const int32 Linha = Casa / Columns;

		int32 CandidatoLado[4];
		int32 Quantos = 0;

		for (int32 Qual = 0; Qual < 4; ++Qual)
		{
			const int32 ProximaColuna = Coluna + LadosDoLabirinto[Qual].PassoColuna;
			const int32 ProximaLinha = Linha + LadosDoLabirinto[Qual].PassoLinha;
			if (!Planta.Contains(ProximaColuna, ProximaLinha))
			{
				continue;
			}
			if (Visitada[Planta.Index(ProximaColuna, ProximaLinha)])
			{
				continue;
			}

			CandidatoLado[Quantos++] = Qual;
		}

		if (Quantos == 0)
		{
			Pilha.Pop();
			continue;
		}

		const int32 Escolhido = CandidatoLado[BattleSpread::Below(Seed, Decisao++, Quantos)];
		const FLado& Lado = LadosDoLabirinto[Escolhido];
		const int32 ProximaColuna = Coluna + Lado.PassoColuna;
		const int32 ProximaLinha = Linha + Lado.PassoLinha;
		const int32 Vizinho = Planta.Index(ProximaColuna, ProximaLinha);

		// A parede cai dos DOIS lados. Derrubar só de um faz a passagem existir
		// para quem vem e não existir para quem volta.
		Planta.Walls[Casa] &= ~Lado.Parede;
		Planta.Walls[Vizinho] &= ~Lado.ParedeOposta;

		Visitada[Vizinho] = true;
		Pilha.Add(Vizinho);
	}

	Planta.EntranceColumn = BattleSpread::Below(Seed, Decisao, Columns);
	Planta.Walls[Planta.Index(Planta.EntranceColumn, 0)] &= ~WallSouth;

	return Planta;
}

int32 CaveLabyrinth::OpenPassageCount(const FCaveGrid& Grid)
{
	if (!Grid.IsValid())
	{
		return 0;
	}

	int32 Passagens = 0;
	for (int32 Linha = 0; Linha < Grid.Rows; ++Linha)
	{
		for (int32 Coluna = 0; Coluna < Grid.Columns; ++Coluna)
		{
			// Só norte e leste, para cada passagem ser contada uma vez.
			if (Grid.Contains(Coluna, Linha + 1) && !Grid.HasWall(Coluna, Linha, WallNorth))
			{
				++Passagens;
			}
			if (Grid.Contains(Coluna + 1, Linha) && !Grid.HasWall(Coluna, Linha, WallEast))
			{
				++Passagens;
			}
		}
	}

	return Passagens;
}

int32 CaveLabyrinth::DeadEndCount(const FCaveGrid& Grid)
{
	if (!Grid.IsValid())
	{
		return 0;
	}

	int32 Becos = 0;
	for (int32 Linha = 0; Linha < Grid.Rows; ++Linha)
	{
		for (int32 Coluna = 0; Coluna < Grid.Columns; ++Coluna)
		{
			if (SaidasDe(Grid, Coluna, Linha) == 1)
			{
				++Becos;
			}
		}
	}

	return Becos;
}

bool CaveLabyrinth::HasPath(const FCaveGrid& Grid,
	int32 FromColumn, int32 FromRow, int32 ToColumn, int32 ToRow)
{
	if (!Grid.Contains(ToColumn, ToRow))
	{
		return false;
	}

	const TArray<int32> Distancia = DistanciasDe(Grid, FromColumn, FromRow);
	if (Distancia.Num() == 0)
	{
		return false;
	}

	return Distancia[Grid.Index(ToColumn, ToRow)] >= 0;
}

int32 CaveLabyrinth::DeepestFrom(const FCaveGrid& Grid,
	int32 FromColumn, int32 FromRow, int32& OutColumn, int32& OutRow)
{
	OutColumn = FromColumn;
	OutRow = FromRow;

	const TArray<int32> Distancia = DistanciasDe(Grid, FromColumn, FromRow);
	if (Distancia.Num() == 0)
	{
		return -1;
	}

	int32 Maior = 0;
	for (int32 Casa = 0; Casa < Distancia.Num(); ++Casa)
	{
		if (Distancia[Casa] > Maior)
		{
			Maior = Distancia[Casa];
			OutColumn = Casa % Grid.Columns;
			OutRow = Casa / Grid.Columns;
		}
	}

	return Maior;
}

// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * O labirinto da caverna — planta baixa, sem nada de Unreal dentro.
 *
 * Separado do ator de propósito. Labirinto errado é o defeito mais caro de
 * achar olhando: um corredor sem saída parece um corredor, e uma sala isolada
 * do resto só aparece quando alguém andou até lá e não conseguiu sair. Aqui a
 * planta é inteiramente de inteiros, e a conectividade vira uma AFIRMAÇÃO que
 * roda em milissegundos sem abrir o editor.
 *
 * A escavação é o backtracker recursivo com pilha explícita, e o resultado é
 * uma árvore geradora: todo par de casas tem exatamente um caminho entre si.
 * Isso não é detalhe de implementação, é o que torna o labirinto justo — quem
 * entra sempre pode sair, e nunca por sorte.
 */
namespace CaveLabyrinth
{
	constexpr uint8 WallNorth = 1;
	constexpr uint8 WallEast = 2;
	constexpr uint8 WallSouth = 4;
	constexpr uint8 WallWest = 8;
	constexpr uint8 AllWalls = WallNorth | WallEast | WallSouth | WallWest;

	/** A planta: uma máscara de paredes por casa. */
	struct BATTLESQUARE_API FCaveGrid
	{
		int32 Columns = 0;
		int32 Rows = 0;

		/** Paredes que SOBRARAM em cada casa, indexadas por `Index`. */
		TArray<uint8> Walls;

		/** Por qual coluna da borda sul se entra. */
		int32 EntranceColumn = 0;

		bool IsValid() const { return Columns > 0 && Rows > 0 && Walls.Num() == Columns * Rows; }
		int32 CellCount() const { return Columns * Rows; }
		int32 Index(int32 Column, int32 Row) const { return Row * Columns + Column; }
		bool Contains(int32 Column, int32 Row) const
		{
			return Column >= 0 && Column < Columns && Row >= 0 && Row < Rows;
		}
		bool HasWall(int32 Column, int32 Row, uint8 Wall) const
		{
			return Contains(Column, Row) && (Walls[Index(Column, Row)] & Wall) != 0;
		}
	};

	/** Escava o labirinto. Grade inválida devolve planta vazia, nunca meia planta. */
	BATTLESQUARE_API FCaveGrid Carve(int32 Columns, int32 Rows, uint32 Seed);

	/** Quantas passagens internas ficaram abertas. Numa árvore geradora: casas − 1. */
	BATTLESQUARE_API int32 OpenPassageCount(const FCaveGrid& Grid);

	/** Quantas casas têm uma saída só — os becos, que é o que faz um labirinto. */
	BATTLESQUARE_API int32 DeadEndCount(const FCaveGrid& Grid);

	/** Se dá para ir de uma casa à outra andando. */
	BATTLESQUARE_API bool HasPath(const FCaveGrid& Grid,
		int32 FromColumn, int32 FromRow, int32 ToColumn, int32 ToRow);

	/**
	 * A casa mais LONGE da entrada, em passos, e a distância até ela.
	 *
	 * É onde faz sentido pôr o que vale a pena procurar: o fundo do labirinto
	 * medido, e não escolhido a dedo.
	 */
	BATTLESQUARE_API int32 DeepestFrom(const FCaveGrid& Grid,
		int32 FromColumn, int32 FromRow, int32& OutColumn, int32& OutRow);
}

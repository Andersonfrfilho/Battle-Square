// Copyright 2026 Anderson. All Rights Reserved.

import { and, eq } from 'drizzle-orm';

import { db } from '../db/client';
import { hasRegrown } from './treeCut.pure';
import { worldTreeCuts } from './treeCut.schema';

type Database = typeof db;

export type CutInput = {
  readonly chunkKey: string;
  readonly cellKey: string;
  readonly cutAtWorldAgeDays: number;
};

export type ActiveCut = {
  readonly cellKey: string;
  readonly cutAtWorldAgeDays: number;
};

/**
 * Registra um corte. Recortar a mesma celula ATUALIZA o carimbo, nunca empilha
 * — uma arvore cortada e uma marca so.
 */
export async function recordCut(input: CutInput, database: Database = db): Promise<void> {
  await database
    .insert(worldTreeCuts)
    .values(input)
    .onConflictDoUpdate({
      target: [worldTreeCuts.chunkKey, worldTreeCuts.cellKey],
      set: { cutAtWorldAgeDays: input.cutAtWorldAgeDays },
    });
}

/**
 * Os cortes AINDA ATIVOS de um pedaco, dada a idade do mundo e o prazo: os ja
 * rebrotados sao filtrados FORA (e a marca morta, podada da tabela). O cliente
 * so recebe o que ainda esta cortado — a base da mata ele planta da semente.
 */
export async function listActiveCuts(
  chunkKey: string,
  currentWorldAgeDays: number,
  deadlineDays: number,
  database: Database = db,
): Promise<ActiveCut[]> {
  const rows = await database
    .select()
    .from(worldTreeCuts)
    .where(eq(worldTreeCuts.chunkKey, chunkKey));

  const active: ActiveCut[] = [];
  const regrownCells: string[] = [];
  for (const row of rows) {
    if (hasRegrown(row.cutAtWorldAgeDays, currentWorldAgeDays, deadlineDays)) {
      regrownCells.push(row.cellKey);
    } else {
      active.push({ cellKey: row.cellKey, cutAtWorldAgeDays: row.cutAtWorldAgeDays });
    }
  }

  // Poda a marca morta: cicatrizou, some da tabela. Sem isto a exceção viraria
  // permanente, o oposto do que "exceção com prazo" quer dizer.
  for (const cellKey of regrownCells) {
    await database
      .delete(worldTreeCuts)
      .where(and(eq(worldTreeCuts.chunkKey, chunkKey), eq(worldTreeCuts.cellKey, cellKey)));
  }

  return active;
}

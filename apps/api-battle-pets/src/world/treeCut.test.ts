// Copyright 2026 Anderson. All Rights Reserved.

import { readdirSync, readFileSync } from 'node:fs';
import { join } from 'node:path';

import { PGlite } from '@electric-sql/pglite';
import { drizzle } from 'drizzle-orm/pglite';
import { beforeAll, describe, expect, test } from 'bun:test';

import { hasRegrown } from './treeCut.pure';
import { listActiveCuts, recordCut } from './treeCut.use-case';
import { worldTreeCuts } from './treeCut.schema';

const client = new PGlite();
type Database = NonNullable<Parameters<typeof recordCut>[1]>;
const pgliteDb = drizzle(client);
const database = pgliteDb as unknown as Database;

beforeAll(async () => {
  const dir = join(import.meta.dir, '../../drizzle');
  for (const file of readdirSync(dir).filter((f) => f.endsWith('.sql')).sort()) {
    for (const stmt of readFileSync(join(dir, file), 'utf8').split('--> statement-breakpoint')) {
      if (stmt.trim()) await client.exec(stmt.trim());
    }
  }
});

describe('a rebrota pura, espelho do cliente', () => {
  test('cortada ate o prazo, rebrotada depois', () => {
    expect(hasRegrown(100, 106, 7)).toBe(false);
    expect(hasRegrown(100, 107, 7)).toBe(true);
    expect(hasRegrown(100, 90, 7)).toBe(false); // relogio incoerente
    expect(hasRegrown(100, 100, 0)).toBe(true); // prazo degenerado
  });
});

describe('a marca do corte no servidor (MV3)', () => {
  const CHUNK = 'chunk-3-4';
  const DEADLINE = 7;

  test('corte fica ATIVO antes do prazo — sobrevive a fechar e reabrir', async () => {
    await recordCut({ chunkKey: CHUNK, cellKey: 'c-1-1', cutAtWorldAgeDays: 100 }, database);

    // "Reabrir" antes do prazo: a idade do mundo andou pouco, o corte continua.
    const antes = await listActiveCuts(CHUNK, 105, DEADLINE, database);
    expect(antes.map((c) => c.cellKey)).toContain('c-1-1');
  });

  test('depois do prazo o corte REBROTA sozinho e some da tabela', async () => {
    await recordCut({ chunkKey: CHUNK, cellKey: 'c-2-2', cutAtWorldAgeDays: 200 }, database);

    // Idade do mundo passou o prazo inteiro: rebrotou, sai da lista ativa...
    const depois = await listActiveCuts(CHUNK, 210, DEADLINE, database);
    expect(depois.map((c) => c.cellKey)).not.toContain('c-2-2');

    // ...e a marca morta foi PODADA da tabela (nao vira excecao permanente).
    const restante = await database
      .select()
      .from(worldTreeCuts);
    expect(restante.map((r) => r.cellKey)).not.toContain('c-2-2');
  });

  test('recortar a mesma celula ATUALIZA o carimbo, nao empilha', async () => {
    await recordCut({ chunkKey: CHUNK, cellKey: 'c-9-9', cutAtWorldAgeDays: 300 }, database);
    await recordCut({ chunkKey: CHUNK, cellKey: 'c-9-9', cutAtWorldAgeDays: 350 }, database);

    const rows = await database
      .select()
      .from(worldTreeCuts);
    const daCelula = rows.filter((r) => r.cellKey === 'c-9-9');
    expect(daCelula).toHaveLength(1);
    expect(daCelula[0]!.cutAtWorldAgeDays).toBe(350);
  });

  test('CONTRAPESO: pedaco sem corte nenhum devolve lista vazia — mata intacta nao grava', async () => {
    const virgem = await listActiveCuts('chunk-nunca-tocado', 100, DEADLINE, database);
    expect(virgem).toHaveLength(0);
  });
});

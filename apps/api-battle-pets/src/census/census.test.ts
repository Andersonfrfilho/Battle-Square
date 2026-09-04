// Copyright 2026 Anderson. All Rights Reserved.

import { readdirSync, readFileSync } from 'node:fs';
import { join } from 'node:path';

import { PGlite } from '@electric-sql/pglite';
import { drizzle } from 'drizzle-orm/pglite';
import { beforeAll, expect, test } from 'bun:test';

import { playerAccounts } from '../account/account.schema';
import { registerCapture } from '../ownership/ownership.use-case';
import { speciesCensus, speciesCountOf } from './census.use-case';

const client = new PGlite();
type Database = NonNullable<Parameters<typeof speciesCensus>[0]>;
const pgliteDb = drizzle(client);
const database = pgliteDb as unknown as Database;

beforeAll(async () => {
  const dir = join(import.meta.dir, '../../drizzle');
  for (const file of readdirSync(dir).filter((f) => f.endsWith('.sql')).sort()) {
    for (const stmt of readFileSync(join(dir, file), 'utf8').split('--> statement-breakpoint')) {
      if (stmt.trim()) await client.exec(stmt.trim());
    }
  }
  const contas = await pgliteDb.insert(playerAccounts).values([
    { email: 'a@t.dev', passwordHash: 'h' },
    { email: 'b@t.dev', passwordHash: 'h' },
  ]).returning();

  // Duas contas com o mesmo pet raro, uma com um comum — o censo AGREGA entre
  // contas, e e isso que a migracao mede.
  await registerCapture({ ownerAccountId: contas[0]!.id, catalogId: 'raro-01', cap: 500 }, database as never);
  await registerCapture({ ownerAccountId: contas[1]!.id, catalogId: 'raro-01', cap: 500 }, database as never);
  await registerCapture({ ownerAccountId: contas[0]!.id, catalogId: 'comum-01', cap: 500 }, database as never);
});

test('o censo agrega a especie entre TODAS as contas (MN6)', async () => {
  const census = await speciesCensus(database);
  const raro = census.find((c) => c.catalogId === 'raro-01');
  const comum = census.find((c) => c.catalogId === 'comum-01');
  expect(raro?.count).toBe(2);
  expect(comum?.count).toBe(1);
});

test('especie que ninguem tem conta ZERO, nao erro', async () => {
  expect(await speciesCountOf('nunca-capturado', database)).toBe(0);
});

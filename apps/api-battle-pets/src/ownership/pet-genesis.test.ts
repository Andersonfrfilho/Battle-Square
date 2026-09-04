// Copyright 2026 Anderson. All Rights Reserved.

import { readdirSync, readFileSync } from 'node:fs';
import { join } from 'node:path';

import { PGlite } from '@electric-sql/pglite';
import { drizzle } from 'drizzle-orm/pglite';
import { beforeAll, expect, test } from 'bun:test';

import { playerAccounts } from '../account/account.schema';
import { registerCapture } from './ownership.use-case';

const client = new PGlite();
type Database = NonNullable<Parameters<typeof registerCapture>[1]>;
const pgliteDb = drizzle(client);
const database = pgliteDb as unknown as Database;

let dono = '';

beforeAll(async () => {
  const dir = join(import.meta.dir, '../../drizzle');
  for (const file of readdirSync(dir).filter((f) => f.endsWith('.sql')).sort()) {
    for (const stmt of readFileSync(join(dir, file), 'utf8').split('--> statement-breakpoint')) {
      if (stmt.trim()) await client.exec(stmt.trim());
    }
  }
  const [conta] = await pgliteDb.insert(playerAccounts).values({ email: 'dono@teste.dev', passwordHash: 'h' }).returning();
  dono = conta!.id;
});

test('o pet nasce carimbado com a idade do mundo (MV6)', async () => {
  const capturado = await registerCapture(
    { ownerAccountId: dono, catalogId: 'moss-turtle-01', cap: 500, genesisWorldAgeDays: 42 },
    database,
  );
  expect(capturado.kind).toBe('captured');
  if (capturado.kind === 'captured') {
    expect(capturado.pet.genesisWorldAgeDays).toBe(42);
  }
});

test('sem carimbo informado, nasce no dia zero — nunca undefined', async () => {
  const capturado = await registerCapture(
    { ownerAccountId: dono, catalogId: 'sun-moth-02', cap: 500 },
    database,
  );
  expect(capturado.kind).toBe('captured');
  if (capturado.kind === 'captured') {
    expect(capturado.pet.genesisWorldAgeDays).toBe(0);
  }
});

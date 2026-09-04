// Copyright 2026 Anderson. All Rights Reserved.

import { readdirSync, readFileSync } from 'node:fs';
import { join } from 'node:path';

import { PGlite } from '@electric-sql/pglite';
import { drizzle } from 'drizzle-orm/pglite';
import { beforeAll, describe, expect, test } from 'bun:test';

import { ageInDaysBetween } from './worldAge.pure';
import { getWorldAge } from './world.use-case';
import { worldState } from './world.schema';

const client = new PGlite();
type Database = NonNullable<Parameters<typeof getWorldAge>[1]>;
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

describe('a subtracao pura (worldAge.pure)', () => {
  test('a idade cresce com o tempo, sem relogio dentro da regra', () => {
    const born = new Date('2026-01-01T00:00:00Z');
    expect(ageInDaysBetween(born, new Date('2026-01-01T00:00:00Z'))).toBe(0);
    expect(ageInDaysBetween(born, new Date('2026-01-11T00:00:00Z'))).toBe(10);
    expect(ageInDaysBetween(born, new Date('2027-01-01T00:00:00Z'))).toBe(365);
  });

  test('relogio torto (agora antes do nascimento) nunca da idade negativa', () => {
    const born = new Date('2026-06-01T00:00:00Z');
    expect(ageInDaysBetween(born, new Date('2026-05-01T00:00:00Z'))).toBe(0);
  });
});

describe('a data de nascimento gravada UMA vez (getWorldAge)', () => {
  test('duas leituras em dias diferentes, sem ninguem jogar, dao idades diferentes', async () => {
    const diaUm = new Date('2026-03-10T12:00:00Z');
    const primeira = await getWorldAge(diaUm, database);

    // Uma semana depois, ninguem jogou entre as duas leituras.
    const diaOito = new Date('2026-03-17T12:00:00Z');
    const segunda = await getWorldAge(diaOito, database);

    // O nascimento NAO mudou — gravado uma vez, na primeira leitura.
    expect(segunda.bornAt.getTime()).toBe(primeira.bornAt.getTime());
    // Mas a idade subiu, que e o que ElapsedHours nao faz hoje (zera no boot).
    expect(primeira.ageInDays).toBe(0);
    expect(segunda.ageInDays).toBe(7);
    expect(segunda.ageInDays).toBeGreaterThan(primeira.ageInDays);
  });
});

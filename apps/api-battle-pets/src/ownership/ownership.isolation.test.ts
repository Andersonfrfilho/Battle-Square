// Copyright 2026 Anderson. All Rights Reserved.

import { readdirSync, readFileSync } from 'node:fs';
import { join } from 'node:path';

import { PGlite } from '@electric-sql/pglite';
import { drizzle } from 'drizzle-orm/pglite';
import { beforeAll, describe, expect, test } from 'bun:test';

import { playerAccounts } from '../account/account.schema';
import { getOwnedPet, listOwnedPets, registerCapture } from './ownership.use-case';

/**
 * PS3/PS4 — o isolamento por OBJETO, provado num Postgres DE VERDADE.
 *
 * PGlite é Postgres em memória: as MESMAS migrations (0000-0009, em ordem — o
 * que já é um teste por si), a MESMA unicidade, o MESMO drizzle. O teste
 * negativo de isolamento aqui cumpre o contrapeso da task: se o `where` do
 * dono sair da query, a lista de A passa a conter o pet de B — e este arquivo
 * REPROVA. Um teste que passasse com e sem o filtro estaria medindo que a
 * rota responde, não que isola.
 */

const client = new PGlite();

// O cast passa por `unknown` DE PROPÓSITO: o driver do teste (pglite) e o de
// produção (postgres-js) têm sessões de tipos diferentes, mas o SQL que o
// drizzle emite é o mesmo — e é o SQL que este arquivo testa. Um `Database`
// estrutural no caso de uso acoplaria produção ao driver de teste, que é pior.
type Database = NonNullable<Parameters<typeof listOwnedPets>[1]>;
const pgliteDb = drizzle(client);
const database = pgliteDb as unknown as Database;

let contaA = '';
let contaB = '';

beforeAll(async () => {
  // AS MIGRATIONS DO REPO, em ordem, inteiras: criar as tabelas à mão aqui
  // seria a segunda cópia do schema — e ela concordaria com a primeira até a
  // próxima migration.
  const migrationsDir = join(import.meta.dir, '../../drizzle');
  const files = readdirSync(migrationsDir).filter((f) => f.endsWith('.sql')).sort();

  for (const file of files) {
    const sql = readFileSync(join(migrationsDir, file), 'utf8');
    for (const statement of sql.split('--> statement-breakpoint')) {
      const trimmed = statement.trim();
      if (trimmed.length > 0) {
        await client.exec(trimmed);
      }
    }
  }

  const inserted = await pgliteDb
    .insert(playerAccounts)
    .values([
      { email: 'a@teste.dev', passwordHash: 'hash-a' },
      { email: 'b@teste.dev', passwordHash: 'hash-b' },
    ])
    .returning();

  contaA = inserted[0]!.id;
  contaB = inserted[1]!.id;
});

describe('a captura idempotente (PS4)', () => {
  test('a mesma captura duas vezes deixa UMA linha — e a segunda devolve a existente', async () => {
    const primeira = await registerCapture(
      { ownerAccountId: contaA, catalogId: 'fire-drake-01', cap: 500 }, database);
    expect(primeira.kind).toBe('captured');

    const segunda = await registerCapture(
      { ownerAccountId: contaA, catalogId: 'fire-drake-01', cap: 500 }, database);
    expect(segunda.kind).toBe('already-owned');
    if (segunda.kind === 'already-owned' && primeira.kind === 'captured') {
      expect(segunda.pet.id).toBe(primeira.pet.id);
    }

    const lista = await listOwnedPets(
      { ownerAccountId: contaA, page: 1, perPage: 50 }, database);
    expect(lista.total).toBe(1);
  });

  test('DUAS CONTAS com o MESMO catalogo e o caso normal — a unicidade e por (dono, catalogo)', async () => {
    // O contrapeso da PS4: uma unicidade mal escrita (só por catálogo)
    // transformaria o caso normal em erro.
    const deB = await registerCapture(
      { ownerAccountId: contaB, catalogId: 'fire-drake-01', cap: 500 }, database);
    expect(deB.kind).toBe('captured');
  });
});

describe('o isolamento por objeto (PS3)', () => {
  test('a lista de A NUNCA contem pet de B — mesmo com o mesmo catalogo', async () => {
    // Este é O teste negativo: remover o `where` do dono da query faz a
    // lista de A conter o fire-drake de B (mesmo catálogo, dono diferente) —
    // e as duas asserções abaixo reprovam.
    const listaDeA = await listOwnedPets(
      { ownerAccountId: contaA, page: 1, perPage: 50 }, database);

    expect(listaDeA.total).toBe(1);
    for (const pet of listaDeA.items) {
      expect(pet.ownerAccountId).toBe(contaA);
    }
  });

  test('A pedindo o pet de B recebe owned-by-another — o 403 da decisao 38-b', async () => {
    const listaDeB = await listOwnedPets(
      { ownerAccountId: contaB, page: 1, perPage: 50 }, database);
    const petDeB = listaDeB.items[0]!;

    const resultado = await getOwnedPet(
      { ownerAccountId: contaA, petId: petDeB.id }, database);
    expect(resultado.kind).toBe('owned-by-another');
  });

  test('pet inexistente e not-found — 403 e so para o que EXISTE', async () => {
    const resultado = await getOwnedPet(
      { ownerAccountId: contaA, petId: '00000000-0000-0000-0000-000000000000' }, database);
    expect(resultado.kind).toBe('not-found');
  });
});

describe('o teto da colecao (decisao 38-b: config)', () => {
  test('alem do teto recusa com codigo estavel — e o teto nao quebra a idempotencia', async () => {
    // Teto de 2, injetado: o teto de produção (500) é config, e testar 500
    // capturas seria testar paciência.
    const segunda = await registerCapture(
      { ownerAccountId: contaA, catalogId: 'moss-turtle-02', cap: 2 }, database);
    expect(segunda.kind).toBe('captured');

    const terceira = await registerCapture(
      { ownerAccountId: contaA, catalogId: 'sky-manta-03', cap: 2 }, database);
    expect(terceira.kind).toBe('collection-full');

    // A REPETIÇÃO de quem já está no teto continua respondendo o pet
    // existente — o teto recusa CRESCER, nunca reler.
    const repetida = await registerCapture(
      { ownerAccountId: contaA, catalogId: 'fire-drake-01', cap: 2 }, database);
    expect(repetida.kind).toBe('already-owned');
  });
});

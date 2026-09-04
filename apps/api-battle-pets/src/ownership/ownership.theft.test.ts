// Copyright 2026 Anderson. All Rights Reserved.

import { readdirSync, readFileSync } from 'node:fs';
import { join } from 'node:path';

import { PGlite } from '@electric-sql/pglite';
import { drizzle } from 'drizzle-orm/pglite';
import { eq } from 'drizzle-orm';
import { beforeAll, describe, expect, test } from 'bun:test';

import { playerAccounts } from '../account/account.schema';
import { isStolen } from './ownership.pure';
import { ownershipAuditLog } from './ownership.schema';
import { listWantedAccounts, registerCapture, stealPet } from './ownership.use-case';

/**
 * CR2/CR3 no servidor — o roubo efetua e DEIXA RASTRO, num Postgres de verdade.
 *
 * A decisão de PODER roubar é do jogo; aqui se prova o EFEITO: a posse muda de
 * mão, o pet fica marcado como roubado (CR6 lê isso), e a mudança é auditada na
 * mesma transação — posse que muda sem registro é roubo que ninguém prova.
 */

const client = new PGlite();
type Database = NonNullable<Parameters<typeof registerCapture>[1]>;
const pgliteDb = drizzle(client);
const database = pgliteDb as unknown as Database;

let vitima = '';
let ladrao = '';
let petId = '';

beforeAll(async () => {
  const dir = join(import.meta.dir, '../../drizzle');
  for (const file of readdirSync(dir).filter((f) => f.endsWith('.sql')).sort()) {
    for (const stmt of readFileSync(join(dir, file), 'utf8').split('--> statement-breakpoint')) {
      if (stmt.trim()) await client.exec(stmt.trim());
    }
  }

  const contas = await pgliteDb.insert(playerAccounts).values([
    { email: 'vitima@teste.dev', passwordHash: 'h' },
    { email: 'ladrao@teste.dev', passwordHash: 'h' },
  ]).returning();
  vitima = contas[0]!.id;
  ladrao = contas[1]!.id;

  const capturado = await registerCapture(
    { ownerAccountId: vitima, catalogId: 'fire-drake-01', cap: 500 }, database);
  if (capturado.kind !== 'captured') throw new Error('setup: captura falhou');
  petId = capturado.pet.id;
});

describe('stealPet (CR2)', () => {
  test('roubar muda o dono, marca STOLEN e guarda de quem foi', async () => {
    const resultado = await stealPet(
      { petId, expectedOwnerAccountId: vitima, thiefAccountId: ladrao }, database);

    expect(resultado.kind).toBe('stolen');
    if (resultado.kind === 'stolen') {
      expect(resultado.pet.ownerAccountId).toBe(ladrao);
      expect(isStolen({
        catalogId: resultado.pet.catalogId,
        ownerAccountId: resultado.pet.ownerAccountId,
        acquisition: resultado.pet.acquisition as never,
        stolenFromAccountId: resultado.pet.stolenFromAccountId,
      })).toBe(true);
      // De quem foi roubado e a VITIMA — e o que a devolucao usa.
      expect(resultado.pet.stolenFromAccountId).toBe(vitima);
    }
  });

  test('a POSSE MUDOU desde a batalha: recusa em vez de roubar de um fantasma', async () => {
    // O pet ja e do ladrao agora; tentar roubar esperando a vitima como dono
    // bate no owner-mismatch.
    const resultado = await stealPet(
      { petId, expectedOwnerAccountId: vitima, thiefAccountId: 'terceiro' }, database);
    expect(resultado.kind).toBe('owner-mismatch');
  });
});

describe('a trilha de auditoria (CR3)', () => {
  test('o roubo deixou UMA entrada, com IDs opacos e SEM PII', async () => {
    const trilha = await pgliteDb
      .select()
      .from(ownershipAuditLog)
      .where(eq(ownershipAuditLog.petId, petId));

    expect(trilha.length).toBe(1);
    const entrada = trilha[0]!;
    expect(entrada.action).toBe('theft');
    expect(entrada.fromAccountId).toBe(vitima);
    expect(entrada.toAccountId).toBe(ladrao);

    // O CONTRAPESO (invariante 17): a trilha SO tem ids opacos. Nenhuma
    // coluna carrega e-mail, nome ou qualquer PII — provado pela forma da
    // linha, que e o unico jeito de garantir "nao loga PII" por construcao.
    const colunas = Object.keys(entrada);
    for (const proibido of ['email', 'name', 'nome', 'password', 'message']) {
      expect(colunas).not.toContain(proibido);
    }
  });
});

describe('a lista de procurados (CR4)', () => {
  test('UM roubo nao procura; a REINCIDENCIA sim', async () => {
    // O setup ja roubou fire-drake uma vez (ladrao). Um roubo so nao procura.
    expect(await listWantedAccounts(database)).not.toContain(ladrao);

    // O ladrao rouba de novo: um segundo pet de outra vitima. Agora e
    // procurado — a reincidencia cruza o limiar.
    const outro = await registerCapture(
      { ownerAccountId: vitima, catalogId: 'moss-turtle-02', cap: 500 }, database);
    if (outro.kind !== 'captured') throw new Error('setup do segundo pet falhou');

    const segundoRoubo = await stealPet(
      { petId: outro.pet.id, expectedOwnerAccountId: vitima, thiefAccountId: ladrao }, database);
    expect(segundoRoubo.kind).toBe('stolen');

    expect(await listWantedAccounts(database)).toContain(ladrao);
  });
});

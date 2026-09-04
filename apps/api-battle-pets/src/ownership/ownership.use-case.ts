// Copyright 2026 Anderson. All Rights Reserved.

import { and, count, eq } from 'drizzle-orm';

import { db } from '../db/client';
import { AcquisitionKind, ownedPets, type OwnedPet } from './ownership.schema';

/**
 * O banco INJETADO com o global por padrão: os testes de isolamento rodam num
 * Postgres de verdade em memória (PGlite), e o teste que prova "a query
 * filtra o dono por construção" só vale se puder apontar para um banco seu.
 */
type Database = typeof db;

export type ListOwnedResult = {
  items: OwnedPet[];
  total: number;
};

/**
 * A coleção DA CONTA — e de mais ninguém, POR CONSTRUÇÃO (PS3).
 *
 * O `where` do dono não é decoração: é a autorização por objeto na própria
 * query (BOLA/API1). O teste negativo de isolamento reprova se ele sair.
 */
export async function listOwnedPets(
  params: { ownerAccountId: string; page: number; perPage: number },
  database: Database = db,
): Promise<ListOwnedResult> {
  const ownedByAccount = eq(ownedPets.ownerAccountId, params.ownerAccountId);

  const [items, [totalRow]] = await Promise.all([
    database
      .select()
      .from(ownedPets)
      .where(ownedByAccount)
      .orderBy(ownedPets.createdAt)
      .limit(params.perPage)
      .offset((params.page - 1) * params.perPage),
    database.select({ value: count() }).from(ownedPets).where(ownedByAccount),
  ]);

  return { items, total: totalRow?.value ?? 0 };
}

export type GetOwnedResult =
  | { kind: 'found'; pet: OwnedPet }
  | { kind: 'not-found' }
  /**
   * Existe, e é de OUTRO (decisão 38-b: a recusa é 403, honesta). A distinção
   * mora AQUI e não no controller, porque ela exige ler o dono real — e quem
   * lê dono é o caso de uso, nunca a camada HTTP.
   */
  | { kind: 'owned-by-another' };

export async function getOwnedPet(
  params: { ownerAccountId: string; petId: string },
  database: Database = db,
): Promise<GetOwnedResult> {
  const [pet] = await database
    .select()
    .from(ownedPets)
    .where(eq(ownedPets.id, params.petId))
    .limit(1);

  if (!pet) {
    return { kind: 'not-found' };
  }

  return pet.ownerAccountId === params.ownerAccountId
    ? { kind: 'found', pet }
    : { kind: 'owned-by-another' };
}

export type CaptureResult =
  | { kind: 'captured'; pet: OwnedPet }
  /** Já era seu — a captura é IDEMPOTENTE, e repetir devolve o que existe. */
  | { kind: 'already-owned'; pet: OwnedPet }
  /** O teto (decisão 38-b: 500, config) — recusa com código estável. */
  | { kind: 'collection-full'; cap: number };

export async function registerCapture(
  params: { ownerAccountId: string; catalogId: string; cap: number },
  database: Database = db,
): Promise<CaptureResult> {
  return database.transaction(async (tx) => {
    // O teto ANTES do insert, na mesma transação: entre a contagem e a
    // escrita não cabe outra captura desta conta.
    const [totalRow] = await tx
      .select({ value: count() })
      .from(ownedPets)
      .where(eq(ownedPets.ownerAccountId, params.ownerAccountId));

    const total = totalRow?.value ?? 0;

    // A IDEMPOTÊNCIA é da CONSTRAINT, não do cliente (PS1): o insert com
    // onConflictDoNothing bate na unicidade (dono, catálogo) e devolve
    // vazio — e aí se lê o que já existia. Checar antes com SELECT seria a
    // idempotência do cliente com roupa de servidor: entre o select e o
    // insert cabe outra escrita.
    const [created] = await tx
      .insert(ownedPets)
      .values({
        ownerAccountId: params.ownerAccountId,
        catalogId: params.catalogId,
        acquisition: AcquisitionKind.CAPTURED,
      })
      .onConflictDoNothing({
        target: [ownedPets.ownerAccountId, ownedPets.catalogId],
      })
      .returning();

    if (created) {
      // O teto só recusa CRESCER: a repetição idempotente de quem já está no
      // teto continua respondendo o pet existente, nunca "cheio" — por isso
      // ele é conferido no caminho do insert NOVO.
      if (total >= params.cap) {
        tx.rollback();
      }
      return { kind: 'captured', pet: created } as const;
    }

    const [existing] = await tx
      .select()
      .from(ownedPets)
      .where(
        and(
          eq(ownedPets.ownerAccountId, params.ownerAccountId),
          eq(ownedPets.catalogId, params.catalogId),
        ),
      )
      .limit(1);

    if (!existing) {
      // Conflito sem linha é corrida com DELETE: sinaliza para o chamador
      // tentar de novo em vez de inventar um pet.
      throw new Error('captura em corrida com remocao');
    }

    return { kind: 'already-owned', pet: existing } as const;
  }).catch((error: unknown) => {
    // O rollback do teto chega aqui como erro de transação abortada.
    if (error instanceof Error && error.message.includes('Rollback')) {
      return { kind: 'collection-full', cap: params.cap } as CaptureResult;
    }
    throw error;
  });
}

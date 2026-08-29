// Copyright 2026 Anderson. All Rights Reserved.

import { and, eq, gt, inArray } from 'drizzle-orm';

import { db } from '../db/client';
import { type NewPet, type Pet, type PetMove, petMoves, pets } from './pet.schema';
import type { CreatePetInput, ListPetsQuery, UpdatePetInput } from './pet.validation';

export type CreatePetResult = Pet;

export async function createPet(input: CreatePetInput): Promise<CreatePetResult> {
  const values: NewPet = {
    name: input.name,
    type: input.type,
    attack: input.attack,
    defense: input.defense,
    speed: input.speed,
    maxHealth: input.maxHealth,
  };

  // TRANSAÇÃO: pet e golpes nascem juntos ou não nascem.
  //
  // Sem ela, uma falha entre os dois inserts deixaria um pet sem golpe no
  // catálogo — e ele passaria a ser exportado, assinado e jogável assim,
  // silenciosamente mais fraco que os outros.
  return db.transaction(async (tx) => {
    const [created] = await tx.insert(pets).values(values).returning();

    if (input.moves && input.moves.length > 0) {
      await tx.insert(petMoves).values(
        input.moves.map((move, slot) => ({
          petId: created!.id,
          // O SLOT vem da posição no array, não do cliente: ele é o índice que
          // viaja no commit da batalha, e deixar o chamador escolher abriria
          // espaço para dois golpes no mesmo slot.
          slot,
          name: move.name,
          power: move.power,
          terrainEffect: move.terrainEffect,
        })),
      );
    }

    return created!;
  });
}

export type ListPetsResult = {
  items: Pet[];
  total: number;
  // Golpes ao lado dos pets, e não dentro deles: `Pet` é o registro da tabela,
  // e enfiar golpe ali faria o tipo do banco mentir sobre o que ele é.
  movesByPetId: Map<string, PetMove[]>;
};

export async function listPets(query: ListPetsQuery): Promise<ListPetsResult> {
  const filters = query.updatedAfter ? [gt(pets.updatedAt, new Date(query.updatedAfter))] : [];
  const whereClause = filters.length > 0 ? and(...filters) : undefined;

  const offset = (query.page - 1) * query.perPage;

  const [items, countRows] = await Promise.all([
    db.select().from(pets).where(whereClause).limit(query.perPage).offset(offset),
    db.select({ id: pets.id }).from(pets).where(whereClause),
  ]);

  const movesByPetId = await loadMovesForPets(items.map((pet) => pet.id));

  return { items, total: countRows.length, movesByPetId };
}

/**
 * Golpes de vários pets numa consulta só.
 *
 * Uma consulta por pet seria N+1 no caminho que o worker de sync percorre a
 * cada ciclo — `code-standart.md` §15 manda auditar exatamente isso.
 */
export async function loadMovesForPets(petIds: string[]): Promise<Map<string, PetMove[]>> {
  const porPet = new Map<string, PetMove[]>();
  if (petIds.length === 0) {
    return porPet;
  }

  const rows = await db.select().from(petMoves).where(inArray(petMoves.petId, petIds));

  for (const row of rows) {
    const existentes = porPet.get(row.petId) ?? [];
    existentes.push(row);
    porPet.set(row.petId, existentes);
  }

  return porPet;
}

export async function getPetById(id: string): Promise<Pet | undefined> {
  const [pet] = await db.select().from(pets).where(eq(pets.id, id));
  return pet;
}

export async function updatePet(id: string, input: UpdatePetInput): Promise<Pet | undefined> {
  const [updated] = await db
    .update(pets)
    .set({
      name: input.name,
      type: input.type,
      attack: input.attack,
      defense: input.defense,
      speed: input.speed,
      maxHealth: input.maxHealth,
      updatedAt: new Date(),
    })
    .where(eq(pets.id, id))
    .returning();
  return updated;
}

export async function deletePet(id: string): Promise<boolean> {
  const deleted = await db.delete(pets).where(eq(pets.id, id)).returning({ id: pets.id });
  return deleted.length > 0;
}

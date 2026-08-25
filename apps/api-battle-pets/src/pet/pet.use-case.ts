// Copyright 2026 Anderson. All Rights Reserved.

import { and, eq, gt } from 'drizzle-orm';

import { db } from '../db/client';
import { type NewPet, type Pet, pets } from './pet.schema';
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
  const [created] = await db.insert(pets).values(values).returning();
  return created!;
}

export type ListPetsResult = { items: Pet[]; total: number };

export async function listPets(query: ListPetsQuery): Promise<ListPetsResult> {
  const filters = query.updatedAfter ? [gt(pets.updatedAt, new Date(query.updatedAfter))] : [];
  const whereClause = filters.length > 0 ? and(...filters) : undefined;

  const offset = (query.page - 1) * query.perPage;

  const [items, countRows] = await Promise.all([
    db.select().from(pets).where(whereClause).limit(query.perPage).offset(offset),
    db.select({ id: pets.id }).from(pets).where(whereClause),
  ]);

  return { items, total: countRows.length };
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

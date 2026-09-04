// Copyright 2026 Anderson. All Rights Reserved.

import { count, eq } from 'drizzle-orm';

import { db } from '../db/client';
import { ownedPets } from '../ownership/ownership.schema';

type Database = typeof db;

export type SpeciesCount = {
  readonly catalogId: string;
  readonly count: number;
};

/**
 * O CENSO AGREGADO de uma especie (mae-natureza MN6): quantos existem entre
 * TODAS as contas — nao por save, nao por conta. E o `FNatureCenso` REAL que a
 * correcao de migracao consome, medido sobre a posse que ja mora no servidor
 * (PS1), nunca um numero inventado.
 */
export async function speciesCensus(database: Database = db): Promise<SpeciesCount[]> {
  const rows = await database
    .select({ catalogId: ownedPets.catalogId, value: count() })
    .from(ownedPets)
    .groupBy(ownedPets.catalogId);

  return rows.map((r) => ({ catalogId: r.catalogId, count: Number(r.value) }));
}

/** O censo de UMA especie — zero quando ninguem a tem (nao um erro). */
export async function speciesCountOf(catalogId: string, database: Database = db): Promise<number> {
  const [row] = await database
    .select({ value: count() })
    .from(ownedPets)
    .where(eq(ownedPets.catalogId, catalogId));
  return Number(row?.value ?? 0);
}

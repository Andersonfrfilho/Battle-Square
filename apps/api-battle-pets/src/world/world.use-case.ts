// Copyright 2026 Anderson. All Rights Reserved.

import { db } from '../db/client';
import { ageInDaysBetween } from './worldAge.pure';
import { worldState } from './world.schema';

type Database = typeof db;

export type WorldAge = {
  readonly bornAt: Date;
  readonly ageInDays: number;
};

/**
 * A idade do mundo AGORA. Semeia a data de nascimento na primeira leitura
 * (o mundo nasce uma vez) e, dali em diante, so a le e subtrai.
 *
 * `now` entra por argumento para o teste controlar o relogio; em producao o
 * chamador passa `new Date()` na fronteira.
 */
export async function getWorldAge(
  now: Date = new Date(),
  database: Database = db,
): Promise<WorldAge> {
  const existing = await database.select().from(worldState).limit(1);

  const bornAt =
    existing[0]?.bornAt ??
    (await database.insert(worldState).values({ bornAt: now }).returning())[0]!.bornAt;

  return { bornAt, ageInDays: ageInDaysBetween(bornAt, now) };
}

// Copyright 2026 Anderson. All Rights Reserved.

import { integer, pgTable, timestamp, uuid, varchar } from 'drizzle-orm/pg-core';

// code-standart.md §8: PK em UUID para dado público; proibido ENUM nativo
// — "type" fica VARCHAR, e é a camada BattleSquare (nunca o núcleo
// BattleSim) quem traduz isso para FGameplayTag (AD-008/AD-012).
export const pets = pgTable('pets', {
  id: uuid('id').primaryKey().defaultRandom(),
  name: varchar('name', { length: 80 }).notNull(),
  type: varchar('type', { length: 40 }).notNull(),
  attack: integer('attack').notNull(),
  defense: integer('defense').notNull(),
  speed: integer('speed').notNull(),
  maxHealth: integer('max_health').notNull(),
  createdAt: timestamp('created_at', { withTimezone: true }).defaultNow().notNull(),
  updatedAt: timestamp('updated_at', { withTimezone: true }).defaultNow().notNull(),
});

export type Pet = typeof pets.$inferSelect;
export type NewPet = typeof pets.$inferInsert;

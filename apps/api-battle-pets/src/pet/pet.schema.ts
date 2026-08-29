// Copyright 2026 Anderson. All Rights Reserved.

import { integer, pgTable, timestamp, unique, uuid, varchar } from 'drizzle-orm/pg-core';

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

// golpes-por-pet, DP-golpe-01/02: QUATRO golpes por pet, vindos do backend e
// não do tipo — é o que permite dois pets do mesmo tipo jogarem diferente.
//
// Tabela normalizada aqui (fonte da verdade), serializada como JSON canônico no
// espelho: o leitor C++ precisa reconstruir o payload assinado byte a byte, e
// um join dentro do SQLite só para isso seria complexidade sem ganho.
export const petMoves = pgTable(
  'pet_moves',
  {
    id: uuid('id').primaryKey().defaultRandom(),
    petId: uuid('pet_id')
      .notNull()
      .references(() => pets.id, { onDelete: 'cascade' }),
    // 0 a 3. A ORDEM faz parte do contrato de assinatura: o índice do golpe é
    // o que viaja no commit (DP-golpe-04), então trocar slot muda a jogada.
    slot: integer('slot').notNull(),
    name: varchar('name', { length: 60 }).notNull(),
    power: integer('power').notNull(),
    /**
     * O que o golpe DEIXA na casa que ele acertou.
     *
     * 'none' | 'water' | 'damage'. VARCHAR e não ENUM nativo (code-standart.md
     * §8), e o significado de cada valor vive no núcleo do jogo — o backend
     * guarda o dado, não a regra.
     */
    terrainEffect: varchar('terrain_effect', { length: 16 }).notNull().default('none'),
  },
  (table) => ({
    petSlotUnique: unique('pet_moves_pet_slot_unique').on(table.petId, table.slot),
  }),
);

export type Pet = typeof pets.$inferSelect;
export type PetMove = typeof petMoves.$inferSelect;
export type NewPetMove = typeof petMoves.$inferInsert;

export const MOVES_PER_PET = 4;
export type NewPet = typeof pets.$inferInsert;

// Copyright 2026 Anderson. All Rights Reserved.

import { index, integer, pgTable, timestamp, unique, uuid, varchar } from 'drizzle-orm/pg-core';

import { playerAccounts } from '../account/account.schema';

/**
 * COMO o pet chegou a este dono. Varchar, nunca ENUM nativo (code-standart §8).
 *
 * Existe porque roubo e captura precisam ser distinguiveis DEPOIS: um pet
 * roubado carrega a marca, e a marca e o que permite devolve-lo e o que faz a
 * batalha delatar quem o carrega.
 */
export const AcquisitionKind = {
  CAPTURED: 'captured',
  STARTER: 'starter',
  TRADED: 'traded',
  STOLEN: 'stolen',
  RECLAIMED: 'reclaimed',
} as const;

export type AcquisitionKind = (typeof AcquisitionKind)[keyof typeof AcquisitionKind];

/**
 * A POSSE mora no servidor, nao no save do processo.
 *
 * Era um USaveGame local, e por isso dois jogadores numa mesma partida
 * escreviam no mesmo arquivo (B-005). Separar por dono tirou a mistura; so
 * mover para ca torna a posse verificavel por quem nao e o dono — que e o que
 * roubo, comercio e recompensa exigem.
 */
export const ownedPets = pgTable(
  'owned_pets',
  {
    id: uuid('id').primaryKey().defaultRandom(),

    ownerAccountId: uuid('owner_account_id')
      .notNull()
      .references(() => playerAccounts.id, { onDelete: 'cascade' }),

    /** Qual pet do catalogo e este. */
    catalogId: varchar('catalog_id', { length: 64 }).notNull(),

    experience: integer('experience').notNull().default(0),
    musculature: integer('musculature').notNull().default(0),
    personality: integer('personality').notNull().default(0),

    acquisition: varchar('acquisition', { length: 16 })
      .notNull()
      .default(AcquisitionKind.CAPTURED),

    /**
     * De quem foi ROUBADO, quando foi.
     *
     * Nulo no caso normal. Preenchido, e o que permite devolver o pet e o que
     * faz a batalha delatar quem o carrega — inclusive quem comprou de boa-fe.
     */
    stolenFromAccountId: uuid('stolen_from_account_id').references(() => playerAccounts.id, {
      onDelete: 'set null',
    }),

    /**
     * A IDADE DO MUNDO em que o pet nasceu (MV6, decisao 33). A idade do pet e
     * "idade do mundo agora - este carimbo" — offline-safe por construcao, o
     * mesmo padrao da idade do mundo (MV1). Nao e createdAt (relogio de
     * parede): o envelhecimento corre no tempo do MUNDO, nao no do servidor.
     */
    genesisWorldAgeDays: integer('genesis_world_age_days').notNull().default(0),

    createdAt: timestamp('created_at', { withTimezone: true }).defaultNow().notNull(),
    updatedAt: timestamp('updated_at', { withTimezone: true }).defaultNow().notNull(),
  },
  (table) => [
    index('owned_pets_owner_idx').on(table.ownerAccountId),

    /**
     * UM dono por instancia e obvio; o que esta unicidade impede e o mesmo
     * jogador ter o mesmo pet do catalogo duas vezes por uma escrita repetida.
     *
     * A captura ja e idempotente no jogo (CaptureIfNew), e sem a constraint
     * essa garantia dependeria do cliente — que e exatamente o que uma posse
     * de servidor existe para nao fazer.
     */
    unique('owned_pets_owner_catalog_unique').on(table.ownerAccountId, table.catalogId),
  ],
);

export type OwnedPet = typeof ownedPets.$inferSelect;
export type NewOwnedPet = typeof ownedPets.$inferInsert;


/**
 * A TRILHA DE POSSE (crime-e-recompensa, CR3): toda transferencia por roubo
 * deixa rastro. Posse que muda sem registro e roubo que ninguem pode provar —
 * e a devolucao, a recompensa e a policia todas leem daqui.
 *
 * SO IDS OPACOS (invariante 17, security.md §1): pet, contas de origem e
 * destino, acao e instante. Nunca nome, e-mail nem qualquer PII — nem em
 * debug.
 */
export const ownershipAuditLog = pgTable(
  'ownership_audit_log',
  {
    id: uuid('id').primaryKey().defaultRandom(),

    petId: uuid('pet_id').notNull(),

    /** 'theft' | 'return' | 'confiscation' — varchar, nunca ENUM (code-standart §8). */
    action: varchar('action', { length: 16 }).notNull(),

    /** De quem saiu, e para quem foi. Contas opacas. */
    fromAccountId: uuid('from_account_id').notNull(),
    toAccountId: uuid('to_account_id').notNull(),

    createdAt: timestamp('created_at', { withTimezone: true }).defaultNow().notNull(),
  },
  (table) => [
    // Por pet: "o historico DESTE pet" e a pergunta da devolucao e da policia.
    index('ownership_audit_pet_idx').on(table.petId),
  ],
);

export type OwnershipAuditEntry = typeof ownershipAuditLog.$inferSelect;

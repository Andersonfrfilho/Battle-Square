// Copyright 2026 Anderson. All Rights Reserved.

import { index, pgTable, text, timestamp, uuid, varchar } from 'drizzle-orm/pg-core';

import { playerAccounts } from '../account/account.schema';

// varchar, nunca ENUM nativo (code-standart.md §8).
export const ModerationEventType = {
  TAMPERED_PET_CACHE: 'tampered_pet_cache',
  MANUAL_REPORT: 'manual_report',
} as const;

export type ModerationEventType = (typeof ModerationEventType)[keyof typeof ModerationEventType];

export const moderationEvents = pgTable(
  'moderation_events',
  {
    id: uuid('id').primaryKey().defaultRandom(),
    accountId: uuid('account_id')
      .notNull()
      .references(() => playerAccounts.id, { onDelete: 'cascade' }),
    type: varchar('type', { length: 40 }).notNull(),
    detail: text('detail'),
    // O ESCOPO do token que agiu, nunca um e-mail: PII não entra em trilha.
    recordedBy: varchar('recorded_by', { length: 20 }).notNull(),
    createdAt: timestamp('created_at', { withTimezone: true }).defaultNow().notNull(),
  },
  (table) => [index('moderation_events_account_id_idx').on(table.accountId)],
);

export const accountBans = pgTable(
  'account_bans',
  {
    id: uuid('id').primaryKey().defaultRandom(),
    accountId: uuid('account_id')
      .notNull()
      .references(() => playerAccounts.id, { onDelete: 'cascade' }),
    reason: varchar('reason', { length: 500 }).notNull(),
    // Nulo é PERMANENTE (DP-mod-01).
    expiresAt: timestamp('expires_at', { withTimezone: true }),
    createdBy: varchar('created_by', { length: 20 }).notNull(),
    createdAt: timestamp('created_at', { withTimezone: true }).defaultNow().notNull(),
    // Levantar marca; nunca DELETE — trilha apagável não é trilha.
    liftedAt: timestamp('lifted_at', { withTimezone: true }),
    liftedBy: varchar('lifted_by', { length: 20 }),
  },
  (table) => [index('account_bans_account_id_idx').on(table.accountId)],
);

export type ModerationEvent = typeof moderationEvents.$inferSelect;
export type AccountBan = typeof accountBans.$inferSelect;

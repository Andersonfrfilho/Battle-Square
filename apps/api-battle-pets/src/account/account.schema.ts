// Copyright 2026 Anderson. All Rights Reserved.

import { index, pgTable, timestamp, uuid, varchar } from 'drizzle-orm/pg-core';

// status como varchar, nunca ENUM nativo (code-standart.md §8).
export const AccountStatus = {
  ACTIVE: 'active',
  DISABLED: 'disabled',
} as const;

export type AccountStatus = (typeof AccountStatus)[keyof typeof AccountStatus];

export const playerAccounts = pgTable('player_accounts', {
  id: uuid('id').primaryKey().defaultRandom(),
  // Guardado NORMALIZADO; a unicidade é sobre a forma normalizada (DP-conta-01).
  email: varchar('email', { length: 320 }).notNull().unique(),
  passwordHash: varchar('password_hash', { length: 255 }).notNull(),
  status: varchar('status', { length: 20 }).notNull().default(AccountStatus.ACTIVE),
  createdAt: timestamp('created_at', { withTimezone: true }).defaultNow().notNull(),
  updatedAt: timestamp('updated_at', { withTimezone: true }).defaultNow().notNull(),
});

export const refreshTokens = pgTable(
  'refresh_tokens',
  {
    id: uuid('id').primaryKey().defaultRandom(),
    accountId: uuid('account_id')
      .notNull()
      .references(() => playerAccounts.id, { onDelete: 'cascade' }),
    // Hash, nunca o token em texto: se o banco vazar, o que vazou não serve.
    tokenHash: varchar('token_hash', { length: 64 }).notNull().unique(),
    expiresAt: timestamp('expires_at', { withTimezone: true }).notNull(),
    rotatedAt: timestamp('rotated_at', { withTimezone: true }),
    revokedAt: timestamp('revoked_at', { withTimezone: true }),
    createdAt: timestamp('created_at', { withTimezone: true }).defaultNow().notNull(),
  },
  (table) => [index('refresh_tokens_account_id_idx').on(table.accountId)],
);

export type PlayerAccount = typeof playerAccounts.$inferSelect;
export type NewPlayerAccount = typeof playerAccounts.$inferInsert;
export type RefreshToken = typeof refreshTokens.$inferSelect;

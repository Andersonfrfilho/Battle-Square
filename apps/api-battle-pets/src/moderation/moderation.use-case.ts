// Copyright 2026 Anderson. All Rights Reserved.

import { and, desc, eq, isNull } from 'drizzle-orm';

import { db } from '../db/client';
import { type BanRecord, type BanState, resolveBanState } from './moderation.ban';
import {
  type AccountBan,
  type ModerationEvent,
  accountBans,
  moderationEvents,
} from './moderation.schema';
import type { CreateBanInput, RecordEventInput } from './moderation.validation';

function toBanRecord(ban: AccountBan): BanRecord {
  return { id: ban.id, reason: ban.reason, expiresAt: ban.expiresAt, liftedAt: ban.liftedAt };
}

export async function recordModerationEvent(input: {
  accountId: string;
  event: RecordEventInput;
  recordedBy: string;
}): Promise<ModerationEvent> {
  // Registrar NÃO bane (DP-mod-04): automatizar puniria falso positivo em
  // silêncio, e AD-017 já diz que adulteração não muda resultado.
  const [created] = await db
    .insert(moderationEvents)
    .values({
      accountId: input.accountId,
      type: input.event.type,
      detail: input.event.detail ?? null,
      recordedBy: input.recordedBy,
    })
    .returning();
  return created!;
}

export async function listModerationHistory(accountId: string): Promise<{
  events: ModerationEvent[];
  bans: AccountBan[];
}> {
  const [events, bans] = await Promise.all([
    db
      .select()
      .from(moderationEvents)
      .where(eq(moderationEvents.accountId, accountId))
      .orderBy(desc(moderationEvents.createdAt)),
    db.select().from(accountBans).where(eq(accountBans.accountId, accountId)).orderBy(desc(accountBans.createdAt)),
  ]);
  return { events, bans };
}

export async function banAccount(input: {
  accountId: string;
  ban: CreateBanInput;
  createdBy: string;
}): Promise<AccountBan> {
  const [created] = await db
    .insert(accountBans)
    .values({
      accountId: input.accountId,
      reason: input.ban.reason,
      expiresAt: input.ban.expiresAt ? new Date(input.ban.expiresAt) : null,
      createdBy: input.createdBy,
    })
    .returning();
  return created!;
}

export async function liftBan(input: { banId: string; liftedBy: string; now: Date }): Promise<boolean> {
  // Marca, nunca apaga (DP-mod-01).
  const lifted = await db
    .update(accountBans)
    .set({ liftedAt: input.now, liftedBy: input.liftedBy })
    .where(and(eq(accountBans.id, input.banId), isNull(accountBans.liftedAt)))
    .returning();
  return lifted.length > 0;
}

export async function getBanState(accountId: string, now: Date): Promise<BanState> {
  const bans = await db.select().from(accountBans).where(eq(accountBans.accountId, accountId));
  return resolveBanState(bans.map(toBanRecord), now);
}

// Copyright 2026 Anderson. All Rights Reserved.

import { z } from 'zod';

import { ModerationEventType } from './moderation.schema';

export const recordEventSchema = z.object({
  type: z.enum([ModerationEventType.TAMPERED_PET_CACHE, ModerationEventType.MANUAL_REPORT]),
  detail: z.string().max(2000).optional(),
});

export const createBanSchema = z.object({
  reason: z.string().min(1).max(500),
  /** Ausente = permanente (DP-mod-01). */
  expiresAt: z.iso.datetime().optional(),
});

export type RecordEventInput = z.infer<typeof recordEventSchema>;
export type CreateBanInput = z.infer<typeof createBanSchema>;

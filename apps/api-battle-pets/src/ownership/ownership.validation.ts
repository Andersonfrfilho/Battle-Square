// Copyright 2026 Anderson. All Rights Reserved.

import { z } from 'zod';

// A fronteira da posse: zod antes de qualquer regra (security.md §3).

export const captureBodySchema = z.object({
  /** O id do CATÁLOGO — mesma forma do espelho ("fire-drake-01"). */
  catalogId: z.string().min(1).max(64),
});

export type CaptureBody = z.infer<typeof captureBodySchema>;

export const listOwnedQuerySchema = z.object({
  page: z.coerce.number().int().min(1).default(1),
  // apis.md: perPage sempre com teto.
  perPage: z.coerce.number().int().min(1).max(100).default(50),
});

export type ListOwnedQuery = z.infer<typeof listOwnedQuerySchema>;

// Copyright 2026 Anderson. All Rights Reserved.

import { z } from 'zod';

/** Onde a arvore estava — chaves estaveis do pedaco e da celula. */
export const recordCutBodySchema = z.object({
  chunkKey: z.string().min(1).max(64),
  cellKey: z.string().min(1).max(64),
});

/**
 * O prazo de rebrota (dias) vem do cliente porque e config do jogo (MV4,
 * DefaultGame.ini). A idade do mundo NAO vem do cliente: o servidor a calcula
 * (MV1, autoritativa) — carimbo de corte e "agora" nao sao forjaveis aqui.
 */
export const listCutsQuerySchema = z.object({
  chunkKey: z.string().min(1).max(64),
  deadlineDays: z.coerce.number().int().min(0),
});

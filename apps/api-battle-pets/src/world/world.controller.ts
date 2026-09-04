// Copyright 2026 Anderson. All Rights Reserved.

import { getWorldAge } from './world.use-case';
import { listActiveCuts, recordCut } from './treeCut.use-case';
import { listCutsQuerySchema, recordCutBodySchema } from './treeCut.validation';

function jsonError(status: number, code: string, message: string) {
  return Response.json({ error: { code, message } }, { status });
}

/**
 * GET /v1/world — a idade do mundo, para o cliente exibir.
 *
 * Sem autenticacao: a idade do mundo e a mesma para todos e nao revela nada de
 * ninguem. Envelope { data } como o resto da API (apis.md).
 */
export async function handleGetWorldAge(): Promise<Response> {
  const world = await getWorldAge();
  return Response.json({
    data: { bornAt: world.bornAt.toISOString(), ageInDays: world.ageInDays },
  });
}

/**
 * POST /v1/world/tree-cuts — registra uma arvore cortada. O corte e carimbado
 * com a idade do mundo do SERVIDOR (MV1), nunca com um valor do cliente.
 */
export async function handleRecordTreeCut(request: Request): Promise<Response> {
  const body = await request.json().catch(() => null);
  const parsed = recordCutBodySchema.safeParse(body);
  if (!parsed.success) {
    return jsonError(400, 'VALIDATION_ERROR', 'Corte invalido');
  }
  const world = await getWorldAge();
  await recordCut({ ...parsed.data, cutAtWorldAgeDays: world.ageInDays });
  return new Response(null, { status: 204 });
}

/**
 * GET /v1/world/tree-cuts?chunkKey=&deadlineDays= — os cortes ativos de um
 * pedaco. O servidor filtra os ja rebrotados com a SUA idade do mundo e o
 * prazo de config; a base da mata o cliente planta da semente.
 */
export async function handleListTreeCuts(request: Request): Promise<Response> {
  const url = new URL(request.url);
  const parsed = listCutsQuerySchema.safeParse(Object.fromEntries(url.searchParams));
  if (!parsed.success) {
    return jsonError(400, 'VALIDATION_ERROR', 'Consulta invalida');
  }
  const world = await getWorldAge();
  const cuts = await listActiveCuts(parsed.data.chunkKey, world.ageInDays, parsed.data.deadlineDays);
  return Response.json({ data: cuts });
}

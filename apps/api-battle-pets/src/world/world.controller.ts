// Copyright 2026 Anderson. All Rights Reserved.

import { getWorldAge } from './world.use-case';

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

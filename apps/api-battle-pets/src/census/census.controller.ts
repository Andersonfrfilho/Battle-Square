// Copyright 2026 Anderson. All Rights Reserved.

import { speciesCensus } from './census.use-case';

/**
 * GET /v1/species-census — o censo agregado por especie, para a migracao de Mae
 * Natureza (MN6). Sem PII: so o id de catalogo e a contagem, nunca de quem.
 */
export async function handleSpeciesCensus(): Promise<Response> {
  const census = await speciesCensus();
  return Response.json({ data: census });
}

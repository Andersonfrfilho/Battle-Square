// Copyright 2026 Anderson. All Rights Reserved.

import { authenticate } from '../auth/auth.middleware';
import { environment } from '../config/environment';
import * as OwnershipUseCase from './ownership.use-case';
import { captureBodySchema, listOwnedQuerySchema, stealBodySchema } from './ownership.validation';

function jsonError(status: number, code: string, message: string, details?: unknown) {
  return Response.json({ error: { code, message, ...(details ? { details } : {}) } }, { status });
}

/**
 * A posse é rota de JOGADOR, e SÓ de jogador (PS2, o contrapeso): admin e
 * sync autenticam, mas não têm conta — e sem conta não há coleção. Deixá-los
 * passar faria um segredo estático ser dono de tudo.
 */
function requirePlayer(request: Request) {
  const result = authenticate(request);
  if (!result.authenticated) {
    return { ok: false as const, response: jsonError(401, 'UNAUTHENTICATED', 'Token ausente ou inválido') };
  }
  if (result.scope !== 'player') {
    return {
      ok: false as const,
      response: jsonError(403, 'NOT_A_PLAYER', 'Este token não pertence a uma conta de jogador'),
    };
  }
  return { ok: true as const, accountId: result.accountId };
}

export async function handleListMyPets(request: Request): Promise<Response> {
  const auth = requirePlayer(request);
  if (!auth.ok) return auth.response;

  const url = new URL(request.url);
  const parsed = listOwnedQuerySchema.safeParse(Object.fromEntries(url.searchParams));
  if (!parsed.success) {
    return jsonError(400, 'VALIDATION_ERROR', 'Query inválida');
  }

  const { items, total } = await OwnershipUseCase.listOwnedPets({
    ownerAccountId: auth.accountId,
    page: parsed.data.page,
    perPage: parsed.data.perPage,
  });

  return Response.json({
    data: items,
    pagination: { total, page: parsed.data.page, perPage: parsed.data.perPage },
  });
}

export async function handleGetMyPet(request: Request, petId: string): Promise<Response> {
  const auth = requirePlayer(request);
  if (!auth.ok) return auth.response;

  const result = await OwnershipUseCase.getOwnedPet({
    ownerAccountId: auth.accountId,
    petId,
  });

  switch (result.kind) {
    case 'found':
      return Response.json({ data: result.pet });
    case 'not-found':
      return jsonError(404, 'PET_NOT_FOUND', 'Pet não encontrado');
    case 'owned-by-another':
      // Decisão 38-b: 403 HONESTO — "existe, mas não é seu". O custo (a rota
      // vira oráculo de existência) foi assumido pelo dono e está anotado na
      // decisão.
      return jsonError(403, 'OWNED_BY_ANOTHER', 'Este pet pertence a outra conta');
  }
}

export async function handleListWanted(request: Request): Promise<Response> {
  const auth = requirePlayer(request);
  if (!auth.ok) return auth.response;
  const wanted = await OwnershipUseCase.listWantedAccounts();
  return Response.json({ data: wanted });
}

export async function handleStealPet(request: Request, petId: string): Promise<Response> {
  const auth = requirePlayer(request);
  if (!auth.ok) return auth.response;

  const body = await request.json().catch(() => null);
  const parsed = stealBodySchema.safeParse(body);
  if (!parsed.success) {
    return jsonError(400, 'VALIDATION_ERROR', 'Payload inválido');
  }

  const result = await OwnershipUseCase.stealPet({
    petId,
    expectedOwnerAccountId: parsed.data.expectedOwnerAccountId,
    thiefAccountId: auth.accountId,
  });

  switch (result.kind) {
    case 'stolen':
      return Response.json({ data: result.pet }, { status: 200 });
    case 'not-found':
      return jsonError(404, 'PET_NOT_FOUND', 'Pet não encontrado');
    case 'owner-mismatch':
      return jsonError(409, 'OWNER_CHANGED', 'A posse deste pet mudou');
  }
}

export async function handleRegisterCapture(request: Request): Promise<Response> {
  const auth = requirePlayer(request);
  if (!auth.ok) return auth.response;

  const body = await request.json().catch(() => null);
  const parsed = captureBodySchema.safeParse(body);
  if (!parsed.success) {
    return jsonError(400, 'VALIDATION_ERROR', 'Payload inválido');
  }

  const result = await OwnershipUseCase.registerCapture({
    ownerAccountId: auth.accountId,
    catalogId: parsed.data.catalogId,
    cap: environment.COLLECTION_CAP,
  });

  switch (result.kind) {
    case 'captured':
      return Response.json({ data: result.pet }, { status: 201 });
    case 'already-owned':
      // apis.md: POST idempotente que encontra o existente responde 200 —
      // e o corpo é o pet que JÁ ERA seu, para o cliente não distinguir os
      // dois caminhos à força de status.
      return Response.json({ data: result.pet }, { status: 200 });
    case 'collection-full':
      return jsonError(422, 'COLLECTION_FULL',
        `A coleção atingiu o teto de ${result.cap} pets`);
  }
}

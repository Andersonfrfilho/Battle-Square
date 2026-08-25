// Copyright 2026 Anderson. All Rights Reserved.

import { authenticate, canWrite } from '../auth/auth.middleware';
import { signPet } from './pet-signing';
import * as PetUseCase from './pet.use-case';
import { createPetSchema, formatValidationErrors, listPetsQuerySchema, updatePetSchema } from './pet.validation';

// Envelope de resposta (apis.md): sucesso em {data}, lista em
// {data, pagination}, erro em {error: {code, message}}. Validação
// retorna todos os erros de campo de uma vez, não só o primeiro.

function jsonError(status: number, code: string, message: string, details?: unknown) {
  return Response.json({ error: { code, message, ...(details ? { details } : {}) } }, { status });
}

function requireAuth(request: Request) {
  const result = authenticate(request);
  if (!result.authenticated) {
    return { ok: false as const, response: jsonError(401, 'UNAUTHENTICATED', 'Token ausente ou inválido') };
  }
  return { ok: true as const, scope: result.scope };
}

export async function handleCreatePet(request: Request): Promise<Response> {
  const auth = requireAuth(request);
  if (!auth.ok) return auth.response;
  if (!canWrite(auth.scope)) {
    return jsonError(403, 'FORBIDDEN', 'Este token não tem permissão de escrita');
  }

  const body = await request.json().catch(() => null);
  const parsed = createPetSchema.safeParse(body);
  if (!parsed.success) {
    return jsonError(400, 'VALIDATION_ERROR', 'Payload inválido', formatValidationErrors(parsed.error));
  }

  const created = await PetUseCase.createPet(parsed.data);
  return Response.json({ data: created }, { status: 201 });
}

export async function handleListPets(request: Request): Promise<Response> {
  const auth = requireAuth(request);
  if (!auth.ok) return auth.response;

  const url = new URL(request.url);
  const parsed = listPetsQuerySchema.safeParse(Object.fromEntries(url.searchParams));
  if (!parsed.success) {
    return jsonError(400, 'VALIDATION_ERROR', 'Query inválida', formatValidationErrors(parsed.error));
  }

  const { items, total } = await PetUseCase.listPets(parsed.data);
  return Response.json({
    data: items,
    pagination: { total, page: parsed.data.page, perPage: parsed.data.perPage },
  });
}

export async function handleGetPet(request: Request, id: string): Promise<Response> {
  const auth = requireAuth(request);
  if (!auth.ok) return auth.response;

  const pet = await PetUseCase.getPetById(id);
  if (!pet) {
    return jsonError(404, 'PET_NOT_FOUND', `Pet com id ${id} não encontrado`);
  }
  return Response.json({ data: pet });
}

export async function handleUpdatePet(request: Request, id: string): Promise<Response> {
  const auth = requireAuth(request);
  if (!auth.ok) return auth.response;
  if (!canWrite(auth.scope)) {
    return jsonError(403, 'FORBIDDEN', 'Este token não tem permissão de escrita');
  }

  const body = await request.json().catch(() => null);
  const parsed = updatePetSchema.safeParse(body);
  if (!parsed.success) {
    return jsonError(400, 'VALIDATION_ERROR', 'Payload inválido', formatValidationErrors(parsed.error));
  }

  const updated = await PetUseCase.updatePet(id, parsed.data);
  if (!updated) {
    return jsonError(404, 'PET_NOT_FOUND', `Pet com id ${id} não encontrado`);
  }
  return Response.json({ data: updated });
}

export async function handleDeletePet(request: Request, id: string): Promise<Response> {
  const auth = requireAuth(request);
  if (!auth.ok) return auth.response;
  if (!canWrite(auth.scope)) {
    return jsonError(403, 'FORBIDDEN', 'Este token não tem permissão de escrita');
  }

  const deleted = await PetUseCase.deletePet(id);
  if (!deleted) {
    return jsonError(404, 'PET_NOT_FOUND', `Pet com id ${id} não encontrado`);
  }
  return new Response(null, { status: 204 });
}

export async function handleExportPets(request: Request): Promise<Response> {
  const auth = requireAuth(request);
  if (!auth.ok) return auth.response;
  // T10: sync OU admin podem exportar — nunca menos que sync (design.md).

  const url = new URL(request.url);
  const parsed = listPetsQuerySchema.safeParse(Object.fromEntries(url.searchParams));
  if (!parsed.success) {
    return jsonError(400, 'VALIDATION_ERROR', 'Query inválida', formatValidationErrors(parsed.error));
  }

  const { items, total } = await PetUseCase.listPets(parsed.data);
  const signedItems = items.map((pet) => signPet(pet));

  return Response.json({
    data: signedItems,
    pagination: { total, page: parsed.data.page, perPage: parsed.data.perPage },
  });
}

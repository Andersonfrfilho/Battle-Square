// Copyright 2026 Anderson. All Rights Reserved.

import { authenticate, canWrite, type AuthScope } from '../auth/auth.middleware';
import * as ModerationUseCase from './moderation.use-case';
import { createBanSchema, recordEventSchema } from './moderation.validation';

function jsonError(status: number, code: string, message: string, details?: unknown) {
  return Response.json({ error: { code, message, ...(details ? { details } : {}) } }, { status });
}

function formatIssues(error: { issues: readonly { path: readonly PropertyKey[]; message: string }[] }) {
  return error.issues.map((issue) => ({ field: issue.path.join('.'), message: issue.message }));
}

type AuthorizedScopes = readonly AuthScope[];

function requireScope(request: Request, allowed: AuthorizedScopes) {
  const result = authenticate(request);
  if (!result.authenticated) {
    return { ok: false as const, response: jsonError(401, 'UNAUTHENTICATED', 'Token ausente ou inválido') };
  }
  if (!allowed.includes(result.scope)) {
    return { ok: false as const, response: jsonError(403, 'FORBIDDEN', 'Este token não tem permissão de moderação') };
  }
  return { ok: true as const, scope: result.scope };
}

export async function handleRecordModerationEvent(request: Request, accountId: string): Promise<Response> {
  // É o worker quem observa adulteração, então `sync` também registra.
  const auth = requireScope(request, ['admin', 'sync']);
  if (!auth.ok) return auth.response;

  const body = await request.json().catch(() => null);
  const parsed = recordEventSchema.safeParse(body);
  if (!parsed.success) {
    return jsonError(400, 'VALIDATION_ERROR', 'Payload inválido', formatIssues(parsed.error));
  }

  const created = await ModerationUseCase.recordModerationEvent({
    accountId,
    event: parsed.data,
    recordedBy: auth.scope,
  });
  return Response.json({ data: created }, { status: 201 });
}

export async function handleListModerationHistory(request: Request, accountId: string): Promise<Response> {
  const auth = requireScope(request, ['admin']);
  if (!auth.ok) return auth.response;

  const history = await ModerationUseCase.listModerationHistory(accountId);
  return Response.json({ data: history });
}

export async function handleBanAccount(request: Request, accountId: string): Promise<Response> {
  const auth = requireScope(request, ['admin']);
  if (!auth.ok) return auth.response;
  if (!canWrite(auth.scope)) {
    return jsonError(403, 'FORBIDDEN', 'Este token não tem permissão de escrita');
  }

  const body = await request.json().catch(() => null);
  const parsed = createBanSchema.safeParse(body);
  if (!parsed.success) {
    return jsonError(400, 'VALIDATION_ERROR', 'Payload inválido', formatIssues(parsed.error));
  }

  const created = await ModerationUseCase.banAccount({ accountId, ban: parsed.data, createdBy: auth.scope });
  return Response.json({ data: created }, { status: 201 });
}

export async function handleLiftBan(request: Request, banId: string): Promise<Response> {
  const auth = requireScope(request, ['admin']);
  if (!auth.ok) return auth.response;

  const lifted = await ModerationUseCase.liftBan({ banId, liftedBy: auth.scope, now: new Date() });
  if (!lifted) {
    return jsonError(404, 'BAN_NOT_FOUND', 'Banimento não encontrado ou já levantado');
  }
  return new Response(null, { status: 204 });
}

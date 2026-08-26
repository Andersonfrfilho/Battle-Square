// Copyright 2026 Anderson. All Rights Reserved.

import { normalizeEmail } from './account.email';
import { FailedLoginLimiter } from './account.limiter';
import * as AccountUseCase from './account.use-case';
import { loginSchema, refreshSchema, registerAccountSchema } from './account.validation';

const loginLimiter = new FailedLoginLimiter();

function jsonError(status: number, code: string, message: string, details?: unknown, headers?: Record<string, string>) {
  return Response.json(
    { error: { code, message, ...(details ? { details } : {}) } },
    { status, ...(headers ? { headers } : {}) },
  );
}

function formatIssues(error: { issues: readonly { path: readonly PropertyKey[]; message: string }[] }) {
  return error.issues.map((issue) => ({ field: issue.path.join('.'), message: issue.message }));
}

export async function handleRegisterAccount(request: Request): Promise<Response> {
  const body = await request.json().catch(() => null);
  const parsed = registerAccountSchema.safeParse(body);
  if (!parsed.success) {
    return jsonError(400, 'VALIDATION_ERROR', 'Payload inválido', formatIssues(parsed.error));
  }

  const result = await AccountUseCase.registerAccount(parsed.data);

  if (!result.ok && result.reason === 'EMAIL_TAKEN') {
    // Código estável e ancorável no campo `email` (web.md, §11).
    return jsonError(409, 'ACCOUNT_EMAIL_TAKEN', 'Já existe uma conta com este e-mail', [
      { field: 'email', message: 'Já existe uma conta com este e-mail' },
    ]);
  }

  if (!result.ok) {
    return jsonError(
      400,
      'ACCOUNT_WEAK_PASSWORD',
      'A senha não atende à política',
      result.violations.map((violation) => ({ field: 'password', message: violation })),
    );
  }

  return Response.json({ data: result.account }, { status: 201 });
}

export async function handleLogin(request: Request): Promise<Response> {
  const body = await request.json().catch(() => null);
  const parsed = loginSchema.safeParse(body);
  if (!parsed.success) {
    return jsonError(400, 'VALIDATION_ERROR', 'Payload inválido', formatIssues(parsed.error));
  }

  const now = new Date();
  const limiterKey = normalizeEmail(parsed.data.email);

  const check = loginLimiter.check(limiterKey, now);
  if (check.blocked) {
    return jsonError(429, 'ACCOUNT_TOO_MANY_ATTEMPTS', 'Tentativas demais. Tente de novo mais tarde', undefined, {
      'Retry-After': String(check.retryAfterSeconds),
    });
  }

  const result = await AccountUseCase.authenticateAccount({ ...parsed.data, now });

  if (!result.ok && 'banned' in result) {
    // Credencial estava CERTA — não conta como tentativa falha, senão banir
    // alguém também o tranca por força bruta que ele não cometeu.
    loginLimiter.registerSuccess(limiterKey);
    return jsonError(403, 'ACCOUNT_BANNED', 'Esta conta está banida', [
      { field: 'account', message: result.reason },
      { field: 'expiresAt', message: result.expiresAt ? result.expiresAt.toISOString() : 'permanente' },
    ]);
  }

  if (!result.ok) {
    loginLimiter.registerFailure(limiterKey, now);
    // MESMA resposta para e-mail inexistente e senha errada: distinguir os dois
    // transforma o login num oráculo de "este e-mail tem conta aqui?".
    return jsonError(401, 'ACCOUNT_INVALID_CREDENTIALS', 'E-mail ou senha inválidos');
  }

  loginLimiter.registerSuccess(limiterKey);
  return Response.json({ data: result.session });
}

export async function handleRefreshSession(request: Request): Promise<Response> {
  const body = await request.json().catch(() => null);
  const parsed = refreshSchema.safeParse(body);
  if (!parsed.success) {
    return jsonError(400, 'VALIDATION_ERROR', 'Payload inválido', formatIssues(parsed.error));
  }

  const result = await AccountUseCase.refreshSession({ ...parsed.data, now: new Date() });

  if (!result.ok && 'banned' in result) {
    return jsonError(403, 'ACCOUNT_BANNED', 'Esta conta está banida', [
      { field: 'account', message: result.reason },
      { field: 'expiresAt', message: result.expiresAt ? result.expiresAt.toISOString() : 'permanente' },
    ]);
  }

  if (!result.ok) {
    return jsonError(401, 'ACCOUNT_REFRESH_INVALID', 'Token de renovação inválido, expirado ou já usado');
  }

  return Response.json({ data: result.session });
}

export async function handleLogout(request: Request): Promise<Response> {
  const body = await request.json().catch(() => null);
  const parsed = refreshSchema.safeParse(body);
  if (!parsed.success) {
    return jsonError(400, 'VALIDATION_ERROR', 'Payload inválido', formatIssues(parsed.error));
  }

  await AccountUseCase.revokeSession({ ...parsed.data, now: new Date() });
  // 204 sempre: dizer se o token existia seria confirmar a existência dele.
  return new Response(null, { status: 204 });
}

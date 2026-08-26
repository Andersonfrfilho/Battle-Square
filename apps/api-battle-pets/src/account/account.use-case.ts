// Copyright 2026 Anderson. All Rights Reserved.

import { createHash, randomBytes } from 'node:crypto';

import { and, eq, isNull } from 'drizzle-orm';

import { environment } from '../config/environment';
import { db } from '../db/client';
import { normalizeEmail } from './account.email';
import {
  AccountStatus,
  type PlayerAccount,
  playerAccounts,
  refreshTokens,
} from './account.schema';
import {
  ACCESS_TOKEN_LIFETIME_SECONDS,
  REFRESH_TOKEN_LIFETIME_SECONDS,
  issueAccessToken,
} from './account.token';
import { validatePasswordPolicy, type PasswordPolicyViolation } from './account.password';

// Um hash descartável, de senha aleatória, usado só para gastar tempo no
// caminho de e-mail inexistente (DP-conta-04). Sem isto, o TEMPO de resposta
// revela o que a mensagem idêntica escondeu — enumeração de usuário por
// canal lateral. Não "otimizar" isto fora.
const DUMMY_PASSWORD_HASH = await Bun.password.hash(randomBytes(32).toString('hex'), {
  algorithm: 'argon2id',
});

export type AccountSession = {
  readonly accountId: string;
  readonly accessToken: string;
  readonly accessTokenExpiresInSeconds: number;
  readonly refreshToken: string;
};

export type RegisterAccountResult =
  | { readonly ok: true; readonly account: PublicAccount }
  | { readonly ok: false; readonly reason: 'EMAIL_TAKEN' }
  | { readonly ok: false; readonly reason: 'WEAK_PASSWORD'; readonly violations: readonly PasswordPolicyViolation[] };

export type PublicAccount = {
  readonly id: string;
  readonly email: string;
  readonly status: string;
  readonly createdAt: Date;
};

/** Nunca devolve passwordHash — a resposta é montada por inclusão, não por omissão. */
function toPublicAccount(account: PlayerAccount): PublicAccount {
  return {
    id: account.id,
    email: account.email,
    status: account.status,
    createdAt: account.createdAt,
  };
}

function hashRefreshToken(token: string): string {
  return createHash('sha256').update(token).digest('hex');
}

export async function registerAccount(input: {
  email: string;
  password: string;
}): Promise<RegisterAccountResult> {
  const violations = validatePasswordPolicy(input.password);
  if (violations.length > 0) {
    return { ok: false, reason: 'WEAK_PASSWORD', violations };
  }

  const email = normalizeEmail(input.email);
  const passwordHash = await Bun.password.hash(input.password, { algorithm: 'argon2id' });

  try {
    const [created] = await db.insert(playerAccounts).values({ email, passwordHash }).returning();
    return { ok: true, account: toPublicAccount(created!) };
  } catch (error) {
    // A checagem prévia não basta: entre ela e o INSERT cabe outra escrita, e
    // só a constraint do banco decide (web.md, campo único).
    if (isUniqueViolation(error)) {
      return { ok: false, reason: 'EMAIL_TAKEN' };
    }
    throw error;
  }
}

function isUniqueViolation(error: unknown): boolean {
  return typeof error === 'object' && error !== null && 'code' in error && (error as { code: unknown }).code === '23505';
}

export type AuthenticateResult =
  | { readonly ok: true; readonly session: AccountSession }
  | { readonly ok: false };

export async function authenticateAccount(input: {
  email: string;
  password: string;
  now: Date;
}): Promise<AuthenticateResult> {
  const email = normalizeEmail(input.email);
  const [account] = await db.select().from(playerAccounts).where(eq(playerAccounts.email, email)).limit(1);

  if (!account) {
    // Gasta o mesmo tempo do caminho com conta — ver DUMMY_PASSWORD_HASH.
    await Bun.password.verify(input.password, DUMMY_PASSWORD_HASH);
    return { ok: false };
  }

  const passwordMatches = await Bun.password.verify(input.password, account.passwordHash);
  if (!passwordMatches || account.status !== AccountStatus.ACTIVE) {
    return { ok: false };
  }

  return { ok: true, session: await issueSession(account.id, input.now) };
}

async function issueSession(accountId: string, now: Date): Promise<AccountSession> {
  const refreshToken = randomBytes(32).toString('base64url');
  const expiresAt = new Date(now.getTime() + REFRESH_TOKEN_LIFETIME_SECONDS * 1000);

  await db.insert(refreshTokens).values({
    accountId,
    tokenHash: hashRefreshToken(refreshToken),
    expiresAt,
  });

  return {
    accountId,
    accessToken: issueAccessToken({ accountId, secret: environment.ACCESS_TOKEN_SECRET, now }),
    accessTokenExpiresInSeconds: ACCESS_TOKEN_LIFETIME_SECONDS,
    refreshToken,
  };
}

export type RefreshResult =
  | { readonly ok: true; readonly session: AccountSession }
  | { readonly ok: false };

export async function refreshSession(input: { refreshToken: string; now: Date }): Promise<RefreshResult> {
  const tokenHash = hashRefreshToken(input.refreshToken);

  const [stored] = await db
    .select()
    .from(refreshTokens)
    .where(and(eq(refreshTokens.tokenHash, tokenHash), isNull(refreshTokens.rotatedAt), isNull(refreshTokens.revokedAt)))
    .limit(1);

  // Ausente, já rotacionado ou revogado caem todos aqui. Reuso de um token já
  // rotacionado é o sinal clássico de token roubado, e é recusado (DP-conta-03).
  if (!stored || stored.expiresAt <= input.now) {
    return { ok: false };
  }

  await db.update(refreshTokens).set({ rotatedAt: input.now }).where(eq(refreshTokens.id, stored.id));

  return { ok: true, session: await issueSession(stored.accountId, input.now) };
}

export async function revokeSession(input: { refreshToken: string; now: Date }): Promise<void> {
  await db
    .update(refreshTokens)
    .set({ revokedAt: input.now })
    .where(and(eq(refreshTokens.tokenHash, hashRefreshToken(input.refreshToken)), isNull(refreshTokens.revokedAt)));
}

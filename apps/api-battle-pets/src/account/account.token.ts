// Copyright 2026 Anderson. All Rights Reserved.

import { createHmac, timingSafeEqual } from 'node:crypto';

export const ACCESS_TOKEN_LIFETIME_SECONDS = 15 * 60;
export const REFRESH_TOKEN_LIFETIME_SECONDS = 30 * 24 * 60 * 60;

export type AccessTokenPayload = {
  readonly accountId: string;
  readonly issuedAtSeconds: number;
  readonly expiresAtSeconds: number;
};

export type IssueAccessTokenParams = {
  readonly accountId: string;
  readonly secret: string;
  readonly now: Date;
};

export type VerifyAccessTokenParams = {
  readonly token: string;
  readonly secret: string;
  readonly now: Date;
};

export const AccessTokenError = {
  MALFORMED: 'ACCESS_TOKEN_MALFORMED',
  BAD_SIGNATURE: 'ACCESS_TOKEN_BAD_SIGNATURE',
  EXPIRED: 'ACCESS_TOKEN_EXPIRED',
} as const;

export type AccessTokenError = (typeof AccessTokenError)[keyof typeof AccessTokenError];

export type VerifyAccessTokenResult =
  | { readonly valid: true; readonly payload: AccessTokenPayload }
  | { readonly valid: false; readonly error: AccessTokenError };

function signPayload(encodedPayload: string, secret: string): Buffer {
  return createHmac('sha256', secret).update(encodedPayload).digest();
}

export function issueAccessToken(params: IssueAccessTokenParams): string {
  const issuedAtSeconds = Math.floor(params.now.getTime() / 1000);
  const payload: AccessTokenPayload = {
    accountId: params.accountId,
    issuedAtSeconds,
    expiresAtSeconds: issuedAtSeconds + ACCESS_TOKEN_LIFETIME_SECONDS,
  };

  const encodedPayload = Buffer.from(JSON.stringify(payload), 'utf8').toString('base64url');
  const signature = signPayload(encodedPayload, params.secret).toString('base64url');
  return `${encodedPayload}.${signature}`;
}

export function verifyAccessToken(params: VerifyAccessTokenParams): VerifyAccessTokenResult {
  const separatorIndex = params.token.indexOf('.');
  if (separatorIndex <= 0 || separatorIndex === params.token.length - 1) {
    return { valid: false, error: AccessTokenError.MALFORMED };
  }

  const encodedPayload = params.token.slice(0, separatorIndex);
  const providedSignature = Buffer.from(params.token.slice(separatorIndex + 1), 'base64url');
  const expectedSignature = signPayload(encodedPayload, params.secret);

  // A assinatura é conferida ANTES de o payload ser interpretado: o contrário
  // é decidir com base em dado não autenticado (DP-conta-02).
  if (providedSignature.length !== expectedSignature.length) {
    // Comparar contra si mesmo mantém o custo constante, para o tamanho da
    // assinatura não virar canal lateral.
    timingSafeEqual(expectedSignature, expectedSignature);
    return { valid: false, error: AccessTokenError.BAD_SIGNATURE };
  }
  if (!timingSafeEqual(providedSignature, expectedSignature)) {
    return { valid: false, error: AccessTokenError.BAD_SIGNATURE };
  }

  let payload: AccessTokenPayload;
  try {
    payload = JSON.parse(Buffer.from(encodedPayload, 'base64url').toString('utf8')) as AccessTokenPayload;
  } catch {
    return { valid: false, error: AccessTokenError.MALFORMED };
  }

  if (
    typeof payload?.accountId !== 'string' ||
    typeof payload?.expiresAtSeconds !== 'number' ||
    typeof payload?.issuedAtSeconds !== 'number'
  ) {
    return { valid: false, error: AccessTokenError.MALFORMED };
  }

  if (Math.floor(params.now.getTime() / 1000) >= payload.expiresAtSeconds) {
    return { valid: false, error: AccessTokenError.EXPIRED };
  }

  return { valid: true, payload };
}

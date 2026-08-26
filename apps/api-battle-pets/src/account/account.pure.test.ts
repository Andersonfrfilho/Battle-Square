// Copyright 2026 Anderson. All Rights Reserved.

import { describe, expect, test } from 'bun:test';

import { maskEmail, normalizeEmail } from './account.email';
import { FailedLoginLimiter } from './account.limiter';
import { PasswordPolicyViolation, validatePasswordPolicy } from './account.password';
import {
  ACCESS_TOKEN_LIFETIME_SECONDS,
  AccessTokenError,
  issueAccessToken,
  verifyAccessToken,
} from './account.token';

const SECRET = 'segredo-de-teste-com-tamanho-suficiente-1234567890';
const OTHER_SECRET = 'outro-segredo-completamente-diferente-0987654321';
const ACCOUNT_ID = '11111111-2222-3333-4444-555555555555';
const NOW = new Date('2026-08-26T12:00:00.000Z');

describe('normalizeEmail', () => {
  test('normaliza caixa e espaços, para não nascerem duas contas do mesmo e-mail', () => {
    expect(normalizeEmail('  Joao@Exemplo.COM ')).toBe('joao@exemplo.com');
  });
});

describe('maskEmail', () => {
  test('não deixa vazar o local part inteiro', () => {
    expect(maskEmail('anderson@exemplo.com')).toBe('a***@exemplo.com');
  });

  test('entrada sem @ vira máscara total em vez de vazar o que quer que seja', () => {
    expect(maskEmail('nao-e-um-email')).toBe('***');
  });
});

describe('validatePasswordPolicy', () => {
  test('senha boa não tem violação', () => {
    expect(validatePasswordPolicy('senhaSegura2026')).toEqual([]);
  });

  test('devolve TODOS os motivos de uma vez, não só o primeiro', () => {
    const violations = validatePasswordPolicy('abc');
    expect(violations).toContain(PasswordPolicyViolation.TOO_SHORT);
    expect(violations).toContain(PasswordPolicyViolation.MISSING_DIGIT);
    expect(violations.length).toBe(2);
  });

  test('teto de tamanho existe — Argon2id sobre entrada gigante é negação de serviço barata', () => {
    expect(validatePasswordPolicy('a1'.repeat(500))).toContain(PasswordPolicyViolation.TOO_LONG);
  });
});

describe('token de acesso', () => {
  test('token recém-emitido verifica e devolve a conta', () => {
    const token = issueAccessToken({ accountId: ACCOUNT_ID, secret: SECRET, now: NOW });
    const result = verifyAccessToken({ token, secret: SECRET, now: NOW });

    expect(result.valid).toBe(true);
    if (result.valid) expect(result.payload.accountId).toBe(ACCOUNT_ID);
  });

  test('expira depois da vida útil', () => {
    const token = issueAccessToken({ accountId: ACCOUNT_ID, secret: SECRET, now: NOW });
    const afterExpiry = new Date(NOW.getTime() + (ACCESS_TOKEN_LIFETIME_SECONDS + 1) * 1000);
    const result = verifyAccessToken({ token, secret: SECRET, now: afterExpiry });

    expect(result).toEqual({ valid: false, error: AccessTokenError.EXPIRED });
  });

  test('assinatura de outro segredo é recusada', () => {
    const token = issueAccessToken({ accountId: ACCOUNT_ID, secret: OTHER_SECRET, now: NOW });
    const result = verifyAccessToken({ token, secret: SECRET, now: NOW });

    expect(result).toEqual({ valid: false, error: AccessTokenError.BAD_SIGNATURE });
  });

  test('payload adulterado é recusado ANTES de ser interpretado', () => {
    const token = issueAccessToken({ accountId: ACCOUNT_ID, secret: SECRET, now: NOW });
    const [, signature] = token.split('.');
    const forgedPayload = Buffer.from(
      JSON.stringify({
        accountId: 'conta-de-outra-pessoa',
        issuedAtSeconds: 0,
        expiresAtSeconds: 99999999999,
      }),
      'utf8',
    ).toString('base64url');

    const result = verifyAccessToken({ token: `${forgedPayload}.${signature}`, secret: SECRET, now: NOW });
    expect(result).toEqual({ valid: false, error: AccessTokenError.BAD_SIGNATURE });
  });

  test('lixo não crasha — devolve malformado', () => {
    for (const garbage of ['', '.', 'semponto', '.semprefixo', 'a.']) {
      const result = verifyAccessToken({ token: garbage, secret: SECRET, now: NOW });
      expect(result.valid).toBe(false);
    }
  });
});

describe('FailedLoginLimiter', () => {
  const KEY = 'joao@exemplo.com';

  test('bloqueia ao atingir o teto e informa Retry-After', () => {
    const limiter = new FailedLoginLimiter(3, 900);
    for (let attempt = 0; attempt < 3; attempt += 1) {
      expect(limiter.check(KEY, NOW).blocked).toBe(false);
      limiter.registerFailure(KEY, NOW);
    }

    const check = limiter.check(KEY, NOW);
    expect(check.blocked).toBe(true);
    expect(check.retryAfterSeconds).toBeGreaterThan(0);
  });

  test('sucesso zera a contagem', () => {
    const limiter = new FailedLoginLimiter(2, 900);
    limiter.registerFailure(KEY, NOW);
    limiter.registerFailure(KEY, NOW);
    expect(limiter.check(KEY, NOW).blocked).toBe(true);

    limiter.registerSuccess(KEY);
    expect(limiter.check(KEY, NOW).blocked).toBe(false);
  });

  test('a janela expira e as tentativas voltam a ser aceitas', () => {
    const limiter = new FailedLoginLimiter(2, 900);
    limiter.registerFailure(KEY, NOW);
    limiter.registerFailure(KEY, NOW);
    expect(limiter.check(KEY, NOW).blocked).toBe(true);

    const afterWindow = new Date(NOW.getTime() + 901 * 1000);
    expect(limiter.check(KEY, afterWindow).blocked).toBe(false);
  });

  test('contas diferentes não interferem uma na outra', () => {
    const limiter = new FailedLoginLimiter(2, 900);
    limiter.registerFailure(KEY, NOW);
    limiter.registerFailure(KEY, NOW);

    expect(limiter.check(KEY, NOW).blocked).toBe(true);
    expect(limiter.check('outra@exemplo.com', NOW).blocked).toBe(false);
  });
});

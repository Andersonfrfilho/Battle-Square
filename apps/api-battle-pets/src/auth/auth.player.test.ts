// Copyright 2026 Anderson. All Rights Reserved.

import { describe, expect, test } from 'bun:test';

import { issueAccessToken } from '../account/account.token';
import { environment } from '../config/environment';
import { authenticate } from './auth.middleware';

/**
 * PS2 — o token do JOGADOR autentica, e `admin` não é dono de nada.
 *
 * O verificador existia e nenhuma rota o chamava (medição da task). Estes
 * testes afirmam a fronteira inteira: quem entra TRAZ o accountId, quem não
 * deve entrar não entra — e o contrapeso, que é o teste mais importante do
 * arquivo, garante que os tokens estáticos NUNCA viram jogador.
 */

function requestWithBearer(token: string): Request {
  return new Request('http://localhost/qualquer', {
    headers: { authorization: `Bearer ${token}` },
  });
}

const NOW = new Date('2026-09-03T12:00:00Z');

describe('authenticate — o jogador', () => {
  test('token de conta valido autentica E traz o accountId', () => {
    const token = issueAccessToken({
      accountId: 'conta-abc',
      secret: environment.ACCESS_TOKEN_SECRET,
      now: NOW,
    });

    const result = authenticate(requestWithBearer(token), NOW);
    expect(result.authenticated).toBe(true);
    if (result.authenticated && result.scope === 'player') {
      expect(result.accountId).toBe('conta-abc');
    } else {
      throw new Error('esperava escopo player com accountId');
    }
  });

  test('token expirado nao autentica — validade nao e sugestao', () => {
    const token = issueAccessToken({
      accountId: 'conta-abc',
      secret: environment.ACCESS_TOKEN_SECRET,
      now: new Date('2026-09-01T00:00:00Z'),
    });

    expect(authenticate(requestWithBearer(token), NOW).authenticated).toBe(false);
  });

  test('token de OUTRO segredo nao autentica — assinatura decide', () => {
    const token = issueAccessToken({
      accountId: 'conta-abc',
      secret: 'um-segredo-completamente-diferente-de-32b',
      now: NOW,
    });

    expect(authenticate(requestWithBearer(token), NOW).authenticated).toBe(false);
  });

  test('token adulterado nao autentica', () => {
    const token = issueAccessToken({
      accountId: 'conta-abc',
      secret: environment.ACCESS_TOKEN_SECRET,
      now: NOW,
    });

    expect(authenticate(requestWithBearer(`${token}x`), NOW).authenticated).toBe(false);
  });
});

describe('authenticate — o contrapeso: admin NAO e dono de nada', () => {
  test('ADMIN_API_TOKEN entra como admin, NUNCA como player', () => {
    // Se o admin valesse como conta, um segredo estatico e sem dono seria o
    // dono de todas as colecoes — e a posse no servidor nao teria ficado
    // mais segura que o arquivo que substitui, so mais dificil de editar.
    const result = authenticate(requestWithBearer(environment.ADMIN_API_TOKEN), NOW);
    expect(result.authenticated).toBe(true);
    if (result.authenticated) {
      expect(result.scope).toBe('admin');
      expect('accountId' in result).toBe(false);
    }
  });

  test('SYNC_API_TOKEN tambem: escopo sync, sem identidade de conta', () => {
    const result = authenticate(requestWithBearer(environment.SYNC_API_TOKEN), NOW);
    expect(result.authenticated).toBe(true);
    if (result.authenticated) {
      expect(result.scope).toBe('sync');
      expect('accountId' in result).toBe(false);
    }
  });
});

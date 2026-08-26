// Copyright 2026 Anderson. All Rights Reserved.

import { describe, expect, test } from 'bun:test';

import { type BanRecord, resolveBanState } from './moderation.ban';

const NOW = new Date('2026-08-26T12:00:00.000Z');

function makeBan(overrides: Partial<BanRecord> = {}): BanRecord {
  return {
    id: 'ban-1',
    reason: 'adulteração de cache',
    expiresAt: null,
    liftedAt: null,
    ...overrides,
  };
}

describe('resolveBanState', () => {
  test('lista vazia não bane', () => {
    expect(resolveBanState([], NOW)).toEqual({ banned: false });
  });

  test('banimento sem prazo é permanente e bane', () => {
    const state = resolveBanState([makeBan()], NOW);
    expect(state.banned).toBe(true);
  });

  test('banimento temporário ainda no prazo bane', () => {
    const ban = makeBan({ expiresAt: new Date(NOW.getTime() + 60_000) });
    expect(resolveBanState([ban], NOW).banned).toBe(true);
  });

  test('banimento temporário expirado não bane', () => {
    const ban = makeBan({ expiresAt: new Date(NOW.getTime() - 1) });
    expect(resolveBanState([ban], NOW)).toEqual({ banned: false });
  });

  test('no instante EXATO da expiração o banimento já acabou', () => {
    const ban = makeBan({ expiresAt: new Date(NOW.getTime()) });
    expect(resolveBanState([ban], NOW)).toEqual({ banned: false });
  });

  test('banimento levantado não bane, mesmo antes do prazo', () => {
    const ban = makeBan({
      expiresAt: new Date(NOW.getTime() + 60_000),
      liftedAt: new Date(NOW.getTime() - 60_000),
    });
    expect(resolveBanState([ban], NOW)).toEqual({ banned: false });
  });

  test('permanente vence temporário quando os dois estão ativos', () => {
    const temporary = makeBan({ id: 'temp', expiresAt: new Date(NOW.getTime() + 60_000) });
    const permanent = makeBan({ id: 'perm', expiresAt: null });

    const state = resolveBanState([temporary, permanent], NOW);
    expect(state.banned).toBe(true);
    if (state.banned) expect(state.ban.id).toBe('perm');
  });

  test('entre dois temporários vence o que termina mais tarde', () => {
    const sooner = makeBan({ id: 'sooner', expiresAt: new Date(NOW.getTime() + 60_000) });
    const later = makeBan({ id: 'later', expiresAt: new Date(NOW.getTime() + 600_000) });

    const state = resolveBanState([sooner, later], NOW);
    expect(state.banned).toBe(true);
    if (state.banned) expect(state.ban.id).toBe('later');
  });

  test('a ordem da lista não muda o resultado — quem decide é a regra, não o banco', () => {
    const temporary = makeBan({ id: 'temp', expiresAt: new Date(NOW.getTime() + 60_000) });
    const permanent = makeBan({ id: 'perm', expiresAt: null });

    const forward = resolveBanState([temporary, permanent], NOW);
    const backward = resolveBanState([permanent, temporary], NOW);
    expect(forward).toEqual(backward);
  });

  test('banimentos inativos ao lado de um ativo não escondem o ativo', () => {
    const expired = makeBan({ id: 'expired', expiresAt: new Date(NOW.getTime() - 1) });
    const lifted = makeBan({ id: 'lifted', liftedAt: NOW });
    const active = makeBan({ id: 'active', expiresAt: new Date(NOW.getTime() + 60_000) });

    const state = resolveBanState([expired, lifted, active], NOW);
    expect(state.banned).toBe(true);
    if (state.banned) expect(state.ban.id).toBe('active');
  });
});

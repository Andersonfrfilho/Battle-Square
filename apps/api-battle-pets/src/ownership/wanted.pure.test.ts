// Copyright 2026 Anderson. All Rights Reserved.

import { describe, expect, test } from 'bun:test';

import { isWanted, wantedAccounts, WANTED_THEFT_THRESHOLD } from './wanted.pure';

/**
 * CR4 — a lista de procurados DERIVA do rastro, e "repetir" e a regra.
 */

describe('wantedAccounts', () => {
  test('UM roubo nao procura ninguem — o primeiro e erro, nao padrao', () => {
    expect(wantedAccounts([{ thiefAccountId: 'a' }])).toEqual([]);
    expect(isWanted([{ thiefAccountId: 'a' }], 'a')).toBe(false);
  });

  test('a REINCIDENCIA procura — o segundo roubo cruza o limiar', () => {
    const thefts = [{ thiefAccountId: 'a' }, { thiefAccountId: 'a' }];
    expect(isWanted(thefts, 'a')).toBe(true);
    expect(wantedAccounts(thefts)).toEqual(['a']);
  });

  test('cada conta conta a SUA reincidencia — nao se soma entre contas', () => {
    // 'a' roubou duas vezes (procurado), 'b' uma (nao). Somar tudo faria 'b'
    // pagar pelo crime de 'a'.
    const thefts = [
      { thiefAccountId: 'a' }, { thiefAccountId: 'a' }, { thiefAccountId: 'b' },
    ];
    expect(wantedAccounts(thefts)).toEqual(['a']);
    expect(isWanted(thefts, 'b')).toBe(false);
  });

  test('o limiar e a constante nomeada, nunca um 2 solto', () => {
    const thefts = Array.from({ length: WANTED_THEFT_THRESHOLD }, () => ({ thiefAccountId: 'a' }));
    expect(isWanted(thefts, 'a')).toBe(true);
  });
});

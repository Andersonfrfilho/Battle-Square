// Copyright 2026 Anderson. All Rights Reserved.

import { describe, expect, test } from 'bun:test';

import { isUniqueViolation } from './postgres-error';

describe('isUniqueViolation', () => {
  test('reconhece o código no topo', () => {
    expect(isUniqueViolation({ code: '23505' })).toBe(true);
  });

  test('reconhece o código na CAUSA — é assim que o Drizzle entrega', () => {
    const wrapped = new Error('Failed query') as Error & { cause: unknown };
    wrapped.cause = { code: '23505', message: 'duplicate key value violates unique constraint' };
    expect(isUniqueViolation(wrapped)).toBe(true);
  });

  test('reconhece o código em causa aninhada mais fundo', () => {
    expect(isUniqueViolation({ cause: { cause: { code: '23505' } } })).toBe(true);
  });

  test('outro código de erro não é violação de unicidade', () => {
    expect(isUniqueViolation({ cause: { code: '23503' } })).toBe(false);
  });

  test('entradas inertes não quebram', () => {
    for (const value of [null, undefined, 'erro', 42, {}]) {
      expect(isUniqueViolation(value)).toBe(false);
    }
  });

  test('cadeia de causas circular não vira laço infinito', () => {
    const a: Record<string, unknown> = {};
    const b: Record<string, unknown> = { cause: a };
    a.cause = b;
    expect(isUniqueViolation(a)).toBe(false);
  });
});

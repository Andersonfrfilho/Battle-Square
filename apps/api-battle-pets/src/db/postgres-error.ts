// Copyright 2026 Anderson. All Rights Reserved.

/** Violação de constraint UNIQUE no PostgreSQL. */
const UNIQUE_VIOLATION_CODE = '23505';

/**
 * O Drizzle embrulha o erro do driver num DrizzleQueryError, e o `code` do
 * Postgres fica na CAUSA, não no topo. Checar só `error.code` devolve false
 * para uma violação real — foi como um e-mail duplicado virou 500 em vez de
 * 409 na primeira vez que este código rodou contra um banco de verdade.
 */
export function isUniqueViolation(error: unknown): boolean {
  let current: unknown = error;
  // Profundidade limitada: cadeia de causas circular não pode virar laço.
  for (let depth = 0; depth < 8 && current; depth += 1) {
    if (
      typeof current === 'object' &&
      current !== null &&
      'code' in current &&
      (current as { code: unknown }).code === UNIQUE_VIOLATION_CODE
    ) {
      return true;
    }
    current = typeof current === 'object' && current !== null ? (current as { cause?: unknown }).cause : undefined;
  }
  return false;
}

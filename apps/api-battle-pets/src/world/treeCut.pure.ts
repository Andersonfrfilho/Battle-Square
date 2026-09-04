// Copyright 2026 Anderson. All Rights Reserved.

/**
 * A rebrota pura, espelho da regra do cliente (`TreeRegrowth::HasRegrown`).
 *
 * Existe no servidor para ele PODAR cortes ja rebrotados na leitura — a tabela
 * nao acumula marca morta. Mesma matematica dos dois lados, uma fonte de
 * verdade conceitual: cortada ate o mundo envelhecer o prazo inteiro.
 */
export function hasRegrown(
  cutAtWorldAgeDays: number,
  currentWorldAgeDays: number,
  deadlineDays: number,
): boolean {
  if (deadlineDays <= 0) return true;
  const elapsed = currentWorldAgeDays - cutAtWorldAgeDays;
  if (elapsed < 0) return false;
  return elapsed >= deadlineDays;
}

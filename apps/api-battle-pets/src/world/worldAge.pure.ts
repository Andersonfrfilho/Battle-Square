// Copyright 2026 Anderson. All Rights Reserved.

/**
 * A subtracao pura da idade do mundo — testavel sem relogio, sem banco.
 *
 * O "agora" entra como argumento (o use-case o fornece na fronteira): a regra
 * em si nao le relogio nenhum, e por isso o teste consegue afirmar "dois dias
 * diferentes dao numeros diferentes" sem esperar o tempo passar.
 */

const MILLISECONDS_PER_DAY = 24 * 60 * 60 * 1000;

/**
 * Quantos dias inteiros o mundo tem, dado quando nasceu e quando e "agora".
 *
 * Nunca negativo: um "agora" anterior ao nascimento (relogio torto) devolve
 * zero, nao um numero negativo que pareceria um mundo do futuro.
 */
export function ageInDaysBetween(bornAt: Date, now: Date): number {
  const elapsed = now.getTime() - bornAt.getTime();
  if (elapsed <= 0) return 0;
  return Math.floor(elapsed / MILLISECONDS_PER_DAY);
}

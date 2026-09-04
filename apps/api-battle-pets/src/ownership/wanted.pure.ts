// Copyright 2026 Anderson. All Rights Reserved.

/**
 * A LISTA DE PROCURADOS (crime-e-recompensa, CR4) — pura, derivada do rastro.
 *
 * Decisao 22: "repetir poe o jogador na lista de procurados". Procurado nao e
 * um estado que alguem escreve — e uma CONSEQUENCIA do que a trilha de posse
 * (CR3) ja registrou. Derivar em vez de guardar e o que impede a lista de
 * divergir do que de fato aconteceu (L-032): a fonte da verdade e o rastro, e
 * ninguem pode ser procurado por um roubo que nao esta la.
 *
 * O PRIMEIRO roubo nao procura ninguem — "repetir" e a palavra da decisao. Um
 * erro unico nao vira pena; a reincidencia vira.
 */

/** Quantos roubos tornam uma conta procurada. Dois: o primeiro e erro, o segundo e padrao. */
export const WANTED_THEFT_THRESHOLD = 2;

export type TheftRecord = {
  /** A conta que roubou. */
  readonly thiefAccountId: string;
};

/**
 * Quantas vezes cada conta roubou, a partir das entradas de roubo da trilha.
 *
 * So conta 'theft' — devolucao e confisco tambem passam pela trilha, e contar
 * qualquer transferencia como roubo faria a policia caçar quem devolveu.
 */
export function countTheftsByAccount(thefts: readonly TheftRecord[]): Map<string, number> {
  const counts = new Map<string, number>();
  for (const record of thefts) {
    counts.set(record.thiefAccountId, (counts.get(record.thiefAccountId) ?? 0) + 1);
  }
  return counts;
}

/** As contas procuradas: as que passaram do limiar de reincidencia. */
export function wantedAccounts(thefts: readonly TheftRecord[]): string[] {
  const wanted: string[] = [];
  for (const [accountId, count] of countTheftsByAccount(thefts)) {
    if (count >= WANTED_THEFT_THRESHOLD) {
      wanted.push(accountId);
    }
  }
  return wanted;
}

/** Esta conta e procurada? */
export function isWanted(thefts: readonly TheftRecord[], accountId: string): boolean {
  return (countTheftsByAccount(thefts).get(accountId) ?? 0) >= WANTED_THEFT_THRESHOLD;
}

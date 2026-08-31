// Copyright 2026 Anderson. All Rights Reserved.

import { AcquisitionKind } from './ownership.schema';

/**
 * O pet como as REGRAS de posse o veem — sem banco, sem HTTP.
 *
 * Puro de proposito: transferencia de posse e a regra que decide se roubo,
 * comercio e devolucao funcionam, e ela precisa ser verificavel sem subir
 * Postgres. Mesma separacao que o jogo usa entre nucleo e mundo.
 */
export type PetOwnership = {
  catalogId: string;
  ownerAccountId: string;
  acquisition: AcquisitionKind;
  stolenFromAccountId: string | null;
};

export type TransferKind = 'capture' | 'trade' | 'theft' | 'return';

/**
 * Passa o pet para outro dono.
 *
 * Devolve um objeto NOVO: posse e historico, e mutar o anterior apagaria o
 * estado de onde ele veio — que e justamente o que a devolucao precisa.
 */
export function transferOwnership(
  pet: PetOwnership,
  toAccountId: string,
  kind: TransferKind,
): PetOwnership {
  if (kind === 'theft') {
    // A marca guarda o dono ORIGINAL, e nao o anterior. Sem isso, revender
    // duas vezes apagaria a origem — e o pet ficaria limpo depois de passar
    // por duas maos, que e o caminho obvio para lavar o roubo.
    return {
      ...pet,
      ownerAccountId: toAccountId,
      acquisition: AcquisitionKind.STOLEN,
      stolenFromAccountId: pet.stolenFromAccountId ?? pet.ownerAccountId,
    };
  }

  if (kind === 'return') {
    // Devolver LIMPA a marca: o pet voltou para casa, e continuar marcado o
    // faria ser delatado na batalha do proprio dono.
    return {
      ...pet,
      ownerAccountId: toAccountId,
      acquisition: AcquisitionKind.RECLAIMED,
      stolenFromAccountId: null,
    };
  }

  // Compra e troca NAO limpam a marca. E o que impede o comprador escondido
  // de ser uma lavanderia: o pet muda de mao e continua identificavel.
  return {
    ...pet,
    ownerAccountId: toAccountId,
    acquisition: kind === 'capture' ? AcquisitionKind.CAPTURED : AcquisitionKind.TRADED,
  };
}

/** O pet esta marcado como roubado? */
export function isStolen(pet: PetOwnership): boolean {
  return pet.stolenFromAccountId !== null;
}

/**
 * Este pet delata quem o carrega numa batalha contra este observador?
 *
 * So delata para quem PODE reconhecer — quem leu a lista de procurados. E
 * nunca delata para o proprio ladrao, que ja sabe.
 */
export function revealsInBattle(
  pet: PetOwnership,
  observerAccountId: string,
  observerHasSeenWantedList: boolean,
): boolean {
  if (!isStolen(pet)) {
    return false;
  }

  if (observerAccountId === pet.ownerAccountId) {
    return false;
  }

  // O DONO ORIGINAL reconhece o proprio pet sem precisar de lista nenhuma.
  if (observerAccountId === pet.stolenFromAccountId) {
    return true;
  }

  return observerHasSeenWantedList;
}

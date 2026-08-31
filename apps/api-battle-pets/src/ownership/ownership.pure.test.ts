// Copyright 2026 Anderson. All Rights Reserved.

import { describe, expect, test } from 'bun:test';

import { AcquisitionKind } from './ownership.schema';
import { isStolen, revealsInBattle, transferOwnership, type PetOwnership } from './ownership.pure';

const DONO = 'conta-do-dono';
const LADRAO = 'conta-do-ladrao';
const COMPRADOR = 'conta-do-comprador';
const TERCEIRO = 'conta-de-um-terceiro';

function meuPet(): PetOwnership {
  return {
    catalogId: 'pet-qualquer',
    ownerAccountId: DONO,
    acquisition: AcquisitionKind.CAPTURED,
    stolenFromAccountId: null,
  };
}

describe('posse de pet', () => {
  test('roubar guarda o dono ORIGINAL, não o anterior', () => {
    // Se a marca guardasse o dono anterior, revender duas vezes apagaria a
    // origem — e o pet ficaria limpo depois de passar por duas mãos. Esse é o
    // caminho óbvio para lavar o roubo, e ele precisa estar fechado.
    const roubado = transferOwnership(meuPet(), LADRAO, 'theft');
    expect(roubado.stolenFromAccountId).toBe(DONO);

    const revendido = transferOwnership(roubado, COMPRADOR, 'trade');
    expect(revendido.ownerAccountId).toBe(COMPRADOR);
    expect(revendido.stolenFromAccountId).toBe(DONO);
  });

  test('comprar não limpa a marca', () => {
    // É o que impede o comprador escondido de ser uma lavanderia: o pet muda
    // de mão e continua identificável.
    const roubado = transferOwnership(meuPet(), LADRAO, 'theft');
    const comprado = transferOwnership(roubado, COMPRADOR, 'trade');

    expect(isStolen(comprado)).toBe(true);
    expect(comprado.acquisition).toBe(AcquisitionKind.TRADED);
  });

  test('devolver LIMPA a marca', () => {
    // Continuar marcado faria o pet ser delatado na batalha do próprio dono —
    // e a justiça viraria uma acusação permanente contra a vítima.
    const roubado = transferOwnership(meuPet(), LADRAO, 'theft');
    const devolvido = transferOwnership(roubado, DONO, 'return');

    expect(devolvido.ownerAccountId).toBe(DONO);
    expect(isStolen(devolvido)).toBe(false);
    expect(devolvido.acquisition).toBe(AcquisitionKind.RECLAIMED);
  });

  test('a transferência não muda o pet de antes', () => {
    // Posse é histórico. Mutar o anterior apagaria de onde o pet veio, que é
    // exatamente o que a devolução precisa consultar.
    const original = meuPet();
    transferOwnership(original, LADRAO, 'theft');

    expect(original.ownerAccountId).toBe(DONO);
    expect(original.stolenFromAccountId).toBeNull();
  });

  test('o pet limpo não delata ninguém', () => {
    expect(revealsInBattle(meuPet(), TERCEIRO, true)).toBe(false);
  });

  test('quem leu a lista reconhece; quem não leu, não', () => {
    // É o que transforma o poste da praça em objeto de jogo: ler dá o poder de
    // reconhecer. Sem isso a lista seria sabor.
    const roubado = transferOwnership(meuPet(), LADRAO, 'theft');

    expect(revealsInBattle(roubado, TERCEIRO, true)).toBe(true);
    expect(revealsInBattle(roubado, TERCEIRO, false)).toBe(false);
  });

  test('o dono original reconhece o próprio pet sem lista nenhuma', () => {
    // Exigir que a vítima leia um cartaz para reconhecer o que era dela seria
    // a regra trabalhando contra quem ela existe para proteger.
    const roubado = transferOwnership(meuPet(), LADRAO, 'theft');
    expect(revealsInBattle(roubado, DONO, false)).toBe(true);
  });

  test('o ladrão não é delatado para si mesmo', () => {
    // Ele já sabe. Avisá-lo seria ruído, e pior: entregaria que o sistema
    // está de olho, num momento em que ninguém mais está.
    const roubado = transferOwnership(meuPet(), LADRAO, 'theft');
    expect(revealsInBattle(roubado, LADRAO, true)).toBe(false);
  });

  test('capturar é posse limpa', () => {
    const capturado = transferOwnership(meuPet(), TERCEIRO, 'capture');
    expect(capturado.acquisition).toBe(AcquisitionKind.CAPTURED);
    expect(isStolen(capturado)).toBe(false);
  });
});

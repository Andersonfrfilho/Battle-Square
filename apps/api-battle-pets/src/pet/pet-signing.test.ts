// Copyright 2026 Anderson. All Rights Reserved.

import { describe, expect, test } from 'bun:test';

import { toSignedMoves } from './pet-signing';

describe('golpes no payload assinado', () => {
  test('a ordem vem do SLOT, nunca da ordem do banco', () => {
    // O banco não garante ordem sem ORDER BY, e o índice do slot é o que viaja
    // no commit: ordem diferente entre backend e cliente faria o jogador usar
    // um golpe e o servidor resolver outro.
    const foraDeOrdem = [
      { slot: 2, name: 'Rajada', power: 30 },
      { slot: 0, name: 'Bote', power: 10 },
      { slot: 3, name: 'Tromba', power: 40 },
      { slot: 1, name: 'Sopro', power: 20 },
    ];

    expect(toSignedMoves(foraDeOrdem).map((move) => move.name)).toEqual([
      'Bote',
      'Sopro',
      'Rajada',
      'Tromba',
    ]);
  });

  test('só nome e poder são assinados', () => {
    // Slot e id NÃO entram: o slot já é a posição no array, e repeti-lo daria
    // duas fontes para a mesma coisa. Id de linha não é do domínio do jogo.
    const assinados = toSignedMoves([{ slot: 0, name: 'Bote', power: 10 }]);

    expect(assinados).toEqual([{ name: 'Bote', power: 10 }]);
  });

  test('pet sem golpe produz lista vazia, não erro', () => {
    // Pet legado, cadastrado antes dos golpes existirem, não pode impedir o
    // espelho de sincronizar — ele entra sem golpe e o jogo degrada.
    expect(toSignedMoves([])).toEqual([]);
  });
});

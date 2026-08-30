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

  test('slot e id NÃO são assinados', () => {
    // O slot já é a posição no array, e repeti-lo daria duas fontes para a
    // mesma coisa. Id de linha não é do domínio do jogo.
    //
    // O efeito de terreno ENTROU no payload em 2026-08-29 (fatia 4): ele muda
    // a casa, logo muda o resultado, logo precisa estar sob assinatura.
    const assinados = toSignedMoves([{ slot: 0, name: 'Bote', power: 10 }]);

    expect(assinados).toEqual([
      {
        name: 'Bote',
        power: 10,
        terrainEffect: 'none',
        requiresAttribute: 'none',
        requiresValue: 0,
        effectStat: 'none',
        effectPercent: 0,
        terrainDuration: 0,
      },
    ]);
  });

  test('pet sem golpe produz lista vazia, não erro', () => {
    // Pet legado, cadastrado antes dos golpes existirem, não pode impedir o
    // espelho de sincronizar — ele entra sem golpe e o jogo degrada.
    expect(toSignedMoves([])).toEqual([]);
  });
});

describe('efeito de terreno do golpe', () => {
  test('ausente vira "none", nunca some do payload', () => {
    // Golpe cadastrado antes do efeito existir não muda a casa. Omitir a
    // chave produziria um payload diferente do que o verificador reconstrói,
    // e a assinatura falharia sem ninguém entender por quê.
    const [assinado] = toSignedMoves([{ slot: 0, name: 'Bote', power: 10 }]);

    expect(assinado?.terrainEffect).toBe('none');
  });

  test('o efeito declarado é preservado', () => {
    const assinados = toSignedMoves([
      { slot: 1, name: 'Jato', power: 40, terrainEffect: 'water' },
      { slot: 0, name: 'Brasa', power: 30, terrainEffect: 'damage' },
    ]);

    expect(assinados.map((move) => move.terrainEffect)).toEqual(['damage', 'water']);
  });
});

describe('requisito de atributo do golpe', () => {
  test('ausente vira "none"/0, nunca some do payload', () => {
    // Mesmo motivo do efeito de terreno: o verificador RESERIALIZA o golpe,
    // então uma chave omitida produz um texto diferente do que foi assinado.
    const [assinado] = toSignedMoves([{ slot: 0, name: 'Bote', power: 10 }]);

    expect(assinado?.requiresAttribute).toBe('none');
    expect(assinado?.requiresValue).toBe(0);
  });

  test('o requisito declarado é preservado', () => {
    const [assinado] = toSignedMoves([
      { slot: 0, name: 'Rasante', power: 45, requiresAttribute: 'flight', requiresValue: 12 },
    ]);

    expect(assinado?.requiresAttribute).toBe('flight');
    expect(assinado?.requiresValue).toBe(12);
  });

  test('o requisito entra NO FIM do objeto do golpe', () => {
    // A posição é contrato, não estética: o verificador reserializa o golpe,
    // então a ordem das chaves dentro dele entra na assinatura tanto quanto
    // a ordem dos campos do pet. Inserir no meio invalidaria todo golpe já
    // assinado que tivesse os campos anteriores.
    const [assinado] = toSignedMoves([
      { slot: 0, name: 'Bote', power: 10, terrainEffect: 'water', requiresAttribute: 'flight', requiresValue: 3 },
    ]);

    expect(Object.keys(assinado ?? {})).toEqual([
      'name',
      'power',
      'terrainEffect',
      'requiresAttribute',
      'requiresValue',
      'effectStat',
      'effectPercent',
      'terrainDuration',
    ]);
  });
});

describe('magia de atributo do golpe', () => {
  test('ausente vira "none"/0, nunca some do payload', () => {
    const [assinado] = toSignedMoves([{ slot: 0, name: 'Bote', power: 10 }]);

    expect(assinado?.effectStat).toBe('none');
    expect(assinado?.effectPercent).toBe(0);
  });

  test('o SINAL do percentual diz o alvo, e viaja assinado', () => {
    // Positivo sobe o do próprio pet; negativo derruba o do oponente. Não
    // existe subir o do oponente nem baixar o próprio, então o sinal cobre os
    // dois casos sem um campo a mais.
    const assinados = toSignedMoves([
      { slot: 0, name: 'Foco', power: 20, effectStat: 'attack', effectPercent: 30 },
      { slot: 1, name: 'Peso', power: 20, effectStat: 'speed', effectPercent: -25 },
    ]);

    expect(assinados[0]?.effectPercent).toBe(30);
    expect(assinados[1]?.effectPercent).toBe(-25);
    expect(assinados[1]?.effectStat).toBe('speed');
  });
});

// Copyright 2026 Anderson. All Rights Reserved.

import { describe, expect, test } from 'bun:test';

import {
  buildEvolutionChains,
  classifyModels,
  elementFromModelName,
  stageFromModelName,
  unclassified,
} from './model-mapping.pure';

/**
 * O gerador de especies (decisao 69): modelo -> elemento -> evolucao. Os
 * contrapesos no centro — nao chutar elemento, e nao prometer evolucao de um.
 */

describe('modelo -> elemento', () => {
  test('le a pista do nome, em qualquer convencao de asset', () => {
    expect(elementFromModelName('SK_FlameImp_01')).toBe('Fogo');
    expect(elementFromModelName('Flame_Imp')).toBe('Fogo');
    expect(elementFromModelName('IceGolem')).toBe('Terra'); // golem = rocha
    expect(elementFromModelName('SK_Skeleton_Warrior')).toBe('Fantasma');
    expect(elementFromModelName('CuteFish_A')).toBe('Agua');
    expect(elementFromModelName('Thunder_Drake')).toBe('Raio');
    expect(elementFromModelName('MushroomMonster')).toBe('Planta');
  });

  test('CONTRAPESO: sem pista NAO chuta elemento — fica para o humano', () => {
    // Chutar encheria o catalogo de erro silencioso. `undefined` e a resposta
    // honesta, e o gerador REPORTA esses modelos.
    expect(elementFromModelName('Monster_07')).toBeUndefined();
    expect(elementFromModelName('SK_Creature_B')).toBeUndefined();
  });
});

describe('modelo -> estagio (EPetGrowthStage)', () => {
  test('reconhece porte pelo nome', () => {
    expect(stageFromModelName('BabyDragon')).toBe('Filhote');
    expect(stageFromModelName('Small_Slime')).toBe('Filhote');
    expect(stageFromModelName('KingSlime')).toBe('Evoluido');
    expect(stageFromModelName('Ancient_Golem')).toBe('Evoluido');
  });

  test('sem pista cai no MEIO (Adulto), nunca num extremo', () => {
    expect(stageFromModelName('FlameImp')).toBe('Adulto');
  });
});

describe('as cadeias de evolucao', () => {
  const modelos = classifyModels([
    'BabyFlame', 'FlameImp', 'KingFlame',   // Fogo: os tres estagios
    'CuteFish', 'SharkLord',                 // Agua: dois estagios
    'Thunder_Drake',                         // Raio: um so
    'Monster_07',                            // sem pista
  ]);

  test('agrupa por elemento, na ordem dos estagios', () => {
    const cadeias = buildEvolutionChains(modelos);
    const fogo = cadeias.find((c) => c.element === 'Fogo');
    expect(fogo?.stages.Filhote).toBe('BabyFlame');
    expect(fogo?.stages.Adulto).toBe('FlameImp');
    expect(fogo?.stages.Evoluido).toBe('KingFlame');
  });

  test('CONTRAPESO: um modelo sozinho NAO vira evolucao', () => {
    // Prometer evolucao que nao acontece e pior que nao prometer.
    const cadeias = buildEvolutionChains(modelos);
    expect(cadeias.find((c) => c.element === 'Raio')).toBeUndefined();
    // Mas dois estagios ja e uma cadeia legitima.
    expect(cadeias.find((c) => c.element === 'Agua')).toBeDefined();
  });

  test('o modelo sem pista fica de fora, e e REPORTADO', () => {
    const cadeias = buildEvolutionChains(modelos);
    expect(cadeias.some((c) => Object.values(c.stages).includes('Monster_07'))).toBe(false);
    expect(unclassified(modelos).map((m) => m.modelName)).toContain('Monster_07');
  });

  test('DETERMINISTICO: a mesma lista da sempre a mesma cadeia', () => {
    expect(buildEvolutionChains(modelos)).toEqual(buildEvolutionChains(modelos));
  });
});

describe('a fronteira que este modulo NAO cruza', () => {
  test('nao conhece BIOMA — bioma->elemento mora no C++ (L-032)', async () => {
    // Estrutural, nao runtime: se alguem escrever a tabela de bioma aqui, este
    // teste reprova. Bioma->pet ja e automatico por elemento.
    const fonte = await Bun.file(
      new URL('./model-mapping.pure.ts', import.meta.url).pathname,
    ).text();
    const corpo = fonte.replace(/\/\*[\s\S]*?\*\//g, ''); // fora os comentarios
    for (const bioma of ['Forest', 'Swamp', 'Glacier', 'Volcano', 'Beach', 'Desert']) {
      expect(corpo).not.toContain(bioma);
    }
  });
});

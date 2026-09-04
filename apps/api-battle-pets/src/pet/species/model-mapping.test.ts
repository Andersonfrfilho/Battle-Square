// Copyright 2026 Anderson. All Rights Reserved.

import { describe, expect, test } from 'bun:test';

import {
  isCreature,
  buildEvolutionChains,
  familyOf,
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
    'Alpaking', 'Alpaking_Evolved',   // familia com duas formas (medido no pack)
    'Dragon', 'Dragon_Evolved',       // outra familia, tambem Fogo
    'Squidle',                        // uma forma so
    'Monster_07',                     // sem pista de elemento
  ]);

  test('agrupa por FAMILIA, e o sufixo _Evolved e a forma evoluida', () => {
    const cadeias = buildEvolutionChains(modelos);
    const alpaking = cadeias.find((c) => c.family === 'Alpaking');
    expect(alpaking?.stages.Adulto).toBe('Alpaking');
    expect(alpaking?.stages.Evoluido).toBe('Alpaking_Evolved');
  });

  test('CONTRAPESO: duas familias do MESMO elemento nao viram uma cadeia so', () => {
    // Dragon e Alpaking podem ser Fogo os dois, e NAO evoluem um no outro.
    // Agrupar por elemento inventaria parentesco que nao existe.
    const cadeias = buildEvolutionChains(modelos);
    expect(cadeias.filter((c) => c.family === 'Dragon')).toHaveLength(1);
    expect(cadeias.find((c) => c.family === 'Dragon')?.stages.Evoluido).toBe('Dragon_Evolved');
  });

  test('a forma evoluida HERDA o elemento da familia', () => {
    expect(elementFromModelName('Dragon_Evolved')).toBe(elementFromModelName('Dragon'));
    expect(familyOf('Dragon_Evolved')).toBe('Dragon');
  });

  test('CONTRAPESO: um modelo sozinho NAO vira evolucao', () => {
    // Prometer evolucao que nao acontece e pior que nao prometer.
    const cadeias = buildEvolutionChains(modelos);
    expect(cadeias.find((c) => c.family === 'Squidle')).toBeUndefined();
    // Mas duas formas ja e uma cadeia legitima.
    expect(cadeias.find((c) => c.family === 'Alpaking')).toBeDefined();
  });

  test('o modelo sem pista fica de fora, e e REPORTADO', () => {
    const cadeias = buildEvolutionChains(modelos);
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

describe('o que NAO e criatura', () => {
  test('rig e prop saem antes de classificar — o relatorio tem de ser legivel', () => {
    // Medido nos packs: o Cute Fish traz iscas e docas; os de personagem, rigs.
    const modelos = classifyModels(['Goldfish', 'Lure_1', 'Dock_Long', 'Rig_Medium_General', 'Boat']);
    expect(modelos.map((m) => m.modelName)).toEqual(['Goldfish']);
  });

  test('CONTRAPESO: nao corta bicho por acidente de substring', () => {
    // "Frigate" contem "rig", e e (hipoteticamente) um bicho. Token exato salva.
    expect(isCreature('Frigate')).toBe(true);
    expect(isCreature('Rig_Medium')).toBe(false);
  });
});

describe('a pista do PACOTE', () => {
  test('preenche o que o nome nao disse — nome de especie num pacote de peixe', () => {
    // Medido: Cute Fish Pack traz Tetra, Betta, Koi. O nome nao diz "agua"; o
    // pacote diz.
    const modelos = classifyModels(['Tetra', 'Betta', 'Koi'], 'Agua');
    expect(modelos.every((m) => m.element === 'Agua')).toBe(true);
  });

  test('CONTRAPESO: o NOME ganha do pacote, sempre', () => {
    // Um monstro de fogo dentro de um pacote de peixe continua sendo de fogo —
    // senao a pista do pacote apagaria a informacao mais forte.
    const [dragao] = classifyModels(['Dragon'], 'Agua');
    expect(dragao!.element).toBe('Fogo');
  });

  test('sem pista de pacote, o desconhecido continua desconhecido', () => {
    const [x] = classifyModels(['Slime']);
    expect(x!.element).toBeUndefined();
  });
});

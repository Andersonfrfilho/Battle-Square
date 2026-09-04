// Copyright 2026 Anderson. All Rights Reserved.

import { describe, expect, test } from 'bun:test';

import { IMPORTED_ASSETS_PATH, loadImportedAssets } from './imported-assets.pure';
import { buildPetModelsHandoff } from './pet-models-report.pure';

/**
 * AR4e — a lista de decisao do usuario, medida no insumo de 04/09/2026.
 * Cada item tem de dizer o PORQUE e o que a resposta desbloqueia.
 */
const insumo = loadImportedAssets(await Bun.file(IMPORTED_ASSETS_PATH).text());
const { decisoes } = buildPetModelsHandoff(insumo);
const ids = decisoes.map((d) => d.id);

describe('a lista de decisoes do usuario', () => {
  test('e curta, e cada item diz o porque e o que desbloqueia', () => {
    expect(decisoes.length).toBeLessThanOrEqual(12);
    for (const d of decisoes) {
      expect(d.porque.length, d.id).toBeGreaterThan(20);
      expect(d.desbloqueia.length, d.id).toBeGreaterThan(10);
    }
  });

  test('as duas cadeias do GOAL, com as opcoes que ele sugeriu', () => {
    expect(decisoes.find((d) => d.id === 'elemento-Alpaking')?.pergunta).toBe('Alpaking (Alpaking, Alpaking_Evolved): Terra ou Planta?');
    expect(decisoes.find((d) => d.id === 'elemento-Armabee')?.pergunta).toBe('Armabee (Armabee, Armabee_Evolved): Ar ou Planta?');
  });

  test('as cadeias de porte sem elemento tambem perguntam — Alien, Ninja, Tribal', () => {
    expect(ids).toEqual(expect.arrayContaining(['elemento-Alien', 'elemento-Ninja', 'elemento-Tribal']));
    expect(decisoes.find((d) => d.id === 'elemento-Alien')?.pergunta).toContain('que elemento?');
  });

  test('o conflito da Bee e a falta de Luz sao decisoes, com o pet que elas travam', () => {
    expect(decisoes.find((d) => d.id === 'conflito-Bee')?.pergunta).toBe('Bee: Planta ou Ar?');
    expect(decisoes.find((d) => d.id === 'falta-pack-Luz')?.pergunta).toContain('Candeia e Farol');
  });

  test('Raio tem modelo e nao tem pet; Fish, Birb e Cactoro nao tem Adulto', () => {
    expect(decisoes.find((d) => d.id === 'sem-pet-Raio')?.pergunta).toContain('Hywirl');
    const semAdulto = decisoes.find((d) => d.id === 'cadeias-sem-adulto');
    for (const f of ['Fish', 'Birb', 'Cactoro']) expect(semAdulto?.pergunta).toContain(f);
  });

  test('os soltos sem pista vao num item so, e nao repetem quem ja esta em cadeia ou conflito', () => {
    const soltos = decisoes.find((d) => d.id === 'sem-pista-soltos');
    expect(soltos?.pergunta).toContain('Slime');
    expect(soltos?.pergunta).toContain('PinkBlob');
    for (const repetido of ['Alien', 'Bee', 'Alpaking', 'Ninja']) expect(soltos?.pergunta).not.toContain(repetido);
  });

  test('a ordem e fixa: o que trava cadeia vem antes do que so engorda o estoque', () => {
    expect(ids[0]).toBe('elemento-Alien');
    expect(ids.at(-1)).toBe('sem-pista-soltos');
  });
});

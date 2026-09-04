// Copyright 2026 Anderson. All Rights Reserved.

import { describe, expect, test } from 'bun:test';

import { ENGINE_TYPES_PATH, loadEngineElements } from './engine-types.schema';
import { IMPORTED_ASSETS_PATH, loadImportedAssets } from './imported-assets.pure';
import { buildPetModelsHandoff } from './pet-models-report.pure';

/**
 * AR4e — a lista de decisao do usuario, medida no insumo de 04/09/2026 e ja
 * com as respostas dele aplicadas (element-decisions.constant.ts). Cada item
 * tem de dizer o PORQUE e o que a resposta desbloqueia.
 */
const insumo = loadImportedAssets(await Bun.file(IMPORTED_ASSETS_PATH).text());
const elementosDoMotor = loadEngineElements(await Bun.file(ENGINE_TYPES_PATH).text());
const { decisoes } = buildPetModelsHandoff({ insumo, elementosDoMotor });
const ids = decisoes.map((d) => d.id);
const byId = (id: string) => decisoes.find((d) => d.id === id);

describe('a lista de decisoes do usuario', () => {
  test('e curta, e cada item diz o porque e o que desbloqueia', () => {
    expect(decisoes.length).toBeLessThanOrEqual(12);
    for (const d of decisoes) {
      expect(d.porque.length, d.id).toBeGreaterThan(20);
      expect(d.desbloqueia.length, d.id).toBeGreaterThan(10);
    }
  });

  test('so o Alpaking ainda pergunta elemento — o insumo nao traz cor, e a cor e o que decidiria', () => {
    expect(byId('elemento-Alpaking')?.pergunta).toBe('Alpaking (Alpaking, Alpaking_Evolved): Terra ou Planta?');
    for (const decidido of ['Armabee', 'Alien', 'Ninja', 'Tribal']) expect(ids).not.toContain(`elemento-${decidido}`);
  });

  test('o que o usuario respondeu SAIU da lista: conflito da Bee e os soltos sem pista', () => {
    expect(ids).not.toContain('conflito-Bee');
    expect(ids).not.toContain('sem-pista-soltos');
  });

  test('Comum existe no gerador e nao no motor: vira item medido, com a receita e as familias que ja o usam', () => {
    const comum = byId('motor-Comum');
    expect(comum?.pergunta).toContain('6 familias');
    expect(comum?.pergunta).toContain('Slime');
    expect(comum?.porque).toContain('PetTypes.json');
    expect(comum?.desbloqueia).toContain('TypeEffectiveness.json');
    expect(ids.filter((id) => id.startsWith('motor-'))).toEqual(['motor-Comum']);
  });

  test('a falta de Luz continua, com o pet que ela trava', () => {
    expect(byId('falta-pack-Luz')?.pergunta).toContain('Candeia e Farol');
  });

  test('Raio agora tem Alien E Hywirl sem pet; Comum tem seis; Fish, Birb e Cactoro nao tem Adulto', () => {
    expect(byId('sem-pet-Raio')?.pergunta).toContain('Alien, Hywirl');
    expect(byId('sem-pet-Comum')?.pergunta).toContain('Cat, Dog');
    const semAdulto = byId('cadeias-sem-adulto');
    for (const f of ['Fish', 'Birb', 'Cactoro']) expect(semAdulto?.pergunta).toContain(f);
  });

  test('a ordem e fixa: o que trava cadeia vem antes, o motor logo depois, o estoque por ultimo', () => {
    expect(ids[0]).toBe('elemento-Alpaking');
    expect(ids.indexOf('motor-Comum')).toBeLessThan(ids.indexOf('falta-pack-Luz'));
    expect(ids.at(-1)).toBe('sem-pet-Comum');
  });
});

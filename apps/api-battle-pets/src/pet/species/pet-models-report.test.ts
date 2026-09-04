// Copyright 2026 Anderson. All Rights Reserved.

import { describe, expect, test } from 'bun:test';

import { IMPORTED_ASSETS_PATH, loadImportedAssets } from './imported-assets.pure';
import { buildPetModelsHandoff, PET_MODELS_HANDOFF_PATH, serializeHandoff } from './pet-models-report.pure';

/**
 * AR4d — o handoff que a trilha A le. O que este teste protege e que o JSON
 * commitado seja o GERADO, nunca um editado a mao que o gerador ja nao produz.
 */
const insumo = loadImportedAssets(await Bun.file(IMPORTED_ASSETS_PATH).text());
const handoff = buildPetModelsHandoff(insumo);

describe('o handoff pets-modelos.json', () => {
  test('o arquivo commitado E o gerado — se divergir, rode generate-pet-models.ts', async () => {
    const commitado = await Bun.file(PET_MODELS_HANDOFF_PATH).text();
    expect(commitado).toBe(serializeHandoff(handoff));
  });

  test('a chave e o NOME do pet, e todo caminho e asset do /Game', () => {
    expect(handoff.chave).toContain('nome do pet');
    for (const pet of handoff.pets) {
      for (const path of [...Object.values(pet.estagios), ...pet.variantes]) {
        expect(path, pet.pet).toMatch(/^\/Game\/Quaternius\/(Monsters|Fish)\/SK_[A-Za-z0-9_]+$/);
      }
    }
  });

  test('carrega a data do insumo, nao a de hoje — regerar sem insumo novo nao muda nada', () => {
    expect(handoff.geradoEm).toBe(insumo.geradoEm);
    expect(serializeHandoff(buildPetModelsHandoff(insumo))).toBe(serializeHandoff(handoff));
  });

  test('a trilha A acha o Faísca e recebe os dois estagios do Dragon', () => {
    const faisca = handoff.pets.find((p) => p.pet === 'Faísca');
    expect(faisca?.estagios.Adulto).toBe('/Game/Quaternius/Monsters/SK_Dragon');
    expect(faisca?.estagios.Evoluido).toBe('/Game/Quaternius/Monsters/SK_Dragon_Evolved');
  });

  test('leva junto o que a trilha A precisa para nao chutar: sem modelo, livres, cadeias, classificacao', () => {
    expect(handoff.semModelo.map((s) => s.pet)).toEqual(['Zunido', 'Candeia', 'Farol']);
    expect(handoff.familiasLivres.Raio).toEqual(['Hywirl']);
    expect(handoff.cadeias.length).toBe(17);
    expect(handoff.classificacao.total).toBe(128);
  });
});

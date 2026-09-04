// Copyright 2026 Anderson. All Rights Reserved.

import { describe, expect, test } from 'bun:test';

import {
  IMPORTED_ASSETS_PATH, classifyImportedAssets, loadImportedAssets, summarizeClassification,
  type ClassifiedAsset,
} from './imported-assets.pure';
import { familyOf, stageFromModelName } from './model-mapping.pure';

/**
 * Cada teste abaixo leva o NOME REAL do arquivo que causou o defeito
 * (invariante 5 do GOAL). O insumo e o arquivo exportado pela trilha A.
 */
const file = loadImportedAssets(await Bun.file(IMPORTED_ASSETS_PATH).text());
const classified = classifyImportedAssets(file.malhas);
const byName = new Map(classified.map((a) => [a.modelName, a]));

function get(modelName: string): ClassifiedAsset {
  const found = byName.get(modelName);
  if (!found) throw new Error(`${modelName} nao esta no insumo`);
  return found;
}

describe('o insumo exportado pela trilha A', () => {
  test('tem as 128 malhas e todas ficam classificadas', () => {
    expect(file.total).toBe(128);
    expect(classified).toHaveLength(128);
    const report = summarizeClassification(classified);
    expect(report.criaturas + report.humanos.length + report.props.length).toBe(128);
  });

  test('rejeita insumo com total que nao bate (truncado ou editado a mao)', () => {
    const truncated = JSON.stringify({ ...file, malhas: file.malhas.slice(0, 3) });
    expect(() => loadImportedAssets(truncated)).toThrow();
  });
});

describe('a PASTA corrige o nome quando o pack e de um elemento so', () => {
  test('SK_FlowerHorn e SK_Sunfish sao peixes, nao Planta nem Luz', () => {
    expect(get('FlowerHorn').element).toBe('Agua');
    expect(get('FlowerHorn').elementSource).toBe('pasta');
    expect(get('Sunfish').element).toBe('Agua');
  });

  test('peixe que o NOME ja diz fica com fonte "nome"', () => {
    expect(get('Fish1').elementSource).toBe('nome');
    expect(get('Shark_AF').elementSource).toBe('nome');
  });

  test('SK_Pigeon_Blob e Ar: "pig" (Terra) nao pode casar por prefixo', () => {
    expect(get('Pigeon_Blob')).toMatchObject({ element: 'Ar', conflito: [], family: 'Pigeon' });
    expect(get('Pig')).toMatchObject({ element: 'Terra', family: 'Pig' });
  });

  test('SK_Worm (isca do Cute Fish Pack) e prop, nao criatura', () => {
    expect(get('Worm').kind).toBe('prop');
    expect(get('Worm').element).toBeUndefined();
  });
});

describe('gente da vila nao e pet (decisao 69)', () => {
  test('os seis aventureiros do KayKit sao humanos', () => {
    for (const human of ['Barbarian', 'Knight', 'Mage', 'Ranger', 'Rogue', 'Rogue_Hooded']) {
      expect(get(human).kind).toBe('humano');
    }
  });

  test('SK_Skeleton_Mage e criatura Fantasma: o morto-vivo ganha da classe', () => {
    expect(get('Skeleton_Mage').kind).toBe('criatura');
    expect(get('Skeleton_Mage').element).toBe('Fantasma');
  });
});

describe('o gerador NUNCA chuta elemento', () => {
  test('SK_Bee sugere Planta E Ar — conflito vira undefined e entra na lista', () => {
    expect(get('Bee').element).toBeUndefined();
    expect(get('Bee').conflito).toEqual(['Planta', 'Ar']);
  });

  test('os sem pista do GOAL ficam undefined, sem conflito', () => {
    const noHint = ['Alpaking', 'Armabee', 'Tribal_Big', 'Ninja_Blob', 'Alien', 'Alien_Tall',
      'Dog', 'Cat', 'Slime', 'PinkBlob', 'GreenBlob', 'GreenSpikyBlob'];
    for (const name of noHint) {
      expect(get(name).element).toBeUndefined();
      expect(get(name).conflito).toEqual([]);
    }
  });

  test('a lista do que nao soube e exatamente esta (19 criaturas)', () => {
    expect(summarizeClassification(classified).semElemento).toEqual([
      'Alien', 'Alien_Big', 'Alien_Blob', 'Alien_Tall', 'Alpaking', 'Alpaking_Evolved',
      'Armabee', 'Armabee_Evolved', 'Bee', 'Cat', 'Dog', 'GreenBlob', 'GreenSpikyBlob',
      'Ninja_Big', 'Ninja_Blob', 'PinkBlob', 'Slime', 'Tribal_Big', 'Tribal_Flying',
    ]);
  });
});

describe('familia e estagio pelos sufixos REAIS do import', () => {
  test('SK_Bat_AM e SK_Bat_CM sao a mesma familia Bat, ambos Adulto', () => {
    expect(get('Bat_AM').family).toBe('Bat');
    expect(get('Bat_CM').family).toBe('Bat');
    expect(get('Bat_CM').stage).toBe('Adulto');
  });

  test('SK_Cactoro_Blob e Filhote e SK_Cactoro_Big e Evoluido, pelo porte', () => {
    expect(get('Cactoro_Blob')).toMatchObject({ family: 'Cactoro', stage: 'Filhote', stageOrigin: 'porte' });
    expect(get('Cactoro_Big')).toMatchObject({ family: 'Cactoro', stage: 'Evoluido', element: 'Planta' });
  });

  test('SK_Blobfish NAO e filhote: "blob" so vale como sufixo de corpo', () => {
    expect(get('Blobfish').stage).toBe('Adulto');
    expect(get('Blobfish').family).toBe('Blobfish');
  });

  test('SK_PinkBlob mantem a familia inteira: o Blob E o bicho', () => {
    expect(get('PinkBlob').family).toBe('PinkBlob');
    expect(get('GreenSpikyBlob').stage).toBe('Adulto');
  });

  test('SK_MushroomKing e o Mushroom evoluido; SK_Alpaking nao e rei de nada', () => {
    expect(get('MushroomKing')).toMatchObject({ family: 'Mushroom', stage: 'Evoluido', stageOrigin: 'autor' });
    expect(get('Alpaking')).toMatchObject({ family: 'Alpaking', stage: 'Adulto' });
    expect(get('Alpaking_Evolved')).toMatchObject({ family: 'Alpaking', stage: 'Evoluido' });
  });

  test('SK_Orc_Skull e SK_Ghost_Skull sao corpos de Orc e Ghost; SK_Skull e familia propria', () => {
    expect(get('Orc_Skull').family).toBe('Orc');
    expect(get('Ghost_Skull').family).toBe('Ghost');
    expect(get('Skull')).toMatchObject({ family: 'Skull', element: 'Fantasma' });
  });

  test('familyOf tira o SK_ do import e stageFromModelName ignora-o', () => {
    expect(familyOf('SK_Dragon_Evolved')).toBe('Dragon');
    expect(stageFromModelName('SK_Dragon_Evolved')).toBe('Evoluido');
  });
});

describe('o relatorio separa o que soube do que nao soube', () => {
  test('totais medidos no insumo de 2026-09-04', () => {
    const report = summarizeClassification(classified);
    expect(report.humanos).toHaveLength(6);
    expect(report.props).toEqual(['Worm']);
    expect(report.criaturas).toBe(121);
    expect(report.semElemento).toHaveLength(19);
    expect(report.comElemento.Agua).toHaveLength(42 + 8);
    expect(report.comElemento.Raio).toEqual(['Hywirl']);
    expect(report.conflitos).toEqual([{ modelName: 'Bee', elementos: ['Planta', 'Ar'] }]);
  });
});

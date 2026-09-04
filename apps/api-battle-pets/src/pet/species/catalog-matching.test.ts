// Copyright 2026 Anderson. All Rights Reserved.

import { describe, expect, test } from 'bun:test';

import { PET_CATALOG_SEED } from '../seed/pet-catalog.seed';
import { PET_MODEL_ASSIGNMENTS, type PetModelAssignment } from './catalog-assignment.constant';
import { CatalogMatchError, matchCatalog } from './catalog-matching.pure';
import { groupAssetsByFamily } from './evolution-chains.pure';
import { classifyImportedAssets, IMPORTED_ASSETS_PATH, loadImportedAssets } from './imported-assets.pure';

/**
 * AR4c — o casamento entre PET_CATALOG_SEED e o insumo de 04/09/2026, com os
 * nomes REAIS. O que este teste afirma e o que a trilha A vai receber.
 */
const insumo = loadImportedAssets(await Bun.file(IMPORTED_ASSETS_PATH).text());
const familias = groupAssetsByFamily(classifyImportedAssets(insumo.malhas));
const casamento = matchCatalog({ seed: PET_CATALOG_SEED, assignments: PET_MODEL_ASSIGNMENTS, families: familias });

const MONSTERS = '/Game/Quaternius/Monsters';

describe('a curadoria cobre o catalogo inteiro', () => {
  test('todo pet do seed aparece: vestido ou registrado sem modelo', () => {
    const nomes = [...casamento.atribuicoes.map((a) => a.pet), ...casamento.semModelo.map((s) => s.pet)].sort();
    expect(nomes).toEqual(PET_CATALOG_SEED.map((p) => p.name).sort());
  });

  test('o catalogo tem 23 pets, nao os 115 do GOAL — medido no seed', () => {
    expect(PET_CATALOG_SEED).toHaveLength(23);
    expect(casamento.atribuicoes).toHaveLength(20);
    expect(casamento.semModelo.map((s) => s.pet)).toEqual(['Zunido', 'Candeia', 'Farol']);
  });

  test('pet sem modelo diz o PORQUE — e uma decisao, nao um esquecimento', () => {
    const zunido = casamento.semModelo.find((s) => s.pet === 'Zunido');
    expect(zunido?.elemento).toBe('Planta');
    expect(zunido?.motivo).toContain('Bee');
    for (const s of casamento.semModelo) expect(s.motivo.length).toBeGreaterThan(20);
  });
});

describe('o que cada pet veste', () => {
  test('Faísca veste a cadeia assinada do Dragon, com o Dragon_Mon como variante', () => {
    const faisca = casamento.atribuicoes.find((a) => a.pet === 'Faísca');
    expect(faisca?.origem).toBe('autor');
    expect(faisca?.estagios).toEqual({ Adulto: `${MONSTERS}/SK_Dragon`, Evoluido: `${MONSTERS}/SK_Dragon_Evolved` });
    expect(faisca?.variantes).toEqual([`${MONSTERS}/SK_Dragon_Mon`]);
  });

  test('Cirro pina a pele Bat_CM como Adulto, e a Bat_AM vira variante', () => {
    const cirro = casamento.atribuicoes.find((a) => a.pet === 'Cirro');
    expect(cirro?.estagios.Adulto).toBe(`${MONSTERS}/SK_Bat_CM`);
    expect(cirro?.variantes).toEqual([`${MONSTERS}/SK_Bat_AM`]);
  });

  test('Rajada veste o Pigeon: o pombo voando e o Adulto, e o Blob e o Filhote', () => {
    const rajada = casamento.atribuicoes.find((a) => a.pet === 'Rajada');
    expect(rajada?.origem).toBe('porte');
    expect(rajada?.estagios).toEqual({ Filhote: `${MONSTERS}/SK_Pigeon_Blob`, Adulto: `${MONSTERS}/SK_Pigeon_Flying` });
  });

  test('todo pet vestido tem forma ADULTO — e o estagio em que o C++ o faz nascer', () => {
    for (const a of casamento.atribuicoes) expect(a.estagios.Adulto, a.pet).toBeDefined();
  });

  test('o elemento do pet e o da familia, sempre — e a escola vem do tipo', () => {
    for (const a of casamento.atribuicoes) {
      const familia = familias.find((f) => f.family === a.familia);
      expect(familia?.element, a.pet).toBe(a.elemento);
      expect(a.escola.length, a.pet).toBeGreaterThan(0);
    }
  });

  test('nenhuma familia veste dois pets', () => {
    const usadas = casamento.atribuicoes.map((a) => a.familia);
    expect(new Set(usadas).size).toBe(usadas.length);
  });
});

describe('o estoque que sobra (livres)', () => {
  test('Raio sobra inteiro: Hywirl sem pet de Raio no catalogo', () => {
    expect(casamento.livres.Raio).toEqual(['Hywirl']);
  });

  test('Luz nao sobra nada — e por isso Candeia e Farol ficam sem modelo', () => {
    expect(casamento.livres.Luz).toEqual([]);
  });

  test('Agua sobra muito: o pack de peixes e maior que o catalogo', () => {
    expect(casamento.livres.Agua.length).toBeGreaterThan(30);
    expect(casamento.livres.Agua).toContain('Clownfish');
  });
});

describe('CONTRAPESO: a curadoria errada REPROVA, com todos os problemas de uma vez', () => {
  const troca = (pet: string, patch: Partial<PetModelAssignment>): PetModelAssignment[] =>
    PET_MODEL_ASSIGNMENTS.map((a) => (a.pet === pet ? { ...a, ...patch } : a));
  const casar = (assignments: readonly PetModelAssignment[]) =>
    () => matchCatalog({ seed: PET_CATALOG_SEED, assignments, families: familias });

  test('familia de outro elemento — Faísca (Fogo) vestindo Glub (Agua)', () => {
    // Glub ja veste Mare: troca a Mare tambem, para isolar o erro de elemento.
    const lista = troca('Maré', { familia: undefined }).map((a) => (a.pet === 'Faísca' ? { ...a, familia: 'Glub' } : a));
    expect(casar(lista)).toThrow(/Faísca \(Fogo\): familia Glub e Agua/);
  });

  test('familia sem elemento — nunca se veste um Slime sem decidir o elemento dele', () => {
    expect(casar(troca('Barro', { familia: 'Slime' }))).toThrow(/Slime e sem elemento/);
  });

  test('cadeia sem Adulto — o Fish do Monsters so tem Blob e Big (medido em 04/09)', () => {
    // Foi a primeira curadoria: Vagalhao vestia o Fish, e o teste de estagios
    // reprovou. Sem esta regra, o pet nasceria sem corpo.
    expect(casar(troca('Vagalhão', { familia: 'Fish', adulto: undefined }))).toThrow(/Fish nao tem forma Adulto/);
    expect(casar(troca('Rajada', { familia: 'Birb' }))).toThrow(/Birb nao tem forma Adulto/);
    expect(casar(troca('Brisa', { familia: 'Cactoro' }))).toThrow(/Cactoro nao tem forma Adulto/);
  });

  test('a mesma familia em dois pets', () => {
    expect(casar(troca('Cinza', { familia: 'Dragon' }))).toThrow(/familia Dragon ja veste Faísca/);
  });

  test('familia que nao esta no insumo, e pele que nao e da familia', () => {
    expect(casar(troca('Barro', { familia: 'Unicornio' }))).toThrow(/Unicornio nao esta no insumo/);
    expect(casar(troca('Cirro', { adulto: 'Bat_XX' }))).toThrow(/Bat_XX nao pertence a familia Bat/);
  });

  test('pet esquecido e pet inventado, reportados JUNTOS', () => {
    const lista = PET_MODEL_ASSIGNMENTS.filter((a) => a.pet !== 'Lume').concat({ pet: 'Fantasminha', motivo: 'x' });
    try {
      casar(lista)();
      throw new Error('devia reprovar');
    } catch (error) {
      expect(error).toBeInstanceOf(CatalogMatchError);
      const problemas = (error as CatalogMatchError).problemas;
      expect(problemas).toContain('Lume: 0 atribuicoes (precisa de exatamente 1)');
      expect(problemas).toContain('Fantasminha: nao existe em PET_CATALOG_SEED');
    }
  });

  test('DETERMINISTICO: a mesma entrada da sempre a mesma saida', () => {
    expect(matchCatalog({ seed: PET_CATALOG_SEED, assignments: PET_MODEL_ASSIGNMENTS, families: familias })).toEqual(casamento);
  });
});

// Copyright 2026 Anderson. All Rights Reserved.

import { describe, expect, test } from 'bun:test';

import { createPetSchema } from '../pet.validation';
import { PET_CATALOG_SEED } from './pet-catalog.seed';

describe('catálogo inicial de pets', () => {
  test('todo pet do seed passa na MESMA validação do cadastro', () => {
    // Seed que burla a validação é uma segunda porta para dado inválido — e a
    // regra do projeto (code-standart.md §5) existe justamente por isso.
    for (const definicao of PET_CATALOG_SEED) {
      const resultado = createPetSchema.safeParse(definicao);
      expect(resultado.success).toBe(true);
    }
  });

  test('todo pet tem exatamente quatro golpes', () => {
    // Quatro é a decisão (DP-golpe-01) e o que a tela mostra. Um pet com três
    // apareceria com um botão morto.
    for (const definicao of PET_CATALOG_SEED) {
      expect(definicao.moves?.length).toBe(4);
    }
  });

  test('cada pet tem um golpe fraco e um forte', () => {
    // Sem diferença de poder, escolher entre quatro golpes é escolher entre
    // quatro nomes — e a decisão do turno deixa de existir.
    for (const definicao of PET_CATALOG_SEED) {
      const poderes = (definicao.moves ?? []).map((move) => move.power);
      const menor = Math.min(...poderes);
      const maior = Math.max(...poderes);

      expect(maior).toBeGreaterThan(menor * 1.5);
    }
  });

  test('golpe que muda terreno nunca é o mais forte do pet', () => {
    // Terreno de graça faria a escolha ser óbvia: o golpe que muda o chão
    // precisa custar dano, senão não há troca nenhuma a decidir.
    for (const definicao of PET_CATALOG_SEED) {
      const golpes = definicao.moves ?? [];
      const maiorPoder = Math.max(...golpes.map((move) => move.power));

      for (const golpe of golpes) {
        if (golpe.terrainEffect && golpe.terrainEffect !== 'none') {
          expect(golpe.power).toBeLessThan(maiorPoder);
        }
      }
    }
  });

  test('há pets de tipos diferentes, e água alaga', () => {
    const tipos = new Set(PET_CATALOG_SEED.map((pet) => pet.type));
    expect(tipos.size).toBeGreaterThanOrEqual(3);

    // O pet de Água precisa poder FABRICAR o terreno da própria skill: sem um
    // golpe que alaga, submergir só funciona onde a arena já tem água.
    const agua = PET_CATALOG_SEED.filter((pet) => pet.type === 'Agua');
    for (const pet of agua) {
      const alaga = (pet.moves ?? []).some((move) => move.terrainEffect === 'water');
      expect(alaga).toBe(true);
    }
  });

  test('nomes de pet não se repetem', () => {
    // O seed é idempotente POR NOME: dois pets com o mesmo nome fariam a
    // segunda execução pular um que nunca foi criado.
    const nomes = PET_CATALOG_SEED.map((pet) => pet.name);
    expect(new Set(nomes).size).toBe(nomes.length);
  });
});

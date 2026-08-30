// Copyright 2026 Anderson. All Rights Reserved.

import { describe, expect, test } from 'bun:test';

import { createPetSchema } from '../pet.validation';
import { PET_CATALOG_SEED } from './pet-catalog.seed';

/** O elemento de 'Natural/Agua' é 'Agua'. */
function elementoDe(tipo: string): string {
  return tipo.split('/')[1] ?? tipo;
}

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

    // O filtro era `pet.type === 'Agua'` e o tipo é 'Natural/Agua': ele não
    // casava com pet NENHUM, e este teste passava sem verificar coisa alguma.
    // Ninguém percebe um teste verde que não roda — por isso a contagem vem
    // junto, e ela é o que denuncia o filtro morto da próxima vez.
    const agua = PET_CATALOG_SEED.filter((pet) => elementoDe(pet.type) === 'Agua');
    expect(agua.length).toBeGreaterThan(0);

    // O pet de Água precisa poder FABRICAR o terreno da própria skill: sem um
    // golpe que alaga, submergir só funciona onde a arena já tem água.
    for (const pet of agua) {
      const alaga = (pet.moves ?? []).some((move) => move.terrainEffect === 'water');
      expect(alaga).toBe(true);
    }
  });

  test('as doze combinações de escola e elemento existem', () => {
    // Combinação que falta é um canto do jogo que ninguém pode jogar. Cinco
    // estavam vazias, e nada acusava: cada pet existente estava correto.
    const escolas = ['Fisica', 'Natural', 'Psiquica'];
    const elementos = ['Fogo', 'Agua', 'Planta', 'Terra'];
    const existentes = new Set(PET_CATALOG_SEED.map((pet) => pet.type));

    const faltando: string[] = [];
    for (const escola of escolas) {
      for (const elemento of elementos) {
        if (!existentes.has(`${escola}/${elemento}`)) {
          faltando.push(`${escola}/${elemento}`);
        }
      }
    }

    expect(faltando).toEqual([]);
  });

  test('só terra e água enlameiam', () => {
    // A lama é chão encharcado: ela sai de quem tem terra ou água. Um pet de
    // fogo criando lama apagaria a razão de os elementos serem diferentes.
    for (const pet of PET_CATALOG_SEED) {
      const enlameia = (pet.moves ?? []).some((move) => move.terrainEffect === 'mud');
      if (enlameia) {
        expect(['Terra', 'Agua']).toContain(elementoDe(pet.type));
      }
    }

    // E ALGUÉM enlameia: sem isto a lama seria terreno que só nasce da água
    // secando, e nenhum jogador teria como provocá-la.
    const quemEnlameia = PET_CATALOG_SEED.filter((pet) =>
      (pet.moves ?? []).some((move) => move.terrainEffect === 'mud'),
    );
    expect(quemEnlameia.length).toBeGreaterThan(0);
  });

  test('o congelamento tem níveis, e o nível é a duração', () => {
    // DP-gelo-01: não existe "nível de congelamento" separado da duração. Um
    // congelamento forte é um que demora mais a derreter — e para isso ser
    // legível, o catálogo precisa ter mais de um.
    const duracoes = PET_CATALOG_SEED.flatMap((pet) =>
      (pet.moves ?? [])
        .filter((move) => move.terrainEffect === 'ice')
        .map((move) => move.terrainDuration ?? 0),
    );

    expect(duracoes.length).toBeGreaterThan(0);
    expect(new Set(duracoes).size).toBeGreaterThanOrEqual(2);

    // Gelo sem prazo seria gelo PERMANENTE: zero é "para sempre", e uma casa
    // congelada para sempre é uma casa bloqueada com outro nome.
    for (const duracao of duracoes) {
      expect(duracao).toBeGreaterThan(0);
    }
  });

  test('nomes de pet não se repetem', () => {
    // O seed é idempotente POR NOME: dois pets com o mesmo nome fariam a
    // segunda execução pular um que nunca foi criado.
    const nomes = PET_CATALOG_SEED.map((pet) => pet.name);
    expect(new Set(nomes).size).toBe(nomes.length);
  });
});

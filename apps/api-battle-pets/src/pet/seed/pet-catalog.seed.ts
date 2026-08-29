// Copyright 2026 Anderson. All Rights Reserved.

import * as PetUseCase from '../pet.use-case';
import { createPetSchema, type CreatePetDeclaration } from '../pet.validation';

/**
 * Catálogo inicial de pets, com golpes.
 *
 * `code-standart.md` §5: seed NUNCA usa INSERT bruto — ele instancia e executa
 * os próprios use cases. Assim toda regra que o cadastro tem (validação,
 * transação, limite de quatro golpes, slot pela posição) vale aqui também, e o
 * seed não vira uma segunda porta para dado inválido.
 *
 * DESENHO DOS GOLPES, e por que cada pet tem os que tem:
 *
 * - Cada pet tem um golpe FRACO e confiável, e um FORTE. Sem essa diferença,
 *   escolher entre quatro seria escolher entre quatro nomes.
 * - O golpe que muda o terreno é sempre o mais caro em algum sentido — ou é o
 *   mais fraco, ou é o de nicho. Terreno de graça faria a escolha ser óbvia.
 * - Só pets de Água e Fogo mudam terreno, e cada um no seu sentido: água
 *   ALAGA (e alagar é o que torna submergir possível — o pet fabrica o terreno
 *   da própria skill), fogo QUEIMA (casa de dano). Planta camufla e não mexe
 *   no chão, porque quem se esconde continua pisando onde estava.
 */
export const PET_CATALOG_SEED: CreatePetDeclaration[] = [
  {
    name: 'Faísca',
    type: 'Fogo',
    attack: 55,
    defense: 40,
    speed: 60,
    maxHealth: 120,
    moves: [
      { name: 'Bote', power: 80, terrainEffect: 'none' },
      { name: 'Chama', power: 110, terrainEffect: 'none' },
      // Queima a casa: forte no terreno, fraco no dano. Quem escolhe isto
      // troca dano AGORA por um tabuleiro pior para o outro DEPOIS.
      { name: 'Brasa Viva', power: 60, terrainEffect: 'damage' },
      { name: 'Explosão', power: 150, terrainEffect: 'none', requiresAttribute: 'musculature', requiresValue: 12 },
    ],
  },
  {
    name: 'Cinza',
    type: 'Fogo',
    attack: 70,
    defense: 30,
    speed: 45,
    maxHealth: 100,
    moves: [
      { name: 'Arranhão', power: 75, terrainEffect: 'none' },
      { name: 'Sopro Quente', power: 105, terrainEffect: 'none' },
      { name: 'Rastro de Fogo', power: 55, terrainEffect: 'damage' },
      { name: 'Fúria', power: 160, terrainEffect: 'none', requiresAttribute: 'musculature', requiresValue: 15 },
    ],
  },
  {
    name: 'Brisa',
    type: 'Planta',
    attack: 45,
    defense: 60,
    speed: 50,
    maxHealth: 140,
    moves: [
      { name: 'Chicote', power: 85, terrainEffect: 'none' },
      { name: 'Espinhos', power: 100, terrainEffect: 'none' },
      { name: 'Raiz Presa', power: 70, terrainEffect: 'none' },
      { name: 'Tempestade Verde', power: 145, terrainEffect: 'none', requiresAttribute: 'camouflage', requiresValue: 8 },
    ],
  },
  {
    name: 'Musgo',
    type: 'Planta',
    attack: 40,
    defense: 75,
    speed: 35,
    maxHealth: 165,
    moves: [
      { name: 'Empurrão', power: 70, terrainEffect: 'none' },
      { name: 'Folha Cortante', power: 95, terrainEffect: 'none' },
      { name: 'Abraço', power: 115, terrainEffect: 'none' },
      { name: 'Colheita', power: 135, terrainEffect: 'none', requiresAttribute: 'personality', requiresValue: 6 },
    ],
  },
  {
    name: 'Maré',
    type: 'Agua',
    attack: 50,
    defense: 55,
    speed: 55,
    maxHealth: 130,
    moves: [
      { name: 'Jato', power: 85, terrainEffect: 'none' },
      // Alaga: é o golpe que FABRICA a condição de submergir. Fraco de
      // propósito — a recompensa dele é o turno seguinte, não este.
      { name: 'Maré Alta', power: 60, terrainEffect: 'water' },
      { name: 'Redemoinho', power: 110, terrainEffect: 'none' },
      { name: 'Tsunami', power: 155, terrainEffect: 'none', requiresAttribute: 'musculature', requiresValue: 14 },
    ],
  },
  {
    name: 'Corrente',
    type: 'Agua',
    attack: 65,
    defense: 45,
    speed: 70,
    maxHealth: 110,
    moves: [
      { name: 'Borrifo', power: 75, terrainEffect: 'none' },
      { name: 'Poça', power: 50, terrainEffect: 'water' },
      { name: 'Correnteza', power: 120, terrainEffect: 'none' },
      { name: 'Vaga', power: 140, terrainEffect: 'none', requiresAttribute: 'underground', requiresValue: 8 },
    ],
  },
];

export type SeedResult = {
  created: number;
  skipped: number;
};

/**
 * Cria os pets do catálogo que ainda não existem.
 *
 * IDEMPOTENTE por nome: rodar duas vezes não duplica. Seed que só funciona em
 * banco vazio é seed que ninguém roda depois da primeira semana — e a segunda
 * execução é o caso normal, não a exceção.
 */
export async function seedPetCatalog(): Promise<SeedResult> {
  const { items } = await PetUseCase.listPets({ page: 1, perPage: 200 });
  const existentes = new Set(items.map((pet) => pet.name));

  let created = 0;
  let skipped = 0;

  for (const definicao of PET_CATALOG_SEED) {
    if (existentes.has(definicao.name)) {
      skipped += 1;
      continue;
    }

    // A semente passa pelo MESMO schema que valida uma requisição HTTP.
    // Ela é dado escrito à mão, e dado escrito à mão erra — sem o parse, um
    // requisito com nome trocado entraria no banco e só apareceria como um
    // golpe que nunca tranca, meses depois, sem pista da causa.
    await PetUseCase.createPet(createPetSchema.parse(definicao));
    created += 1;
  }

  return { created, skipped };
}

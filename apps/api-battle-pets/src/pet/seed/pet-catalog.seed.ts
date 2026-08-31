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
 * - Cada elemento mexe no chão no SENTIDO dele: água ALAGA (e alagar é o que
 *   torna submergir possível — o pet fabrica o terreno da própria skill) e
 *   CONGELA; fogo QUEIMA (casa de dano); terra e água ENLAMEIAM, que é o
 *   terreno incerto. Planta camufla e não mexe no chão, porque quem se esconde
 *   continua pisando onde estava.
 * - O congelamento é medido em DURAÇÃO, e não num número abstrato de "nível"
 *   (DP-gelo-01): dois slots na Friagem, quatro na Nevasca. É a mesma escala.
 * - Todo pet de Água tem um golpe que ALAGA. Sem ele o pet só submerge onde a
 *   arena já tem rio, e a skill do elemento dele vira sorte de mapa.
 * - Nenhum golpe cria água RASA: a poça é o que o gelo deixa ao derreter, e
 *   ter uma origem só a mantém ligada ao gelo em vez de virar um alagamento
 *   mais fraco.
 */
export const PET_CATALOG_SEED: CreatePetDeclaration[] = [
  {
    name: 'Faísca',
    type: 'Natural/Fogo',
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
    type: 'Natural/Fogo',
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
    type: 'Natural/Planta',
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
    type: 'Natural/Planta',
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
    type: 'Natural/Agua',
    attack: 50,
    defense: 55,
    speed: 55,
    maxHealth: 130,
    moves: [
      { name: 'Jato', power: 85, terrainEffect: 'none' },
      // Alaga: é o golpe que FABRICA a condição de submergir. Fraco de
      // propósito — a recompensa dele é o turno seguinte, não este.
      { name: 'Maré Alta', power: 60, terrainEffect: 'water' },
      // Água sobre terra dá LAMA. Este é o outro lado do que a Maré Alta faz:
      // um alaga para o pet poder submergir, o outro encharca para o OUTRO
      // não conseguir sair do lugar.
      { name: 'Lodaçal', power: 55, terrainEffect: 'mud' },
      { name: 'Tsunami', power: 155, terrainEffect: 'none', requiresAttribute: 'musculature', requiresValue: 14 },
    ],
  },
  {
    name: 'Corrente',
    type: 'Natural/Agua',
    attack: 65,
    defense: 45,
    speed: 70,
    maxHealth: 110,
    moves: [
      { name: 'Borrifo', power: 75, terrainEffect: 'none' },
      // Este golpe era a "Poça", e virou água RASA quando a fundura passou a
      // existir. O nome ficou honesto e o pet ficou aleijado: sem um golpe
      // que aprofunda, a Corrente não fabricava mais o terreno da própria
      // skill, e só submergia onde a arena já tivesse rio. A poça continua
      // existindo — ela é o que o gelo deixa ao derreter.
      { name: 'Vazante', power: 60, terrainEffect: 'water' },
      // CONGELA a casa por dois slots: nega o terreno a quem contava com ele,
      // e devolve o que estava embaixo quando derrete. Poder baixo de
      // proposito — a recompensa e o que o OUTRO deixa de fazer.
      { name: 'Friagem', power: 45, terrainEffect: 'ice', terrainDuration: 2 },
      { name: 'Vaga', power: 140, terrainEffect: 'none', requiresAttribute: 'underground', requiresValue: 8 },
    ],
  },
  {
    name: 'Zunido',
    type: 'Fisica/Planta',
    attack: 60,
    defense: 35,
    speed: 80,
    maxHealth: 95,
    moves: [
      { name: 'Ferroada', power: 80, terrainEffect: 'none' },
      { name: 'Enxame', power: 110, terrainEffect: 'none' },
      // Deixa a casa hostil sem bater forte: a teia é armadilha, não golpe.
      { name: 'Teia Ácida', power: 55, terrainEffect: 'damage' },
      { name: 'Revoada', power: 145, terrainEffect: 'none', requiresAttribute: 'flight', requiresValue: 10 },
    ],
  },
  {
    name: 'Vigília',
    type: 'Psiquica/Agua',
    attack: 65,
    defense: 45,
    speed: 65,
    maxHealth: 105,
    moves: [
      { name: 'Pressentir', power: 75, terrainEffect: 'none' },
      // A ESCOLA PSÍQUICA muda a luta em vez de encerrá-la: poder baixo, e o
      // que ela entrega é o turno seguinte. É a razão de existir dela, e o
      // que substituiu o 150% contra todo tipo natural que o psíquico tinha.
      { name: 'Concentração', power: 35, terrainEffect: 'none', effectStat: 'attack', effectPercent: 40 },
      // A Vigília é de Água e NUNCA teve como alagar — buraco que passou
      // despercebido porque o teste que o pegaria filtrava por um nome de
      // tipo que não existe mais.
      { name: 'Marejar', power: 55, terrainEffect: 'water' },
      // Personalidade alta: quem hesita não sustenta o olhar.
      { name: 'Colapso', power: 150, terrainEffect: 'none', requiresAttribute: 'personality', requiresValue: 8 },
    ],
  },
  {
    name: 'Alfarrábio',
    type: 'Psiquica/Fogo',
    attack: 75,
    defense: 30,
    speed: 60,
    maxHealth: 90,
    moves: [
      { name: 'Fagulha Arcana', power: 85, terrainEffect: 'none' },
      // Derruba a DEFESA do outro: o golpe que prepara o Grimório.
      { name: 'Fissura Arcana', power: 30, terrainEffect: 'none', effectStat: 'defense', effectPercent: -45 },
      // O mago FABRICA o terreno de que precisa — a versão dele do que a Maré
      // faz com água, e o que dá sentido a ele não ter skill própria.
      { name: 'Poça Invocada', power: 50, terrainEffect: 'water' },
      { name: 'Grimório', power: 165, terrainEffect: 'none', requiresAttribute: 'musculature', requiresValue: 10 },
    ],
  },
  {
    name: 'Estalagmite',
    type: 'Fisica/Terra',
    attack: 55,
    defense: 75,
    speed: 35,
    maxHealth: 145,
    moves: [
      { name: 'Cabeçada', power: 70, terrainEffect: 'none' },
      { name: 'Desmoronar', power: 120, terrainEffect: 'none' },
      // Terra rachada vira atoleiro. Dá ao elemento Terra a coisa que ele não
      // tinha: até aqui ele era o único sem nada de seu — sem skill própria e
      // sem terreno próprio, o que o deixava genérico apesar do nome.
      { name: 'Barreira de Lama', power: 60, terrainEffect: 'mud' },
      { name: 'Abalo', power: 140, terrainEffect: 'none', requiresAttribute: 'underground', requiresValue: 8 },
    ],
  },
  {
    // Natural/Terra — não existia. Das doze combinações de escola e elemento,
    // cinco nunca tinham sido escritas, e a Terra tinha um pet só.
    name: 'Barro',
    type: 'Natural/Terra',
    attack: 50,
    defense: 70,
    speed: 40,
    maxHealth: 150,
    moves: [
      { name: 'Patada', power: 75, terrainEffect: 'none' },
      // O especialista em lama: fraco no dano, e o que ele entrega é o
      // tabuleiro. Quem pisa ali escorrega, atrasa, ou passa — e não sabe
      // qual antes de tentar.
      { name: 'Atoleiro', power: 55, terrainEffect: 'mud' },
      { name: 'Deslizamento', power: 110, terrainEffect: 'none' },
      { name: 'Terra Firme', power: 145, terrainEffect: 'none', requiresAttribute: 'underground', requiresValue: 10 },
    ],
  },
  {
    name: 'Fornalha',
    type: 'Fisica/Fogo',
    attack: 80,
    defense: 40,
    speed: 55,
    maxHealth: 110,
    moves: [
      { name: 'Marreta', power: 85, terrainEffect: 'none' },
      { name: 'Brasa Batida', power: 60, terrainEffect: 'damage' },
      { name: 'Golpe Rubro', power: 115, terrainEffect: 'none' },
      { name: 'Forja', power: 160, terrainEffect: 'none', requiresAttribute: 'musculature', requiresValue: 13 },
    ],
  },
  {
    name: 'Vagalhão',
    type: 'Fisica/Agua',
    attack: 70,
    defense: 50,
    speed: 60,
    maxHealth: 120,
    moves: [
      { name: 'Investida', power: 80, terrainEffect: 'none' },
      // Todo pet de Água precisa poder FABRICAR o terreno da própria skill,
      // senão submergir só funciona onde a arena já tem água.
      { name: 'Enseada', power: 60, terrainEffect: 'water' },
      // O congelamento mais LONGO do catálogo: quatro slots. Quem o usa
      // compra o turno inteiro do outro naquela casa.
      { name: 'Nevasca', power: 45, terrainEffect: 'ice', terrainDuration: 4 },
      { name: 'Arrebentação', power: 150, terrainEffect: 'none', requiresAttribute: 'musculature', requiresValue: 12 },
    ],
  },
  {
    name: 'Névoa',
    type: 'Psiquica/Planta',
    attack: 55,
    defense: 60,
    speed: 70,
    maxHealth: 115,
    moves: [
      { name: 'Sussurro', power: 70, terrainEffect: 'none' },
      { name: 'Torpor', power: 35, terrainEffect: 'none', effectStat: 'speed', effectPercent: -40 },
      { name: 'Viço', power: 40, terrainEffect: 'none', effectStat: 'defense', effectPercent: 45 },
      { name: 'Miragem', power: 150, terrainEffect: 'none', requiresAttribute: 'camouflage', requiresValue: 9 },
    ],
  },
  {
    name: 'Menir',
    type: 'Psiquica/Terra',
    attack: 60,
    defense: 65,
    speed: 45,
    maxHealth: 135,
    moves: [
      { name: 'Toque de Pedra', power: 75, terrainEffect: 'none' },
      { name: 'Lodo Mental', power: 50, terrainEffect: 'mud' },
      { name: 'Peso do Chão', power: 40, terrainEffect: 'none', effectStat: 'attack', effectPercent: -40 },
      { name: 'Monólito', power: 155, terrainEffect: 'none', requiresAttribute: 'personality', requiresValue: 9 },
    ],
  },
{
    // O arquétipo: drena, atravessa e some. É o pet que a escola espiritual
    // e o elemento fantasma existem para produzir.
    name: 'Véu',
    type: 'Espiritual/Fantasma',
    attack: 60,
    defense: 40,
    speed: 75,
    maxHealth: 95,
    moves: [
      { name: 'Toque Frio', power: 70, terrainEffect: 'none' },
      // Fraco no dano e forte na sobrevivência: quem escolhe isto troca o
      // turno de agora pela vida que sustenta os próximos.
      { name: 'Sanguessuga', power: 55, terrainEffect: 'none', drainPercent: 50 },
      { name: 'Lamento', power: 100, terrainEffect: 'none' },
      { name: 'Assombro', power: 150, terrainEffect: 'none', requiresAttribute: 'personality', requiresValue: 10 },
    ],
  },
  {
    // A RESPOSTA. Sem um pet de luz jogável, "luz é forte contra fantasma"
    // seria um número numa tabela que ninguém alcança.
    name: 'Candeia',
    type: 'Espiritual/Luz',
    attack: 55,
    defense: 60,
    speed: 60,
    maxHealth: 125,
    moves: [
      { name: 'Clarão', power: 75, terrainEffect: 'none' },
      { name: 'Fagulha Pura', power: 45, terrainEffect: 'none', effectStat: 'defense', effectPercent: 40 },
      { name: 'Raio Claro', power: 110, terrainEffect: 'none' },
      { name: 'Aurora', power: 155, terrainEffect: 'none', requiresAttribute: 'personality', requiresValue: 8 },
    ],
  },
  {
    name: 'Farol',
    type: 'Fisica/Luz',
    attack: 75,
    defense: 55,
    speed: 50,
    maxHealth: 130,
    moves: [
      { name: 'Cabeçada Luminosa', power: 85, terrainEffect: 'none' },
      { name: 'Brasa Branca', power: 60, terrainEffect: 'damage' },
      { name: 'Golpe Solar', power: 120, terrainEffect: 'none' },
      { name: 'Meio-dia', power: 160, terrainEffect: 'none', requiresAttribute: 'musculature', requiresValue: 12 },
    ],
  },
  {
    name: 'Lume',
    type: 'Psiquica/Luz',
    attack: 60,
    defense: 50,
    speed: 70,
    maxHealth: 110,
    moves: [
      { name: 'Vislumbre', power: 70, terrainEffect: 'none' },
      { name: 'Foco', power: 35, terrainEffect: 'none', effectStat: 'attack', effectPercent: 40 },
      { name: 'Ofuscar', power: 40, terrainEffect: 'none', effectStat: 'speed', effectPercent: -40 },
      { name: 'Revelação', power: 150, terrainEffect: 'none', requiresAttribute: 'personality', requiresValue: 9 },
    ],
  },
  {
    name: 'Bruma',
    type: 'Natural/Fantasma',
    attack: 55,
    defense: 50,
    speed: 65,
    maxHealth: 115,
    moves: [
      { name: 'Sopro Gelado', power: 70, terrainEffect: 'none' },
      { name: 'Névoa Densa', power: 50, terrainEffect: 'ice', terrainDuration: 2 },
      { name: 'Frio na Espinha', power: 60, terrainEffect: 'none', drainPercent: 40 },
      { name: 'Vendaval Pálido', power: 145, terrainEffect: 'none', requiresAttribute: 'camouflage', requiresValue: 9 },
    ],
  },
  {
    name: 'Afogado',
    type: 'Espiritual/Agua',
    attack: 65,
    defense: 45,
    speed: 55,
    maxHealth: 120,
    moves: [
      { name: 'Puxão', power: 80, terrainEffect: 'none' },
      { name: 'Ressurgência', power: 60, terrainEffect: 'water' },
      { name: 'Sede Antiga', power: 55, terrainEffect: 'none', drainPercent: 45 },
      { name: 'Maré Negra', power: 150, terrainEffect: 'none', requiresAttribute: 'underground', requiresValue: 10 },
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

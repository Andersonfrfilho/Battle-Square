// Copyright 2026 Anderson. All Rights Reserved.

/**
 * O GERADOR DE ESPECIES — de MODELO para PET (decisao 69).
 *
 * O problema real: chegam ~50+ modelos de monstro de um pacote, e o catalogo ja
 * tem 115 pets. Casar isso a mao e digitar 165 linhas e errar em silencio. Este
 * modulo faz a ponte, e ele so precisa acertar UMA coisa: o ELEMENTO do modelo.
 *
 * Por que so o elemento — e por que isto NAO duplica nada:
 *   - `BiomeEncounterFilter` (C++) ja liga BIOMA -> ELEMENTO;
 *   - o catalogo ja identifica o pet por `Escola/Elemento`;
 *   - logo BIOMA -> PET ja e automatico. Escrever a tabela de bioma aqui seria a
 *     segunda fonte de verdade que L-032 cobra. Este modulo nao conhece bioma.
 *
 * O estagio (Filhote/Adulto/Evoluido) espelha `EPetGrowthStage` do C++, que ja
 * existe — a evolucao nao nasce aqui, ela e RECONHECIDA aqui.
 */

/** Os oito elementos do jogo (Config/PetTypes.json). */
export const ELEMENTS = [
  'Fogo', 'Agua', 'Planta', 'Terra', 'Fantasma', 'Luz', 'Ar', 'Raio',
] as const;
export type Element = (typeof ELEMENTS)[number];

/** As tres fases de EPetGrowthStage (C++), na ordem da evolucao. */
export const STAGES = ['Filhote', 'Adulto', 'Evoluido'] as const;
export type Stage = (typeof STAGES)[number];

/**
 * Pistas de NOME por elemento. O nome do modelo e o unico dado que um pacote da
 * de graca, e ele quase sempre diz o elemento ("FlameImp", "IceGolem").
 * Minusculas, comparadas por conteudo.
 */
const ELEMENT_HINTS: ReadonlyArray<readonly [Element, readonly string[]]> = [
  ['Fogo',     ['flame', 'fire', 'lava', 'magma', 'ember', 'burn', 'inferno', 'demon', 'imp']],
  ['Agua',     ['water', 'aqua', 'fish', 'shark', 'squid', 'crab', 'wave', 'sea', 'octo', 'frog']],
  ['Planta',   ['plant', 'leaf', 'vine', 'flower', 'mush', 'fung', 'tree', 'forest', 'ent', 'moss']],
  ['Terra',    ['rock', 'stone', 'golem', 'earth', 'sand', 'crystal', 'dino', 'raptor', 'rex']],
  ['Fantasma', ['ghost', 'spirit', 'skele', 'undead', 'zombie', 'wraith', 'phantom', 'bone', 'reaper']],
  ['Luz',      ['light', 'holy', 'angel', 'radiant', 'sun', 'star', 'divine', 'cleric']],
  ['Ar',       ['air', 'wind', 'bird', 'wing', 'sky', 'cloud', 'bat', 'harpy', 'gryph']],
  ['Raio',     ['thunder', 'lightning', 'shock', 'spark', 'volt', 'storm', 'electric']],
];

/** Pistas de NOME por estagio — o pacote costuma marcar o porte no nome. */
const STAGE_HINTS: ReadonlyArray<readonly [Stage, readonly string[]]> = [
  ['Filhote',  ['baby', 'small', 'mini', 'cute', 'little', 'young', 'tiny', 'cub']],
  ['Evoluido', ['king', 'giant', 'alpha', 'elder', 'greater', 'lord', 'ancient', 'boss', 'large']],
];

function normalize(modelName: string): string {
  // "SK_FlameImp_01" -> "flameimp01": tira prefixo de asset, separador e caixa,
  // para a pista casar tanto em "Flame_Imp" quanto em "FlameImp".
  return modelName.toLowerCase().replace(/[^a-z0-9]/g, '');
}

/**
 * O elemento que o nome do modelo sugere, ou `undefined` quando nao ha pista.
 *
 * `undefined` e resposta VALIDA e importante: modelo sem pista NAO recebe um
 * elemento chutado — ele fica para a decisao humana (o gerador reporta a lista).
 * Chutar encheria o catalogo de erro silencioso.
 */
export function elementFromModelName(modelName: string): Element | undefined {
  const n = normalize(modelName);
  for (const [element, hints] of ELEMENT_HINTS) {
    if (hints.some((h) => n.includes(h))) return element;
  }
  return undefined;
}

/** O estagio que o nome sugere. Sem pista, `Adulto` — o meio, nunca um extremo. */
export function stageFromModelName(modelName: string): Stage {
  const n = normalize(modelName);
  for (const [stage, hints] of STAGE_HINTS) {
    if (hints.some((h) => n.includes(h))) return stage;
  }
  return 'Adulto';
}

export type ClassifiedModel = {
  readonly modelName: string;
  readonly element: Element | undefined;
  readonly stage: Stage;
};

/** Classifica uma leva de modelos (o que a MCP AssetTools devolve ao listar). */
export function classifyModels(modelNames: readonly string[]): ClassifiedModel[] {
  return modelNames.map((modelName) => ({
    modelName,
    element: elementFromModelName(modelName),
    stage: stageFromModelName(modelName),
  }));
}

/** Os que o gerador NAO soube classificar — a lista que pede olho humano. */
export function unclassified(models: readonly ClassifiedModel[]): ClassifiedModel[] {
  return models.filter((m) => m.element === undefined);
}

export type EvolutionChain = {
  readonly element: Element;
  /** Um modelo por estagio, na ordem Filhote -> Adulto -> Evoluido. */
  readonly stages: Partial<Record<Stage, string>>;
};

/**
 * Monta as cadeias de evolucao: por ELEMENTO, um modelo de cada estagio.
 *
 * Uma cadeia so entra se tiver ao menos DOIS estagios — um modelo sozinho nao e
 * evolucao, e prometer evolucao que nao acontece e pior que nao prometer.
 * Modelos sem elemento ficam de fora (ver `unclassified`).
 */
export function buildEvolutionChains(models: readonly ClassifiedModel[]): EvolutionChain[] {
  const byElement = new Map<Element, Partial<Record<Stage, string>>>();

  for (const m of models) {
    if (!m.element) continue;
    const chain = byElement.get(m.element) ?? {};
    // O primeiro de cada estagio fica: determinismo por ordem de entrada, sem
    // sorteio — a mesma lista de modelos da sempre a mesma cadeia.
    if (!chain[m.stage]) chain[m.stage] = m.modelName;
    byElement.set(m.element, chain);
  }

  return [...byElement.entries()]
    .map(([element, stages]) => ({ element, stages }))
    .filter((c) => Object.keys(c.stages).length >= 2);
}

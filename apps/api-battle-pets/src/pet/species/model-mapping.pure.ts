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
  ['Fogo',     ['flame', 'fire', 'lava', 'magma', 'ember', 'burn', 'inferno', 'demon', 'imp',
                'dragon']],
  ['Agua',     ['water', 'aqua', 'fish', 'shark', 'squid', 'squidle', 'crab', 'wave', 'sea',
                'octo', 'frog', 'glub', 'penguin', 'turtle']],
  ['Planta',   ['plant', 'leaf', 'vine', 'flower', 'mush', 'fung', 'tree', 'forest', 'ent',
                'moss', 'cact', 'bee', 'deer']],
  ['Terra',    ['rock', 'stone', 'golem', 'gole', 'earth', 'sand', 'crystal', 'dino', 'raptor',
                'rex', 'orc', 'cyclops', 'yeti', 'panda', 'pig', 'bunny', 'monk']],
  ['Fantasma', ['ghost', 'spirit', 'skele', 'skull', 'undead', 'zombie', 'wraith', 'phantom',
                'bone', 'reaper', 'cthulhu']],
  ['Luz',      ['light', 'holy', 'angel', 'radiant', 'sun', 'star', 'divine', 'cleric',
                'wizard']],
  ['Ar',       ['air', 'wind', 'bird', 'birb', 'wing', 'sky', 'cloud', 'bat', 'harpy',
                'gryph', 'pigeon', 'chicken', 'bee']],
  ['Raio',     ['thunder', 'lightning', 'shock', 'spark', 'volt', 'storm', 'electric',
                'hywirl']],
];

/** Pistas de NOME por estagio — o pacote costuma marcar o porte no nome. */
const STAGE_HINTS: ReadonlyArray<readonly [Stage, readonly string[]]> = [
  ['Filhote',  ['baby', 'small', 'mini', 'little', 'young', 'tiny', 'cub']],
  ['Evoluido', ['king', 'giant', 'alpha', 'elder', 'greater', 'lord', 'ancient', 'boss', 'large']],
];

/**
 * O sufixo que o pacote usa para marcar a forma evoluida (medido no Ultimate
 * Monsters: Alpaking/Alpaking_Evolved, Dragon/Dragon_Evolved, ...). Isto NAO e
 * heuristica de nome — e convencao explicita do autor, e por isso ganha da
 * heuristica.
 */
const EVOLVED_SUFFIX = /_evolved$/i;

/**
 * A FAMILIA de um modelo: o nome sem o sufixo de forma. `Alpaking_Evolved` e
 * `Alpaking` sao a MESMA familia — e e por familia que a evolucao se agrupa,
 * nunca por elemento (agrupar por elemento fundiria Dragon e Demon na mesma
 * cadeia, que sao bichos diferentes).
 */
export function familyOf(modelName: string): string {
  return modelName.replace(EVOLVED_SUFFIX, '').replace(/[_-]+$/, '');
}

/**
 * Quebra o nome em TOKENS: "SK_FlameImp_01" -> ["sk","flame","imp","01"].
 *
 * Por token, e nao por substring, porque substring produz falso positivo caro:
 * "Alpaking" contem "king" e viraria forma evoluida por acidente; "Cactoro"
 * contem "cact" de verdade, e esse deve valer. Token separa os dois casos.
 */
function tokensOf(modelName: string): string[] {
  return modelName
    .replace(/([a-z0-9])([A-Z])/g, '$1 $2') // camelCase vira duas palavras
    .toLowerCase()
    .split(/[^a-z0-9]+/)
    .filter(Boolean);
}

/** A pista casa quando um TOKEN comeca por ela (cobre plural e sufixo curto). */
function matchesHint(tokens: readonly string[], hint: string): boolean {
  return tokens.some((t) => t === hint || t.startsWith(hint));
}

/**
 * O elemento que o nome do modelo sugere, ou `undefined` quando nao ha pista.
 *
 * `undefined` e resposta VALIDA e importante: modelo sem pista NAO recebe um
 * elemento chutado — ele fica para a decisao humana (o gerador reporta a lista).
 * Chutar encheria o catalogo de erro silencioso.
 */
export function elementFromModelName(modelName: string): Element | undefined {
  // A forma evoluida herda o elemento da familia: `Dragon_Evolved` e Fogo
  // porque `Dragon` e Fogo — o sufixo nao muda o que o bicho e.
  const tokens = tokensOf(familyOf(modelName));
  for (const [element, hints] of ELEMENT_HINTS) {
    if (hints.some((h) => matchesHint(tokens, h))) return element;
  }
  return undefined;
}

/** O estagio que o nome sugere. Sem pista, `Adulto` — o meio, nunca um extremo. */
export function stageFromModelName(modelName: string): Stage {
  // A convencao do autor manda: o sufixo e explicito, a heuristica e palpite.
  if (EVOLVED_SUFFIX.test(modelName)) return 'Evoluido';
  const tokens = tokensOf(modelName);
  for (const [stage, hints] of STAGE_HINTS) {
    if (hints.some((h) => matchesHint(tokens, h))) return stage;
  }
  return 'Adulto';
}

/**
 * Tokens que denunciam que o arquivo NAO e criatura: rig, prop, cenario.
 *
 * Medido: o Cute Fish Pack traz `Lure_1..6`, `Dock_Long`, `Boat`; os packs de
 * personagem trazem `Rig_Medium_General`. Reportar isso como "sem pista, decida
 * o elemento" seria pedir ao humano que classificasse uma DOCA como bicho — o
 * relatorio precisa ser curto para ser lido, e ruido o mata.
 */
const NON_CREATURE_TOKENS = [
  'rig', 'lure', 'dock', 'boat', 'prop', 'anim', 'skeleton_rig', 'socket',
  // Medidos no import de 04/09, que escaparam da primeira lista: o Cute Fish
  // traz `FishingRod_Lvl1..5` (vara de pesca), e barco/doca ja estavam. Cada
  // token aqui veio de um arquivo REAL que quase entrou como bicho.
  'rod', 'fishingrod', 'crate', 'barrel', 'chest',
] as const;

/** Este arquivo e uma CRIATURA, ou e rig/prop/cenario que entrou junto? */
export function isCreature(modelName: string): boolean {
  const tokens = tokensOf(modelName);
  // `Skeletons` (o pack) e criatura; `Rig_Medium` nao. Token exato, nao prefixo:
  // "rig" pega `Rig_Medium`, e nao pega `Frigate`.
  return !NON_CREATURE_TOKENS.some((bad) => tokens.includes(bad));
}

export type ClassifiedModel = {
  readonly modelName: string;
  readonly element: Element | undefined;
  readonly stage: Stage;
};

/**
 * Classifica uma leva de modelos (o que a MCP AssetTools devolve ao listar).
 *
 * `packElement` e a pista do PACOTE, e ela existe por medicao: o Cute Fish Pack
 * traz `Tetra`, `Betta`, `Koi`, `Piranha` — nomes de ESPECIE, que o nome sozinho
 * nao denuncia, mas cujo pacote nao deixa duvida. Ela e o ULTIMO recurso: o nome
 * ganha sempre, para um monstro de fogo dentro de um pacote de peixe continuar
 * sendo de fogo.
 */
export function classifyModels(
  modelNames: readonly string[],
  packElement?: Element,
): ClassifiedModel[] {
  // Rig e prop saem ANTES de classificar: eles nao sao bicho, e pedir elemento
  // para uma doca e ruido no relatorio que o humano vai ler.
  return modelNames.filter(isCreature).map((modelName) => ({
    modelName,
    // O nome primeiro; o pacote so preenche o que o nome nao soube dizer.
    element: elementFromModelName(modelName) ?? packElement,
    stage: stageFromModelName(modelName),
  }));
}

/** Os que o gerador NAO soube classificar — a lista que pede olho humano. */
export function unclassified(models: readonly ClassifiedModel[]): ClassifiedModel[] {
  return models.filter((m) => m.element === undefined);
}

export type EvolutionChain = {
  /** A familia: `Alpaking` para `Alpaking` + `Alpaking_Evolved`. */
  readonly family: string;
  readonly element: Element | undefined;
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
  // Por FAMILIA (o nome-base), nao por elemento: `Dragon` e `Demon` sao ambos
  // Fogo e NAO evoluem um no outro. Agrupar por elemento inventaria parentesco.
  const byFamily = new Map<string, { element: Element | undefined; stages: Partial<Record<Stage, string>> }>();

  for (const m of models) {
    const family = familyOf(m.modelName);
    const entry = byFamily.get(family) ?? { element: m.element, stages: {} };
    // O primeiro de cada estagio fica: determinismo por ordem de entrada, sem
    // sorteio — a mesma lista de modelos da sempre a mesma cadeia.
    if (!entry.stages[m.stage]) entry.stages[m.stage] = m.modelName;
    entry.element = entry.element ?? m.element;
    byFamily.set(family, entry);
  }

  return [...byFamily.entries()]
    .map(([family, e]) => ({ family, element: e.element, stages: e.stages }))
    .filter((c) => Object.keys(c.stages).length >= 2);
}

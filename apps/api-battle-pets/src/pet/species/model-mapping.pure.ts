// Copyright 2026 Anderson. All Rights Reserved.

/**
 * O GERADOR DE ESPECIES — de MODELO para PET (decisao 69).
 *
 * O problema real: chegam 128 modelos de tres pacotes, e o catalogo tem pets
 * identificados por `Escola/Elemento`. Casar isso a mao e errar em silencio.
 * Este modulo faz a ponte, e ele so precisa acertar UMA coisa: o ELEMENTO.
 *
 * Por que so o elemento — e por que isto NAO duplica nada:
 *   - `BiomeEncounterFilter` (C++) ja liga BIOMA -> ELEMENTO;
 *   - o catalogo ja identifica o pet por `Escola/Elemento`;
 *   - logo BIOMA -> PET ja e automatico. Escrever a tabela de bioma aqui seria a
 *     segunda fonte de verdade que L-032 cobra. Este modulo nao conhece bioma.
 *
 * O estagio (Filhote/Adulto/Evoluido) espelha `EPetGrowthStage` do C++, que ja
 * existe — a evolucao nao nasce aqui, ela e RECONHECIDA aqui.
 * As pistas vivem em `model-hints.constant.ts`.
 */
import {
  BODY_MARKERS, BODY_STAGE_BIG, BODY_STAGE_BLOB, ELEMENT_HINTS, EVOLVED_SUFFIX,
  MODEL_PREFIX, NON_CREATURE_TOKENS, PACK_MARKERS, SHORT_HINT_LENGTH, STAGE_HINTS,
} from './model-hints.constant';

/**
 * Os elementos do jogo. Os oito primeiros existem em `Config/PetTypes.json`;
 * `Comum` foi pedido pelo usuario (04/09) para o bicho sem elemento, e o
 * gerador AVISA enquanto o motor nao o conhecer — quem confere e o teste, nao
 * a memoria de quem le esta lista.
 */
export const ELEMENTS = [
  'Fogo', 'Agua', 'Planta', 'Terra', 'Fantasma', 'Luz', 'Ar', 'Raio', 'Comum',
] as const;
export type Element = (typeof ELEMENTS)[number];

/** As tres fases de EPetGrowthStage (C++), na ordem da evolucao. */
export const STAGES = ['Filhote', 'Adulto', 'Evoluido'] as const;
export type Stage = (typeof STAGES)[number];

const TRAILING_MARKER = new RegExp(
  `_(${[...BODY_MARKERS, ...PACK_MARKERS, 'Evolved'].join('|')})$`, 'i',
);
const CAMEL_STAGE_WORD = new RegExp(
  `(?<=[a-z])(${STAGE_HINTS.flatMap(([, w]) => w).map(capitalize).join('|')})$`,
);

function capitalize(word: string): string {
  return word.charAt(0).toUpperCase() + word.slice(1);
}

/**
 * A FAMILIA de um modelo: o nome sem prefixo de import e sem sufixo de forma.
 * `Alpaking_Evolved`, `Cactoro_Blob`, `Bat_CM` e `MushroomKing` pertencem a
 * `Alpaking`, `Cactoro`, `Bat` e `Mushroom`. E por familia que a evolucao se
 * agrupa, nunca por elemento (agrupar por elemento fundiria Dragon e Demon).
 *
 * A palavra de porte so sai em camelCase depois de minuscula: `MushroomKing`
 * perde `King`; `Alpaking` nao — medido, e o teste tem o nome real.
 */
export function familyOf(modelName: string): string {
  let family = modelName.replace(MODEL_PREFIX, '');
  let previous = '';
  while (previous !== family) {
    previous = family;
    family = family.replace(TRAILING_MARKER, '').replace(/[_-]+$/, '');
  }
  return family.replace(CAMEL_STAGE_WORD, '');
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

/** Pista longa casa por prefixo (plural, sufixo); pista curta so inteira. */
function matchesHint(tokens: readonly string[], hint: string): boolean {
  if (hint.length <= SHORT_HINT_LENGTH) return tokens.includes(hint);
  return tokens.some((t) => t === hint || t.startsWith(hint));
}

/** Todos os elementos que o nome sugere — mais de um e CONFLITO, nao escolha. */
export function elementsSuggestedBy(modelName: string): Element[] {
  const tokens = tokensOf(familyOf(modelName));
  return ELEMENT_HINTS
    .filter(([, hints]) => hints.some((h) => matchesHint(tokens, h)))
    .map(([element]) => element);
}

/**
 * O elemento que o nome do modelo sugere, ou `undefined` quando nao ha pista —
 * ou quando ha DUAS ("Bee" e planta e ar ao mesmo tempo). Nos dois casos a
 * resposta e do humano: chutar encheria o catalogo de erro silencioso.
 */
export function elementFromModelName(modelName: string): Element | undefined {
  const suggested = elementsSuggestedBy(modelName);
  return suggested.length === 1 ? suggested[0] : undefined;
}

/** De onde veio o estagio: do AUTOR (sufixo/palavra), do PORTE do corpo, ou padrao. */
export type StageOrigin = 'autor' | 'porte' | 'padrao';

export function stageOriginOf(modelName: string): StageOrigin {
  if (EVOLVED_SUFFIX.test(modelName)) return 'autor';
  const tokens = tokensOf(modelName);
  if (STAGE_HINTS.some(([, hints]) => hints.some((h) => matchesHint(tokens, h)))) return 'autor';
  if (BODY_STAGE_BLOB.test(modelName) || BODY_STAGE_BIG.test(modelName)) return 'porte';
  return 'padrao';
}

/** O estagio que o nome sugere. Sem pista, `Adulto` — o meio, nunca um extremo. */
export function stageFromModelName(modelName: string): Stage {
  if (EVOLVED_SUFFIX.test(modelName)) return 'Evoluido';
  const tokens = tokensOf(modelName);
  for (const [stage, hints] of STAGE_HINTS) {
    if (hints.some((h) => matchesHint(tokens, h))) return stage;
  }
  if (BODY_STAGE_BLOB.test(modelName)) return 'Filhote';
  if (BODY_STAGE_BIG.test(modelName)) return 'Evoluido';
  return 'Adulto';
}

/** Este arquivo e uma CRIATURA, ou e rig/prop/cenario que entrou junto? */
export function isCreature(modelName: string): boolean {
  const tokens = tokensOf(modelName);
  return !NON_CREATURE_TOKENS.some((bad) => tokens.includes(bad));
}

export type ClassifiedModel = {
  readonly modelName: string;
  readonly element: Element | undefined;
  readonly stage: Stage;
};

/**
 * Classifica uma leva de NOMES. `packElement` e a pista do PACOTE e preenche
 * so o que o nome nao disse (`Tetra`, `Betta`, `Koi` sao especies). Para o
 * import real, com pasta e caminho, ver `imported-assets.pure.ts`.
 */
export function classifyModels(
  modelNames: readonly string[],
  packElement?: Element,
): ClassifiedModel[] {
  return modelNames.filter(isCreature).map((modelName) => ({
    modelName,
    element: elementFromModelName(modelName) ?? packElement,
    stage: stageFromModelName(modelName),
  }));
}

/** Os que o gerador NAO soube classificar — a lista que pede olho humano. */
export function unclassified(models: readonly ClassifiedModel[]): ClassifiedModel[] {
  return models.filter((m) => m.element === undefined);
}

export type FamilyGroup = {
  /** A familia: `Alpaking` para `Alpaking` + `Alpaking_Evolved`. */
  readonly family: string;
  readonly element: Element | undefined;
  /** Um modelo por estagio, na ordem Filhote -> Adulto -> Evoluido. */
  readonly stages: Partial<Record<Stage, string>>;
  /** Outra pele do MESMO estagio (`Bat_CM` ao lado de `Bat_AM`): nao e evolucao. */
  readonly variantes: readonly string[];
};
export type EvolutionChain = FamilyGroup;

/** Agrupa por FAMILIA, um modelo por estagio; o que sobra no estagio e variante. */
export function groupByFamily(models: readonly ClassifiedModel[]): FamilyGroup[] {
  const byFamily = new Map<string, { element: Element | undefined; stages: Partial<Record<Stage, string>>; variantes: string[] }>();

  for (const m of models) {
    const family = familyOf(m.modelName);
    const entry = byFamily.get(family) ?? { element: m.element, stages: {}, variantes: [] };
    if (entry.stages[m.stage]) entry.variantes.push(m.modelName);
    else entry.stages[m.stage] = m.modelName;
    entry.element = entry.element ?? m.element;
    byFamily.set(family, entry);
  }

  return [...byFamily.entries()].map(([family, e]) => ({ family, ...e }));
}

/**
 * As cadeias de evolucao: familias com ao menos DOIS estagios. Um modelo
 * sozinho nao e evolucao, e prometer evolucao que nao acontece e pior que nao
 * prometer. Modelos sem elemento entram na cadeia mas ficam para decisao.
 */
export function buildEvolutionChains(models: readonly ClassifiedModel[]): EvolutionChain[] {
  return groupByFamily(models).filter((c) => Object.keys(c.stages).length >= 2);
}

// Copyright 2026 Anderson. All Rights Reserved.

/**
 * AR4c — valida a curadoria contra o insumo e o catalogo, e devolve o
 * casamento pronto para virar dado (AR4d).
 *
 * Reprova TUDO de uma vez, nao o primeiro erro: quem corrige uma lista de 23
 * quer ver os 23 problemas numa rodada.
 */
import type { CreatePetDeclaration } from '../pet.validation';
import type { AssetFamily, ChainOrigin } from './evolution-chains.pure';
import type { PetModelAssignment } from './catalog-assignment.constant';
import { ELEMENTS, type Element, type Stage } from './model-mapping.pure';

export type PetModelMatch = {
  readonly pet: string;
  readonly escola: string;
  readonly elemento: Element;
  readonly familia: string;
  readonly origem: ChainOrigin;
  readonly estagios: Partial<Record<Stage, string>>;
  readonly variantes: readonly string[];
  readonly motivo: string;
};

export type PetWithoutModel = { readonly pet: string; readonly elemento: Element; readonly motivo: string };

export type CatalogMatch = {
  readonly atribuicoes: readonly PetModelMatch[];
  readonly semModelo: readonly PetWithoutModel[];
  /** Familias COM elemento que nenhum pet veste, por elemento: o estoque. */
  readonly livres: Readonly<Record<Element, readonly string[]>>;
};

export class CatalogMatchError extends Error {
  constructor(readonly problemas: readonly string[]) {
    super(`curadoria invalida:\n- ${problemas.join('\n- ')}`);
  }
}

export type MatchCatalogParams = {
  readonly seed: readonly CreatePetDeclaration[];
  readonly assignments: readonly PetModelAssignment[];
  readonly families: readonly AssetFamily[];
};

function isElement(value: string | undefined): value is Element {
  return ELEMENTS.some((e) => e === value);
}

function splitType(pet: CreatePetDeclaration): { escola: string; elemento: Element } {
  const [escola = '', elemento] = pet.type.split('/');
  if (!isElement(elemento)) throw new CatalogMatchError([`${pet.name}: tipo "${pet.type}" sem elemento conhecido`]);
  return { escola, elemento };
}

function findProblems(params: MatchCatalogParams, familyByName: ReadonlyMap<string, AssetFamily>): string[] {
  const problems: string[] = [];
  const seedNames = new Set(params.seed.map((p) => p.name));
  const used = new Map<string, string>();

  for (const name of seedNames) {
    const count = params.assignments.filter((a) => a.pet === name).length;
    if (count !== 1) problems.push(`${name}: ${count} atribuicoes (precisa de exatamente 1)`);
  }
  for (const a of params.assignments) {
    if (!seedNames.has(a.pet)) problems.push(`${a.pet}: nao existe em PET_CATALOG_SEED`);
    if (!a.familia) continue;
    const family = familyByName.get(a.familia);
    if (!family) { problems.push(`${a.pet}: familia ${a.familia} nao esta no insumo`); continue; }
    const owner = used.get(a.familia);
    if (owner) problems.push(`${a.pet}: familia ${a.familia} ja veste ${owner}`);
    used.set(a.familia, a.pet);
    if (a.adulto && !memberPaths(family).some((p) => p.endsWith(`/SK_${a.adulto}`))) {
      problems.push(`${a.pet}: ${a.adulto} nao pertence a familia ${a.familia}`);
    }
  }
  return problems;
}

function memberPaths(family: AssetFamily): string[] {
  return [...Object.values(family.estagios), ...family.variantes];
}

function toMatch(pet: CreatePetDeclaration, a: PetModelAssignment, family: AssetFamily): PetModelMatch | string {
  const { escola, elemento } = splitType(pet);
  if (family.element !== elemento) {
    return `${pet.name} (${elemento}): familia ${family.family} e ${family.element ?? 'sem elemento'}`;
  }
  const pinned = a.adulto ? memberPaths(family).find((p) => p.endsWith(`/SK_${a.adulto}`)) : undefined;
  const estagios = pinned ? { ...family.estagios, Adulto: pinned } : family.estagios;
  if (!estagios.Adulto) return `${pet.name}: familia ${family.family} nao tem forma Adulto (o pet nasce Adulto)`;
  const variantes = memberPaths(family).filter((p) => !Object.values(estagios).includes(p));
  return { pet: pet.name, escola, elemento, familia: family.family, origem: family.origem, estagios, variantes, motivo: a.motivo };
}

export function matchCatalog(params: MatchCatalogParams): CatalogMatch {
  const familyByName = new Map(params.families.map((f) => [f.family, f]));
  const problems = findProblems(params, familyByName);
  if (problems.length > 0) throw new CatalogMatchError(problems);

  const atribuicoes: PetModelMatch[] = [];
  const semModelo: PetWithoutModel[] = [];
  for (const pet of params.seed) {
    const a = params.assignments.find((x) => x.pet === pet.name);
    const family = a?.familia ? familyByName.get(a.familia) : undefined;
    if (!a || !family) { semModelo.push({ pet: pet.name, elemento: splitType(pet).elemento, motivo: a?.motivo ?? '' }); continue; }
    const match = toMatch(pet, a, family);
    if (typeof match === 'string') problems.push(match);
    else atribuicoes.push(match);
  }
  if (problems.length > 0) throw new CatalogMatchError(problems);

  const usedFamilies = new Set(atribuicoes.map((m) => m.familia));
  const livres = {} as Record<Element, readonly string[]>;
  for (const e of ELEMENTS) {
    livres[e] = params.families.filter((f) => f.element === e && !usedFamilies.has(f.family)).map((f) => f.family);
  }
  return { atribuicoes, semModelo, livres };
}

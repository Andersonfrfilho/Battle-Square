// Copyright 2026 Anderson. All Rights Reserved.

/**
 * AR4e — o que so o USUARIO decide, derivado do dado, com o porque.
 *
 * Nada aqui e opiniao do gerador: cada item nasce de uma medicao (cadeia sem
 * elemento, conflito de pista, elemento sem pack, cadeia sem Adulto, elemento
 * que o MOTOR nao conhece) e diz o que a resposta DESBLOQUEIA. Responder e barato; chutar no lugar do usuario
 * encheria o catalogo de erro silencioso (invariante 1).
 */
import { PET_CATALOG_SEED } from '../seed/pet-catalog.seed';
import { SUGGESTED_ELEMENT_OPTIONS } from './catalog-assignment.constant';
import type { CatalogMatch } from './catalog-matching.pure';
import type { AssetFamily } from './evolution-chains.pure';
import type { ClassificationReport } from './imported-assets.pure';
import { ELEMENTS, type Element } from './model-mapping.pure';

export type UserDecision = {
  readonly id: string;
  readonly pergunta: string;
  readonly porque: string;
  readonly desbloqueia: string;
};

export type BuildUserDecisionsParams = {
  readonly classificacao: ClassificationReport;
  readonly cadeias: readonly AssetFamily[];
  readonly match: CatalogMatch;
  /** Os nomes de `Config/PetTypes.json` — lidos do arquivo, nunca lembrados. */
  readonly elementosDoMotor: readonly string[];
};

function stageNames(family: AssetFamily): string {
  return Object.values(family.estagios).map((p) => p.split('SK_')[1] ?? p).join(', ');
}

function chainsWithoutElement(cadeias: readonly AssetFamily[]): UserDecision[] {
  return cadeias.filter((c) => c.element === undefined).map((c) => ({
    id: `elemento-${c.family}`,
    pergunta: `${c.family} (${stageNames(c)}): ${SUGGESTED_ELEMENT_OPTIONS[c.family] ?? 'que elemento'}?`,
    porque: 'e uma cadeia de evolucao inteira sem pista de elemento no nome; o gerador nao chuta',
    desbloqueia: `uma cadeia de ${Object.keys(c.estagios).length} estagios para um pet desse elemento`,
  }));
}

function conflicts(classificacao: ClassificationReport): UserDecision[] {
  return classificacao.conflitos.map((c) => ({
    id: `conflito-${c.modelName}`,
    pergunta: `${c.modelName}: ${c.elementos.join(' ou ')}?`,
    porque: 'o nome sugere dois elementos, e o gerador so aceita um',
    desbloqueia: 'o pet que hoje esta sem modelo por causa dele (ver semModelo)',
  }));
}

function singlesWithoutHint(classificacao: ClassificationReport, cadeias: readonly AssetFamily[]): UserDecision[] {
  const inChains = new Set(cadeias.flatMap((c) => [...Object.values(c.estagios), ...c.variantes].map((p) => p.split('SK_')[1])));
  const conflicted = new Set(classificacao.conflitos.map((c) => c.modelName));
  const soltos = classificacao.semElemento.filter((m) => !inChains.has(m) && !conflicted.has(m));
  if (soltos.length === 0) return [];
  return [{
    id: 'sem-pista-soltos',
    pergunta: `${soltos.join(', ')}: que elemento tem cada um — ou ficam de fora?`,
    porque: 'modelos unicos sem pista no nome; sao estoque, nao cadeia, entao decidir nao e urgente',
    desbloqueia: `${soltos.length} modelos a mais no estoque de familias livres`,
  }];
}

function elementsWithoutStock(match: CatalogMatch): UserDecision[] {
  return ELEMENTS.filter((e) => match.semModelo.some((s) => s.elemento === e) && match.livres[e].length === 0).map((e) => ({
    id: `falta-pack-${e}`,
    pergunta: `${e}: comprar/importar pack, ou aceitar ${match.semModelo.filter((s) => s.elemento === e).map((s) => s.pet).join(' e ')} sem modelo?`,
    porque: 'o insumo nao tem nenhuma familia livre desse elemento',
    desbloqueia: 'os pets desse elemento que hoje ficam sem corpo',
  }));
}

function elementsWithoutPet(match: CatalogMatch): UserDecision[] {
  const inSeed = new Set(PET_CATALOG_SEED.map((p) => p.type.split('/')[1]));
  return ELEMENTS.filter((e: Element) => !inSeed.has(e) && match.livres[e].length > 0).map((e) => ({
    id: `sem-pet-${e}`,
    pergunta: `${e}: criar pet no seed para ${match.livres[e].join(', ')}, ou deixar o modelo parado?`,
    porque: 'ha modelo desse elemento e nenhum pet no catalogo para vesti-lo',
    desbloqueia: 'um pet novo com modelo pronto',
  }));
}

function chainsWithoutAdult(cadeias: readonly AssetFamily[]): UserDecision[] {
  const semAdulto = cadeias.filter((c) => c.element !== undefined && !c.estagios.Adulto);
  if (semAdulto.length === 0) return [];
  return [{
    id: 'cadeias-sem-adulto',
    pergunta: `${semAdulto.map((c) => `${c.family} (${stageNames(c)})`).join('; ')}: o Big vira Adulto e o Blob Filhote?`,
    porque: 'sao cadeias de porte com Blob e Big mas sem forma do meio, e EPetGrowthStage nasce em Adulto',
    desbloqueia: 'tres cadeias de dois estagios que hoje ficam livres',
  }];
}

function elementsMissingInEngine(match: CatalogMatch, elementosDoMotor: readonly string[]): UserDecision[] {
  const known = new Set(elementosDoMotor);
  return ELEMENTS.filter((e) => !known.has(e)).map((e) => ({
    id: `motor-${e}`,
    pergunta: `${e}: ${match.livres[e].length} familias ja o usam (${match.livres[e].join(', ')}); a trilha A o cria no motor?`,
    porque: 'o elemento existe no gerador e NAO em Config/PetTypes.json; sem ele, pet desse tipo nao carrega',
    desbloqueia: 'uma entrada em PetTypes.json (cor, tilt, skill reusada) + as linhas em TypeEffectiveness.json; exige build, logo e da trilha A',
  }));
}

export function buildUserDecisions(params: BuildUserDecisionsParams): UserDecision[] {
  const { classificacao, cadeias, match, elementosDoMotor } = params;
  return [
    ...chainsWithoutElement(cadeias),
    ...conflicts(classificacao),
    ...elementsMissingInEngine(match, elementosDoMotor),
    ...elementsWithoutStock(match),
    ...chainsWithoutAdult(cadeias),
    ...elementsWithoutPet(match),
    ...singlesWithoutHint(classificacao, cadeias),
  ];
}

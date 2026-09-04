// Copyright 2026 Anderson. All Rights Reserved.

/**
 * AR4a — do INSUMO REAL (caminho, nome, pasta) para classificacao.
 *
 * `model-mapping.pure.ts` so ve o NOME. Aqui entra o que o nome nao tem: a
 * PASTA do import, que e fato medido (o pack de peixes fica em `Fish`), e o
 * caminho do asset, que e o que a trilha A vai apontar no Blueprint.
 */
import { HUMAN_CLASS_TOKENS, MODEL_PREFIX } from './model-hints.constant';
import type { Element, Stage, StageOrigin } from './model-mapping.pure';
import {
  ELEMENTS, elementsSuggestedBy, familyOf, isCreature, stageFromModelName, stageOriginOf,
} from './model-mapping.pure';
import { importedAssetsFileSchema, type ImportedAsset, type ImportedAssetsFile } from './imported-assets.schema';

export const IMPORTED_ASSETS_PATH = new URL(
  '../../../../../.specs/handoffs/assets-importados.json', import.meta.url,
);

export function loadImportedAssets(jsonText: string): ImportedAssetsFile {
  return importedAssetsFileSchema.parse(JSON.parse(jsonText));
}

/**
 * O que a PASTA diz do elemento. `autoritaria` quando a pasta e o pack inteiro
 * de um so elemento: `Fish` so tem peixe, e `FlowerHorn` e `Sunfish` sao peixes
 * apesar de `flower` e `sun` — medido, o nome enganou os dois.
 */
export type FolderPolicy = { readonly element: Element; readonly autoritaria: boolean };
export const FOLDER_POLICIES: Readonly<Record<string, FolderPolicy>> = {
  Fish: { element: 'Agua', autoritaria: true },
};

export type AssetKind = 'criatura' | 'humano' | 'prop';
export type ElementSource = 'nome' | 'pasta';

export type ClassifiedAsset = {
  readonly asset: string;
  /** O nome sem `SK_`: e como o relatorio e as decisoes falam dele. */
  readonly modelName: string;
  readonly pasta: string;
  readonly kind: AssetKind;
  readonly element: Element | undefined;
  readonly elementSource: ElementSource | undefined;
  /** Quando o nome sugeriu mais de um elemento — a lista, para o humano decidir. */
  readonly conflito: readonly Element[];
  readonly stage: Stage;
  readonly stageOrigin: StageOrigin;
  readonly family: string;
};

function hasHumanClass(modelName: string): boolean {
  const tokens = modelName.toLowerCase().split(/[^a-z0-9]+/);
  return HUMAN_CLASS_TOKENS.some((cls) => tokens.includes(cls));
}

/** Humano so quando a classe aparece E nada diz criatura: `Skeleton_Mage` e morto-vivo. */
function kindOf(modelName: string, byName: readonly Element[]): AssetKind {
  if (!isCreature(modelName)) return 'prop';
  if (hasHumanClass(modelName) && byName.length === 0) return 'humano';
  return 'criatura';
}

type ResolvedElement = { element: Element | undefined; source: ElementSource | undefined };

function resolveElement(byName: readonly Element[], policy: FolderPolicy | undefined): ResolvedElement {
  const fromName = byName.length === 1 ? byName[0] : undefined;
  if (policy?.autoritaria) {
    return fromName === policy.element
      ? { element: fromName, source: 'nome' }
      : { element: policy.element, source: 'pasta' };
  }
  if (fromName) return { element: fromName, source: 'nome' };
  if (policy) return { element: policy.element, source: 'pasta' };
  return { element: undefined, source: undefined };
}

export function classifyImportedAsset(asset: ImportedAsset): ClassifiedAsset {
  const modelName = asset.nome.replace(MODEL_PREFIX, '');
  const byName = elementsSuggestedBy(modelName);
  const kind = kindOf(modelName, byName);
  const resolved = kind === 'criatura'
    ? resolveElement(byName, FOLDER_POLICIES[asset.pasta])
    : { element: undefined, source: undefined };
  return {
    asset: asset.asset,
    modelName,
    pasta: asset.pasta,
    kind,
    element: resolved.element,
    elementSource: resolved.source,
    conflito: byName.length > 1 ? byName : [],
    stage: stageFromModelName(modelName),
    stageOrigin: stageOriginOf(modelName),
    family: familyOf(modelName),
  };
}

export function classifyImportedAssets(assets: readonly ImportedAsset[]): ClassifiedAsset[] {
  return assets.map(classifyImportedAsset);
}

/** O relatorio de AR4a: o que o gerador SOUBE, separado do que NAO soube. */
export type ClassificationReport = {
  readonly total: number;
  readonly criaturas: number;
  readonly humanos: readonly string[];
  readonly props: readonly string[];
  readonly comElemento: Readonly<Record<Element, readonly string[]>>;
  readonly semElemento: readonly string[];
  readonly conflitos: ReadonlyArray<{ readonly modelName: string; readonly elementos: readonly Element[] }>;
};

export function summarizeClassification(assets: readonly ClassifiedAsset[]): ClassificationReport {
  const criaturas = assets.filter((a) => a.kind === 'criatura');
  const names = (list: readonly ClassifiedAsset[]): string[] => list.map((a) => a.modelName);
  const comElemento = {} as Record<Element, readonly string[]>;
  for (const e of ELEMENTS) comElemento[e] = names(criaturas.filter((a) => a.element === e));
  return {
    total: assets.length,
    criaturas: criaturas.length,
    humanos: names(assets.filter((a) => a.kind === 'humano')),
    props: names(assets.filter((a) => a.kind === 'prop')),
    comElemento,
    semElemento: names(criaturas.filter((a) => a.element === undefined)),
    conflitos: criaturas
      .filter((a) => a.conflito.length > 0)
      .map((a) => ({ modelName: a.modelName, elementos: a.conflito })),
  };
}

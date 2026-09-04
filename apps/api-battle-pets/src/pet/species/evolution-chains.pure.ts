// Copyright 2026 Anderson. All Rights Reserved.

/**
 * AR4b — as CADEIAS DE EVOLUCAO do insumo real, por caminho de asset.
 *
 * `groupByFamily` (model-mapping) trabalha com nomes; aqui o mesmo agrupamento
 * carrega o CAMINHO que a trilha A aponta no Blueprint, e diz de onde veio a
 * cadeia: do AUTOR (`_Evolved`, `MushroomKing`) ou do PORTE do corpo
 * (`Cactoro_Blob` -> `Cactoro_Big`). Cadeia de porte e leitura nossa, nao do
 * autor — por isso ela vai para a lista de decisao, nao para o catalogo direto.
 */
import type { ClassifiedAsset } from './imported-assets.pure';
import type { Element, Stage } from './model-mapping.pure';

export type ChainOrigin = 'autor' | 'porte';

export type AssetFamily = {
  readonly family: string;
  readonly element: Element | undefined;
  /** Caminho do asset por estagio. */
  readonly estagios: Partial<Record<Stage, string>>;
  /** Outra pele do mesmo estagio (`Dragon_Mon` ao lado de `Dragon`). */
  readonly variantes: readonly string[];
  readonly origem: ChainOrigin;
};

type MutableFamily = {
  family: string;
  element: Element | undefined;
  estagios: Partial<Record<Stage, string>>;
  variantes: string[];
  hasAuthoredStage: boolean;
};

/** Agrupa so as CRIATURAS por familia; humano e prop nao evoluem para pet. */
export function groupAssetsByFamily(assets: readonly ClassifiedAsset[]): AssetFamily[] {
  const byFamily = new Map<string, MutableFamily>();

  for (const a of assets) {
    if (a.kind !== 'criatura') continue;
    const entry = byFamily.get(a.family)
      ?? { family: a.family, element: a.element, estagios: {}, variantes: [], hasAuthoredStage: false };
    if (entry.estagios[a.stage]) entry.variantes.push(a.asset);
    else entry.estagios[a.stage] = a.asset;
    entry.element = entry.element ?? a.element;
    entry.hasAuthoredStage = entry.hasAuthoredStage || a.stageOrigin === 'autor';
    byFamily.set(a.family, entry);
  }

  return [...byFamily.values()].map(({ hasAuthoredStage, ...rest }) => ({
    ...rest,
    origem: hasAuthoredStage ? 'autor' : 'porte',
  }));
}

/** Cadeia = familia com ao menos DOIS estagios. Um modelo sozinho nao e evolucao. */
export function buildAssetChains(assets: readonly ClassifiedAsset[]): AssetFamily[] {
  return groupAssetsByFamily(assets).filter((f) => Object.keys(f.estagios).length >= 2);
}

export function chainsByOrigin(chains: readonly AssetFamily[], origem: ChainOrigin): AssetFamily[] {
  return chains.filter((c) => c.origem === origem);
}

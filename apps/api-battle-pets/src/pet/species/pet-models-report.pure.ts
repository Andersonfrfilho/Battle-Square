// Copyright 2026 Anderson. All Rights Reserved.

/**
 * AR4d — o mapa `pet -> asset` que a trilha A consome.
 *
 * A chave e o NOME do pet, nao o catalogId: o C++ identifica o pet por
 * `FString Name` (PetDataLoader.h), e o catalogId e um UUID que so nasce no
 * seed do banco — nao existe antes de a trilha A precisar do modelo.
 *
 * Tudo aqui deriva do insumo e do seed; nada e escrito a mao alem da
 * curadoria em `catalog-assignment.constant.ts`. Por isso o JSON commitado
 * tem teste de snapshot: se ele divergir do gerado, alguem editou a mao.
 */
import { PET_CATALOG_SEED } from '../seed/pet-catalog.seed';
import { PET_MODEL_ASSIGNMENTS } from './catalog-assignment.constant';
import { matchCatalog, type CatalogMatch } from './catalog-matching.pure';
import { buildAssetChains, groupAssetsByFamily, type AssetFamily } from './evolution-chains.pure';
import { classifyImportedAssets, summarizeClassification, type ClassificationReport } from './imported-assets.pure';
import type { ImportedAssetsFile } from './imported-assets.schema';
import { buildUserDecisions, type UserDecision } from './user-decisions.pure';

export const PET_MODELS_HANDOFF_PATH = new URL('../../../../../.specs/handoffs/pets-modelos.json', import.meta.url);

export const HANDOFF_KEY_NOTE = 'nome do pet (PetDataLoader.h identifica por Name; catalogId so nasce no seed do banco)';

export type PetModelsHandoff = {
  readonly geradoEm: string;
  readonly fonte: string;
  readonly chave: string;
  readonly pets: CatalogMatch['atribuicoes'];
  readonly semModelo: CatalogMatch['semModelo'];
  readonly familiasLivres: CatalogMatch['livres'];
  readonly cadeias: readonly AssetFamily[];
  readonly classificacao: ClassificationReport;
  readonly decisoes: readonly UserDecision[];
};

export type BuildPetModelsHandoffParams = {
  readonly insumo: ImportedAssetsFile;
  readonly elementosDoMotor: readonly string[];
};

export function buildPetModelsHandoff({ insumo, elementosDoMotor }: BuildPetModelsHandoffParams): PetModelsHandoff {
  const assets = classifyImportedAssets(insumo.malhas);
  const families = groupAssetsByFamily(assets);
  const match = matchCatalog({ seed: PET_CATALOG_SEED, assignments: PET_MODEL_ASSIGNMENTS, families });
  const cadeias = buildAssetChains(assets);
  const classificacao = summarizeClassification(assets);
  return {
    geradoEm: insumo.geradoEm,
    fonte: 'assets-importados.json',
    chave: HANDOFF_KEY_NOTE,
    pets: match.atribuicoes,
    semModelo: match.semModelo,
    familiasLivres: match.livres,
    cadeias,
    classificacao,
    decisoes: buildUserDecisions({ classificacao, cadeias, match, elementosDoMotor }),
  };
}

export function serializeHandoff(handoff: PetModelsHandoff): string {
  return `${JSON.stringify(handoff, null, 2)}\n`;
}

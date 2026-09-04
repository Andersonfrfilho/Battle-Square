// Copyright 2026 Anderson. All Rights Reserved.

/**
 * Gera `.specs/handoffs/pets-modelos.json` a partir do insumo da trilha A.
 *
 *   cd apps/api-battle-pets && bun src/pet/species/generate-pet-models.ts
 *
 * Nao toca a Unreal: le um arquivo e escreve outro.
 */
import { IMPORTED_ASSETS_PATH, loadImportedAssets } from './imported-assets.pure';
import { buildPetModelsHandoff, PET_MODELS_HANDOFF_PATH, serializeHandoff } from './pet-models-report.pure';

const insumo = loadImportedAssets(await Bun.file(IMPORTED_ASSETS_PATH).text());
const handoff = buildPetModelsHandoff(insumo);
await Bun.write(PET_MODELS_HANDOFF_PATH, serializeHandoff(handoff));
console.log(`${PET_MODELS_HANDOFF_PATH.pathname}: ${handoff.pets.length} pets com modelo, ${handoff.semModelo.length} sem`);

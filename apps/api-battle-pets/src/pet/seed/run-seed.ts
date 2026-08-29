// Copyright 2026 Anderson. All Rights Reserved.

import { seedPetCatalog } from './pet-catalog.seed';

// Entrada do seed: `bun run src/pet/seed/run-seed.ts`.
const resultado = await seedPetCatalog();

// Log estruturado, sem PII (security.md §1): nome de pet é dado de catálogo,
// não de pessoa.
console.log(JSON.stringify({
  level: 'info',
  event: 'pet_catalog_seeded',
  created: resultado.created,
  skipped: resultado.skipped,
}));

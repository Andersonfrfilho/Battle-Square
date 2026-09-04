// Copyright 2026 Anderson. All Rights Reserved.

import { z } from 'zod';

/**
 * So o que o gerador precisa de `Config/PetTypes.json`: os NOMES dos elementos
 * que o motor conhece. Elemento que existe aqui no TypeScript e nao existe la
 * vira item de decisao, porque criar pet dele exige mexer no motor.
 */
export const engineTypesFileSchema = z.object({
  elements: z.array(z.object({ name: z.string().min(1) })).min(1),
});

export const ENGINE_TYPES_PATH = new URL('../../../../../Config/PetTypes.json', import.meta.url);

export function loadEngineElements(jsonText: string): string[] {
  return engineTypesFileSchema.parse(JSON.parse(jsonText)).elements.map((e) => e.name);
}

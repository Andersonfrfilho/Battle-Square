// Copyright 2026 Anderson. All Rights Reserved.

import { z } from 'zod';

/**
 * O INSUMO da trilha B: o que a trilha A exportou do editor para
 * `.specs/handoffs/assets-importados.json`. Esta trilha le o ARQUIVO, nunca o
 * MCP — o editor esta ocupado do outro lado, e abrir outro derrubaria os dois.
 */
export const importedAssetSchema = z.object({
  asset: z.string().startsWith('/Game/'),
  nome: z.string().min(1),
  pasta: z.string().min(1),
});
export type ImportedAsset = z.infer<typeof importedAssetSchema>;

export const importedAssetsFileSchema = z
  .object({
    geradoEm: z.string().min(1),
    total: z.number().int().nonnegative(),
    malhas: z.array(importedAssetSchema),
  })
  .refine((file) => file.total === file.malhas.length, {
    message: 'total nao bate com a quantidade de malhas: insumo truncado ou editado a mao',
  });
export type ImportedAssetsFile = z.infer<typeof importedAssetsFileSchema>;

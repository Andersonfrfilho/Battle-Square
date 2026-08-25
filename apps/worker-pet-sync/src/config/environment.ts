// Copyright 2026 Anderson. All Rights Reserved.

import { z } from 'zod';

// nodejs.md: acesso a env var só via módulo validado. Falha alto e claro
// no boot se algo obrigatório faltar.
const environmentSchema = z.object({
  API_BATTLE_PETS_URL: z.url(),
  SYNC_API_TOKEN: z.string().min(32, 'SYNC_API_TOKEN precisa ter ao menos 32 caracteres'),
  ED25519_PUBLIC_KEY_PEM: z.string().min(1, 'ED25519_PUBLIC_KEY_PEM é obrigatória para verificar exportações de pet'),
  LOCAL_MIRROR_PATH: z.string().min(1).default('./data/pets-mirror.sqlite'),
  MIRROR_ENCRYPTION_KEY: z.string().min(32, 'MIRROR_ENCRYPTION_KEY precisa ter ao menos 32 caracteres'),
  SYNC_INTERVAL_SECONDS: z.coerce.number().int().positive().default(30), // DP-07
});

export const environment = environmentSchema.parse(process.env);

// Copyright 2026 Anderson. All Rights Reserved.

import { z } from 'zod';

// nodejs.md: acesso a env var só através de módulo validado — nunca
// process.env.X espalhado pelo código. Falha alto e claro no boot se
// algo obrigatório faltar, nunca segue com default silencioso.
const environmentSchema = z.object({
  DATABASE_URL: z.url(),
  PORT: z.coerce.number().default(3100),

  /**
   * O teto da coleção por conta (decisão 38-b: 500). Config do servidor, e
   * não constante: "sem limite" seria decisão financeira disfarçada de
   * ausência de decisão — e o número certo se descobre operando.
   */
  COLLECTION_CAP: z.coerce.number().int().positive().default(500),
  NODE_ENV: z.enum(['development', 'production', 'test']).default('development'),
  ADMIN_API_TOKEN: z.string().min(32, 'ADMIN_API_TOKEN precisa ter ao menos 32 caracteres'),
  SYNC_API_TOKEN: z.string().min(32, 'SYNC_API_TOKEN precisa ter ao menos 32 caracteres'),
  ED25519_PRIVATE_KEY_PEM: z.string().min(1, 'ED25519_PRIVATE_KEY_PEM é obrigatória para assinar exportações de pet'),
  ACCESS_TOKEN_SECRET: z.string().min(32, 'ACCESS_TOKEN_SECRET precisa ter ao menos 32 caracteres'),
});

export const environment = environmentSchema.parse(process.env);

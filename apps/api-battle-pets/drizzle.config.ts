// Copyright 2026 Anderson. All Rights Reserved.

import { defineConfig } from 'drizzle-kit';

import { environment } from './src/config/environment';

export default defineConfig({
  schema: [
    './src/pet/pet.schema.ts',
    './src/account/account.schema.ts',
    './src/moderation/moderation.schema.ts',
    // PS1: a tabela da posse estava ESCRITA e o gerador não a via. Listada
    // aqui por INTENÇÃO (como as outras) — mas ⚠️ o gerador está inutilizável
    // desde 0003: o diário parou em 0002 e as migrations de golpes foram
    // escritas à mão por fora dele, então `drizzle-kit generate` tenta
    // recriar `pet_moves` junto de qualquer coisa nova. Até o diário ser
    // reconciliado, migration nova é SQL manual (0009 é o exemplo).
    './src/ownership/ownership.schema.ts',
  ],
  out: './drizzle',
  dialect: 'postgresql',
  dbCredentials: { url: environment.DATABASE_URL },
});

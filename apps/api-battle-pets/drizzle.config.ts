// Copyright 2026 Anderson. All Rights Reserved.

import { defineConfig } from 'drizzle-kit';

import { environment } from './src/config/environment';

export default defineConfig({
  schema: './src/pet/pet.schema.ts',
  out: './drizzle',
  dialect: 'postgresql',
  dbCredentials: { url: environment.DATABASE_URL },
});

// Copyright 2026 Anderson. All Rights Reserved.

import { Database } from 'bun:sqlite';
import { randomUUID } from 'node:crypto';
import { existsSync, readFileSync, rmSync, writeFileSync } from 'node:fs';
import { join } from 'node:path';

import { environment } from '../config/environment';
import { decryptBuffer, encryptBuffer } from './mirror-crypto';
import { resolveTempDirectory } from './mirror-tempdir';

// design.md: "o espelho É o payload persistido" — mesma estrutura do
// SignedPetExport, sem transformação.
export type MirrorPetRow = {
  id: string;
  name: string;
  type: string;
  attack: number;
  defense: number;
  speed: number;
  maxHealth: number;
  updatedAt: string;
  signature: string;
};

const CREATE_TABLE_SQL = `
  CREATE TABLE IF NOT EXISTS pets (
    id TEXT PRIMARY KEY,
    name TEXT NOT NULL,
    type TEXT NOT NULL,
    attack INTEGER NOT NULL,
    defense INTEGER NOT NULL,
    speed INTEGER NOT NULL,
    maxHealth INTEGER NOT NULL,
    updatedAt TEXT NOT NULL,
    signature TEXT NOT NULL
  );
`;

// Abre o espelho para uso: decifra (se já existir) num temporário em
// texto plano, abre com bun:sqlite, devolve o db + uma função de
// finalização que cifra de volta e apaga o temporário — SEMPRE, mesmo
// em erro (equivalente ao trap do probe_isolation.sh).
export async function withMirror<T>(callback: (db: Database) => Promise<T> | T): Promise<T> {
  const { path: tempDir } = resolveTempDirectory();
  const tempPlainPath = join(tempDir, `pet-mirror-${randomUUID()}.sqlite`);

  try {
    if (existsSync(environment.LOCAL_MIRROR_PATH)) {
      const encrypted = readFileSync(environment.LOCAL_MIRROR_PATH);
      const plaintext = decryptBuffer(encrypted, environment.MIRROR_ENCRYPTION_KEY);
      writeFileSync(tempPlainPath, plaintext);
    }

    const db = new Database(tempPlainPath, { create: true });
    db.exec(CREATE_TABLE_SQL);

    try {
      return await callback(db);
    } finally {
      db.close();
    }
  } finally {
    if (existsSync(tempPlainPath)) {
      const plaintext = readFileSync(tempPlainPath);
      const encrypted = encryptBuffer(plaintext, environment.MIRROR_ENCRYPTION_KEY);
      writeFileSync(environment.LOCAL_MIRROR_PATH, encrypted);
      rmSync(tempPlainPath, { force: true });
    }
  }
}

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
  moves: {
    name: string;
    power: number;
    terrainEffect: string;
    // Presentes só nos golpes assinados DEPOIS do requisito existir. Ausência
    // é golpe sem requisito, e o verificador reserializa o que veio — então
    // acrescentar a chave num golpe antigo quebraria a assinatura DELE.
    requiresAttribute?: string;
    requiresValue?: number;
    effectStat?: string;
    effectPercent?: number;
    terrainDuration?: number;
  }[];
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
    -- Golpes como JSON CANÔNICO, não como tabela separada.
    --
    -- O leitor C++ precisa reconstruir o payload assinado byte a byte; guardar
    -- a serialização pronta evita um join dentro do SQLite só para remontar o
    -- que já veio pronto do backend — e evita que a ordem se perca no caminho,
    -- que é o que faria a assinatura falhar.
    moves TEXT NOT NULL DEFAULT '[]',
    signature TEXT NOT NULL
  );
`;

// Abre o espelho para uso: decifra (se já existir) num temporário em
// texto plano, abre com bun:sqlite, devolve o db + uma função de
// finalização que cifra de volta e apaga o temporário — SEMPRE, mesmo
// em erro (equivalente ao trap do probe_isolation.sh).
/**
 * Acrescenta colunas que o espelho ainda não tem.
 *
 * `CREATE TABLE IF NOT EXISTS` NÃO altera tabela existente: sem isto, um
 * espelho já em uso nunca ganharia a coluna de golpes, e o worker gravaria num
 * campo inexistente a cada ciclo. Migrar aqui é o que torna a mudança segura
 * para quem já está rodando.
 */
function ensureMovesColumn(db: Database): void {
  const colunas = db.query('PRAGMA table_info(pets)').all() as { name: string }[];
  if (colunas.some((coluna) => coluna.name === 'moves')) {
    return;
  }

  // Default '[]' e não NULL: pet gravado antes dos golpes foi ASSINADO com
  // moves:[], e qualquer outro valor invalidaria a assinatura dele.
  db.run("ALTER TABLE pets ADD COLUMN moves TEXT NOT NULL DEFAULT '[]'");
}

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
    ensureMovesColumn(db);

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

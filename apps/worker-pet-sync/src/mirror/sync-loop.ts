// Copyright 2026 Anderson. All Rights Reserved.

import { Database } from 'bun:sqlite';

import { environment } from '../config/environment';
import { withMirror } from './mirror.schema';
import { verifyPetSignature } from './signature-verifier';

type ExportedPet = {
  id: string;
  name: string;
  type: string;
  attack: number;
  defense: number;
  speed: number;
  maxHealth: number;
  updatedAt: string;
  // Opcional na ENTRADA: um backend antigo não manda o campo, e exigi-lo faria
  // o sync parar por causa de dado que o outro lado ainda não tem.
  moves?: { name: string; power: number }[];
  signature: string;
};

type SyncCycleResult = {
  fetched: number;
  written: number;
  rejected: number;
  backendUnreachable: boolean;
};

function getLastSyncedTimestamp(db: Database): string | null {
  const row = db.query('SELECT MAX(updatedAt) as latest FROM pets').get() as { latest: string | null } | undefined;
  return row?.latest ?? null;
}

function upsertPet(db: Database, pet: ExportedPet): void {
  db.run(
    `INSERT INTO pets (id, name, type, attack, defense, speed, maxHealth, updatedAt, moves, signature)
     VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
     ON CONFLICT(id) DO UPDATE SET
       name = excluded.name, type = excluded.type, attack = excluded.attack,
       defense = excluded.defense, speed = excluded.speed, maxHealth = excluded.maxHealth,
       updatedAt = excluded.updatedAt, moves = excluded.moves, signature = excluded.signature`,
    [
      pet.id, pet.name, pet.type, pet.attack, pet.defense, pet.speed, pet.maxHealth, pet.updatedAt,
      // JSON.stringify do array COMO VEIO: reserializar campo a campo aqui
      // arriscaria produzir um texto diferente do que foi assinado, e a
      // verificação falharia sem ninguém entender por quê.
      JSON.stringify(pet.moves ?? []),
      pet.signature,
    ],
  );
}

// T14 (tasks.md, PETDB-06/07/08). Backend inacessível NÃO derruba o
// processo nem apaga o espelho — mantém o que já tem, tenta de novo no
// próximo ciclo (design.md, Tratamento de Erro).
export async function runSyncCycle(): Promise<SyncCycleResult> {
  const lastSynced = await withMirror((db) => getLastSyncedTimestamp(db));

  const url = new URL('/v1/pets/export', environment.API_BATTLE_PETS_URL);
  if (lastSynced) url.searchParams.set('updatedAfter', lastSynced);

  let exportedPets: ExportedPet[];
  try {
    const response = await fetch(url, {
      headers: { authorization: `Bearer ${environment.SYNC_API_TOKEN}` },
    });
    if (!response.ok) {
      console.warn(`[sync-loop] backend respondeu ${response.status} — mantendo espelho atual, tentando de novo no próximo ciclo`);
      return { fetched: 0, written: 0, rejected: 0, backendUnreachable: true };
    }
    const body = (await response.json()) as { data: ExportedPet[] };
    exportedPets = body.data;
  } catch (error) {
    console.warn('[sync-loop] backend inacessível — mantendo espelho atual, tentando de novo no próximo ciclo:', (error as Error).message);
    return { fetched: 0, written: 0, rejected: 0, backendUnreachable: true };
  }

  let written = 0;
  let rejected = 0;

  await withMirror((db) => {
    for (const pet of exportedPets) {
      if (!verifyPetSignature({ ...pet, moves: pet.moves ?? [] })) {
        rejected += 1;
        console.warn(`[sync-loop] assinatura inválida para o pet ${pet.id} — descartado, NÃO entra no espelho`);
        continue;
      }
      upsertPet(db, pet);
      written += 1;
    }
  });

  return { fetched: exportedPets.length, written, rejected, backendUnreachable: false };
}

export function startSyncLoop(): void {
  let isFirstCycleDone = false;

  const tick = async () => {
    const result = await runSyncCycle();
    console.log(
      `[sync-loop] ciclo: ${result.fetched} recebidos, ${result.written} gravados, ${result.rejected} rejeitados` +
        (result.backendUnreachable ? ' (backend inacessível)' : ''),
    );
    isFirstCycleDone = true;
  };

  // PETDB-06: sync completo antes de sinalizar "pronto" para o servidor
  // de combate assemblar batalhas.
  tick().then(() => {
    if (!isFirstCycleDone) {
      throw new Error('Primeira sincronização não completou — não é seguro montar batalhas sem dados de pet');
    }
    setInterval(tick, environment.SYNC_INTERVAL_SECONDS * 1000);
  });
}

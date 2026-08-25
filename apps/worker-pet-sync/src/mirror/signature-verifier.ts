// Copyright 2026 Anderson. All Rights Reserved.

import { createPublicKey, verify } from 'node:crypto';

import { environment } from '../config/environment';
import type { MirrorPetRow } from './mirror.schema';

// Espelha pet-signing.ts (backend) — MESMA serialização canônica dos
// dois lados, ou a verificação quebra em silêncio (design.md, T9).
// Nunca precisa da chave privada — só a pública.
function toCanonicalPayload(pet: Pick<MirrorPetRow, 'id' | 'name' | 'type' | 'attack' | 'defense' | 'speed' | 'maxHealth' | 'updatedAt'>): string {
  return JSON.stringify({
    id: pet.id,
    name: pet.name,
    type: pet.type,
    attack: pet.attack,
    defense: pet.defense,
    speed: pet.speed,
    maxHealth: pet.maxHealth,
    updatedAt: pet.updatedAt,
  });
}

const publicKey = createPublicKey({ key: environment.ED25519_PUBLIC_KEY_PEM, format: 'pem' });

export function verifyPetSignature(pet: MirrorPetRow): boolean {
  const canonicalPayload = toCanonicalPayload(pet);
  try {
    return verify(null, Buffer.from(canonicalPayload, 'utf-8'), publicKey, Buffer.from(pet.signature, 'base64'));
  } catch {
    // Assinatura malformada (base64 inválido, etc.) — trata como
    // verificação falha, nunca deixa a exceção propagar e derrubar o
    // ciclo de sincronização inteiro por causa de 1 registro ruim.
    return false;
  }
}

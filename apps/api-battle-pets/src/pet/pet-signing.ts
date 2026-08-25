// Copyright 2026 Anderson. All Rights Reserved.

import { createPrivateKey, sign } from 'node:crypto';

import { environment } from '../config/environment';
import type { Pet } from './pet.schema';

// PETDB-10, refinado no design.md: Ed25519 assimétrico, não HMAC — só o
// backend guarda a chave privada; qualquer verificador (sidecar de sync,
// servidor de combate) só precisa da pública. Comprometer um verificador
// nunca dá poder de forjar uma assinatura nova.

export type SignedPetExport = {
  id: string;
  name: string;
  type: string;
  attack: number;
  defense: number;
  speed: number;
  maxHealth: number;
  updatedAt: string;
  signature: string; // base64
};

// A ORDEM DAS CHAVES AQUI É PARTE DO CONTRATO DE ASSINATURA. Mudar a
// ordem, adicionar ou remover um campo sem atualizar TODOS os
// verificadores (sidecar TypeScript, leitor C++ na Unreal) quebra a
// verificação silenciosamente — a serialização precisa ser canônica e
// idêntica dos dois lados.
function toCanonicalPayload(pet: Pet): string {
  const canonical = {
    id: pet.id,
    name: pet.name,
    type: pet.type,
    attack: pet.attack,
    defense: pet.defense,
    speed: pet.speed,
    maxHealth: pet.maxHealth,
    updatedAt: pet.updatedAt.toISOString(),
  };
  return JSON.stringify(canonical);
}

const privateKey = createPrivateKey({ key: environment.ED25519_PRIVATE_KEY_PEM, format: 'pem' });

export function signPet(pet: Pet): SignedPetExport {
  const canonicalPayload = toCanonicalPayload(pet);
  const signature = sign(null, Buffer.from(canonicalPayload, 'utf-8'), privateKey);

  return {
    id: pet.id,
    name: pet.name,
    type: pet.type,
    attack: pet.attack,
    defense: pet.defense,
    speed: pet.speed,
    maxHealth: pet.maxHealth,
    updatedAt: pet.updatedAt.toISOString(),
    signature: signature.toString('base64'),
  };
}

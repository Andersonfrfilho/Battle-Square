// Copyright 2026 Anderson. All Rights Reserved.

import { createPrivateKey, sign } from 'node:crypto';

import { environment } from '../config/environment';
import type { Pet } from './pet.schema';

// PETDB-10, refinado no design.md: Ed25519 assimétrico, não HMAC — só o
// backend guarda a chave privada; qualquer verificador (sidecar de sync,
// servidor de combate) só precisa da pública. Comprometer um verificador
// nunca dá poder de forjar uma assinatura nova.

// Golpe como ele viaja: nome e poder, na ordem do slot.
export type SignedPetMove = {
  name: string;
  power: number;
  terrainEffect: string;
};

export type SignedPetExport = {
  id: string;
  name: string;
  type: string;
  attack: number;
  defense: number;
  speed: number;
  maxHealth: number;
  updatedAt: string;
  moves: SignedPetMove[];
  signature: string; // base64
};

// A ORDEM DAS CHAVES AQUI É PARTE DO CONTRATO DE ASSINATURA. Mudar a
// ordem, adicionar ou remover um campo sem atualizar TODOS os
// verificadores (sidecar TypeScript, leitor C++ na Unreal) quebra a
// verificação silenciosamente — a serialização precisa ser canônica e
// idêntica dos dois lados.
function toCanonicalPayload(pet: Pet, moves: SignedPetMove[]): string {
  const canonical = {
    id: pet.id,
    name: pet.name,
    type: pet.type,
    attack: pet.attack,
    defense: pet.defense,
    speed: pet.speed,
    maxHealth: pet.maxHealth,
    updatedAt: pet.updatedAt.toISOString(),
    // APÊNDICE, no fim: acrescentar no fim é a única mudança de payload que o
    // verificador C++ acompanha com uma alteração local, sem reordenar nada
    // do que já era assinado.
    //
    // DP-golpe-03: golpe FORA da assinatura seria o caminho óbvio para
    // adulterar dano, e a assinatura existe exatamente para isso.
    moves,
  };
  return JSON.stringify(canonical);
}

/**
 * Ordena por slot e devolve só o que é assinado.
 *
 * A ordem NÃO pode vir do banco por acaso: o índice do slot é o que viaja no
 * commit, e uma ordem diferente entre backend e cliente faria o jogador usar
 * um golpe e o servidor resolver outro.
 */
export function toSignedMoves(
  moves: { slot: number; name: string; power: number; terrainEffect?: string }[],
): SignedPetMove[] {
  return [...moves]
    .sort((left, right) => left.slot - right.slot)
    .map((move) => ({
      name: move.name,
      power: move.power,
      // Ausente vira 'none': golpe cadastrado antes do efeito de terreno
      // existir não muda a casa, e omitir a chave produziria um payload
      // diferente do que o verificador espera.
      terrainEffect: move.terrainEffect ?? 'none',
    }));
}

const privateKey = createPrivateKey({ key: environment.ED25519_PRIVATE_KEY_PEM, format: 'pem' });

export function signPet(pet: Pet, moves: SignedPetMove[] = []): SignedPetExport {
  const canonicalPayload = toCanonicalPayload(pet, moves);
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
    moves,
    signature: signature.toString('base64'),
  };
}

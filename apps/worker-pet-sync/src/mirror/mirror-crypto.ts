// Copyright 2026 Anderson. All Rights Reserved.

import { createCipheriv, createDecipheriv, randomBytes } from 'node:crypto';

// AD-018: bun:sqlite não tem SQLCipher — criptografia é do ARQUIVO
// inteiro (AES-256-GCM, autenticado), não do banco em si. O SQLite opera
// sobre um temporário em texto plano em tmpfs (RAM), nunca em disco.

const ALGORITHM = 'aes-256-gcm';
const IV_LENGTH_BYTES = 12; // recomendado para GCM
const AUTH_TAG_LENGTH_BYTES = 16;

function deriveKeyBuffer(hexKey: string): Buffer {
  const buffer = Buffer.from(hexKey, 'hex');
  if (buffer.length !== 32) {
    throw new Error('MIRROR_ENCRYPTION_KEY precisa decodificar para exatamente 32 bytes (64 caracteres hex) para AES-256');
  }
  return buffer;
}

// Formato do arquivo cifrado: [IV 12 bytes][AuthTag 16 bytes][ciphertext...]
export function encryptBuffer(plaintext: Buffer, hexKey: string): Buffer {
  const key = deriveKeyBuffer(hexKey);
  const iv = randomBytes(IV_LENGTH_BYTES);
  const cipher = createCipheriv(ALGORITHM, key, iv);
  const ciphertext = Buffer.concat([cipher.update(plaintext), cipher.final()]);
  const authTag = cipher.getAuthTag();
  return Buffer.concat([iv, authTag, ciphertext]);
}

export function decryptBuffer(encrypted: Buffer, hexKey: string): Buffer {
  const key = deriveKeyBuffer(hexKey);
  const iv = encrypted.subarray(0, IV_LENGTH_BYTES);
  const authTag = encrypted.subarray(IV_LENGTH_BYTES, IV_LENGTH_BYTES + AUTH_TAG_LENGTH_BYTES);
  const ciphertext = encrypted.subarray(IV_LENGTH_BYTES + AUTH_TAG_LENGTH_BYTES);

  const decipher = createDecipheriv(ALGORITHM, key, iv);
  decipher.setAuthTag(authTag);
  // Se o arquivo foi adulterado, decipher.final() lança — é a detecção
  // de integridade do GCM funcionando (AD-018).
  return Buffer.concat([decipher.update(ciphertext), decipher.final()]);
}

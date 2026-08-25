// Copyright 2026 Anderson. All Rights Reserved.

import { timingSafeEqual } from 'node:crypto';

import { environment } from '../config/environment';

// security.md §2: comparação de segredo sempre timingSafeEqual sobre
// digests de tamanho fixo — nunca === direto em string (vaza tempo de
// comparação, que é informação sobre o quanto do segredo já acertou).
export type AuthScope = 'admin' | 'sync';

export type AuthResult = { authenticated: true; scope: AuthScope } | { authenticated: false };

function safeCompare(a: string, b: string): boolean {
  const bufferA = Buffer.from(a);
  const bufferB = Buffer.from(b);
  // Tamanhos diferentes: ainda assim compara contra um buffer do mesmo
  // tamanho de A, para não vazar "tamanho bateu, mas conteúdo não" via
  // tempo de execução diferente entre os dois ramos.
  if (bufferA.length !== bufferB.length) {
    timingSafeEqual(bufferA, bufferA);
    return false;
  }
  return timingSafeEqual(bufferA, bufferB);
}

export function authenticate(request: Request): AuthResult {
  const header = request.headers.get('authorization');
  if (!header?.startsWith('Bearer ')) {
    return { authenticated: false };
  }

  const token = header.slice('Bearer '.length);

  if (safeCompare(token, environment.ADMIN_API_TOKEN)) {
    return { authenticated: true, scope: 'admin' };
  }
  if (safeCompare(token, environment.SYNC_API_TOKEN)) {
    return { authenticated: true, scope: 'sync' };
  }
  return { authenticated: false };
}

const WRITE_SCOPES: readonly AuthScope[] = ['admin'];

export function canWrite(scope: AuthScope): boolean {
  return WRITE_SCOPES.includes(scope);
}

// Copyright 2026 Anderson. All Rights Reserved.

import { timingSafeEqual } from 'node:crypto';

import { verifyAccessToken } from '../account/account.token';
import { environment } from '../config/environment';

// security.md §2: comparação de segredo sempre timingSafeEqual sobre
// digests de tamanho fixo — nunca === direto em string (vaza tempo de
// comparação, que é informação sobre o quanto do segredo já acertou).
export type AuthScope = 'admin' | 'sync' | 'player';

/**
 * O resultado carrega QUEM, não só que entrou (PS2): o jogador autenticado
 * traz o `accountId`, porque é ele que a autorização por OBJETO confere —
 * escopo sem identidade autoriza rota, e rota não é dono de nada.
 */
export type AuthResult =
  | { authenticated: true; scope: 'admin' | 'sync' }
  | { authenticated: true; scope: 'player'; accountId: string }
  | { authenticated: false };

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

export function authenticate(request: Request, now: Date = new Date()): AuthResult {
  const header = request.headers.get('authorization');
  if (!header?.startsWith('Bearer ')) {
    return { authenticated: false };
  }

  const token = header.slice('Bearer '.length);

  // OS ESTÁTICOS PRIMEIRO, e eles NUNCA viram jogador (contrapeso da PS2):
  // se o ADMIN_API_TOKEN valesse como conta, um segredo estático e sem dono
  // seria o dono de todas as coleções — e a posse no servidor não teria
  // ficado mais segura que o arquivo que ela substitui, só mais difícil de
  // editar. Admin autoriza ROTA de operação; conta é outra espécie de coisa.
  if (safeCompare(token, environment.ADMIN_API_TOKEN)) {
    return { authenticated: true, scope: 'admin' };
  }
  if (safeCompare(token, environment.SYNC_API_TOKEN)) {
    return { authenticated: true, scope: 'sync' };
  }

  // O TOKEN DO JOGADOR: o verificador da conta, que já existia e nenhuma
  // rota chamava (medição da PS2). Assinado, com validade — e ele traz o
  // accountId, que é o que a autorização por objeto confere.
  const playerToken = verifyAccessToken({
    token,
    secret: environment.ACCESS_TOKEN_SECRET,
    now,
  });
  if (playerToken.valid) {
    return {
      authenticated: true,
      scope: 'player',
      accountId: playerToken.payload.accountId,
    };
  }

  return { authenticated: false };
}

const WRITE_SCOPES: readonly AuthScope[] = ['admin'];

export function canWrite(scope: AuthScope): boolean {
  return WRITE_SCOPES.includes(scope);
}

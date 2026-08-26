// Copyright 2026 Anderson. All Rights Reserved.

export type BanRecord = {
  readonly id: string;
  readonly reason: string;
  /** Nulo é PERMANENTE. Uma sentinela tipo "ano 9999" esconde a intenção e acaba comparada errado. */
  readonly expiresAt: Date | null;
  readonly liftedAt: Date | null;
};

export type BanState =
  | { readonly banned: false }
  | { readonly banned: true; readonly ban: BanRecord };

function isActive(ban: BanRecord, now: Date): boolean {
  if (ban.liftedAt !== null) {
    return false;
  }
  if (ban.expiresAt === null) {
    return true;
  }
  // No instante EXATO da expiração o banimento já acabou.
  return ban.expiresAt.getTime() > now.getTime();
}

/**
 * Havendo mais de um banimento ativo, o MAIS RESTRITIVO vence (DP-mod-02):
 * permanente ganha de temporário; entre temporários, o que termina mais tarde.
 * Sem esta regra, quem decide é a ordem que o banco devolveu — e dois
 * banimentos simultâneos é um caso real (dois moderadores, dois episódios).
 */
export function resolveBanState(bans: readonly BanRecord[], now: Date): BanState {
  let strongest: BanRecord | undefined;

  for (const ban of bans) {
    if (!isActive(ban, now)) {
      continue;
    }
    if (!strongest) {
      strongest = ban;
      continue;
    }
    if (strongest.expiresAt === null) {
      continue;
    }
    if (ban.expiresAt === null || ban.expiresAt.getTime() > strongest.expiresAt.getTime()) {
      strongest = ban;
    }
  }

  return strongest ? { banned: true, ban: strongest } : { banned: false };
}

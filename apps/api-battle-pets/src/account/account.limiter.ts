// Copyright 2026 Anderson. All Rights Reserved.

export const FAILED_LOGIN_MAX_ATTEMPTS = 5;
export const FAILED_LOGIN_WINDOW_SECONDS = 15 * 60;

export type FailedLoginCheck = {
  readonly blocked: boolean;
  readonly retryAfterSeconds: number;
};

/**
 * Limite de tentativas de login POR CONTA (DP-conta-05). A chave é o e-mail
 * normalizado, não o IP: limitar só por IP protege contra um atacante burro e
 * não protege a conta contra um distribuído.
 *
 * LIMITE HONESTO: em memória significa POR PROCESSO. Com mais de uma instância,
 * o teto efetivo multiplica pelo número de instâncias. Mover para Redis é a
 * evolução óbvia quando houver mais de uma — hoje não há.
 */
export class FailedLoginLimiter {
  private readonly attemptsByKey = new Map<string, number[]>();

  constructor(
    private readonly maxAttempts: number = FAILED_LOGIN_MAX_ATTEMPTS,
    private readonly windowSeconds: number = FAILED_LOGIN_WINDOW_SECONDS,
  ) {}

  check(key: string, now: Date): FailedLoginCheck {
    const recent = this.recentAttempts(key, now);
    if (recent.length < this.maxAttempts) {
      return { blocked: false, retryAfterSeconds: 0 };
    }

    const oldest = recent[0]!;
    const retryAfterSeconds = Math.max(
      1,
      Math.ceil(this.windowSeconds - (now.getTime() - oldest) / 1000),
    );
    return { blocked: true, retryAfterSeconds };
  }

  registerFailure(key: string, now: Date): void {
    const recent = this.recentAttempts(key, now);
    recent.push(now.getTime());
    this.attemptsByKey.set(key, recent);
  }

  registerSuccess(key: string): void {
    this.attemptsByKey.delete(key);
  }

  private recentAttempts(key: string, now: Date): number[] {
    const cutoff = now.getTime() - this.windowSeconds * 1000;
    const kept = (this.attemptsByKey.get(key) ?? []).filter((timestamp) => timestamp > cutoff);
    this.attemptsByKey.set(key, kept);
    return kept;
  }
}

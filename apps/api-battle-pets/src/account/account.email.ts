// Copyright 2026 Anderson. All Rights Reserved.

/**
 * Normalização e redação de e-mail.
 *
 * A unicidade da conta é sobre a forma NORMALIZADA (DP-conta-01): sem isso,
 * `Joao@x.com` e `joao@x.com` viram duas contas, e a descoberta acontece com um
 * usuário confuso do outro lado.
 */
export function normalizeEmail(email: string): string {
  return email.trim().toLowerCase();
}

const MASKED_LOCAL_PART_VISIBLE_CHARACTERS = 1;

/**
 * E-mail é PII e nunca vai para log (security.md §1, DP-conta-06). Quando a
 * correlação for indispensável, é esta função — central, nunca formatada no
 * ponto de uso — que decide o que sobra.
 */
export function maskEmail(email: string): string {
  const normalized = normalizeEmail(email);
  const atIndex = normalized.lastIndexOf('@');
  if (atIndex <= 0) {
    return '***';
  }

  const localPart = normalized.slice(0, atIndex);
  const domain = normalized.slice(atIndex + 1);
  const visible = localPart.slice(0, MASKED_LOCAL_PART_VISIBLE_CHARACTERS);
  return `${visible}***@${domain}`;
}

// Copyright 2026 Anderson. All Rights Reserved.

export const PASSWORD_MINIMUM_LENGTH = 12;
export const PASSWORD_MAXIMUM_LENGTH = 200;

export const PasswordPolicyViolation = {
  TOO_SHORT: 'PASSWORD_TOO_SHORT',
  TOO_LONG: 'PASSWORD_TOO_LONG',
  MISSING_LETTER: 'PASSWORD_MISSING_LETTER',
  MISSING_DIGIT: 'PASSWORD_MISSING_DIGIT',
} as const;

export type PasswordPolicyViolation =
  (typeof PasswordPolicyViolation)[keyof typeof PasswordPolicyViolation];

/**
 * Devolve TODOS os motivos de uma vez, nunca só o primeiro (apis.md, Validação):
 * um por vez transforma a correção em tentativa e erro.
 */
export function validatePasswordPolicy(password: string): readonly PasswordPolicyViolation[] {
  const violations: PasswordPolicyViolation[] = [];

  if (password.length < PASSWORD_MINIMUM_LENGTH) {
    violations.push(PasswordPolicyViolation.TOO_SHORT);
  }
  // O teto existe por custo: Argon2id sobre entrada arbitrariamente longa é um
  // vetor de negação de serviço barato de disparar.
  if (password.length > PASSWORD_MAXIMUM_LENGTH) {
    violations.push(PasswordPolicyViolation.TOO_LONG);
  }
  if (!/\p{L}/u.test(password)) {
    violations.push(PasswordPolicyViolation.MISSING_LETTER);
  }
  if (!/\d/.test(password)) {
    violations.push(PasswordPolicyViolation.MISSING_DIGIT);
  }

  return violations;
}

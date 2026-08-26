// Copyright 2026 Anderson. All Rights Reserved.

import { z } from 'zod';

import { PASSWORD_MAXIMUM_LENGTH } from './account.password';

// O schema valida FORMA (é string? cabe?); as REGRAS de senha vivem em
// validatePasswordPolicy. Ter `.min()` aqui fazia o Zod recusar antes, e a
// política — que existe para devolver TODOS os motivos de uma vez — nunca
// rodava. O `.max()` fica: é proteção contra Argon2id sobre entrada gigante,
// e precisa recusar ANTES de hashear.
export const registerAccountSchema = z.object({
  email: z.email('email precisa ser um endereço válido').max(320),
  password: z.string().max(PASSWORD_MAXIMUM_LENGTH),
});

export const loginSchema = z.object({
  email: z.email('email precisa ser um endereço válido').max(320),
  password: z.string().min(1).max(PASSWORD_MAXIMUM_LENGTH),
});

export const refreshSchema = z.object({
  refreshToken: z.string().min(1),
});

export type RegisterAccountInput = z.infer<typeof registerAccountSchema>;
export type LoginInput = z.infer<typeof loginSchema>;
export type RefreshInput = z.infer<typeof refreshSchema>;

// Copyright 2026 Anderson. All Rights Reserved.

import { z } from 'zod';

import { PASSWORD_MAXIMUM_LENGTH, PASSWORD_MINIMUM_LENGTH } from './account.password';

export const registerAccountSchema = z.object({
  email: z.email('email precisa ser um endereço válido').max(320),
  password: z.string().min(PASSWORD_MINIMUM_LENGTH).max(PASSWORD_MAXIMUM_LENGTH),
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

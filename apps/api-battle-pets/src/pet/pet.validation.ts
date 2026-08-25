// Copyright 2026 Anderson. All Rights Reserved.

import { z } from 'zod';

// apis.md, Validação: "Retornar todos os erros de validação de uma vez,
// não só o primeiro" — Zod já faz isso por padrão em .safeParse(); o
// ponto de atenção é NÃO usar .parse() (lança na primeira falha).

const attributeSchema = z
  .number()
  .int('Deve ser um número inteiro')
  .nonnegative('Não pode ser negativo');

export const createPetSchema = z.object({
  name: z.string().min(1, 'Nome do pet é obrigatório').max(80),
  type: z.string().min(1, 'Tipo do pet é obrigatório').max(40),
  attack: attributeSchema,
  defense: attributeSchema,
  speed: attributeSchema,
  maxHealth: attributeSchema,
});

export const updatePetSchema = createPetSchema;

export const listPetsQuerySchema = z.object({
  page: z.coerce.number().int().positive().default(1),
  perPage: z.coerce.number().int().positive().max(100).default(20),
  updatedAfter: z.iso.datetime().optional(),
});

export type CreatePetInput = z.infer<typeof createPetSchema>;
export type UpdatePetInput = z.infer<typeof updatePetSchema>;
export type ListPetsQuery = z.infer<typeof listPetsQuerySchema>;

// Formato do envelope de erro (apis.md): { error: { code, message } } para
// falha única, ou uma lista de campos para falha de validação com múltiplos erros.
export type FieldError = { field: string; code: string; message: string };

// Código estável derivado de campo + forma real do erro do Zod — nunca do
// texto da mensagem (que é para humano ler, não para o cliente comparar por
// string). "too_small" em string vazia (origin: "string") é campo ausente;
// "too_small" em número abaixo do mínimo (origin: "number") é valor
// inválido — são causas diferentes e precisam de códigos diferentes.
function deriveErrorCode(field: string, issue: z.core.$ZodIssue): string {
  const fieldSlug = field.toUpperCase() || 'PAYLOAD';

  if (issue.code === 'invalid_type') {
    return `PET_${fieldSlug}_REQUIRED`;
  }
  if (issue.code === 'too_small' && issue.origin === 'string') {
    return `PET_${fieldSlug}_REQUIRED`;
  }
  return `PET_${fieldSlug}_INVALID`;
}

export function formatValidationErrors(error: z.ZodError): FieldError[] {
  return error.issues.map((issue) => ({
    field: issue.path.join('.'),
    code: deriveErrorCode(issue.path.join('.'), issue),
    message: issue.message,
  }));
}

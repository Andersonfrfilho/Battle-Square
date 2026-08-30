// Copyright 2026 Anderson. All Rights Reserved.

import { z } from 'zod';

// apis.md, Validação: "Retornar todos os erros de validação de uma vez,
// não só o primeiro" — Zod já faz isso por padrão em .safeParse(); o
// ponto de atenção é NÃO usar .parse() (lança na primeira falha).

const attributeSchema = z
  .number()
  .int('Deve ser um número inteiro')
  .nonnegative('Não pode ser negativo');

// Golpe de pet: nome, poder e o que ele deixa na casa que acertou.
//
// O conjunto de efeitos é FECHADO (enum do Zod, e CHECK no banco): efeito
// desconhecido faria o núcleo escolher entre ignorar em silêncio ou recusar a
// batalha, e as duas saídas são piores que barrar na escrita.
export const petMoveSchema = z.object({
  name: z.string().min(1, 'Nome do golpe é obrigatório').max(60),
  power: z.number().int().min(1, 'Poder do golpe precisa ser ao menos 1').max(500),
  terrainEffect: z.enum(['none', 'water', 'damage', 'shallow_water', 'ice', 'mud']).default('none'),
  /**
   * Atributo exigido para usar o golpe, e o mínimo dele.
   *
   * Conjunto FECHADO pelo mesmo motivo do efeito de terreno: requisito com
   * nome errado nunca tranca nada (o jogo trata desconhecido como "sem
   * requisito", que é a escolha segura lá), então o erro passaria despercebido
   * — o golpe simplesmente ficaria livre para todos, e ninguém notaria.
   * Barrar na escrita é o único lugar em que isso aparece.
   */
  requiresAttribute: z
    .enum(['none', 'musculature', 'personality', 'camouflage', 'flight', 'underground'])
    .default('none'),
  requiresValue: z.number().int().min(0).max(999).default(0),
  /**
   * Magia de atributo. Conjunto FECHADO pelo mesmo motivo dos outros: o jogo
   * trata nome desconhecido como "sem efeito", entao um erro de cadastro
   * produziria um golpe que simplesmente nao faz nada — e a escrita e o unico
   * lugar em que isso aparece.
   *
   * O teto de 60 e o mesmo do nucleo (BattleStatEffectMaxPercent), e ele
   * recorta de novo la: amarra de jogo nao e acordo entre camadas.
   */
  effectStat: z.enum(['none', 'attack', 'defense', 'speed']).default('none'),
  effectPercent: z.number().int().min(-60).max(60).default(0),

  /**
   * O teto de 5 e o mesmo do tradutor, e ele recorta de novo la: gelo que
   * atravessa varios turnos deixa de ser jogada e vira mudanca de arena, e
   * quem decide arena e a montagem, nao um golpe.
   */
  terrainDuration: z.number().int().min(0).max(5).default(0),
});

export const createPetSchema = z.object({
  name: z.string().min(1, 'Nome do pet é obrigatório').max(80),
  type: z.string().min(1, 'Tipo do pet é obrigatório').max(40),
  attack: attributeSchema,
  defense: attributeSchema,
  speed: attributeSchema,
  maxHealth: attributeSchema,
  // ATÉ quatro, e opcional: pet sem golpe continua válido enquanto a migração
  // não termina, e exigir quatro quebraria todo cadastro existente.
  moves: z.array(petMoveSchema).max(4, 'Um pet tem no máximo 4 golpes').optional(),
});

export const updatePetSchema = createPetSchema;

export const listPetsQuerySchema = z.object({
  page: z.coerce.number().int().positive().default(1),
  perPage: z.coerce.number().int().positive().max(100).default(20),
  updatedAfter: z.iso.datetime().optional(),
});

export type CreatePetInput = z.infer<typeof createPetSchema>;

/**
 * O que se ESCREVE ao declarar um pet, antes de o schema aplicar os padrões.
 *
 * `CreatePetInput` é a SAÍDA do parse: ali todo campo com `.default()` já é
 * obrigatório, porque depois de validar ele sempre existe. Usar esse tipo para
 * literal escrito à mão — a semente do catálogo — obriga a repetir cada padrão
 * em cada golpe, e transforma toda opção nova com padrão numa edição de
 * arquivo inteiro.
 */
export type CreatePetDeclaration = z.input<typeof createPetSchema>;
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

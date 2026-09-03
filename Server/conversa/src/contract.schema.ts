/**
 * Copyright 2026 Anderson. All Rights Reserved. — Ada Technology
 *
 * O CONTRATO DO JOGO — espelho de `NpcDialogue.h`, que é a fonte da verdade.
 *
 * A resposta é `{npcSays}` seco, sem envelope: o formato é do jogo, e o jogo
 * já foi lançado com ele. Quem manda no contrato é quem não pode atualizar
 * junto — e aqui é o cliente.
 *
 * Validar na fronteira é o que impede um digesto malformado de virar prompt:
 * entrada hostil não toca o modelo (security.md §3 — payload de fora é
 * hostil até o zod dizer o contrário).
 */
import { z } from 'zod';

export const dialogueRequestSchema = z.object({
  npc: z.object({
    name: z.string().min(1).max(80),
    storyStage: z.string().max(400),
    meetings: z.number().int().min(0).max(100000),
    homeStartHour: z.number().int().min(0).max(23),
  }),
  deeds: z.object({
    beatChampion: z.boolean(),
    soldAPet: z.boolean(),
    hasWaterPet: z.boolean(),
  }),
  facts: z.array(z.string().max(300)).max(64),

  /** Teto curto: fala de jogador é frase, não redação — e prompt não cresce. */
  playerSays: z.string().min(1).max(500),
});

export type DialogueRequest = z.infer<typeof dialogueRequestSchema>;

export const dialogueResponseSchema = z.object({
  npcSays: z.string().min(1),
});

export type DialogueResponse = z.infer<typeof dialogueResponseSchema>;

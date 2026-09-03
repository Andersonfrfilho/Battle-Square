/**
 * Copyright 2026 Anderson. All Rights Reserved. — Ada Technology
 *
 * O ambiente validado no boot, num lugar só (nodejs.md): variável esquecida
 * derruba o processo AGORA, com o nome dela — nunca no primeiro pedido, com
 * um stack trace de fetch.
 */
import { z } from 'zod';

const environmentSchema = z.object({
  /** Onde ESTE servidor escuta. É a URL que o jogador aponta no bs.Conversa. */
  PORT: z.coerce.number().default(8080),

  /**
   * O modelo, em API OpenAI-compatível (chat completions). O llama-server
   * local expõe isso nativamente; a infra (vLLM e afins) também — e é por
   * isso que este servidor serve os DOIS modos com o mesmo código.
   */
  MODEL_URL: z
    .string()
    .url()
    .default('http://localhost:8081/v1/chat/completions'),

  /** O nome do modelo, quando o backend exige um. llama-server ignora. */
  MODEL_NAME: z.string().default('local'),

  /**
   * Mais folgado que os 3 s do jogo, de propósito: quem corta é o JOGO — o
   * fallback restrito é dele. Cortar aqui primeiro só esconderia do log
   * quanto o modelo realmente demora.
   */
  MODEL_TIMEOUT_MS: z.coerce.number().default(8000),
});

export const environment = environmentSchema.parse(process.env);

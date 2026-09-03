/**
 * Copyright 2026 Anderson. All Rights Reserved. — Ada Technology
 *
 * O SERVIDORZINHO DE REFERÊNCIA da conversa dinâmica (decisão 67).
 *
 * Uma rota, um papel: traduzir o contrato do jogo (`NpcDialogue.h`) para a
 * API OpenAI-compatível que o modelo fala — o llama-server local e a infra
 * (vLLM e afins) expõem a MESMA, e é isso que faz um código só servir os
 * dois modos.
 *
 * Falhar aqui é BARATO por desenho: o jogo tem o modo restrito como
 * para-quedas, então todo erro responde depressa e com status — segurar o
 * pedido seria pior que recusa-lo.
 */
import { environment } from './config/environment';
import { dialogueRequestSchema } from './contract.schema';
import { buildChatMessages } from './promptBuilder';

const DIALOGUE_ROUTE = '/fala';

type ModelChatResponse = {
  choices?: { message?: { content?: string } }[];
};

async function askModel(messages: ReturnType<typeof buildChatMessages>): Promise<string> {
  const response = await fetch(environment.MODEL_URL, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({
      model: environment.MODEL_NAME,
      messages,
      // Curto e morno: vizinho de vila, nao contador de epopeia.
      max_tokens: 160,
      temperature: 0.8,
    }),
    signal: AbortSignal.timeout(environment.MODEL_TIMEOUT_MS),
  });

  if (!response.ok) {
    throw new Error(`modelo respondeu ${response.status}`);
  }

  const payload = (await response.json()) as ModelChatResponse;
  const content = payload.choices?.[0]?.message?.content?.trim();
  if (!content) {
    throw new Error('modelo respondeu vazio');
  }

  return content;
}

export const server = Bun.serve({
  port: environment.PORT,
  async fetch(request: Request): Promise<Response> {
    const url = new URL(request.url);
    if (request.method !== 'POST' || url.pathname !== DIALOGUE_ROUTE) {
      return Response.json(
        { error: { code: 'NOT_FOUND', message: `use POST ${DIALOGUE_ROUTE}` } },
        { status: 404 });
    }

    // A fronteira: digesto malformado nao vira prompt. O jogo bem-comportado
    // nunca cai aqui; quem cai e curl errado ou versao velha — e o 400 diz.
    const parsed = dialogueRequestSchema.safeParse(await request.json().catch(() => null));
    if (!parsed.success) {
      return Response.json(
        { error: { code: 'INVALID_DIGEST', message: parsed.error.message } },
        { status: 400 });
    }

    try {
      return Response.json({ npcSays: await askModel(buildChatMessages(parsed.data)) });
    } catch (unknownError) {
      // O erro do modelo vira 502 seco: o jogo cai no restrito sozinho, e o
      // detalhe fica no log do servidor — nunca na resposta (security.md §1).
      console.error(JSON.stringify({
        level: 'error',
        source: 'conversa-server',
        message: unknownError instanceof Error ? unknownError.message : 'erro desconhecido',
      }));
      return Response.json(
        { error: { code: 'MODEL_UNAVAILABLE', message: 'o modelo nao respondeu' } },
        { status: 502 });
    }
  },
});

console.log(JSON.stringify({
  level: 'info',
  source: 'conversa-server',
  message: `escutando em http://localhost:${environment.PORT}${DIALOGUE_ROUTE}, modelo em ${environment.MODEL_URL}`,
}));

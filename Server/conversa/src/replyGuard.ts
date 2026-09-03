/**
 * Copyright 2026 Anderson. All Rights Reserved. — Ada Technology
 *
 * A GUARDA DA SAÍDA — a fronteira dura contra a alucinação de segredo.
 *
 * Medido em 03/09, com um qwen 3B de verdade: pressionado com "eu sei que
 * você sabe", o modelo INVENTOU um endereço de mercado-negro. O digesto não
 * tinha nada — não foi vazamento, foi confabulação — mas para o jogador dá no
 * mesmo: o NPC apontou o que a carta não aponta.
 *
 * Regra de prompt não segura modelo pequeno sob pressão social. Esta guarda
 * segura QUALQUER modelo: o digesto nunca contém segredo, então toda resposta
 * que menciona um é invenção POR DEFINIÇÃO — e invenção não sai do servidor.
 */

/**
 * O léxico proibido na saída. Curto de propósito: cada termo aqui é algo que
 * NUNCA está no digesto — mencioná-lo é prova de invenção, sem falso
 * positivo possível.
 */
const FORBIDDEN_REPLY_TERMS: readonly string[] = [
  'mercado-negro',
  'mercado negro',
  'lugar escondido',
  'lugares escondidos',
];

/** A recusa, em personagem — a mesma honestidade do modo restrito do jogo. */
const DEFLECTION =
  'disso eu nao sei falar, e quem sabe nao conta... me pergunta outra coisa?';

export function guardReply(npcSays: string): string {
  const normalized = npcSays.toLowerCase();

  return FORBIDDEN_REPLY_TERMS.some((term) => normalized.includes(term))
    ? DEFLECTION
    : npcSays;
}

/**
 * Copyright 2026 Anderson. All Rights Reserved. — Ada Technology
 *
 * O PROMPT — puro, e é a peça que carrega as regras da casa para dentro do
 * modelo. O digesto é tudo que o modelo sabe; o prompt é como ele é obrigado
 * a se comportar com o que sabe.
 *
 * Puro de propósito (recebe o pedido, devolve mensagens): é a única parte
 * com regra de verdade, então é a parte que tem teste — o resto é fio.
 */
import type { DialogueRequest } from './contract.schema';

export type ChatMessage = {
  readonly role: 'system' | 'user';
  readonly content: string;
};

/** O que o morador chama o jogador conforme a intimidade — espelha o jogo. */
function treatmentForMeetings(meetings: number): string {
  if (meetings <= 1) return 'um desconhecido que acabou de chegar';
  if (meetings <= 3) return 'um conhecido que ja te visitou algumas vezes';
  return 'um amigo que te visita sempre';
}

export function buildChatMessages(request: DialogueRequest): ChatMessage[] {
  const { npc, deeds, facts, playerSays } = request;

  const deedLines = [
    deeds.beatChampion ? '- ele ja VENCEU o campeao da Arena' : null,
    deeds.soldAPet ? '- ele ja vendeu um pet no Mercado' : null,
    deeds.hasWaterPet ? '- o pet que anda com ele e de AGUA' : null,
  ].filter((line): line is string => line !== null);

  // As regras, numeradas e curtas: modelo pequeno obedece lista, não ensaio.
  const system = [
    `Voce e ${npc.name}, morador(a) de uma vila num mundo de pets e batalhas.`,
    `Quem fala com voce e ${treatmentForMeetings(npc.meetings)}.`,
    `Sua historia pessoal (o que voce anda contando): "${npc.storyStage}".`,
    `Voce costuma estar em casa por volta das ${npc.homeStartHour}h.`,
    '',
    'O que voce sabe sobre o visitante:',
    ...(deedLines.length > 0 ? deedLines : ['- nada de especial ainda']),
    '',
    'FATOS DO MUNDO — a unica fonte do que voce afirma sobre o jogo:',
    ...facts.map((fact) => `- ${fact}`),
    '',
    'REGRAS, e elas mandam em voce:',
    '1. Responda EM PERSONAGEM, em portugues simples de gente do interior.',
    '2. CURTO: uma a tres frases. Vizinho não discursa.',
    '3. Sobre o JOGO, afirme SOMENTE o que esta nos fatos acima.',
    '4. Se perguntarem algo fora dos fatos, admita no seu jeito',
    '   ("disso eu nao sei falar...") e puxe para o que voce sabe.',
    '5. NUNCA invente mecanica, lugar, preco ou segredo.',
    '6. Voce NUNCA OUVIU FALAR de mercado-negro nem de lugares escondidos.',
    '   Se insistirem, diga que nao sabe do que estao falando.',
    '7. Nada de listas, markdown ou tom de assistente: voce e uma pessoa.',
  ].join('\n');

  return [
    { role: 'system', content: system },
    { role: 'user', content: playerSays },
  ];
}

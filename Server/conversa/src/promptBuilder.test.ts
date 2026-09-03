/**
 * Copyright 2026 Anderson. All Rights Reserved. — Ada Technology
 *
 * O prompt é a única peça com regra — então é a peça com teste. As duas
 * fronteiras dele são as mesmas do digesto no jogo: o que PRECISA estar
 * (identidade, feitos, fatos, regras) e o que NUNCA pode aparecer.
 */
import { describe, expect, test } from 'bun:test';

import { dialogueRequestSchema, type DialogueRequest } from './contract.schema';
import { buildChatMessages } from './promptBuilder';

function requestForTest(overrides: Partial<DialogueRequest> = {}): DialogueRequest {
  return {
    npc: {
      name: 'Dona Iraci',
      storyStage: 'guardo as sementes da primeira colheita',
      meetings: 4,
      homeStartHour: 14,
    },
    deeds: { beatChampion: true, soldAPet: false, hasWaterPet: false },
    facts: ['o Centro de Recuperacao cura de graca'],
    playerSays: 'como vai a lavoura?',
    ...overrides,
  };
}

describe('buildChatMessages', () => {
  test('o sistema carrega identidade, feitos, fatos e as regras', () => {
    const [system, user] = buildChatMessages(requestForTest());

    expect(system?.role).toBe('system');
    expect(system?.content).toContain('Dona Iraci');
    expect(system?.content).toContain('sementes da primeira colheita');
    expect(system?.content).toContain('VENCEU o campeao');
    expect(system?.content).toContain('cura de graca');

    // As regras que fazem o modelo pequeno se comportar: só os fatos, e
    // admitir o que não sabe — o mesmo contrato do modo restrito.
    expect(system?.content).toContain('SOMENTE o que esta nos fatos');
    expect(system?.content).toContain('disso eu nao sei falar');

    expect(user?.role).toBe('user');
    expect(user?.content).toBe('como vai a lavoura?');
  });

  test('a intimidade acompanha as visitas, como no jogo', () => {
    const [primeira] = buildChatMessages(
      requestForTest({ npc: { ...requestForTest().npc, meetings: 1 } }));
    const [amigo] = buildChatMessages(requestForTest());

    expect(primeira?.content).toContain('desconhecido');
    expect(amigo?.content).toContain('amigo');
  });

  test('feito ausente nao vira frase — mentir sobre o visitante e pior que nada', () => {
    const [system] = buildChatMessages(
      requestForTest({ deeds: { beatChampion: false, soldAPet: false, hasWaterPet: false } }));

    expect(system?.content).not.toContain('VENCEU');
    expect(system?.content).toContain('nada de especial ainda');
  });
});

describe('dialogueRequestSchema — a fronteira', () => {
  test('o digesto do jogo passa', () => {
    expect(dialogueRequestSchema.safeParse(requestForTest()).success).toBe(true);
  });

  test('fala vazia e fala-redacao sao recusadas na porta', () => {
    expect(dialogueRequestSchema.safeParse(
      requestForTest({ playerSays: '' })).success).toBe(false);
    expect(dialogueRequestSchema.safeParse(
      requestForTest({ playerSays: 'a'.repeat(501) })).success).toBe(false);
  });
});

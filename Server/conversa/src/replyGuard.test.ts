/**
 * Copyright 2026 Anderson. All Rights Reserved. — Ada Technology
 */
import { describe, expect, test } from 'bun:test';

import { guardReply } from './replyGuard';

describe('guardReply — a fronteira dura', () => {
  test('a resposta que aponta segredo morre em personagem', () => {
    // O caso REAL, medido: o qwen 3B inventou este endereço sob pressão.
    const invented =
      'O mercado-negro está no lado oposto da vila, perto do rio.';

    const guarded = guardReply(invented);
    expect(guarded).not.toContain('mercado');
    expect(guarded).toContain('nao sei falar');
  });

  test('as variantes tambem: sem hifen, e em qualquer caixa', () => {
    expect(guardReply('O Mercado Negro fica ali')).toContain('nao sei falar');
    expect(guardReply('conheco uns LUGARES ESCONDIDOS...')).toContain('nao sei falar');
  });

  test('a conversa inocente passa INTACTA — guarda que poda demais e mordaça', () => {
    // "Mercado" sozinho é prédio legítimo do jogo: o léxico proibido é só o
    // que NUNCA está no digesto, sem falso positivo possível.
    const innocent = 'pet capturado se vende no Mercado, e a lavoura vai bem!';
    expect(guardReply(innocent)).toBe(innocent);
  });
});

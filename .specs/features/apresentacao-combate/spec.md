# Apresentação do Combate — Especificação

**Status:** ✅ Aprovada — design e tasks.md também aprovados (2026-08-25)
**Depende de:** Combate Núcleo (concluído) e Backend de Dados de Pet (concluído) — esta feature é a primeira coisa que um jogador realmente vê e toca.

---

## Problem Statement

O núcleo de combate resolve turnos e produz um trace de eventos; o backend entrega pets reais. Nenhum dos dois é jogável ainda — não existe forma de um jogador **escolher** uma ação, nem de **ver** o resultado. Esta feature fecha os dois requisitos que ficaram pendentes da spec do Combate Núcleo (BTL-02, P7/BTL-22) e é a primeira vez que a direção de arte (AD-003, estilo Link's Awakening — low-poly, material fosco) aparece de verdade em tela.

O problema de design mais duro, sinalizado desde a decisão de arquitetura da ação (AD-009): uma ação é o par **(Tipo, Direção)** — 6 tipos × 8 direções. Uma UI ingênua de "um botão por combinação" vira 12+ botões na tela, ilegível em mobile.

## Goals

- [ ] Um jogador consegue montar as 3 ações de um turno e confirmar, sem ver nada do oponente
- [ ] A resolução do turno se lê como uma sequência de acontecimentos, não como uma tabela de números
- [ ] A câmera e a arena comunicam a direção de arte decidida (AD-003) desde o primeiro frame
- [ ] Nenhum número exibido na tela é calculado pela apresentação — todos vêm do trace (BTL-22, já fechado na spec do núcleo)

## Out of Scope

| Item | Razão |
|---|---|
| Multiplayer real / rede | M2 — esta feature roda contra um oponente local/IA burra por enquanto |
| Seleção de pet / equipe | Cobertura do backend de dados já existe; montar o "menu de time" é conteúdo, não apresentação de combate |
| Menu principal, progressão, telas fora da arena | Fora do escopo de uma tela de batalha |
| Suporte a gamepad | Mouse/teclado (PC) e toque (mobile) — gamepad é M6 |
| Efeitos visuais finais (partículas, pós-processamento) | Placeholder visual é aceitável; polimento é depois do combate provado divertido |

---

## User Stories

### P1: Selecionar uma ação sem virar 12 botões ⭐ MVP

**User Story:** Como jogador, quero escolher tipo e direção de uma ação de forma rápida e legível, para montar meu turno sem a UI virar ruído visual.

**Why P1:** é o problema de design mais arriscado da feature — sinalizado desde AD-009. Sem resolver isso, nada mais importa.

**Acceptance Criteria:**
1. WHEN o jogador escolhe um tipo que precisa de direção (Mover, Atacar, Magia) THEN o sistema SHALL pedir a direção em seguida, num seletor de 8 posições
2. WHEN o jogador escolhe um tipo que não usa direção (Defender, Esquivar, Aguardar) THEN o sistema SHALL confirmar a ação imediatamente, sem pedir direção
3. WHEN o jogador está escolhendo a direção e muda de ideia THEN o sistema SHALL permitir voltar e trocar o tipo, sem perder as ações já confirmadas
4. WHEN a fila já tem 3 ações (BTL-01) THEN o sistema SHALL desabilitar a seleção de tipo, com indicação visual clara de "3/3"
5. WHEN a tela é menor que um limiar mobile THEN o sistema SHALL manter os alvos de toque em tamanho mínimo acessível (44pt), sem sobreposição

**Independent Test:** montar as 3 ações de um turno, incluindo ao menos uma de cada categoria (com e sem direção), sem consultar documentação.

---

### P1: Commit esconde a intenção do oponente ⭐ MVP (fecha BTL-02)

**User Story:** Como jogador, quero que minha fila fique travada e oculta depois de confirmar, para que a leitura de intenção do combate às cegas não seja violada pela própria UI.

**Acceptance Criteria:**
1. WHEN o jogador confirma o commit THEN o sistema SHALL travar a fila e desabilitar novas seleções (BTL-01, critério 5)
2. WHEN o commit está travado THEN o sistema SHALL mostrar um estado de espera ("aguardando oponente"), nunca as ações do oponente
3. WHEN os dois lados confirmam THEN o sistema SHALL disparar a resolução do turno automaticamente

---

### P1: Câmera e arena comunicam a direção de arte ⭐ MVP

**User Story:** Como jogador, quero que a arena 3x3 seja legível de cima e visualmente coerente com a proposta de arte do projeto, para que a leitura tática (onde cada pet está) nunca seja um problema de câmera.

**Acceptance Criteria:**
1. WHEN a cena carrega THEN o sistema SHALL posicionar a câmera de forma que as 9 casas da arena fiquem visíveis e legíveis, sem sobreposição
2. WHEN um pet ocupa uma casa THEN o sistema SHALL deixar claro visualmente qual casa é aquela (grade visível, não implícita)
3. WHEN a arena é renderizada THEN o sistema SHALL usar material fosco e proporções de brinquedo (AD-003) — nenhuma textura de alta resolução nem PBR complexo

---

### P1: Trace vira animação legível ⭐ MVP (fecha P7/BTL-22 do núcleo)

Já especificado com critérios de aceite completos na spec do Combate Núcleo (P7). Esta feature é quem **implementa** aqueles 4 critérios — não redefine nada, só entrega:
1. Eventos animados na ordem do trace, respeitando fronteira de slot
2. Eventos da mesma fase, simultâneos
3. Todo número exibido vem do trace
4. Pular a animação salta ao estado final sem divergir

**Acceptance Criteria adicionais, específicos de apresentação:**
5. WHEN um evento `AtaqueAcertou`/`DanoAplicado` chega THEN o sistema SHALL mostrar o dano como número flutuante e atualizar a barra de vida
6. WHEN um evento `MovimentoBloqueado` chega THEN o sistema SHALL mostrar um feedback claro de "não pode" (não apenas silêncio)
7. WHEN um evento `PetMorreu` chega THEN o sistema SHALL remover ou marcar visualmente o pet como derrotado
8. WHEN um evento `BatalhaEncerrada` chega THEN o sistema SHALL mostrar o resultado (vitória/derrota/empate) lido do `Value` do evento, nunca recalculado

---

### P2: Feedback de ação inválida no momento da escolha

**User Story:** Como jogador, quero saber antes de confirmar se uma ação não vai fazer nada (ex.: atacar para um lado vazio), para não perder o turno por engano.

**Why P2:** melhora a experiência, mas o jogo funciona sem isso — o trace já mostra `AtaqueErrou` depois. Antecipar na UI é qualidade de vida, não bloqueio.

**Acceptance Criteria:**
1. WHEN o jogador aponta uma direção sem alvo alcançável THEN o sistema SHALL indicar visualmente (não bloquear — o jogador pode confirmar mesmo assim, é uma decisão válida)

---

## Edge Cases

- WHEN o jogador tenta confirmar o commit com menos de 3 ações THEN o sistema SHALL completar automaticamente com Aguardar (mesma regra do núcleo, BTL-01/BTL-20) e avisar visualmente antes de confirmar
- WHEN a resolução do turno está em andamento (fila travada dos dois lados) THEN o sistema SHALL impedir qualquer input de ação, mesmo que a animação ainda não tenha começado
- WHEN o jogador pula a animação no meio de uma morte mútua THEN o sistema SHALL mostrar os dois pets já mortos no estado final, sem exigir que a animação de morte tenha rodado
- WHEN dois eventos da mesma fase afetam o MESMO pet (ex.: dano de dois atacantes na coabitação) THEN o sistema SHALL empilhar os números flutuantes de dano, nunca sobrepor ilegivelmente

---

## Requirement Traceability

| ID | Story | Fase | Status |
|---|---|---|---|
| PRES-01 | P1: Seleção em 2 passos (tipo, depois direção se aplicável) | Specify | Pending |
| PRES-02 | P1: Trocar tipo sem perder ações já confirmadas | Specify | Pending |
| PRES-03 | P1: Indicação visual de 3/3 | Specify | Pending |
| PRES-04 | P1: Alvo de toque mínimo acessível em mobile | Specify | Pending |
| PRES-05 | P1: Commit trava e esconde (fecha BTL-02) | Specify | Pending |
| PRES-06 | P1: Câmera legível da arena 3x3 | Specify | Pending |
| PRES-07 | P1: Grade visualmente explícita | Specify | Pending |
| PRES-08 | P1: Material fosco, sem PBR complexo (AD-003) | Specify | Pending |
| PRES-09 | P1: Trace anima em ordem, por fase (fecha P7 do núcleo) | Specify | Pending |
| PRES-10 | P1: Pular animação sem divergir do estado final | Specify | Pending |
| PRES-11 | P1: Dano flutuante + barra de vida | Specify | Pending |
| PRES-12 | P1: Feedback de movimento bloqueado | Specify | Pending |
| PRES-13 | P1: Feedback de morte | Specify | Pending |
| PRES-14 | P1: Resultado final lido do trace | Specify | Pending |
| PRES-15 | P2: Indicação de ataque sem alvo, sem bloquear | Specify | Pending |

**Cobertura:** 15 requisitos, 0 mapeados para tarefas.

---

## Success Criteria

- [ ] Um jogador novo monta um turno completo (3 ações, incluindo ao menos uma com direção) em menos de 20 segundos, sem instrução externa
- [ ] Uma pessoa observando a animação consegue narrar o que aconteceu no turno sem olhar números crus
- [ ] Zero número exibido na tela é recalculado fora do trace — auditável pela mesma disciplina de sonda usada no núcleo (grep por qualquer fórmula de dano fora de `BattleSim`)
- [ ] Testado em pelo menos uma resolução mobile (375px) e uma desktop, sem sobreposição de UI

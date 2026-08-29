# Moderação e Banimento — Especificação

**Status:** Concluída e verificada por teste (status corrigido em 29/08/2026 — a linha dizia "Draft" enquanto a feature já rodava)
**Depende de:** Conta de Jogador (primeira feature de M7, ✅ concluída) — banir exige saber quem reconhecer amanhã. Segunda e última feature de M7.

---

## Problem Statement

`AD-017` decidiu, em 2026-08-25, que adulterar o cache local de pets **não muda resultado nenhum** — o servidor é autoritativo (AD-005), então o cliente adulterado só sinaliza intenção hostil. A resposta escolhida foi consequência de **sessão**: encerra aquela sessão, o oponente vence por W.O.

E a mesma decisão registrou, com todas as letras, o que ficava de fora e por quê:

> *"Banimento persistente entre partidas fica fora de escopo até existir conta de jogador. (…) A trilha de auditoria do Nível 1 é desenhada para ser reaproveitada por um sistema de moderação futuro, sem precisar ser reconstruída quando conta de jogador existir."*

**A conta de jogador agora existe.** Esta feature é o Nível 2 que aquela decisão prometeu: transformar evidência de sessão em consequência que sobrevive ao fim dela.

**O que ela deliberadamente não é:** um sistema antitrapaça. AD-005 já garante que trapaça não funciona. Isto é sobre **consequência para quem tenta**, e sobre uma trilha que uma pessoa possa auditar depois — não sobre impedir um resultado que a arquitetura já protege.

## Goals

- [ ] Um evento de moderação (evidência de adulteração) pode ser registrado contra uma conta, com trilha auditável
- [ ] Uma conta pode ser banida por tempo determinado ou permanentemente, e o banimento sobrevive ao fim da sessão
- [ ] Uma conta banida é recusada ao usar a API, com código estável e o momento em que o banimento termina
- [ ] Um banimento pode ser **levantado** — engano de moderação é um estado real e precisa de saída
- [ ] Toda ação de moderação registra **quem** a executou, quando e sobre quem (`security.md` §10)
- [ ] A decisão "esta conta está banida agora?" é pura e testável sem banco

## Out of Scope

| Item | Razão |
|---|---|
| Detectar trapaça nova (heurística, análise de comportamento) | Isto consome a evidência que AD-017 já definiu. Inventar detecção nova é outro projeto, e um que exige dados de produção que não existem |
| Painel de moderação (UI) | Autoria visual, adiada desde `apresentacao-combate`. A API é o contrato; quem consome é decisão de outra hora |
| Banir por IP ou por dispositivo | IP é compartilhado e muda; dispositivo exige fingerprint, que é coleta de dado pessoal sem base legal declarada (LGPD). Conta é o que existe e é o que se bane |
| Apelação, prazo de recurso, notificação ao jogador | Processo de produto, não de engenharia. O "levantar banimento" desta feature é a primitiva que qualquer processo desses vai usar |
| Cliente C++ reportando adulteração à API | Mesma razão da feature anterior: a integração Unreal↔conta não foi decidida. A API entrega o endpoint e o contrato |

---

## User Stories

### P1: Registrar evidência contra uma conta ⭐ MVP

**User Story:** Como dono do serviço, quero que uma evidência de adulteração fique registrada contra a conta, para que a decisão de banir seja tomada sobre histórico e não sobre um episódio isolado.

**Acceptance Criteria:**
1. WHEN um evento de moderação é registrado THEN o sistema SHALL persistir conta, tipo do evento, momento e quem registrou
2. WHEN o evento é registrado THEN o sistema SHALL **não** aplicar banimento automaticamente — registrar e punir são passos separados
3. WHEN alguém consulta o histórico de uma conta THEN o sistema SHALL devolver os eventos em ordem, para uma pessoa poder julgar

**Independent Test:** o contrato do endpoint e a exigência de escopo administrativo; a ordenação é verificada contra o banco no roteiro manual.

---

### P1: Banir e levantar ⭐ MVP

**User Story:** Como moderador, quero banir uma conta por um prazo ou para sempre, e poder desfazer, para que a consequência exista sem ser irreversível quando eu errar.

**Acceptance Criteria:**
1. WHEN um banimento é criado com prazo THEN o sistema SHALL expirá-lo sozinho quando o prazo passar, sem ninguém precisar agir
2. WHEN um banimento é criado sem prazo THEN o sistema SHALL tratá-lo como permanente
3. WHEN um banimento é levantado THEN o sistema SHALL deixar de recusar a conta imediatamente, e SHALL manter o registro do banimento levantado — histórico não some
4. WHEN qualquer ação de moderação acontece THEN o sistema SHALL registrar o autor dela

**Independent Test:** a resolução "esta conta está banida em tal instante?" é pura, com o tempo injetado — banimento expirado não bane, levantado não bane, permanente bane sempre, e o mais restritivo vence quando há mais de um.

---

### P1: A conta banida é recusada ⭐ MVP

**User Story:** Como dono do serviço, quero que a conta banida não consiga usar a API, para que o banimento seja consequência real e não uma anotação.

**Acceptance Criteria:**
1. WHEN uma conta banida tenta autenticar THEN o sistema SHALL recusar com `403` e código estável `ACCOUNT_BANNED`
2. WHEN o banimento tem prazo THEN a resposta SHALL informar quando ele termina
3. WHEN a conta banida apresenta um token de acesso ainda válido THEN o sistema SHALL recusá-lo também — o banimento **não** espera o token expirar

**Independent Test:** o critério 3 é o que separa banimento real de banimento decorativo, e é verificável no roteiro contra banco real.

---

## Decisões Pendentes

**PROPOSTA** — mudam se você discordar.

1. **Registrar e punir são passos separados.** PROPOSTA: evidência não vira banimento automático. Automatizar puniria falso positivo sem ninguém ver, e AD-017 já diz que a adulteração **não muda resultado** — logo não há urgência que justifique automatizar.
2. **Banimento é por conta, e a conta é opcional.** PROPOSTA: quem joga sem conta não é banível, e isso é aceito. Obrigar conta para jogar sozinho seria cobrar fricção de todo mundo por causa de uma minoria hostil que a arquitetura já neutraliza.
3. **O histórico nunca é apagado.** PROPOSTA: levantar um banimento marca `lifted_at`, não remove a linha. Uma trilha de auditoria que pode ser apagada não é trilha de auditoria.

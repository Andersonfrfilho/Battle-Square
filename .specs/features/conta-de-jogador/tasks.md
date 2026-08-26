# Conta de Jogador — Tarefas

**Design:** `.specs/features/conta-de-jogador/design.md`
**Status:** Draft — aguarda aprovação
**Escopo:** `apps/api-battle-pets`. Unreal não é tocada.

---

## Plano de Execução

> 🤖 Modelo: `sonnet` — **T2 é 🧠** (token: assinatura, comparação em tempo constante e ordem verificar-antes-de-interpretar)

```
T1 → T2 → T3 → T4 → T5 → T6 → T7
```

---

## T1 — Puros de e-mail e senha

**Arquivos:** `src/account/account.email.ts`, `src/account/account.password.ts`, testes ao lado.
**O que fazer:** `normalizeEmail` (minúsculo, trim), `maskEmail` (DP-conta-06), e `validatePasswordPolicy` devolvendo **todos** os motivos de uma vez.
**Testes:** e-mail com maiúsculas e espaços normaliza; máscara não deixa vazar o local part inteiro; senha curta + sem número devolve **dois** motivos, não um.
**Pronto quando:** `bun test` passa e nenhuma função lê `process.env` ou o relógio.

---

## T2 — Token de acesso 🧠

**Arquivos:** `src/account/account.token.ts` + teste.
**O que fazer:** `issueAccessToken` / `verifyAccessToken` conforme DP-conta-02 — HMAC-SHA256, `timingSafeEqual`, assinatura verificada ANTES do payload, segredo e instante injetados.
**Testes:** token recém-emitido verifica; expirado falha; payload adulterado falha; assinatura adulterada falha; token de outro segredo falha; lixo não crasha.
**Pronto quando:** `bun test` passa e nada no módulo importa `environment`.

---

## T3 — Limitador de tentativas

**Arquivos:** `src/account/account.limiter.ts` + teste.
**O que fazer:** janela e teto nomeados, chave por e-mail normalizado, tempo injetado (DP-conta-05).
**Testes:** estoura no teto; sucesso zera; janela expirada volta a aceitar; contas diferentes não interferem.
**Pronto quando:** `bun test` passa.

---

## T4 — Schema e migration

**Arquivos:** `src/account/account.schema.ts`, migration Drizzle.
**O que fazer:** `player_accounts` e `refresh_tokens` conforme DP-conta-01 — unique no e-mail normalizado, `status` como varchar (nunca ENUM nativo), refresh guardado como hash.
**Pronto quando:** `drizzle-kit generate` produz migration aditiva e ela é revisável.

---

## T5 — Use cases e rotas

**Arquivos:** `src/account/account.validation.ts`, `account.use-case.ts`, `account.controller.ts`, e as rotas em `src/index.ts`.
**O que fazer:** registrar, entrar, renovar e sair. `409 ACCOUNT_EMAIL_TAKEN`; `401 ACCOUNT_INVALID_CREDENTIALS` idêntico para os dois casos, com o gasto de tempo equivalente (DP-conta-04); `429` com `Retry-After`.
**Pronto quando:** compila, `tsc --noEmit` limpo, e nenhuma resposta devolve hash, token de renovação em texto no corpo de registro, ou campo interno.

---

## T6 — Roteiro de verificação

**Arquivo:** `docs/verification/conta-de-jogador.md`.
**O que fazer:** o que só Postgres de pé prova (DP-conta-07), com o `curl` exato de cada caso.
**Pronto quando:** o roteiro existe e é executável por quem tiver o banco.

---

## T7 — Regressão

**Pronto quando:**
- [ ] `bun test` em `apps/api-battle-pets` — todos passam
- [ ] `tsc --noEmit` limpo
- [ ] Unreal intocada: `Automation RunTests BattleSquare` (118) e `BattleSim` (52) continuam limpos, e as três sondas `exit 0`

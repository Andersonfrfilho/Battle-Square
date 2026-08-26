# Moderação e Banimento — Tarefas

**Design:** `.specs/features/moderacao-e-banimento/design.md`
**Status:** ✅ CONCLUÍDO (T1–T5, 2026-08-26). Os 8 itens de `docs/verification/moderacao-e-banimento.md` seguem **não verificados** — exigem Postgres de pé.
**Escopo:** `apps/api-battle-pets`. Unreal não é tocada.

---

## Plano de Execução

> 🤖 Modelo: `sonnet` — **T1 é 🧠** (a regra do "mais restritivo vence" é onde um banimento some sem ninguém notar)

```
T1 → T2 → T3 → T4 → T5
```

---

## T1 — `resolveBanState`, puro 🧠 ✅

**Arquivos:** `src/moderation/moderation.ban.ts` + teste.
**O que fazer:** conforme DP-mod-02 — levantado nunca bane, expirado nunca bane, sem prazo bane sempre, mais restritivo vence.
**Testes:** lista vazia; permanente; temporário ativo; temporário expirado; levantado antes do prazo; permanente + temporário (permanente vence); dois temporários (vence o que termina depois); instante EXATO da expiração não bane.
**Pronto quando:** `bun test` passa e a função não lê relógio nem banco.

---

## T2 — Schema e migration ✅

**Arquivos:** `src/moderation/moderation.schema.ts`, migration.
**O que fazer:** `moderation_events` e `account_bans` conforme DP-mod-01 — `expires_at` nulo é permanente, `lifted_at` em vez de `DELETE`.
**Pronto quando:** `drizzle-kit generate` produz migration aditiva.

---

## T3 — Use cases, rotas e recusa no login ✅

**Arquivos:** `src/moderation/moderation.validation.ts`, `.use-case.ts`, `.controller.ts`; alteração em `src/account/account.use-case.ts`; rotas em `src/index.ts`.
**O que fazer:** registrar evento (escopo `admin` ou `sync`), banir, levantar, listar histórico (escopo `admin`); login e verificação de token recusam conta banida com `403 ACCOUNT_BANNED` e o prazo (DP-mod-03).
**Pronto quando:** `tsc --noEmit` limpo e nenhuma rota de moderação é acessível sem escopo.

---

## T4 — Roteiro de verificação ✅

**Arquivo:** `docs/verification/moderacao-e-banimento.md`.
**Pronto quando:** existe, com o `curl` de cada caso, e destaca o item que separa banimento real de decorativo (token válido de conta banida).

---

## T5 — Regressão ✅

**Pronto quando:**
- [x] `bun test` — **25 pass, 0 fail** (15 de conta + 10 de banimento)
- [x] `bunx tsc --noEmit` — limpo
- [x] Unreal intocada e ainda **118/0**; `BattleSim` **52/0**; as três sondas `exit 0`
- [x] Migration `0002` puramente aditiva — nenhum `ALTER` ou `DROP` em tabela existente

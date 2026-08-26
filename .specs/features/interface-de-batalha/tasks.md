# Interface de Batalha — Tarefas

**Design:** `.specs/features/interface-de-batalha/design.md`
**Status:** ✅ CONCLUÍDO (T1–T6, 2026-08-26). Os 5 itens de `docs/verification/interface-de-batalha.md` seguem **não verificados** — exigem olho humano.

---

## Plano de Execução

> 🤖 Modelo: `sonnet` — **T3 é 🧠** (montar a árvore de widgets por código é onde o layout nasce torto e ninguém percebe até abrir)

```
T1 → T2 → T3 → T4 → T5 → T6
```

---

## T1 — Testes do espelho do widget ✅

**Arquivo:** `Source/BattleSquare/Private/Tests/BattleActionSelectorWidgetTest.cpp`.
**O que fazer:** cobrir o que DP-ui-05 marca como automatizável — cada método encaminha ao componente, e `CurrentStep`/`ConfirmedActionCount`/`bIsCommitted` acompanham o estado real.
**Pronto quando:** `Automation RunTests BattleSquare.UI.BattleActionSelector` passa, sem UMG.

---

## T2 — `ABattleScreenGameMode` ✅

**O que fazer:** GameMode que monta uma partida (reusando `FEncounterMatchAssembler` e o espelho de pets já configurado em `DefaultGame.ini`), spawna `ABattleArena`, e cria a interface.
**Pronto quando:** teste headless confirma que o GameMode monta a arena com dois pets, sem depender de mundo aberto.

---

## T3 — `WBP_BattleActionSelector` 🧠 ✅

**Como:** via `unreal-mcp` (UMGToolSet). **Aplicar L-024** (salvar a cada passo).
**O que fazer:** árvore de widgets conforme DP-ui-03 — contador, 6 botões de tipo, grade 3x3 de direções, commit/desfazer/cancelar. Cada botão ligado ao método correspondente.
**Pronto quando:** o Blueprint compila, abre no Designer e cada botão chama o método certo.

---

## T4 — Nível `BattleScreen` ✅

**O que fazer:** nível simples (sem World Partition) com o GameMode de T2 como override.
**Pronto quando:** abrir o nível e dar Play mostra a arena com os dois pets e a interface por cima.

---

## T5 — Roteiro de verificação ✅

**Arquivo:** `docs/verification/interface-de-batalha.md`.
**Pronto quando:** existe, cobrindo o que só o olho prova (layout aparece, botões clicáveis, texto cabe, jogar um turno inteiro).

---

## T6 — Regressão ✅

**Pronto quando:**
- [x] `Automation RunTests BattleSquare` — **130 Success, 0 Fail** (121 anteriores + 9 novos)
- [x] `Automation RunTests BattleSim` — **52 Success, 0 Fail**, zero linha tocada (total 182/182)
- [x] As três sondas — `exit 0`
- [x] **L-020/L-025/L-026 aplicadas** — Editor fechado, rebuild depois da sonda, manifesto sincronizado, log sem `StaticShutdownAfterError`

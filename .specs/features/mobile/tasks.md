# Mobile — Tarefas

**Design:** `.specs/features/mobile/design.md`
**Status:** ✅ CONCLUÍDO no que a máquina permite (T1–T5). MOB-01 a MOB-05 do roteiro seguem BLOQUEADOS por B-006/B-006b/B-007 — ver `docs/verification/mobile.md`.
**Escopo:** `BattleSquare` (C++, T2), documentação (T1) e `Config/` (T3). `BattleSim` não é tocado.

---

## Plano de Execução

> 🤖 Modelo: `sonnet` — **T1 é 🧠** (o orçamento é decisão estrutural: M5 inteiro foi decidido contra um número provisório)

```
T1 → T2 → T3 → T4 → T5
```

---

## T1 — Orçamento de performance de mobile 🧠 ✅

**Arquivo:** `docs/performance/orcamento-mobile.md`.
**O que fazer:** aparelho de referência, memória, frame time e resolução conforme DP-mobile-01, cada número marcado **alvo** ou **medido**, com a tabela de medição preparada e vazia.
**Pronto quando:** o documento existe e o Todo "Definir orçamento de performance de mobile" sai de aberto em `STATE.md`.

---

## T2 — `FTouchMovementInput` ✅

**Arquivos:** `Source/BattleSquare/Public/World/TouchMovementInput.h`, `.../Private/World/TouchMovementInput.cpp`, teste em `.../Private/Tests/TouchMovementInputTest.cpp`.
**O que fazer:** `ComputeMovementAxis` puro, com zona morta e raio máximo nomeados, e a inversão do Y de tela (DP-mobile-02).
**Testes:** arrasto para cima na tela → frente (+X do eixo); dentro da zona morta → vetor nulo; além do raio máximo → comprimento exatamente 1; diagonal normalizada; composto com `FWorldTraversalMotion`, o eixo de toque produz a mesma direção que o eixo de teclado equivalente.
**Pronto quando:** `Automation RunTests BattleSquare.World.TouchMovementInput` passa, sem `UWorld`.

---

## T3 — Perfis de dispositivo e escalabilidade ✅

**Arquivos:** `Config/DefaultDeviceProfiles.ini`, `Config/DefaultScalability.ini`.
**O que fazer:** declarar o perfil mobile coerente com o orçamento de T1 (DP-mobile-03).
**Pronto quando:** os arquivos existem e os valores derivam do orçamento — **sem alegar verificação**, que está atrás de B-006/B-007.

---

## T4 — Roteiro de verificação e registro dos bloqueios ✅

**Arquivos:** `docs/verification/mobile.md`; blockers em `STATE.md`.
**O que fazer:** roteiro dos itens que só o aparelho prova, cada um marcado **BLOQUEADO** com o blocker correspondente; e registrar B-006, B-006b e B-007 com evidência e remédio.
**Pronto quando:** o roteiro existe, e cada item bloqueado nomeia o que precisa acontecer antes de alguém tentar rodá-lo.

---

## T5 — Regressão completa ✅

**Pronto quando:**
- [x] `Automation RunTests BattleSquare` — **118 Success, 0 Fail** (112 anteriores + 6 novos de toque)
- [x] `Automation RunTests BattleSim` — **52 Success, 0 Fail**, zero linha tocada (total 170/170)
- [x] As três sondas — todas `exit 0`
- [x] **L-020/L-025/L-026 aplicadas:** Editor fechado, rebuild real depois da sonda, `sync_module_manifest.sh` rodado, e `StaticShutdownAfterError` ausente do log (0 ocorrências)

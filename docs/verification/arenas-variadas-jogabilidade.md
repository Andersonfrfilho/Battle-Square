# Roteiro de Verificação — Jogabilidade de Arenas Variadas

**Feature:** `.specs/features/arenas-variadas/`
**Status:** **não verificado ainda** — nenhum item deste roteiro foi rodado.

Todo o mecanismo (bloqueio, dano de casa, buff contextual, morte simultânea combinando dano de casa e combate) já está provado com o resolvedor real em `Automation RunTests BattleSim`. O que falta é o item ❌ do design: **a arena "parecer justa" e "legível" para uma pessoa jogando de verdade**, não só nos testes.

## Item 1 — Combate manual em cada tipo de arena

- [ ] **Não verificado**

**Passo concreto:**
1. Cadastrar (ou usar fixture) um layout de arena com pelo menos uma casa de cada propriedade (Bloqueada, Dano, Buff) via `Config/ArenaLayouts.json` (ou o arquivo que `FArenaLayoutCatalog` vier a consumir em produção).
2. Jogar manualmente pelo menos 5 partidas completas nessa arena.
3. Confirmar que uma casa bloqueada é visualmente óbvia ANTES de tentar mover para ela (não só depois do movimento falhar) — item de layout/UMG, fora do que o C++ garante.
4. Confirmar que o dano de permanecer numa casa de dano é perceptível na barra de vida, turno a turno, sem precisar abrir log.
5. Confirmar que a vantagem de atacar/defender numa casa de buff é perceptível comparando dois combates idênticos, um com e outro sem a casa.

## Item 2 — Comparação com a ferramenta de simulação em lote

- [ ] **Não verificado**

**Passo concreto:**
1. Rodar `FBattleBalanceSimulator::RunBatchSimulation` (Escala de Pets e Skills, reaproveitada) com os mesmos pets, uma vez numa arena neutra e outra na arena de teste do Item 1.
2. Comparar a impressão subjetiva do Item 1 com os números agregados — devem contar a mesma história (a arena favorecer visivelmente o lado que ocupa a casa de buff, por exemplo).

---

## O que NÃO precisa deste roteiro

Já coberto por `Automation RunTests BattleSim`/`BattleSquare`: bloqueio de movimento, acúmulo de dano em `PendingDamage`, morte simultânea combinando dano de casa e combate, buff contextual (Attack ao atacar, Defense ao defender), `ComputeHash` incluindo o layout, carga/consulta de `FArenaLayoutCatalog`, validação de posição inicial contra casa bloqueada.

## Registro de execução

| Data | Quem | Itens verificados | Resultado |
|---|---|---|---|
| — | — | nenhum ainda | — |

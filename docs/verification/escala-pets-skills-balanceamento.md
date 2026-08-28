# Roteiro de Verificação — Sensação de Balanceamento

**Feature:** `.specs/features/escala-pets-skills/`
**Status:** **não verificado ainda** — nenhum item deste roteiro foi rodado.

O `BattleSquare.Balance.RunBatchSimulation.ReportsAggregateStats` já prova, com números, que uma composição super efetiva vence mais que uma neutra (design.md, Limite de Ferramenta — linhas ✅). O que falta é o item ❌: **a efetividade "parecer certa" para uma pessoa jogando de verdade**, não só nos agregados.

## Item 1 — Combate manual entre tipos conhecidos

- [ ] **Não verificado**

**Passo concreto:**
1. Cadastrar (ou usar fixture) dois pets com tipos de efetividade conhecida (ex.: `Fogo` vs `Planta`, 150%) e atributos base idênticos.
2. Jogar manualmente pelo menos 5 partidas completas entre eles (via `ABattleArena`/`FDumbOpponentAI`, mesmo fluxo de M1).
3. Confirmar que o número flutuante de dano do lado super efetivo é visivelmente maior que o do lado neutro, e que isso é perceptível SEM abrir nenhum log — só olhando a barra de vida.
4. Comparar a impressão subjetiva com o relatório de `RunBatchSimulation` rodado com a mesma composição (mesma seed base) — devem contar a mesma história.

## Item 2 — Tipo novo cadastrado só por dado

- [ ] **Não verificado**

**Passo concreto:**
1. Editar `Config/TypeEffectiveness.json` adicionando um tipo que não existia (ex.: `Eletrico`).
2. Reiniciar o servidor/editor (sem recompilar nada).
3. Cadastrar um pet desse tipo novo no backend e jogar uma partida contra ele.
4. Confirmar que a efetividade do tipo novo é aplicada — prova viva de ESCALA-06 em condição real, não só em teste automatizado com fixture.

---

## O que NÃO precisa deste roteiro

Já coberto por `Automation RunTests BattleSquare`: `FTypeEffectivenessTable` (consulta e carga de JSON), `TranslateMatchup` (efetividade aplicada corretamente, caso assimétrico), `RunBatchSimulation` (determinismo por seed, composição super efetiva vence mais em 200 simulações).

## Registro de execução

| Data | Quem | Itens verificados | Resultado |
|---|---|---|---|
| — | — | nenhum ainda | — |

---

## BAL-10 — A efetividade APARECE (2026-08-28)

Até hoje a efetividade de tipo nem sequer se aplicava (ninguém carregava a
tabela). Aplicada, ela seguia invisível: o jogador tomava mais dano sem ter como
saber por quê, e regra que só se descobre perdendo não ensina nada.

- [ ] Golpe de Fogo em Planta: o feed diz **"É super efetivo!"** logo abaixo do
      acerto.
- [ ] Golpe de Fogo em Água: **"Não é muito efetivo..."**.
- [ ] Confronto neutro: nenhuma das duas linhas aparece — ruído em toda troca
      de golpes ensinaria menos, não mais.
- [ ] A frase só sai no golpe que ACERTOU, nunca num erro ou num movimento.
- [ ] Julgamento: dá para deduzir a tabela jogando, sem ninguém explicar?

# Contexto de I.A. — Battle Square

Leia também `.specs/project/STATE.md` (histórico, lições L-001+, blockers) e
`.specs/project/ROADMAP.md` antes de começar.

---

## Regra: o que for implementado precisa aparecer na tela

**Toda vez que implementar comportamento observável em jogo, mostre na tela o
que está acontecendo.** Use `FBattleDebugScreen::Show` (`Debug/BattleDebugScreen.h`).

### Por que esta regra existe

Em 26–27/08/2026, ao dar interface ao combate, **oito defeitos sérios
apareceram — e sete só foram encontrados porque um humano olhou a tela**:

| Defeito | Por que nenhum teste pegou |
|---|---|
| Pets invisíveis (`APetView` sem componente visual) | O ator existia e a lógica passava |
| Câmera apontada para longe da arena | A batalha rodava, só que fora de campo |
| Pet afundando meio corpo no tabuleiro | Posição correta, offset visual errado |
| "Baixo" andava para a direita | Eixos da grade contra os da câmera |
| Batalha de um turno só | 189 testes verdes, e um deles *afirmava* o defeito |
| Três ações instantâneas | "Animação" sem tempo nenhum |
| Oponente previsível (semente sempre 0) | Determinismo correto, produto errado |

O ciclo "usuário joga → descreve o que viu → eu leio o log → deduzo" custou
horas. Informação na tela encurta para "joga → lê". O usuário é o instrumento
de medição mais eficiente que este projeto tem; **facilite o trabalho dele.**

### Como aplicar

- **Estado que muda a cada turno** → `Key` fixa, para a linha se atualizar no
  lugar em vez de empilhar.
- **Evento pontual** → `Key = -1`, para empilhar.
- **Cor com significado**: um lado numa cor, o outro em outra.
- Nunca custa em Shipping (compilado fora) e some com `bs.ShowBattleDebug 0`.

### O que a regra NÃO autoriza

- Log de PII (`security.md` §1) — nem na tela, nem em arquivo.
- Substituir teste por log. O log encontra o defeito; **o teste impede que ele
  volte**. Todo defeito achado na tela ganha teste antes de ser fechado.

---

## Verificação: rode antes de dar qualquer coisa por pronta

```bash
./Tools/audit_determinism.sh && ./Tools/audit_no_recalculation.sh && ./Tools/probe_isolation.sh
./Tools/sync_module_manifest.sh   # L-025: manifesto defasado faz teste novo sumir da contagem
```

Depois de `probe_isolation.sh`, **recompile** (L-020) — ela deixa o `.dylib` do
`BattleSim` quebrado.

**Feche o Editor antes de compilar** e reabra com `open -a` (não por shell em
segundo plano): editor lançado em background não recebe teclado no PIE, o que
custou várias rodadas de investigação inútil.

---

## Fronteiras que não se cruzam

- **`BattleSim` é o núcleo determinístico.** Sem float, sem `FMath::Rand`, sem
  relógio. Semente e montagem são decisão da camada de fora.
- **A tela não decide regra** (DP-ui-01). Todo botão encaminha ao
  `UBattleActionQueueComponent`, que já tem a regra e o teste.
- **Uma fonte de verdade por regra.** Duplicar uma tabela ou uma validação foi
  a causa de L-032, L-033 e de um defeito de direção — as cópias concordam até
  a primeira edição.

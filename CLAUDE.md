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
- Nunca custa em Shipping (compilado fora).

### O que fica visível durante a batalha

| Recurso | Para que serve |
|---|---|
| **Grade desenhada no mundo** | cada casa com `(coluna,linha)` e quem está nela; casa ocupada fica amarela. Torna posição e direção legíveis — teria mostrado na hora que "Baixo" andava para a direita, e mostra a coabitação sem explicação |
| **Indicador de fase** | `reproduzindo fase 3 de 11`, que desfaz a impressão de "fez tudo de uma vez" |
| **Painel de texto** | o que cada lado escolheu e onde cada pet terminou |

### O painel, e por que ele é copiável

`ABattleDebugHUD` desenha as últimas 16 linhas numa caixa fixa no canto
superior direito. É HUD, não UMG, de propósito: não depende de asset autorado,
então funciona em qualquer nível assim que o GameMode o declara.

| Tecla / console | O que faz |
|---|---|
| **F9** | copia o painel para a área de transferência **e** grava `Saved/BattleDebug.txt` |
| **F10** | esvazia o painel |
| `bs.ShowBattleDebug 0` | esconde |

O painel é **desenhado**, não é campo de texto: o mouse nunca consegue
selecioná-lo. Por isso a cópia é por tecla — e por isso ela também grava
arquivo, que é o caminho que não depende da área de transferência funcionar.

Altura **fixa**, teto de 12 linhas: painel que cresce a cada linha muda de
tamanho o tempo todo e mesmo assim não rola. As mais antigas saem por cima.

Mensagem que some obriga a ler depressa e a repetir a partida para reler; e
transcrever da tela à mão perde justamente o detalhe que importa — foi assim
que `Atacar Esquerda` virou "ele foi para a esquerda" numa investigação real.
Por isso o painel **persiste** e **copia**.

### O que a regra NÃO autoriza

- Log de PII (`security.md` §1) — nem na tela, nem em arquivo.
- Substituir teste por log. O log encontra o defeito; **o teste impede que ele
  volte**. Todo defeito achado na tela ganha teste antes de ser fechado.

---

## Depurar: teste primeiro, conserto depois

**Diante de um defeito, a primeira ação é escrever um teste que o reproduza —
não um conserto.** Em 26–27/08 consertei por hipótese três vezes seguidas; cada
correção estava certa isoladamente, e nenhuma era comprovadamente *a* causa. Só
parei de errar quando medi.

Ordem de eficácia, do que mais resolveu para o que menos:

1. **Teste headless que reproduz** — respondeu "o gerador avança e o inimigo se
   move" em um ciclo de build, sem envolver o usuário, e ficou como proteção.
2. **Inspecionar o estado vivo** (PIE + consulta ao mundo) — achou a batalha
   acontecendo a um milhão de unidades da câmera.
3. **Instrumentação dirigida no log** — decisiva quando a dúvida é sequência.
4. **O usuário olhando a tela** — insubstituível para o que é visual.

**Sempre abrir o Editor com `-log=BattleSquare.log`**, para haver um arquivo só
a consultar em vez de caçar o mais recente.

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

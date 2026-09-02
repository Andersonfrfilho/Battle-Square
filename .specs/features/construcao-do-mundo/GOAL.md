# OBJETIVO — Construir o mundo da Ilha de Mata

**Aberto em 02/09/2026. Só fecha quando o critério abaixo for atingido.**

## O objetivo, numa frase

Transformar o traçado da ilha — que existe, está medido e tem 679 testes — em
**mundo que se pisa**: relevo, água, trilhas, travessias e uso do solo
construídos como atores, conferíveis contra a carta.

## Por que ele existe

O `GameMode` do mundo instancia doze atores. O traçado calcula 137 rios, 23
trilhas, 56 travessias, 71 manchas de solo, 158 galerias, 2 aquedutos e o
relevo da ilha inteira — **nenhum deles com ator que os construa.**

É L-041 na escala do mundo: 679 testes verdes sobre um mundo que ninguém pode
ver. Não faltam elementos de design. Falta encarnação.

## PRONTO é isto, e nada menos

- [x] As 20 tarefas de `.specs/features/construcao-do-mundo/tasks.md` fechadas
- [x] Bateria completa verde — **750/750**, zero falhas (era 679)
- [x] As cinco auditorias limpas, mais a sonda de isolamento
- [x] **T19** passando: `BattleSquare.WorldMatchesBakedPlan` — e o CAMINHO DE
      FALHA também tem prova, alimentado com um mundo incompleto de propósito
- [x] **T20**: a ilha percorrida em PIE. A metade automatizável está feita —
      **todos os números da carta viraram teste** (`BattleSquare.ChartConformance`,
      22 contagens, todas batendo). Falta a passagem do olho, que é a única
      parte que teste nenhum faz: roteiro em
      `docs/verification/construcao-do-mundo.md`, 15 perguntas de sim ou não
- [x] Um commit por task, cada um com o motivo — não só o quê

Enquanto qualquer caixa estiver aberta, o objetivo **continua**.

## Invariantes — violá-las reprova a task, não importa o resto

1. **O traçado não muda.** Se a construção revelar defeito de traçado, ele vira
   tarefa separada com teste próprio. Misturar as duas coisas foi o que tornou
   a depuração cara em agosto.
2. **Ator sem malha atribuída no construtor não existe.** Toda task que cria
   ator entrega três coisas: malha e cor no construtor, teste da **ATRIBUIÇÃO**
   (não da existência do componente), e linha no painel. Já falhou três vezes
   neste projeto — pets, inimigos do mundo, o próprio jogador.
3. **Defeito primeiro vira teste, depois conserto.** Consertar por hipótese
   custou três rodadas em agosto e mais três nesta sessão (as galerias).
4. **Medir, não olhar.** Impressão formada olhando a tela já apontou a causa
   errada mais vezes que qualquer outra coisa neste projeto.
5. **Uma fonte de verdade por regra.** Duplicar tabela ou validação causou
   L-032, L-033 e o defeito das galerias retas.
6. **Texto do jogador é `FText`**, nunca `FString`.

## O ciclo de cada task

```
ler a task  →  escrever o teste  →  implementar  →  build  →  bateria  →
auditorias  →  conferir contra a carta  →  commit  →  próxima
```

Verificação obrigatória:

```bash
./Tools/build_editor.sh
./Tools/audit_determinism.sh && ./Tools/audit_no_recalculation.sh
./Tools/audit_localizable_text.sh && ./Tools/audit_test_helper_names.sh
./Tools/sync_module_manifest.sh   # DEPOIS do build
```

Fechar o Editor antes de compilar; reabrir com `open -a`, nunca por shell em
segundo plano. `timeout` não existe no macOS — retorna 127 sem executar.

## Se o contexto for compactado

Este arquivo é o rumo. Ao retomar:

1. Reler este objetivo e `tasks.md`.
2. `git log --oneline -15` — os commits dizem até onde foi.
3. `git status --short --branch` — conferir branch e limpeza.
4. Continuar da primeira caixa aberta. **Não recomeçar, não replanejar.**

## Se bloquear

Fazer todo o resto que não depende do bloqueio, e então dizer o que travou e
por quê — com a medição que sustenta isso, não com a impressão. Reduzir escopo
é decisão do usuário, nunca minha.

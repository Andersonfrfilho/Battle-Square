# Tamanho de Campo Variável — Especificação

**Status:** Aprovado e executado (29/08/2026)
**Depende de:** Combate Núcleo, Arenas Variadas (as duas concluídas).

---

## Problem Statement

A grade era 3x3 **fixa em constante global**, e a spec de Arenas Variadas
registrava tamanho variável como fora de escopo. Duas consequências:

1. **O campo não podia crescer.** Nove casas com dois pets adjacentes deixam
   pouco espaço para o movimento importar: quase toda casa é vizinha de quase
   toda casa, e posicionar-se raramente é uma escolha.
2. **A casa inicial era coordenada escrita à mão** — `(1,1)` e `(2,1)`, repetida
   em `BattleDataTranslator`, no comentário de `Arenas.json` e em cada teste. Os
   dois pets nasciam no meio, colados, de modo que o primeiro turno já começava
   dentro do alcance de ataque.

O pedido que originou isto foi direto: o pet do jogador deve começar em `(0,1)`,
e o campo deve poder ser 4x4, 5x5, 3x2 ou 4x6.

## Goals

- [x] As dimensões da grade vivem em `FBattleState` (`GridColumns`, `GridRows`) e
      **entram no hash** — mesma semente em campos diferentes é batalha diferente
- [x] Colunas e linhas são independentes: 3x2 e 4x6 são campos legítimos
- [x] A casa inicial é DERIVADA da grade — lado 0 na primeira coluna, lado 1 na
      última, ambos na linha do meio. Num 3x3 isso dá `(0,1)` e `(2,1)`
- [x] O tamanho é configurável em `DefaultGame.ini`
- [x] Layout de arena pode declarar `columns`/`rows`; sem declaração vale 3x3
- [x] Tabuleiro, clareira, moldura, posição dos donos e enquadramento da câmera
      derivam da grade ativa — nenhum ponto do ator assume três

## Out of Scope

| Item | Razão |
|---|---|
| Grade acima de 15 por eixo | `PackCell` guarda cada eixo em 4 bits do `uint8` do trace; 16 daria a volta para 0, calado |
| Mais de um pet por lado | Continua sendo M3; o campo maior habilita, não implementa |
| Editor visual de arena | Autoria de conteúdo, como já dizia Arenas Variadas |
| Câmera que se ajusta sozinha a campos muito grandes | O enquadramento hoje deriva da grade, mas não foi calibrado além de ~6 casas; medir isso é roteiro de verificação, não código |

## Decision Points

### DP-campo-01 — Onde mora o tamanho da grade

**Decisão: no estado, e no hash.**

A alternativa era uma constante de configuração lida por quem precisasse. Ela
falha por uma razão específica e não estética: `FBattleState::ComputeHash()` é a
assinatura que existe para detectar divergência entre cliente e servidor. Um
tamanho lido de fora deixaria duas simulações com grades diferentes produzindo
hashes iguais até o primeiro movimento de borda — que é exatamente o caso em que
a divergência importa e o mais difícil de reproduzir depois.

### DP-campo-02 — Casa inicial derivada, nunca escrita

**Decisão: `FBattleState::PlaceDuelistsAtStartingCells()` decide, depois de os
pets entrarem no estado.**

`TranslateMatchup` não sabe o tamanho da grade e não deveria saber — ele traduz
pet, não posiciona. As coordenadas que ele escrevia viraram provisórias, e a
posição real é aplicada por quem monta o estado. Deixar a montagem escolher a
casa daria duas respostas para a mesma pergunta assim que o campo deixasse de ser
3x3 — que foi precisamente o defeito que esta feature corrige.

**Linha do meio arredonda para baixo** em altura par: num 4x6 os dois ficam na
linha 2. O que importa é que fiquem na MESMA linha; um duelo que começa em
diagonal é uma perseguição, não um duelo.

### DP-campo-03 — Índice de casa sem valor padrão

**Decisão: `CellLayoutIndex(Column, Row, GridColumns)` exige as colunas.**

Manter `= 3` como padrão teria evitado tocar em trinta chamadas. Também teria
reintroduzido o 3 fixo em toda chamada que esquecesse de passá-lo, e o erro
apareceria como **casa trocada** — água no lugar errado, dano onde não havia —
não como falha de compilação. Um padrão silencioso aqui custa mais caro que a
churn de atualizar os chamadores.

### DP-campo-04 — Layout de arena com tamanho declarado

**Decisão: `columns`/`rows` opcionais em `Arenas.json`, 3x3 quando ausentes; o
comprimento do array precisa bater.**

Tornar as dimensões obrigatórias invalidaria o arquivo existente inteiro por uma
informação que ele nunca precisou dar. A verificação que importa é o
**comprimento**: um array com casas a menos deslocaria toda linha depois da
primeira, e a arena abriria com a água no lugar errado — de novo, silenciosamente.

Um layout só é aplicado a uma arena do mesmo tamanho. Campo sem layout do tamanho
dele abre neutro, que é o comportamento de sempre, não uma falha.

## O que ficou provado por teste

| Teste | O que ele impede de voltar |
|---|---|
| `Grid.ResizeMatchesLayout` | layout com tamanho antigo indexado com colunas novas |
| `Grid.ResizeClampsToPackableRange` | grade acima de 15 estourando `PackCell` |
| `Grid.SizeEntersStateHash` | grades diferentes com a mesma assinatura |
| `Grid.DuelistsStartOnOppositeEdges` | casa inicial voltando a ser coordenada escrita à mão (5 formatos de campo) |
| `Grid.MovementRespectsPerAxisBounds` | limite único aplicado aos dois eixos num campo retangular |
| `ArenaLayoutCatalog.AcceptsDeclaredSize` | layout curto aceito e deslocando as linhas |

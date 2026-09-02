# Cavernas nas quedas — o que é, e por que mexe no gerador

Vem de **P9** em `.specs/features/pendencias/BLOQUEIO-P7-P8-P9-P10.md`, que foi
aberta como "parcial" e reclassificada pela medição.

A promessa é curta: **entrar numa gruta pela lâmina da cachoeira**. Hoje isso não
existe em lugar nenhum do mapa — e a razão não é uma consulta que falta.

---

## O que a medição disse, e por que ela ordena a feature

Medido em `Saved/IslandMap.json`, 16 cavernas × 13 quedas com poço:

| | unidades |
|---|---|
| caverna mais perto de uma queda | **2 710** |
| segunda mais perto | 2 711 |
| **mediana** | 3 943 |
| meia-largura do poço da queda | **886** |

Uma gruta "atrás da cachoeira" precisa cair **dentro** do poço, algo como 900
unidades. A mais próxima está a **três vezes** isso, e a mediana a mais de
quatro.

**Portanto não é `CavernaAtrasDaQueda()` que falta, é a POSIÇÃO das cavernas.**
Escrevê-la hoje devolveria "nenhuma" para as dezesseis, e um recurso que não
existe em lugar nenhum do mapa não é recurso — é uma função que só o teste chama.
Isto mexe na **colocação**, que é o gerador.

---

## As 16 cavernas vêm de dois lugares, e só um deles é o assunto

`IslandBakedPlan.cpp`, em `AllCaves()`, junta duas listas:

| origem | quantas | quem decide |
|---|---|---|
| `IslandFeatureLayout::Plan()` | **3** — grande (lado 11), de lava (7), de água (5) | ângulo escrito à mão, decisão de projeto |
| `FreshWater::PlanGrottoes()` | **13** — uma por queda com chão seco ao lado | busca ao redor da queda |

As três do plano estão onde estão de propósito: *"uma montanha que muda de lugar
a cada partida não é paisagem"*. **Elas não se movem.** O assunto desta feature
são as **grutas**, que já nascem da queda e já são procuradas.

---

## Por que a gruta que JÁ É da cachoeira ainda está a 2 710 dela

`FreshWater::PlanGrottoes()` foi escrita com o cabeçalho que diz a intenção:
*"Ela fica ao LADO da queda: a água caindo ocupa o lugar imediatamente abaixo, e
uma gruta ali seria uma boca com o rio entrando por dentro."*

**Três coisas empurram a gruta para fora do poço, e as três são deliberadas.**
Derivadas das constantes do código, com `WorldIslandRadiusUnits=140000.0` em
`Config/DefaultGame.ini`:

| o que | conta | vale |
|---|---|---|
| meia-calha do rio | `140000 × 0,0055` | **770** |
| meia-largura do poço | `MeiaCalhaDoRio() × 1,15` | **885,5** |
| pegada da gruta | `3 × 240 + 2 × 120` | **960** |
| folga que a gruta reserva | `0,5 × 960 × √2` | **678,8** |
| mais `FolgaDaGruta` | `678,8 + 300` | **978,8** |
| primeira distância que a busca tenta | `MeiaCalhaDoRio() × 1,6` | **1 232** |

1. **A busca nunca olha dentro do poço.** Ela começa a **1 232** da queda, e o
   poço tem **886** de raio. O lugar procurado está fora do poço antes da
   primeira tentativa.
2. **A regra da margem exige 978,8 de chão seco**, medidos até a margem do
   curso. O poço fica **em cima** da calha; a calha tem 770 de meia-largura.
   Todo ponto do poço reprova — não por ser poço, mas por ser rio.
3. **Existe teste afirmando isso.**
   `BattleSquare.Environment.FreshWater.GrottoStandsClearOfTheChannel` cobra,
   para toda gruta e todo curso, `Ate > HalfWidthAtProgress + ClearanceUnits`.

⚠️ **A medição do BLOQUEIO tem um detalhe que muda a leitura:** os 886 não são a
medida de um poço escavado, são o **piso** — `FMath::Max(MeiaCalhaDoRio() * 1.15f,
Fundo * AlargaSobreAprofunda)`, e `885,5` é o primeiro termo. Os treze poços têm
todos a mesma meia-largura porque **todos os treze estão no piso**: a fundura
medida (30 a 51) multiplicada pelo fator nunca passa dele. O poço, hoje, é a
calha do rio um pouco mais larga.

---

## O que a feature de fato muda: a boca pode molhar, o corpo não

O que a regra de hoje trata como um caso só são **dois** casos, e a diferença é a
feature inteira:

| o que acontece | é |
|---|---|
| o rio entra pela boca e corre por dentro da gruta | **defeito** — três grutas já ficaram assim, com a quina no rio |
| a lâmina de água cai **na frente** da boca, e a gruta é seca por dentro | **a feature** |

Hoje um argumento só (`MargemDaAgua > ClearanceUnits + FolgaDaGruta`) governa a
gruta inteira. Ele precisa passar a governar o **corpo**, e o **poço da queda**
precisa virar exceção declarada — só o poço, e só para a boca.

**Exceção declarada, e não regra afrouxada.** Afrouxar a folga devolveria as três
quinas dentro do rio, que é o defeito que a busca existe para não ter; e o
afrouxamento valeria em todo curso, não só onde há queda.

---

## A boca tem lado, e o lado hoje é sempre o mesmo

`CaveLabyrinth::FCaveGrid` guarda `EntranceColumn` com o comentário *"por qual
coluna da borda sul se entra. A boca principal."* **A boca é sempre ao sul.**

Uma gruta atrás da cachoeira com a boca ao norte tem a lâmina de água pelas
costas — a peça existe, a promessa não se cumpre, e nada reprova. O `FBakedCave`
agrava: ele assa `EntranceColumn` e **descarta `ExtraMouths`**, então nem as
outras bocas chegam ao mundo.

E a **boca não tem posição no mundo em lugar nenhum**: existe a coluna na grade,
existe o centro da caverna, e não existe a função que converte um no outro. É por
isso que a primeira tarefa é **medir**: a distância que interessa — boca até a
queda — nunca foi medida, porque não havia o que medir.

---

## O que se move junto, e não pode se mover em silêncio

- **A rede subterrânea nasce das grutas.** `FreshWater::PlanUnderwaterLinks()`
  cresce a bacia a partir de `PlanGrottoes()`. Mover as grutas muda as galerias,
  e a carta afirma **158**. O número velho vira o novo, na carta **e** no teste.
- **A semente do labirinto é o ÂNGULO arredondado.**
  `SeedForPlacement(Semente, Peca)` soma `RoundToInt(AngleDegrees)` e nada mais.
  Mover uma gruta troca o labirinto dela; **duas grutas no mesmo grau recebem o
  mesmo labirinto**, e treze grutas num anel é colisão plausível.
- **O templo de Corrente já está na margem da primeira cachoeira.**
  `LandUseLayout.cpp` o põe lá, empurrado até sair da lâmina. `PlanGrottoes()`
  confere sobreposição contra as peças da ilha e contra as outras grutas — **não
  contra o uso do solo.** A gruta não pode passar a perguntar ao uso do solo: o
  uso do solo já lê `FreshWater::Plan()`, e a pergunta de volta fecha o ciclo que
  `PlanReentryGuard` existe para nomear (regra de mapas §14).

---

## O teste que prova isto não pode refazer a escolha

Regra de mapas **§12**. Um teste na forma *"a queda mais perto desta gruta é X,
logo a boca dela tem de estar no poço de X"* mede o **desempate**: a medição já
mostrou duas cavernas a 2 710 e 2 711 de uma queda — um empate de uma unidade. O
gerador elege uma, o teste refaz a conta e elege a outra, e a mensagem não dá
nenhuma pista de que o problema é o desempate.

| forma | o que mede |
|---|---|
| "a queda mais perto é X, logo a boca está no poço de X" | ❌ o desempate |
| "toda gruta tem a boca no poço da sua queda" | ❌ e reprova as 12 que ficam ao lado |
| "**EXISTE** queda para a qual há gruta com a boca dentro do poço" | ✅ a propriedade |

E o teto é **parâmetro**, nunca zero: quantas quedas ganham gruta na lâmina é
decisão de arte (regra de mapas §5), e cobrar "todas" congelaria essa decisão
dentro de um teste.

---

## Aceite

**A MESMA queda, olhada de dentro:** existe pelo menos uma cachoeira cuja lâmina
de água tem uma boca de gruta atrás dela, e o jogador que entra por ali fica
seco. Sem a feature, toda gruta está ao lado da queda e nenhuma atrás — que é
exatamente o mundo de hoje, com a caverna mais próxima a três poços de distância.

E **aparece na tela**: entrar pela lâmina anuncia onde se está. Uma gruta atrás
da cachoeira que não se anuncia é indistinguível de ter errado o caminho.

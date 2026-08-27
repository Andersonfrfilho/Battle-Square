# Oponente com Critério

**Status:** Executado (2026-08-27) — com duas decisões ABERTAS ao fim.

## Problema

O oponente sorteava **uniformemente**: 1 em 6 turnos ele apenas aguardava, e
atacava sem olhar onde o jogador estava — ataque em direção vazia é erro
garantido pelo resolvedor, não risco calculado.

Isso anulava o trabalho de legibilidade recém-entregue. A narração ensina
*"atacar quem está evasivo é desperdício"*, mas **contra uma moeda não há o que
aprender**: padrão não existe, e ler a tela não recompensa.

## Comportamento

Aproxima quando está longe, ataca quando alcança, se protege quando ferido —
alternando esquiva e defesa, porque esquivar anula o físico inteiro mas é
furado por magia.

**Simula a própria posição ao longo dos três slots.** A IA antiga decidia os
três a partir da casa inicial, então "aproximar e depois atacar" era
literalmente impossível para ela.

## Decisões

**DP-ia-01 — Intenção não pode virar previsibilidade.** O commit é simultâneo e
às cegas: um oponente que sempre repete a mesma sequência é decorado na segunda
partida. A intenção decide QUAIS ações fazem sentido; o sorteio decide qual sai.

**DP-ia-02 — `FDumbOpponentAI` continua existindo.** Ele é instrumento de
medição do `BattleBalanceSimulator`: jogo aleatório é justamente o que isola
efetividade de tipo da tática de quem joga. Trocá-lo faria os números de
balanceamento medirem outra coisa sem avisar.

**DP-ia-03 — A IA não prevê o movimento do inimigo.** Ela planeja contra a casa
onde ele está. Prever seria a IA sabendo o que o commit às cegas esconde.

---

## Olhar: metade feita, metade é regra nova

O pedido foi que os pets sempre se olhem, e que o movimento seja compatível com
o olhar — salvo camuflagem, voo alto ou estar embaixo.

**Feito:** cada pet vira para o adversário a cada reposicionamento. A esfera é
simétrica, então girá-la seria invisível: quem mostra a direção é uma **marca**
que orbita para o lado do oponente. A barra de vida NÃO acompanha o giro —
precisa continuar de frente para a câmera para permanecer legível.

**Não feito, e de propósito — DECISÃO EM ABERTO:**

**DP-ia-04 (PROPOSTA) — Camuflagem, voo e subsolo são MECÂNICAS, não visual.**
Elas não existem no núcleo. Quem pode ser alvo e quem enxerga quem é regra de
combate, e inventá-la de improviso dentro da IA criaria a segunda fonte de
verdade que já custou L-032, L-033 e o defeito de direção.

Proposta, para confirmar antes de implementar:

| Estado | Efeito na regra | Efeito no olhar |
|---|---|---|
| Camuflado | não pode ser alvo de ataque direcionado; magia em área ainda pega | o outro perde a referência e olha para a última casa conhecida |
| Voando alto | imune a ataque físico terrestre; vulnerável a magia | o outro olha para CIMA |
| Sob o solo | imune enquanto submerso; não pode atacar no mesmo slot | o outro olha para BAIXO |

Cada linha acima é uma regra nova no `BattleSim`, com teste. Não é trabalho de
apresentação, e por isso não entrou junto.

# Achados da caminhada — T20

**Respondido em 02/09/2026, contra `docs/verification/construcao-do-mundo.md`.**

## O veredito

**Nenhum defeito confirmado.** As sete perguntas que cobram o que foi
construído deram sim: chão sólido (1), cor mudando com o painel concordando
(2), rocha queimada legível (3), água sobre o leito sem piscar (4), rio
engrossando (5), trilha assentada (10), templo nomeado e nome mudando (12).

A #13 saiu do roteiro: era função pura, e virou teste
(`GroundUseSacred.TheLineGoesAwayWhenYouLeave`). Perguntar a um humano o que
uma função pura responde gasta a rodada dele com o que a máquina cobra melhor
— erro meu ao escrever o roteiro.

## O que as outras respostas são

Seis respostas **não descrevem o mundo que existe** — descrevem o mundo que
deveria existir. Elas são escopo novo, e por isso ficam aqui em vez de virarem
conserto silencioso. **Reduzir ou aumentar escopo é decisão do usuário.**

| # | O que foi pedido | Contra o que existe hoje |
|---|---|---|
| 6 | Cavernas entrando por dentro das cachoeiras | Hoje a queda tem poço; a gruta é peça separada, sem ligação com a queda |
| 7 | O atraso na água ler como lama ou neve funda | Hoje é um multiplicador de passo (vau 55%, fundo 25%) — número certo, sem textura de movimento |
| 8 | Nadar quando a água passa da CINTURA do jogador | Hoje quem decide o vau é o traçado (as 30 travessias), não a altura do personagem contra a fundura |
| 9 | A balsa flutuar por empuxo, não ser sólido parado | Hoje é plataforma estática acima da lâmina — **ver a pergunta aberta abaixo** |
| 11 | Aqueduto podendo ENTRAR no morro, saindo por túnel | Hoje ele nunca entra: o teste exige isso. A implementação atual (contornar por cima) satisfaz "a estrutura dá a volta", mas o túnel não existe |
| 14 | A leitura do poço dependendo da FUNDURA dele | Hoje é binário: dá água (azul) ou não dá (pedra). O traçado não guarda fundura de poço |
| 15 | Coisas escondidas, fora da carta | Hoje a conferência exige carta e mundo idênticos — esconder algo reprovaria em `ChartConformance` |

Cada uma dessas é **uma tarefa com teste próprio**, não um ajuste de passagem.
A #8 e a #14 mexem no traçado (altura do personagem contra fundura; fundura de
poço), e por invariante isso é feature separada.

## A pergunta que ficou aberta

**#9: "ela bate na ponte pois é um sólido no jogo que o fluido é mais leve que
a água".**

Isto não fecha, e não vou adivinhar: **esta ilha não tem ponte nenhuma** — 0 de
56 travessias, medido, e a carta concorda. Então "bate na ponte" não pode estar
descrevendo o que está na tela.

Duas leituras possíveis, e elas levam a consertos opostos:

1. **É defeito visto agora:** a plataforma da balsa está colidindo com alguma
   outra geometria (a rampa do barranco? o aqueduto?). Se for isso, é T20 de
   verdade, e vira teste antes de conserto.
2. **É design futuro:** a balsa deveria flutuar por empuxo em vez de ser
   geometria estática. Aí é escopo novo, como as outras seis.

O que existe hoje, medido e testado: a plataforma fica logo acima da lâmina
(30 unidades), nunca na altura de um tabuleiro de ponte (120), e a prova disso
lê a altura de CADA obra pelo ator — não por proximidade, que foi o erro que
fez a rampa do barranco passar por balsa.

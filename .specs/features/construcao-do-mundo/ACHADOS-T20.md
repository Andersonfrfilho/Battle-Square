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

## A #9, esclarecida — e era LACUNA MINHA, não escopo novo

**"A balsa é geometria sólida que flui sobre a água e bate em outras
geometrias que tiverem no caminho."**

A T13 dizia, com todas as letras: *"vau se atravessa a pé, ponte é geometria,
barranco exige subida, **balsa é interação**"*. Eu entreguei uma laje parada
acima da lâmina e testei a ALTURA dela. Altura certa, contagem certa, material
certo — e 25 decks no meio dos rios. Plataforma que não leva ninguém a lugar
nenhum não é balsa, e a distinção que o traçado fez entre "largo demais para
ponte" e "ponte" tinha sumido.

Fechado em `AFerryActor`: ela **anda** entre as margens e volta, **flutua** na
lâmina, e é **sólida** — esbarra no que houver e inverte, em vez de atravessar
por dentro. Quatro provas, e a que importa aqui é `BumpsIntoWhatIsInTheWay`:
planta-se um obstáculo no meio do vão e cobra-se que ela não chegue do outro
lado.

E um defeito de arquitetura veio junto: eu tinha posto a balsa a NASCER dentro
da montagem da travessia. Um ator que monta malha instanciando outros atores
por dentro mistura duas responsabilidades — e derrubou o processo com uma
asserção de thread ao erguer as 25 de uma vez (SIGSEGV, e a bateria perdeu
todos os testes seguintes em silêncio). Agora a travessia PLANEJA e o
`GameMode` INSTANCIA, que é como todo o resto deste projeto funciona.

## O que segue em aberto

As seis da tabela acima continuam sendo escopo novo, e nenhuma foi implementada.

Uma delas ganhou vizinhança com o que acabou de ser feito: a balsa agora anda e
esbarra, mas **ainda não é empuxo** — a altura dela vem da lâmina, não de
densidade. Se o que se quer é flutuação física de verdade, isso segue como
tarefa própria.

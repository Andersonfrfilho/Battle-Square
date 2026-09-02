# Construção do mundo — do traçado ao chão que se pisa

**Escrito em 02/09/2026.**

O traçado da ilha está pronto e medido: relevo, água, trilhas, região, uso do
solo, cavernas, aquedutos. **Nada disso existe no jogo.**

## A medição que originou esta spec

O `GameMode` do mundo instancia doze atores: montanha, vulcão, caverna, vila,
mata, água de borda, aurora, luz, campos de treino, inimigos, arena. Contra
isso, o que o traçado calcula e ninguém constrói:

| plano | quantos no mapa | ator no mundo |
|---|---|---|
| `FreshWater` — rios, córregos, fontes | 137 · 5 · 5 | — nenhum — |
| `TrailLayout` — trilhas | 23 | — nenhum — |
| travessias — pontes, balsas, vaus | 56 | — nenhum — |
| `LandUseLayout` — 15 usos de solo | 71 | — nenhum — |
| galerias subterrâneas | 158 | — nenhum — |
| `AqueductLayout` | 2 | — nenhum — |
| **relevo** (`GroundHeightAt`) | a ilha inteira | — nenhum — |

**Não faltam elementos de design. Falta encarnação.** É L-041 na escala do
mundo: 679 testes verdes sobre um mundo que ninguém pode ver.

## A regra que governa esta feature inteira

Do `CLAUDE.md`, e ela já falhou **três vezes** neste projeto — pets, inimigos
do mundo, e o próprio jogador:

> Ator que nasce com componente visual mas SEM asset atribuído passa em todo
> teste de lógica e não existe na tela.

Toda tarefa abaixo que cria ator carrega, obrigatoriamente:

1. malha e cor **atribuídas no construtor**, nunca esperando edição de asset;
2. um teste que verifica a **ATRIBUIÇÃO**, não só a existência do componente;
3. uma linha no painel de depuração dizendo o que foi construído e quantos.

Sem os três, a tarefa não está pronta — está invisível.

## O mundo é ASSADO, não recalculado — e o motivo não é velocidade

O traçado não muda entre partidas: ele não depende de jogador, de relógio nem
de semente de sessão. Então ele se calcula **fora do jogo**, uma vez, e o
resultado é embarcado como dado.

**O argumento decisivo é a fonte única.** Hoje a carta é o retrato de *uma*
execução do gerador. Se o jogo recalcular por conta própria, carta e mundo
podem divergir em silêncio, e "conferir contra a carta" vira disciplina — o
tipo de coisa que falha na terceira edição. Lendo os dois o **mesmo arquivo**,
divergir deixa de ser possível. É L-032 na escala do mundo.

### E a medição fecha a questão

A alternativa honesta era rodar os mesmos algoritmos no jogo, produzindo o
mesmo formato. Ela se decide por medição, não por gosto — e a medição foi
feita: montar o mundo inteiro leva **136 segundos**.

(O número inclui a serialização do JSON, então o plano puro é menos. Mas mesmo
metade disso é um minuto, e ninguém espera um minuto para entrar no mundo.)

Há um segundo motivo, técnico: o traçado usa `float` — `sin`, `PI`, `FMath` —
ao contrário do `BattleSim`, que é inteiro por decisão. Determinismo em ponto
flutuante **entre plataformas** não é garantido, e gerar em runtime poderia
produzir ilhas sutilmente diferentes no Mac e no Windows. O assado elimina a
pergunta em vez de apostar nela.

**Isto não é abandonar a geração procedural — é mudar quem a executa.** Nenhuma
coordenada passa a ser escrita à mão, o gerador continua sendo a fonte de tudo,
e um bioma novo continua sendo uma linha no `WorldBudget` mais um reassar.

O ganho de tempo é consequência, não a razão. E o tamanho não é obstáculo: o
plano inteiro cabe em **601 KB** — 6.947 pontos de traçado e uma grade de
32.400 alturas.

### A guarda contra o assado velho

O modo de falhar é silencioso e conhecido: alguém muda `WorldBudget`, o raio da
ilha ou a forma da costa, e o assado continua respondendo — o mundo passa a ser
de uma configuração que não existe mais, sem nada quebrar.

Por isso o assado grava o **hash dos parâmetros que o geraram**, e o jogo
compara na carga. Não bateu, ele diz alto qual parâmetro mudou e manda reassar.
Um aviso silencioso aqui seria pior que nenhum.

E o gerador **continua existindo e continua testado** — as 679 provas atuais
são sobre ele. O que muda é quem o executa: uma ferramenta de editor, não o
jogo.

## A ordem, e por que ela não é negociável

**O relevo primeiro.** Sem terreno, o rio flutua, a trilha não sobe nada e o
barranco não barra ninguém. Hoje o chão é sondado por *line trace* contra o que
houver; o relevo calculado nunca vira geometria.

Depois a água, porque a trilha atravessa ela (56 travessias) e precisa dela no
lugar para a ponte fazer sentido. Depois as trilhas. Por último o uso do solo,
que é o que enche o mundo de motivo para andar.

Cada fase termina com o mundo **jogável e conferível contra a carta** —
`docs/mundo/carta-ilha-de-mata.html` é o gabarito, e a conferência é o aceite.

## O que esta spec NÃO faz

- **Não muda o traçado.** Se a construção revelar defeito de traçado, ele vira
  tarefa separada com teste próprio — misturar as duas coisas foi o que tornou
  a depuração cara em agosto.
- **Não busca arte final.** Malha primitiva com cor de token é o alvo: o mundo
  precisa ser legível, não bonito. Arte é fase posterior.
- **Não faz streaming.** A ilha inteira de uma vez; `streaming-de-mundo` já
  existe como feature separada e entra depois que houver o que transmitir.

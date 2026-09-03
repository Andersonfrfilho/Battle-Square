# OBJETIVO — Segredos e a carta

> **As decisões de conteúdo desta feature foram respondidas em 02/09/2026.**
> Elas vivem em `.specs/project/DECISOES.md` — fonte única, não copiar para cá.
> Três itens seguem em aberto, e estão listados no fim daquele arquivo.


**Aberto em 02/09/2026.** Vem de **P10** em
`.specs/features/pendencias/BLOQUEIO-P7-P8-P9-P10.md`, que a fechou como parada:
o gabarito de aceite é a carta, e a carta hoje é promessa de **completude** —
esconder qualquer coisa reprova a bateria.

## O objetivo, numa frase

Existe coisa no mundo que a carta não mostra, e o painel do jogador só a
menciona **depois** de ele andar até lá.

## PRONTO é isto, e nada menos

- [⛔] **SC1** — o gabarito aprende a dizer "escondido": mostrado, escondido e a SOMA
      ⚠️ **JÁ FEITA em 03/09/2026 pela M9 de `a-carta-muda-uma-vez`**, depois de
      as respostas de 02/09 (K3, K4, J4) fecharem a dúvida para o mercado-negro.
      A terceira contagem existe em `World/IslandChart.h`, e a marca mora na
      mancha (`bHidden`). **Não reescrever** — ler o que está lá.
- [⛔] **SC2** — "escondido" é marca na mancha, não lista ao lado
- [⛔] **SC3** — o mapa do jogador revela por andar; a carta continua calada
- [⛔] **SC4** — o achado aparece na tela, em `LOCTEXT`
- [ ] Esconder passa e APAGAR reprova — as duas metades, provadas
- [ ] `PomarSelvagem`, `CemiterioEsquecido` e as 16 cavernas afirmados em teste
- [ ] Assado antigo carrega e conta ZERO escondidos
- [ ] Bateria completa verde (hoje **858**; o número só sobe)
- [ ] As cinco auditorias limpas, e as três culturas coletando
- [ ] Um commit por task, cada um com o motivo — não só o quê

Enquanto qualquer caixa estiver aberta, o objetivo **continua**.

## Esta feature está PARADA, e por quê

Três decisões são de **produto antes de serem de código**, e estão listadas em
`tasks.md`: **quantos** segredos e de que tipo, se a carta os **conta**, e se
contá-los ainda os deixa ser segredo.

A segunda decide se `ChartConformance` tem número para afirmar. Começar SC1 sem
ela é escolher o gabarito por omissão — e o gabarito é justamente o que esta
feature muda.

Trocar `[⛔]` por `[ ]` ABRE esta feature — é decisão do usuário, e até lá
`goal_status.sh` não a elege como próxima.

## Invariantes

As doze de `corrente`, e três que esta feature acrescenta:

18. **Contar o mostrado não basta: afirma-se mostrado, escondido e a SOMA.**
    Só o mostrado deixa passar esconder e apagar com a mesma cara. É o mesmo
    raciocínio que o teste já escreveu nos `0 pontes` — *"o zero é uma medição,
    não uma ausência de medição"*.

19. **A carta e o painel do jogador são visões DIFERENTES, e não se fundem.**
    A carta é documento do mundo e o escondido nunca aparece nela; o painel é o
    que aquele jogador viu. Fundir as duas tira do gabarito a estabilidade que
    `ChartConformance` espelha.

20. **Marca de estado mora na MANCHA, e ausente é o comportamento de hoje.**
    `bYieldsWater` é o precedente. `UPROPERTY` ausente vira `false` =
    "mostrado", então assado velho continua válido e ninguém ganha segredo de
    carona.

## O que este objetivo NÃO faz

- **Não decide quantos segredos existem.** Isso é decisão do usuário, e a
  feature nasce capaz de expressar **zero** — o gabarito com zero escondidos é
  o mundo de hoje, afirmado de propósito.
- **Não mexe no traçado.** Nenhuma mancha muda de lugar.
- **Não mexe nas cavernas.** As 16 entram só como contagem afirmada; onde elas
  ficam é `cavernas-nas-quedas`.
- **Não inventa sistema de descoberta.** `FWorldDiscovery`, `FWorldMapPins` e
  `FWorldStatusReadout` já existem e já têm teste.
- **Não autora asset.** O achado aparece por texto no painel.

## Se o contexto for compactado

Reler este objetivo, `spec.md`, `./Tools/goal_status.sh`, `git log --oneline -15`,
e continuar da primeira caixa aberta. **Não recomeçar, não replanejar.** Se as
caixas ainda estiverem `[⛔]`, as três decisões não vieram — não abrir por conta
própria, e em especial não escolher se a carta conta os segredos.

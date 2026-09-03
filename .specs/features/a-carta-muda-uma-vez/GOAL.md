# OBJETIVO — A carta muda UMA VEZ

**Aberto em 02/09/2026**, pelo usuário, depois de ele responder as decisões que
estavam paradas em quatro features.

## O objetivo, numa frase

A água ganha **fundura de verdade**, a queda ganha **poço**, o mapa ganha
**pontes** e **segredos** — e o gabarito da carta é reescrito **uma vez só**,
com tudo dentro.

## Por que ele existe, e por que junto

Quatro coisas decididas mexem no MESMO gabarito:

| decisão | o que ela move na carta |
|---|---|
| degrau na rocha (B4) | costa, vilas e trilhas — a rocha é a camada mais baixa |
| fundura por ponto (B2) | reclassifica travessia: rio largo e raso vira vau |
| pontes de bloco, madeira e destruída (B3) | o `0 pontes` deixa de ser verdade |
| mercado-negro escondido (K3) | lugar que a carta CONTA e não aponta |

Fazer uma de cada vez reescreveria a carta **quatro vezes**, e cada reescrita é
uma chance de o mundo e a carta divergirem sem ninguém ver — que é exatamente o
que o gabarito existe para impedir.

**A carta hoje afirma dezesseis números.** Estão medidos e listados em
`spec.md`; entre eles o `0 pontes`, cujo comentário diz que *"o zero é uma
medição, não uma ausência de medição"*. Esta feature é o dia que aquele
comentário previu.

## PRONTO é isto, e nada menos

- [x] **M1** — o gabarito de hoje está CONGELADO em números, antes de mexer
- [x] **M2** — a rocha ganha ESCARPA, a água acha o degrau, e o poço aprofunda
      sozinho *(reescrita: degrau NA QUEDA recriaria um ciclo já consertado)*
- [x] **M3** — a fundura existe POR PONTO no assado, e quem a decide é o gerador
- [x] **M4** — a estimativa privada do traçado MORRE, e o traçado passa a LER
- [ ] **M5** — a cintura é 40% da altura de quem pisa, não uma constante
- [ ] **M6** — pontes existem: bloco, madeira e DESTRUÍDA
- [ ] **M7** — algumas grutas se ligam entre si, e "algumas" é medido
- [ ] **M8** — três mercados-negros, e "bem espalhados" é medido
- [ ] **M9** — a carta aprende a dizer "escondido" sem dizer o quê
- [ ] **M10** — o gabarito novo, escrito UMA VEZ, com o motivo de cada número
- [ ] **M11** — na tela: fundura, ponte e o que se achou
- [ ] Bateria completa verde (hoje **873**; o número só sobe)
- [ ] As sete auditorias limpas
- [ ] Um commit por task, cada um com o motivo — não só o quê

Enquanto qualquer caixa estiver aberta, o objetivo **continua**.

## Não pare entre tarefas

`./Tools/goal_status.sh` diz a próxima. Só se para por três motivos, e todos com
a medição junto: decisão de conteúdo que é do usuário, bateria vermelha que não
é do teste novo, e descobrir que uma task mexe em coisa que outra feature já
tem dono.

## Invariantes

As doze de `corrente`, mais as duas de `itens-e-biologia`, e três desta:

15. **A CARTA MUDA UMA VEZ.** Nenhuma task de M2 a M9 reescreve número de
    gabarito por conta própria: elas medem o que mudou e a M10 escreve tudo.
    Quatro reescritas seriam quatro chances de divergir, e a divergência entre
    mundo e carta é silenciosa por natureza.

16. **A FUNDURA TEM UMA FONTE SÓ.** Hoje o traçado ESTIMA a fundura pela
    largura (`FunduraSobreLargura = 0.065`), e o comentário dele defende a
    estimativa dizendo que a alternativa seria "uma segunda fonte da mesma
    verdade". Com a fundura assada, a estimativa **vira** a segunda fonte — e
    por isso ela morre na M4, não convive.

17. **"ALGUMAS" E "BEM ESPALHADOS" SÃO MEDIÇÃO, NUNCA IMPRESSÃO.** Grutas
    ligadas e mercados-negros distantes viram número derivado (proporção,
    limiar do raio), e o teste reprova o caso que a palavra existe para
    impedir: todas ligadas, ou dois vizinhos.

## O que este objetivo NÃO faz

- **Não cria uma segunda ilha.** Medido: 79 manchas de uso do solo, 7
  assentamentos e 16 cavernas que quase nada faz reagir ainda. Ilha nova
  multiplicaria conteúdo que a primeira não usa (B-001).
- **Não faz área de criminoso que MUDA de lugar.** Todo lugar deste mundo é
  assado uma vez e fica; lugar que se move é camada nova, e é feature própria.
- **Não implementa estações.**
- **Não autora asset.** Ponte e poço aparecem por malha procedural e cor, como
  o resto do mundo.

## Se um item pedir decisão de conteúdo

**Perguntar, não escolher** — menos os dois números que o usuário já delegou:
quantos mercados-negros (piso 3) e quantas grutas se ligam.

## Se o contexto for compactado

Reler este objetivo, `./Tools/goal_status.sh`, `git log --oneline -15`, e
continuar da primeira caixa aberta. **Não recomeçar, não replanejar.**

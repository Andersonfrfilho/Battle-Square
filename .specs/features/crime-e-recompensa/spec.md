# Crime e recompensa — roubo de pet, polícia e procurados

**Escrito em 31/08/2026.**

## A decisão que define o resto

**O crime é ROUBAR PET.** Não é atacar jogador.

A diferença é o jogo inteiro. Se o crime fosse agressão, o PvP teria de ser
possível sem consentimento — e aí todo mundo é atacável a qualquer momento. O
mundo deixa de ser lugar onde se explora e vira lugar onde se olha por cima do
ombro. Com o roubo, o crime é específico e raro, e quem não se mete continua
em paz.

## O que isso EXIGE antes

**O conceito de DONO.** A spec `captura-roubo-e-treinador` está parada há dias
exatamente por isso: sem dono, "sem dono", "selvagem" e "roubado" não têm como
ser distinguidos. Roubo é a primeira coisa que obriga a resolvê-lo.

**B-005 deixa de ser limitação conhecida e vira BLOQUEIO.** Hoje a coleção e a
progressão são gravadas no save do PROCESSO onde a arena roda — o servidor,
numa partida em rede. Está documentado como "correto para single-player e host".

Roubo entre jogadores não sobrevive a isso: o servidor precisa saber de quem é
cada pet para saber que um mudou de mão. **A posse tem de ser do jogador, não
do processo**, e isso é identidade persistente — que M7 já entregou e que a
coleção nunca passou a usar.

Sem esse conserto, tudo abaixo é desenho no papel.

## Como o roubo acontece

Em aberto, e a escolha muda o tom do jogo:

- **Vencendo o dono** — o roubo é consequência de uma batalha ganha. Exige que o
  dono aceite a batalha, então ninguém é roubado dormindo.
- **Do jogador offline** — mais assustador, e injusto: perder um pet sem poder
  reagir é a fonte clássica de abandono de jogo.

**Recomendação: só vencendo, e só um por vez.** O medo tem de ser proporcional
à chance de defesa.

## A lista de procurados

Fica no **poste da praça**, que já existe no traçado da vila e cuja função era
justamente dizer o que está acontecendo. Nome, o que levou, e quanto vale.

É a informação que transforma um crime privado em assunto público — e é o que
dá ao caçador um motivo para sair de casa.

## A recompensa sai do CRIMINOSO, não do ar

**Esta é a regra que impede o conluio**, e ela é a mais importante da spec.

Dois amigos alternando — um rouba, o outro "captura", dividem — é o que toda
economia com recompensa entre jogadores já viu. Se a recompensa viesse do
jogo, seria uma impressora de dinheiro operada por duas pessoas.

Saindo do que o criminoso carrega, o conluio move dinheiro de um bolso para o
outro **menos o atrito**, e deixa de valer a pena. O crime paga o próprio preço.

## O pet roubado fica MARCADO

Ele carrega de quem foi, e enquanto estiver marcado **não se vende nem se
troca**. Sem isso, roubar e vender fecha o ciclo em um minuto e o dono não tem
como reaver nada.

A marca também é o que permite devolver: quem captura o criminoso devolve o pet
a quem o perdeu, e é isso que faz a polícia servir ao jogador roubado em vez de
servir só ao caçador.

## Deslogar não apaga o crime

Se o criminoso some quando a polícia chega, a caçada é teatro. O estado de
procurado **persiste**, e ele reaparece onde parou.

## A POLÍCIA como o primeiro NPC com trabalho

O jogador pediu NPCs com história e roteiro. O policial é o primeiro que ganha
os dois de graça, porque o trabalho dele é observável: patrulha, reage a quem
está na lista, e prende.

E ele resolve um problema do desenho: sem polícia, caçar criminoso seria tarefa
só de jogador — e num servidor vazio de madrugada, ninguém caçaria ninguém.


## O pet roubado se denuncia na BATALHA

**Decidido em 31/08/2026, e é o coração do sistema.**

Existem compradores de pet roubado, e eles são escondidos. Mas a venda **não
limpa nada**: o pet continua identificável, e quem o revela é a BATALHA.

Quando um jogador que viu a lista de procurados enfrenta quem está com o pet
roubado — o ladrão ou quem comprou dele —, o pet é reconhecido. A partir daí:

- **Quem carrega o pet é sinalizado no mapa.**
- **O DONO é avisado** de que o pet dele apareceu, e onde.
- Quem viu pode **chamar a polícia**, e o comprador é punido.

Três coisas boas saem disso, e nenhuma precisou ser inventada à parte:

1. **A lista de procurados vira objeto de jogo.** Ler o poste da praça deixa de
   ser sabor e passa a ser o que te dá o poder de reconhecer. Quem não leu, não
   vê.
2. **A comunidade é a rede de detecção.** O jogo não precisa vigiar ninguém — os
   jogadores reconhecem, e é por isso que o sistema funciona num servidor
   pequeno.
3. **Receptação é crime.** Comprar sabendo é punido, e isso é o que torna o
   comprador escondido um risco em vez de uma saída limpa.

### E o pet roubado é um ATIVO RUIM

Junte com o envelhecimento: usar o pet roubado o expõe, **não usar não o
impede de envelhecer**. Quem rouba fica com uma coisa que não pode mostrar e
que decai na mão.

É o desenho que faz o crime não compensar sem que nenhuma regra diga "roubar é
proibido".

### O risco, dito na cara

**Sinalizar jogador no mapa é ferramenta de perseguição.** Um marcador
permanente sobre alguém é convite para um grupo cercá-lo, e isso deixa de ser
justiça e vira assédio. A sinalização precisa **durar pouco** e vir do
reconhecimento — não de estar na lista.

## Envelhecer, abandonar e reivindicar

**Pets envelhecem, e envelhecem mais rápido quando malcuidados.** É uma
variável constante, não um evento — o que muda a relação com o pet: ele deixa
de ser inventário e passa a ser algo que se mantém.

**Pet abandonado pode ser REIVINDICADO** por quem o trate e cuide dele. É o
caminho de volta para o pet que ninguém quer, e dá função ao abrigo do Centro
de Recuperação além de guardar coleção.

E dá ao criminoso uma saída que não é perdão barato: abandonar o roubado, e
alguém o recupera.

### O que isso exige

Uma variável de IDADE e outra de CUIDADO por pet, correndo com o tempo real ou
com o tempo de jogo — e essa escolha é pesada. Com tempo real, quem some por
uma semana volta e encontra o pet envelhecido, e isso pune ausência em vez de
punir descuido.

## Perguntas em aberto (acrescentadas)

- **A idade tem fim?** Pet que morre de velho é perda permanente, e perda
  permanente afasta muita gente. Ou ele só decai até um piso?
- **O tempo corre offline?** Punir quem não joga é o modo mais rápido de fazer
  alguém não voltar.
- **Cuidar é ativo ou passivo?** Se exige visita diária, o jogo vira tarefa.

## Perguntas em aberto (do desenho original)

- **Quanto tempo dura a marca de procurado?** Eterna transforma um erro numa
  sentença; curta demais faz o crime valer a pena.
- **O criminoso pode se redimir?** Devolver o pet, pagar a fiança, cumprir pena?
- **Organizações criminosas** são grupos de jogadores, ou facções de NPC com
  quem se pode aliar? A segunda é conteúdo; a primeira é sistema social.
- **A polícia pode errar?** Prender quem não roubou é história boa e frustração
  garantida.

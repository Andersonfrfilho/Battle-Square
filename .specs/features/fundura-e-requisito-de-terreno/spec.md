# Fundura da Água e Requisito de Terreno — Especificação

**Status:** Aguarda UMA decisão do usuário (ver "Migração"), o resto aprovado.
**Depende de:** Arenas Variadas, Skills por Pet, Golpes por Pet.

---

## Problem Statement

Duas coisas separadas que se resolvem pela mesma mudança.

**A do usuário, vendo o jogo:** *"água no campo deve ter níveis de fundura […]
uma poça de água, alguns poderes de água não funcionam"*. Hoje água é um valor
só: ou a casa é água ou não é. Uma poça e um rio são a mesma casa.

**A que estava registrada e parada:** o tipo Terra queria a skill *escavar* — a
mesma postura de subsolo que submergir usa, exigindo pedra em vez de água — e
não pôde tê-la, porque **o requisito de terreno mora no código do núcleo**:

```cpp
State.CellLayout[...] != static_cast<uint8>(ECellProperty::Water)
```

Uma skill nova exige editar `BattlePhasePosture.cpp`. Isso está escrito no
`PetTypes.json` como ausência declarada desde 30/08.

## Goals

- [ ] Água tem NÍVEL: poça e fundo são casas diferentes
- [ ] Submergir exige o nível fundo; na poça ele FALHA e diz por quê
- [ ] Um golpe pode exigir terreno — e o nível dele
- [ ] `escavar` deixa de ser impossível: vira dado, não código
- [ ] Nenhuma partida existente muda de comportamento

## Out of Scope

| Item | Razão |
|---|---|
| Mais de dois níveis | Poça e fundo já produzem a decisão ("vale andar até o fundo?"); um terceiro divide sem acrescentar escolha |
| Afogamento, nado, corrente | Mecânicas novas, cada uma com regra própria de turno |
| Nível de água mudando durante a batalha | O golpe que alaga já existe (`terrainEffect: water`) e cria água FUNDA; alagar em etapas é feature à parte |

---

## Decision Points

### DP-fundura-01 — O nível é um VALOR DE CASA, não um campo paralelo

**Decisão: `ECellProperty` ganha `AguaRasa`, acrescentado ao FIM.**

Um array paralelo de profundidade custaria mais um campo no hash do estado e
mais uma coisa que pode discordar do `CellLayout`. A casa já tem um byte que
diz o que ela é; o nível é parte do que ela é.

**`Water` continua sendo a FUNDA.** É onde submergir funciona hoje, então todo
dado existente — `Arenas.json`, golpes com `terrainEffect: water`, snapshots de
determinismo — mantém exatamente o comportamento atual. A rasa é a novidade, e
novidade que não mexe no que existe é novidade barata.

Ao fim do enum, pela regra de sempre: os valores viajam no hash e no traço, e
inserir no meio reinterpreta o que já foi gravado.

### DP-fundura-02 — O requisito de terreno vira DADO do golpe

**Decisão: o golpe declara o terreno que exige, na mesma cadeia assinada de
`requiresAttribute`.**

Hoje "submergir precisa de água" é um `if` dentro do núcleo. Isso torna cada
skill nova uma edição de `BattlePhasePosture.cpp` — e foi o que impediu
`escavar` de existir.

Como dado, três coisas se resolvem com a mesma peça:

| O quê | Declara |
|---|---|
| Poder de água que não funciona em poça | exige água FUNDA |
| `escavar`, do tipo Terra | exige pedra |
| `submergir` | deixa de ser exceção no código |

**Entra sob assinatura**, como `power` e `requiresAttribute`, e pelo mesmo
motivo: um requisito fora dela seria o caminho para usar na poça o que precisa
de rio.

### DP-fundura-03 — A rasa é ANDÁVEL, e é isso que a torna uma decisão

**Decisão: poça não bloqueia movimento; ela só não serve para o que precisa de
fundura.**

Se a poça barrasse, ela seria uma casa bloqueada molhada, e o jogador a
trataria como parede. Andável, ela vira pergunta: *vale gastar duas casas de
movimento para chegar ao fundo, ou ataco daqui?*

### DP-fundura-04 — Lâmina e pé, já separados

A geometria para isto já existe, e nasceu de um defeito: a água afundava
dezoito unidades e ficava **debaixo** do solo maciço da clareira — o jogador
via terra numa casa que a regra chamava de água.

O conserto separou **lâmina** (o que se vê, acima do solo) de **pé** (onde o
pet pousa, dentro dela). A fundura é essa separação com dois valores: a poça
molha o pé, o fundo o engole.

> **A primeira tentativa de conserto subiu as duas juntas**, apagando a
> profundidade — e dois testes antigos reclamaram. Eles estavam certos: eu ia
> atropelar uma intenção para consertar outra.

### DP-fundura-05 — A recusa DIZ o motivo

Submergir numa poça já tem caminho: `PosturaFalhou` existe e é narrado. A
mensagem precisa dizer **fundura**, e não "não há água" — porque há água, e um
aviso que descreve errado o mundo é pior que nenhum.

---

## Migração — a decisão que falta

**Quais casas de água existentes viram rasas?**

- **Nenhuma** (recomendado): toda água de hoje continua funda, e a rasa só
  aparece em arena nova ou no mundo. Zero mudança de comportamento, e a
  fundura entra como escolha de quem desenha a arena.
- **Algumas, à mão**: converter parte das arenas do `Arenas.json` para rasa,
  o que faz a fundura aparecer já nas partidas de hoje — ao custo de mudar
  arenas que o usuário talvez já conheça.

---

---

## Fatia 2 — Gelo (decidido em 30/08/2026)

**O usuário:** *"derrete com o tempo e depende do nível de congelamento"*.

### DP-gelo-01 — O NÍVEL É O TEMPO

**Decisão: não existe "nível de congelamento" separado da duração. Um
congelamento forte é um que demora mais a derreter.**

Isso não é economia de campo — é o que a coisa É. Duas casas de gelo que duram
o mesmo e diferem num "nível" abstrato seriam indistinguíveis para quem joga, e
o número existiria só no código.

E resolve como guardar o estado: uma casa temporária precisa de um contador, e
o contador é o nível.

### DP-gelo-02 — Terreno TEMPORÁRIO é um conceito, não um caso de gelo

**Decisão: `FBattleState` ganha UM array de contador por casa. Zero é
permanente.**

A alternativa era `Gelo1`/`Gelo2`/`Gelo3` no enum, decrementando o valor. Ela
funciona e cabe no byte que já existe — e foi recusada por dois motivos: três
valores para um conceito só poluem o enum que os golpes usam como
`terrainEffect`, e o próximo terreno temporário (lama? fogo no chão?) pediria
mais três.

Com um contador genérico, o gelo é a PRIMEIRA casa temporária, não a única
possível.

**Para o que ela volta** é implícito: gelo volta a ser água. Guardar o destino
custaria um segundo array para uma pergunta que hoje tem uma resposta só, e
acrescentá-lo depois é aditivo.

### DP-gelo-03 — Derrete no fim do SLOT, como a postura expira

O lugar já existe: `BattlePhaseResolution` decrementa duração de efeito e expira
postura. O gelo entra ali, pelo mesmo motivo — é o único ponto do turno em que
"passou tempo" tem significado.

E o degelo é **narrado**, como tudo que muda o tabuleiro sem o jogador mandar:
casa que volta a ser água sem aviso faz a jogada seguinte parecer defeito.

### DP-gelo-04 — Congelar é um golpe, e a força vem do atributo

Congelar é `terrainEffect: ice` — a mesma peça que já alaga. **Quanto** ele
dura vem do atributo do pet, pela mesma regra que a spec propõe para a
quantidade de água: poça é barata, rio exige atributo alto.

### O que o gelo AINDA não decide

- **Congelar prende quem está submerso?** Sair, ficar preso um turno, ou tomar
  dano — as três são jogáveis e produzem jogos diferentes.
- **Escorregar é movimento a mais ou movimento perdido?** Idem.

As duas podem esperar: gelo que só impede submergir e derrete sozinho já é uma
jogada completa — nega o terreno do oponente por alguns turnos.

---

## Onde isto encontra a arena no lugar

A proposta de **lutar onde o encontro aconteceu** (registrada em conversa,
ainda sem spec) muda de onde vem o terreno: em vez do `Arenas.json`, ele viria
do mundo naquele ponto. Aí a fundura deixa de ser um valor escolhido por quem
desenha a arena e passa a ser **o rio que está ali**.

As duas features não dependem uma da outra, e a ordem certa é esta: a fundura
primeiro, porque ela é pequena e o requisito de terreno como dado é peça que a
outra vai usar.

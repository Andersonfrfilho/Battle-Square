# Atributos que Evoluem, e Golpes que os Exigem

**Status:** Aprovado para execução (2026-08-29).
**Substitui** a proposta anterior de "desbloqueio por proficiência de skill",
que eu tinha entendido errado — ver a nota de correção ao fim.

## O desenho, na descrição do usuário

> "todos os pets podem evoluir skills e personalidade e musculatura que ficam
> gravados como atributos, isso desbloqueia as escolhas de ataques e magias que
> dependem desses atributos"

Três ideias, e a terceira é a que organiza tudo:

1. Todo pet **evolui** — não só sobe de nível.
2. O que evolui vira **atributo gravado**: skill, personalidade, musculatura.
3. Golpes **exigem** atributos. O pet que atende, pode usar.

## Por que isto é melhor que uma marca de desbloqueio

Na proposta anterior, "desbloqueado" seria um estado guardado — uma lista de
golpes liberados por pet ou por espécie. Isso traz um problema conhecido: quem
guarda a lista decide o próprio dano, e save editado libera tudo.

Aqui **não existe lista**. A disponibilidade é **derivada**: o golpe aparece se
o atributo alcança o requisito. Isso muda três coisas de uma vez:

- **A pergunta "é do pet ou da espécie?" se dissolve.** O requisito é sobre
  atributo, e atributo é do indivíduo. Dois Faísca treinados diferente têm
  golpes diferentes — sem nenhuma regra nova dizendo isso.
- **Não há estado novo para forjar.** Save editado pode mentir atributo, que é
  exatamente o que ele já podia fazer com nível. Nenhum buraco novo.
- **O jogador entende sem tutorial.** "Este golpe pede musculatura 60, você tem
  45" é legível. "Este golpe está bloqueado" não explica nada.

## Os três atributos novos

| Atributo | O que representa | Cresce quando |
|---|---|---|
| **Musculatura** | força bruta, impacto | o pet ataca e causa dano |
| **Skill** | técnica, precisão, controle | o pet usa skills (camuflar, voar, submergir) e esquiva/defende com sucesso |
| **Personalidade** | temperamento | conforme COMO o jogador joga — ver abaixo |

Eles ficam ao lado de ataque, defesa, velocidade e vida, que já existem.

### Personalidade é o único que não é um número "quanto maior melhor"

Musculatura e skill sobem. Personalidade **não sobe: ela se inclina.** É um
eixo, e o pet anda para um lado ou para o outro conforme o jogo:

```
CAUTELOSO  ← ────────────── 0 ────────────── →  AGRESSIVO
   defender, esquivar,            atacar, magia,
   camuflar, submergir            golpe forte, avançar
```

Isso importa porque permite golpes que exigem **um lado**, não um mínimo: um
golpe de contra-ataque pede cautela; um golpe de fúria pede agressividade. Um
pet não pode ter os dois — e é aí que a personalidade vira decisão em vez de
mais um número para maximizar.

**PROPOSTA, e é a única coisa aqui que eu inventei:** o eixo. Se você preferir
personalidade como traço nomeado (calmo, feroz, esperto) em vez de eixo, muda o
modelo e eu refaço — mas o eixo é o que permite requisito de golpe sem uma
tabela de traços que precisa ser mantida à mão.

## Decisões

**DP-atr-01 — Disponibilidade é DERIVADA, nunca guardada.** Não existe lista de
golpes desbloqueados. Existe atributo, e existe requisito.

**DP-atr-02 — O requisito é DADO ASSINADO**, junto do golpe. O backend diz
"Explosão pede musculatura 60"; o cliente compara com o atributo local. O
critério nunca é regra do save.

**DP-atr-03 — Atributo cresce por uso EFETIVO.** Musculatura sobe com dano
causado, não com ataque desferido; skill sobe com esquiva que evitou golpe, não
com esquiva no vazio. Sem isso, o jogador é recompensado por moer contra um bot
parado, e não por jogar.

**DP-atr-04 — Todo pet evolui, sem exceção.** Não há pet que não cresce.

**DP-atr-05 — Golpe que o pet não alcança APARECE, trancado, com o requisito
visível.** Esconder faria o jogador não saber que existe algo a perseguir — e
`Explosão — pede musculatura 60 (você tem 45)` é o que transforma o atributo em
objetivo em vez de número.

## Atributo alto também muda a RESOLUÇÃO, não só o que aparece

> "quando temos bastante alto algum número específico, ele quando recebe um
> ataque desvia dependendo da numeração randômica que tirar ali; ou quando ele
> é muito agressivo, o ataque perto do oponente fica mais constante conforme a
> numeração que ele tira"

Ou seja: atributo não serve só para **destravar golpe** — ele muda a **chance**
dentro do combate. Duas formas, uma para cada ponta do eixo:

| | Quem ganha | O que muda |
|---|---|---|
| **Esquiva por reflexo** | skill / cautela alta | ao RECEBER ataque, chance de desviar sozinho, sem ter gastado a ação em Esquivar |
| **Golpe constante** | agressividade alta | ao ATACAR adjacente, o dano varia menos — o agressivo é confiável no que ele faz |

### O que isso introduz no jogo, e precisa estar dito

**Hoje o combate não tem acaso nenhum.** Ataque adjacente sempre acerta; o
único sorteio da partida é a escolha da IA. Isto acrescenta **acaso na
resolução**, e a consequência é real: **dá para perder por causa de um número.**

Isso não é necessariamente ruim — é o que faz atributo alto valer sem ser
garantia — mas exige três amarras, ou vira frustração:

**DP-atr-06 — Todo sorteio sai do `FBattleRandom` do estado.** Nunca
`FMath::Rand`, nunca relógio (AD-004). Sem isso o replay diverge, e numa
partida em rede os dois lados resolvem diferente — o defeito mais caro que este
projeto pode ter.

**DP-atr-07 — O acaso é ESTREITO, e o atributo domina.** A esquiva por reflexo
tem teto (proposta: 25% no atributo máximo) e a constância reduz variação, não
garante dano. Se o número decidir mais que a decisão, o jogo deixa de premiar
quem joga melhor — e o commit às cegas já traz incerteza suficiente.

**DP-atr-08 — Todo sorteio que muda o resultado é NARRADO.** `Brisa desviou por
reflexo` precisa aparecer. Dano que some sem explicação parece defeito, e este
projeto já gastou rodadas com jogador achando que a regra estava quebrada
quando ela estava funcionando.

### Proposta concreta, para você aceitar ou corrigir

- **Esquiva por reflexo:** chance = `skill / 4`, teto 25%. Só contra ataque
  FÍSICO — magia já ignora esquiva declarada (BTL-10), e ignorar as duas
  tornaria magia obrigatória.
- **Golpe constante:** o dano do golpe varia ±20% por padrão; a agressividade
  reduz essa faixa até ±5% no extremo. O cauteloso bate mais irregular, o
  agressivo bate no que promete.

Os dois números são chute meu e existem para dar o que criticar — a decisão de
equilíbrio é sua, e só se toma jogando.

## Fatiamento

**Fatia 1 — os três atributos existem, crescem e APARECEM.** Sem trancar nada.
Entrega a progressão e o retorno na tela, e responde sozinha a pergunta que
decide o resto: **ver o pet mudar é interessante?**

**Fatia 2 — requisito assinado no golpe** (`requiresAttribute`,
`requiresValue`), a mesma cadeia de cinco lugares das fatias anteriores.

**Fatia 3 — a tela mostra o golpe trancado com o requisito**, e anuncia quando
ele abre.

**Fatia 4 — atributo alto muda a resolução**: esquiva por reflexo e golpe
constante, sorteados pelo gerador do estado e narrados. Por último de propósito:
ela acrescenta acaso ao combate, e acaso só deve entrar depois que o atributo já
significa alguma coisa visível para o jogador.

## Onde o treino acontece (usuário, 2026-08-29)

> "temos campos de treinamento para as coisas, e também o dono do pet pode
> estudar para treinar os pets"

Duas camadas novas, e elas mudam de onde vem a progressão:

**Campo de treinamento** — lugares no mundo que treinam um atributo específico.
Isso dá **destino** ao mundo aberto: hoje ele só serve para encontrar inimigos,
e caminhar até um lugar por um motivo é o que separa mapa de corredor.

**O dono estuda** — o treinador tem progressão própria, e ela melhora o que ele
consegue treinar. É a primeira coisa no jogo que pertence ao JOGADOR e não a um
pet: ela atravessa a coleção inteira e sobrevive à troca de pet favorito.

### O que isso resolve, e o que abre

**Resolve** o problema mais chato de progressão por uso: sem campo de treino, o
único jeito de subir musculatura é batalhar, e batalhar contra um bot parado é
moagem. Com campo, o jogador escolhe **investir tempo** num atributo.

**Abre** uma pergunta de ritmo que precisa de resposta antes de virar código:
treinar é **instantâneo** (chegou, treinou, subiu), **por tempo** (deixa lá e
volta), ou **por desafio** (o campo propõe algo que precisa ser cumprido)?
Instantâneo vira clicar num poste; por tempo vira esperar; por desafio é o
único que é jogo — e é o mais caro.

**DP-atr-09 (PROPOSTA) — Estudo do dono MULTIPLICA, não substitui.** O
treinador que estudou faz o mesmo treino render mais; ele nunca treina no lugar
do pet. Assim o estudo tem valor sem tornar o pet irrelevante, e um jogador
novo não fica travado atrás de uma barra que ele nem sabia que existia.

## Decidido por mim, na falta de resposta

**Skill é POR SKILL, não um número só.** O pet tem proficiência separada em
camuflagem, voo e subsolo. Foi o que escolhi quando perguntei e a resposta foi
"pode seguir".

Razão: "evoluir skills" está no plural, e a versão separada é a única em que
**voar muito destrava golpe aéreo, e não golpe de camuflagem** — que é o que
torna o caminho do jogador legível no pet dele. O custo é três números em vez
de um, e três números são baratos.

Se a intenção era um atributo de técnica geral, isto se colapsa em um campo e
eu refaço — mas prefiro registrar a escolha a deixá-la implícita.

## Perguntas abertas

1. **Personalidade: eixo ou traço nomeado?** (proponho eixo)
2. Atributo tem **teto**? Sem teto vira número que só cresce; com teto, o
   jogador sabe quando terminou aquele caminho.
3. Um golpe pode exigir **mais de um** atributo? (ex.: musculatura 50 **e**
   cautela)
4. O pet começa com quantos golpes ao alcance — **um** ou dois?
5. Os números da fatia 4 (25% de esquiva, ±20% a ±5% de variação) são chute meu
   — servem de começo, e só o jogo diz se estão certos.
6. **Campo de treinamento: instantâneo, por tempo, ou por desafio?** Só o
   terceiro é jogo, e é o mais caro.
7. O estudo do dono tem **custo** (tempo, item, dinheiro) ou é só acúmulo?

---

## Nota de correção

A versão anterior desta spec descrevia desbloqueio por **proficiência de
skill**, com marca guardada por espécie. Foi um mal-entendido meu: o usuário
falava de **atributos que evoluem e golpes que os exigem**, que é um sistema
diferente e melhor — não guarda estado, dissolve a pergunta "pet ou espécie", e
não abre superfície nova para adulteração.

Fica registrado porque o erro foi meu de leitura, e porque a decisão de espécie
(DP-desb-01 da versão anterior) **deixou de existir**: com requisito por
atributo, a pergunta não se aplica.

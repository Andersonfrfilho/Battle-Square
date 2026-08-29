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

## Fatiamento

**Fatia 1 — os três atributos existem, crescem e APARECEM.** Sem trancar nada.
Entrega a progressão e o retorno na tela, e responde sozinha a pergunta que
decide o resto: **ver o pet mudar é interessante?**

**Fatia 2 — requisito assinado no golpe** (`requiresAttribute`,
`requiresValue`), a mesma cadeia de cinco lugares das fatias anteriores.

**Fatia 3 — a tela mostra o golpe trancado com o requisito**, e anuncia quando
ele abre.

## Perguntas abertas

1. **Personalidade: eixo ou traço nomeado?** (proponho eixo)
2. Atributo tem **teto**? Sem teto vira número que só cresce; com teto, o
   jogador sabe quando terminou aquele caminho.
3. Um golpe pode exigir **mais de um** atributo? (ex.: musculatura 50 **e**
   cautela)
4. O pet começa com quantos golpes ao alcance — **um** ou dois?

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

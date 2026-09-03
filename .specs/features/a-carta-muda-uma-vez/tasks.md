# A carta muda uma vez — tarefas

## M1 — CONGELAR o gabarito de hoje, antes de mexer ✅ FEITO
> 🤖 Modelo: `sonnet`

**Dependências:** nenhuma. É a primeira porque só se sabe o que mudou quem
anotou o que era.

Os dezesseis números da carta viram um teste que os afirma **de uma vez**, com a
data da medição. Ele existe para reprovar quando a M2 mexer na rocha — e é
reprovando que ele diz **o que** o degrau moveu.

⚠️ **Isto não duplica `ChartConformanceTest`.** Aquele afirma os números como
gabarito de produto; este os afirma como *estado congelado antes da mudança*, e
morre na M10. Dois testes com o mesmo número e propósitos diferentes é aceitável
por um commit; três seria copiar.

*Aceite:* o teste passa hoje, com os dezesseis números.

*Contrapeso:* ele DIZ a data e o motivo de existir, e a M10 o apaga. Teste
temporário que sobrevive vira um segundo gabarito.

---

## M2 — A rocha ganha DEGRAU na queda
> 🤖 Modelo: `opus` 🧠 — mexe na camada mais baixa do mundo

**Dependências:** M1.

O poço tem 886 de meia-largura e 30–51 de fundura: é prato. A rocha ganha um
degrau na queda, e o poço passa a cavar.

⚠️ **A rocha é a camada mais baixa** (rocha → água → região → relevo → trilhas →
solo). Mexer nela move costa, vilas e trilhas de tabela — e a M1 vai reprovar
dizendo quanto.

*Aceite:* o poço fica mais FUNDO que largo? **NÃO** — e este é o erro que já
custou uma vez: o "dez vezes" da literatura é razão de VELOCIDADE de erosão,
vertical sobre lateral, não a forma do buraco. O aceite é que a fundura do poço
**cresça em relação à de hoje** e que ele deixe de ser plano, medido.

*Contrapeso:* nenhuma queda SEM poço ganha degrau, e o degrau não aparece longe
da queda — senão o relevo inteiro vira escada.

---

## M3 — A FUNDURA existe por ponto no assado
> 🤖 Modelo: `opus` 🧠 — muda o formato do assado

**Dependências:** M2 (a rocha decide o leito).

Cada ponto de curso passa a levar **fundura**, como já leva meia-largura. Quem a
decide é o gerador, não quem lê.

*Aceite:* o assado traz fundura por ponto, e ela VARIA ao longo do curso —
cabeceira rasa, curso baixo fundo. Fundura constante seria a estimativa com
outro nome.

*Contrapeso:* o hash de parâmetros do assado muda, e a divergência **nomeia o
parâmetro novo** — é o guarda que já existe.

---

## M4 — A estimativa privada MORRE, e o traçado LÊ
> 🤖 Modelo: `sonnet`

**Dependências:** M3.

`FunduraSobreLargura = 0.065` sai. `FunduraEm()` passa a ler o assado.

**Por que matar e não manter como fallback:** o comentário dela diz que a
alternativa seria "uma segunda fonte da mesma verdade". Com a fundura assada, é
ELA que virou a segunda fonte. Fallback de fonte de verdade é fonte de verdade.

*Aceite:* o grep por `FunduraSobreLargura` não acha nada, e as travessias
continuam sendo classificadas — com os números que a M10 vai gravar.

*Contrapeso:* um teste que prova que a classificação agora RESPONDE à fundura
assada: mudar a fundura de um curso muda a travessia dele. Sem isso, o traçado
poderia estar lendo e ignorando.

---

## M5 — A cintura é 40% da ALTURA de quem pisa
> 🤖 Modelo: `sonnet`

**Dependências:** M3.

Deixa de ser constante e vira fórmula. As três âncoras (100, 88, todas-abaixo-
de-94) param de brigar porque nenhuma precisa ganhar.

*Aceite:* dois pets de alturas diferentes na MESMA água — um se molha, o outro
passa. É a diferença que é o aceite; um valor só não prova fórmula nenhuma.

*Contrapeso:* pet sem altura declarada usa a altura padrão e se comporta como
hoje — cadastro antigo não muda de comportamento sozinho.

---

## M6 — PONTES existem: bloco, madeira e destruída
> 🤖 Modelo: `opus` 🧠 — o `0 pontes` deixa de ser verdade

**Dependências:** M4.

Três materiais, e o terceiro é o que muda o mapa: **destruída** é travessia que
existe e não serve.

*Aceite:* o traçado produz ponte, com material, e a destruída **não deixa
passar**. Uma ponte destruída que se atravessa é decoração.

*Contrapeso obrigatório:* a ponte destruída **aparece** na travessia como
travessia — quem chega vê que houve caminho ali. Sumir com ela seria o mesmo que
não a ter.

⚠️ A forma precisa comportar **ligar ilha a ilha** (F3), ainda que só haja uma
ilha hoje.

---

## M7 — ALGUMAS grutas se ligam
> 🤖 Modelo: `sonnet`

**Dependências:** nenhuma (as cavernas já existem).

*Aceite:* a proporção de grutas ligadas é PARÂMETRO medido, e o teste cobra o
teto — nunca zero, e nunca todas.

*Contrapeso obrigatório:* **não todas.** Se todas se ligassem, o subsolo viraria
um corredor só e achar passagem deixaria de ser achado. O teste reprova o
"todas" tanto quanto o "nenhuma".

---

## M8 — TRÊS mercados-negros, e "bem espalhados" é medido
> 🤖 Modelo: `sonnet`

**Dependências:** M2 (a rocha move as manchas).

*Aceite:* pelo menos 3, e **nenhum par mais perto que um limiar derivado do raio
da ilha** — nunca um número escrito à mão, para continuarem espalhados se a ilha
mudar de tamanho.

*Contrapeso:* o teste reprova dois vizinhos, que é o defeito que "bem
espalhados" existe para impedir.

---

## M9 — A carta aprende a dizer "escondido"
> 🤖 Modelo: `opus` 🧠 — muda o gabarito de aceite

**Dependências:** M8.

A carta passa a ter três contagens: **mostrado**, **escondido**, e a **soma**.
Sem isso, o mercado-negro precisaria de exceção — e exceção no gabarito é o
começo do gabarito não valer.

*Aceite:* a carta conta o escondido e **não o aponta**; o mapa do jogador o
revela andando.

*Contrapeso:* contar não pode vazar QUAL. O teste afirma que a carta sabe o
número e não sabe a posição.

---

## M10 — O gabarito NOVO, escrito uma vez
> 🤖 Modelo: `opus` 🧠

**Dependências:** M2 a M9.

`ChartConformanceTest` recebe os números novos, **todos de uma vez**, cada um com
o motivo de ter mudado. O teste congelado da M1 morre aqui.

*Aceite:* os dezesseis números viram os números de agora, e o `0 pontes` vira o
número que a M6 produziu.

*Contrapeso:* cada número que MUDOU tem o motivo escrito ao lado; cada número
que NÃO mudou continua afirmado. Um gabarito que só lista o que mudou deixa de
proteger o que ficou.

---

## M11 — Na tela
> 🤖 Modelo: `sonnet`

**Dependências:** M10.

Painel diz a fundura sob os pés; a ponte aparece com o material; o que se achou
escondido é anunciado. Roteiro em `docs/verification/`.

*Aceite:* dá para ver a diferença entre andar num vau e nadar, e para ver que
uma ponte está destruída antes de tentar passar.

*Contrapeso:* nada de linha fixa dizendo "fundura: 0" em terra seca — ausência
não ocupa altura no painel.

---

## Decisões que são do usuário, e que ainda não foram tomadas

1. **Quantos segredos ao todo, e de que tipos além dos três** (lugares
   escondidos, itens, pets raros).
2. **Se a ponte destruída pode ser CONSERTADA** — vira trabalho, e trabalho é
   `cidades-do-interior`.
3. **Quanto degrau** a rocha ganha na queda: é arte, e move o mundo junto.

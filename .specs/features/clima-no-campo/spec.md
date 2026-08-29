# Clima no Campo

**Status:** PROPOSTA — aguarda decisão. Levantada pelo usuário em 2026-08-27, e **redirecionada por ele em 2026-08-28**: o clima nasce da jogada, não de fora.
**Na fila:** sim. Não bloqueia nada em andamento.

## O pedido

> "pegar o clima de algum lugar todo dia e imitar ele no jogo para o campo ter
> alterações no meio da batalha"

Duas ideias distintas, e vale separá-las porque uma é muito mais barata:

| | O que é | Custo |
|---|---|---|
| **Clima do dia** | o mundo real decide o tempo de hoje | busca externa, mas entra UMA vez |
| **Campo mudando no meio da batalha** | a arena se altera entre turnos | mexe no núcleo determinístico |

A segunda **não depende** da primeira: o campo pode mudar durante a batalha com
clima sorteado, e isso já entrega o que a ideia tem de melhor.

## A restrição que decide o desenho

**O `BattleSim` não consulta nada.** Sem relógio, sem rede, sem estado externo
(AD-004). Se o clima for consultado *durante* a batalha:

- o **replay** abre diferente do que aconteceu;
- numa partida **em rede**, os dois lados podem receber climas distintos e
  divergir na resolução — o defeito mais caro que este projeto pode ter.

**Portanto:** o clima é buscado FORA, vira **parâmetro da montagem** (ao lado da
semente), e as mudanças no meio da batalha são calculadas pelo núcleo a partir
dele. Determinístico, no traço, igual para os dois lados e para o replay.

Isso não é limitação técnica contornável — é a fronteira que mantém o combate
verificável.

## Perguntas que precisam de resposta antes de código

### Sobre o clima real

1. **De onde?** Uma cidade fixa (todo jogador vê o mesmo tempo), ou a região de
   cada jogador?
2. Se for por região: **dois jogadores em climas diferentes** enfrentam campos
   diferentes na MESMA partida em rede. Quem manda — o anfitrião, o sorteio,
   ou a média? Sem resposta, isso vira vantagem geográfica silenciosa.
3. **E offline?** Se o clima entra na partida, jogar sem rede é jogar outro
   jogo. Cai para um padrão? Usa o último conhecido? Quanto tempo ele vale?
4. **Localização do jogador é dado pessoal** (`security.md` §1). Usar região
   exige consentimento e nunca pode ir para log. Cidade fixa não tem esse
   problema — é o caminho mais barato em todos os sentidos.
5. **A busca falha.** API fora do ar, cota estourada, rede lenta. O jogo espera,
   ou começa com o padrão? (Esperar por clima para poder lutar é inaceitável.)

### Sobre o campo mudando

6. **O que o clima MUDA?** Propostas, da mais barata à mais cara:
   - chuva transforma casas neutras em **água** (e habilita submergir onde
     antes não dava);
   - sol seca casas de água;
   - vento impede **voar**;
   - neblina dá camuflagem a todos.
7. **Quando muda?** A cada turno, em turno fixo, ou por evento? Mudança a cada
   turno pode fazer a decisão do turno anterior virar pó — e commit às cegas
   com o chão mudando embaixo é frustração, não profundidade.
8. **O jogador é AVISADO antes de escolher?** Se o campo muda depois do commit,
   ele perde por algo que não podia prever. Um aviso de "vai chover no próximo
   turno" transforma sorte em leitura.
9. **Vale em partida ranqueada?** Clima que muda o resultado e vem de fora do
   jogo é justo numa disputa?

---

## Reviravolta: o clima nasce da JOGADA (usuário, 2026-08-28)

> "os pets com alguns poderes podem mudar o clima como jogar fogo na grama gera
> nuvens e pode chover... água se jogar algum poder ou se chover muito pode
> alagar"

**Isto é melhor que o clima de fora, e por um motivo técnico, não só de gosto.**

Ação já está no traço. Clima *causado por ação* é determinístico **por
construção**: mesma semente, mesmas ações, mesmo clima — sem rede, sem offline,
sem divergência entre os dois lados, sem as nove perguntas acima. A restrição
que tornava o clima externo caro simplesmente não se aplica aqui.

E o encadeamento fecha um laço que o jogo já tem:

```
fogo na grama  →  fumaça/nuvem  →  chuva  →  casa vira ÁGUA  →  submergir passa a valer
poder de água (ou chuva forte)  →  alagamento  →  mais casas de água
```

Ou seja: o pet de Água **cria o próprio terreno**. Hoje ele tem uma skill que só
funciona onde já existe água; com isto, ele fabrica a condição dela. Isso é
identidade de tipo virando decisão, que é o que as skills por pet buscavam.

### O que isso decide sozinho

- O clima deixa de precisar de **fonte externa** para existir. A fatia 3 (clima
  real) vira enfeite opcional, não pré-requisito.
- O terreno passa a ser **mutável durante a batalha** — e isso já é a "alteração
  no meio da batalha" que o pedido original queria.

### Perguntas novas que ele abre

1. **Que ação muda o quê?** Precisa de uma tabela ação × terreno → terreno
   (fogo em grama vira fumaça; água em qualquer coisa vira água), no mesmo
   padrão dos catálogos que já existem.
2. **A grama existe?** Hoje `ECellProperty` tem neutra, bloqueada, dano, bônus e
   água. "Grama" e "nuvem/fumaça" seriam valores novos — e cada valor novo é
   estado que entra no hash.
3. **A mudança é imediata ou demora turnos?** Fogo → nuvem → chuva é uma
   CADEIA: se ela resolve num slot, ninguém a percebe; se demora, vira plano.
4. **Alagar prejudica quem não nada?** Se sim, é a primeira regra que pune um
   tipo por existir — e isso muda o equilíbrio inteiro.
5. **O jogador vê a cadeia se formando?** Nuvem visível antes da chuva é a
   diferença entre plano e sorte (mesma razão do aviso em P8).

### Fatiamento revisado

**Fatia 1 — terreno mutável por ação.** Uma tabela ação × terreno, aplicada no
núcleo, no traço. Sem clima, sem rede: fogo em grama deixa fumaça, água alaga.
**É a fatia que tem quase todo o valor**, e agora ela não depende de nada
externo.

**Fatia 2 — a cadeia com atraso** (nuvem → chuva), com o estado intermediário
VISÍVEL na grade.

**Fatia 3 — clima ambiente** (de fora, ou sorteado), que dá a condição inicial.
Opcional, e a única com dependência externa.

---

## Proposta de fatiamento original (clima externo)

**Fatia 1 — clima como parâmetro, sem rede.** `FBattleState` ganha um clima; o
núcleo aplica o efeito dele; a montagem escolhe por semente. Entrega o campo
mudando, testável, sem nenhuma dependência externa. **É a fatia que tem quase
todo o valor.**

**Fatia 2 — mudança ao longo da batalha**, com aviso antes do commit (P8).

**Fatia 3 — clima real**, de cidade fixa, buscado na abertura da sessão, com
padrão para falha e offline. Só aqui entra rede.

A ordem importa: cada fatia é jogável sozinha, e a 3 é a única que traz
dependência externa — deixá-la por último é o que permite descobrir se o clima
melhora o jogo ANTES de pagar por ela.

## Fora de escopo

- Efeito visual de chuva/neve (é apresentação, e não bloqueia a regra).
- Estações, ciclo dia-noite.

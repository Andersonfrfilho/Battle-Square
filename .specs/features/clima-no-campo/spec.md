# Clima no Campo

**Status:** PROPOSTA — aguarda decisão. Levantada pelo usuário em 2026-08-27.
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

## Proposta de fatiamento, se a decisão for seguir

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

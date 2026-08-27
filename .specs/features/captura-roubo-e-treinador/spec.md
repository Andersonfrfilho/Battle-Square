# Captura, Roubo e o Treinador

**Status:** PROPOSTA — aguarda decisão. Levantada pelo usuário em 2026-08-27.

## O pedido

> "temos opções de captura de pets se eles estiverem sem dono ou selvagens
> também podemos roubar e atacar o treinador"

Três capacidades, e elas **não são a mesma coisa**:

| | O que é | O que muda na regra |
|---|---|---|
| **Capturar** | tomar posse de um pet SEM DONO | já existe (`FPetCollectionService::CaptureIfNew`), mas hoje o critério é a derrota, não a posse |
| **Roubar** | tomar um pet QUE TEM DONO | novo: exige que o pet saiba de quem é, e cria consequência para o dono |
| **Atacar o treinador** | agredir a pessoa, não o pet | novo: o treinador precisa existir como alvo, com vida própria ou com uma regra de rendição |

## Por que isto é decisão, não implementação

**O jogo hoje não tem dono.** `FOwnedPetInstance` diz o que É seu; nada diz de
quem é um pet que não é seu. Sem esse conceito, "sem dono", "selvagem" e
"roubar" não têm como ser distinguidos — e inventá-los dentro da captura
criaria a segunda fonte de verdade que já custou L-032, L-033 e um defeito de
direção neste projeto.

**E o treinador não existe como entidade.** Existe o explorador (o corpo que
anda) e existem pets. Atacar o treinador exige decidir o que acontece quando
ele perde: a partida acaba? você perde pets? há captura de pessoa? Cada resposta
é um jogo diferente.

## Perguntas que precisam de resposta antes de código

1. **Um pet selvagem é definido por quê** — ausência de dono, ou uma marca
   própria? (Um pet abandonado é selvagem?)
2. **Roubar é durante a batalha ou depois?** Se durante, é uma AÇÃO no turno,
   e entra no triângulo com atacar/defender/esquivar.
3. **Roubar pode falhar?** Se nunca falha, é dominante. Se falha, o que se
   perde ao tentar — o slot, ou algo mais?
4. **O dono roubado fica sabendo?** Em jogo local não importa; com contas e
   rede (M7, pronto) isso vira consequência real entre jogadores.
5. **Atacar o treinador acaba a partida** ou só interrompe a batalha?
6. **Isto é permitido contra outro JOGADOR**, ou só contra treinadores do
   mundo? Roubo entre jogadores muda o jogo de "coleção" para "conflito".

## Proposta mínima, se a decisão for seguir

Fatiar, e a primeira fatia é a que não exige entidade nova:

**Fatia 1 — posse explícita.** Todo pet do mundo carrega `OwnerId` (vazio =
selvagem). A captura passa a exigir "sem dono", em vez de só "derrotado".
Nenhuma mecânica nova; só a regra que falta para as outras duas existirem.

**Fatia 2 — roubar** como ação de turno, com chance e custo, sobre pet COM dono.

**Fatia 3 — o treinador** como alvo, com regra de derrota própria.

Cada fatia é jogável sozinha, e a 1 é pré-requisito das outras duas.

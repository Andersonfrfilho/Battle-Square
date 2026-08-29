# Golpes por Pet

**Status:** Aprovado para execução (2026-08-28). Decisões do usuário: os golpes
vêm do **backend**, e são **quatro** por pet.

## O problema

Hoje o turno é *"Atacar → direção"*. A direção existia para um motivo real — o
commit é às cegas, então mirar era apostar onde o inimigo **vai estar**. Mas ela
respondia a pergunta errada: num 3x3 com um oponente, "onde" quase sempre é
"nele", e a decisão virava cerimônia com uma chance de errar o vazio.

O usuário decidiu: a escolha do turno passa a ser **qual golpe**, com alvo
automático.

## Por que isso é melhor

Direção dava **uma** decisão (aposta de posição). Golpe dá uma decisão por
turno que envolve tipo, efeito, terreno e o que o oponente fez — e é onde a
identidade do pet vira jogo. É também o que torna possível o que já foi
decidido: **golpes é que mudam o terreno** (fogo na grama vira fumaça, água
alaga).

## Decisões

**DP-golpe-01 — Quatro por pet.** Convenção do gênero, e cabe na tela sem
rolagem. Mais que isso vira lista, e lista é onde a decisão se dilui.

**DP-golpe-02 — Vêm do BACKEND, por pet.** Não do tipo. Skills (camuflar/voar/
submergir) continuam vindo do tipo; golpes são do indivíduo. É o que permite
dois pets do mesmo tipo jogarem diferente.

**DP-golpe-03 — Entram no payload ASSINADO.** O espelho de pets é verificado
registro a registro (PETDB-10). Golpe fora da assinatura seria o caminho óbvio
para adulterar dano — e a assinatura existe exatamente para isso.

**DP-golpe-04 — O segundo byte de `FBattleAction` passa a significar golpe
quando a ação é de ataque.** Ele já é 2 bytes (tipo + direção), e esse tamanho é
a base do custo de rede do commit. Para `Mover` continua sendo direção; para
`Atacar`/`Magia` passa a ser o índice do golpe (0–3). Nada cresce.

**DP-golpe-05 — Alvo automático: o oponente adjacente.** Sem adjacente, o golpe
erra — e o feed diz isso, como já diz hoje.

## Perguntas respondidas por decisão, não por proposta

- **Golpe pode falhar?** Sim: sem alvo adjacente, erra. Precisão por golpe fica
  fora desta fatia.
- **Golpe tem custo?** Fora desta fatia. Sem custo, o melhor golpe é sempre o
  mais forte — mas resolver isso exige uma economia (energia/uso limitado) que é
  decisão própria.

## Fatiamento

**Fatia 1 — backend.** Tabela de golpes, quatro por pet, no payload assinado e
no espelho. Testável com `bun test`, sem tocar no jogo.

**Fatia 2 — o jogo lê e oferece.** `FLoadedPetRecord` carrega os golpes; a tela
mostra quatro botões nomeados no lugar da grade de direção para ataque.

**Fatia 3 — efeito próprio por golpe** (dano, tipo, e o que faz com o terreno).
É aqui que "fogo na grama" acontece.

## Fora de escopo

- Aprender golpe novo subindo de nível.
- Golpe com alcance além do adjacente.
- Custo/energia.

# Roteiro de verificação — Camuflagem, Voo e Subsolo (DP-ia-04)

**Status:** **não verificado ainda.**

Os testes provam as regras. O que eles não decidem: as três se **sentem**
diferentes umas das outras, ou viraram três nomes para "esquivar melhor"?

## Trocar de jogador controlado

Botão **"Controlando jogador N — clique para o jogador M"** na barra do canto
inferior esquerdo. O bot joga pelo outro.

Para os itens que exigem os DOIS agindo no mesmo turno (a trombada), clique em
**"Controlar também o jogador 2"**. O turno passa a pedir duas escolhas, e na
vez do jogador 2 aparece o painel de ações dele na barra. O turno passa a pedir duas escolhas: primeiro
pelo **jogador 1**, depois pelo **jogador 2** — a tela diz por qual você está
escolhendo, e as direções oferecidas seguem o pet dele.

Sem isso, quase todo item abaixo depende do oponente sortear exatamente a ação
que você quer ver. Clicar de novo devolve a decisão do jogador 2 à IA.

## Como usar (enquanto o WBP não tem os botões)

Teclado, durante a escolha de ação: **C** camufla, **V** voa, **B** submerge.
Os três botões precisam ser adicionados a `WBP_BattleActionSelector` —
o C++ já os espera por `BindWidgetOptional`, então basta criá-los com os nomes
`Button_Camuflar`, `Button_Voar`, `Button_Submergir`.

## ESC-01 — As trocas são distintas

- [ ] **Camuflar**: nem físico nem magia acertam — e no slot seguinte seu
      ataque erra ("se revelando").
- [ ] **Voar**: físico erra, magia acerta e dói **mais**. Casa de dano não pega.
- [ ] **Submergir**: nada acerta, casa de dano não pega — e no slot seguinte
      você não anda **nem** ataca.
- [ ] Depois de jogar as três, dá para dizer **quando** usar cada uma?

## ESC-02 — O olhar conta a história

- [ ] Oponente voando → seu pet olha para **cima**.
- [ ] Oponente submerso → olha para **baixo**.
- [ ] Oponente camuflado → o olhar **fica onde estava** (última casa
      conhecida). Ele não deve apontar para o lugar certo: isso entregaria o
      que a camuflagem esconde.

## ESC-03 — O feed explica

- [ ] Cada uma tem frase própria, e não um genérico "assumiu postura".
- [ ] O erro por "se revelando" é distinguível de um erro por mira?
      (Hoje NÃO é — os dois saem como ataque que errou. Decidir se merece
      frase própria é julgamento de jogo, não de código.)

## ESC-04 — Equilíbrio

- [ ] Camuflar/submergir todo slot é forte demais, mesmo custando a ação
      seguinte?
- [ ] Voar compensa, dado que magia dói 50% a mais?

---

## ESC-05 — Encontro no mesmo ponto (DP-02, invertido)

- [ ] Os dois indo para a mesma casa: **nenhum** entra, e os dois se ferem.
- [ ] Andar para cima de quem está parado: mesma coisa.
- [ ] **Trocar de casas continua funcionando** — ninguém termina no mesmo ponto.
- [ ] Quem esquivou não se fere na trombada; quem defendeu se fere menos.
- [ ] O feed diz que trombaram, e não um "movimento bloqueado" genérico.
- [ ] Julgamento: a trombada dói o suficiente para desencorajar, sem virar a
      forma mais barata de causar dano?

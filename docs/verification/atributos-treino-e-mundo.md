# Roteiro de verificação — Atributos, Treino e o Mundo (M8)

**Status:** **não verificado ainda.**

Cobre tudo o que M8 acrescentou em 2026-08-29. Os testes provam que os números
somam, que a fila recusa e que o hash muda. O que eles **não** decidem, e por
isso este roteiro existe:

- **Nada disto foi visto na tela.** Sol, mata, painel do mundo, campos de
  treino, golpe trancado e esquiva por reflexo estão verificados por build e
  por teste — não por olho. Este projeto já teve **três atores invisíveis**
  passando por baterias inteiras (`CLAUDE.md`), e o padrão é sempre o mesmo:
  componente criado sem asset atribuído.
- **Se ver o pet mudar é interessante.** É a pergunta que a fatia 1 existia
  para responder, e nenhum teste responde.
- **Se os números estão certos.** Três são chute meu, e estão nomeados abaixo.

O painel se grava sozinho em `Saved/BattleDebug.txt` — não depende de tecla,
de foco nem da área de transferência. É o caminho para me mandar o resultado.

---

## MUNDO-01 — O mundo é um lugar

- [ ] Ao abrir, há **sol e céu** — não a iluminação azul chapada do ambiente
      padrão da engine.
- [ ] Há **chão e mata** em volta, e o chão vai **além** de onde os inimigos
      aparecem. (Andar até o inimigo mais distante sem sair do chão.)
- [ ] A mata **não flutua** nem afunda: o disco encosta no piso do nível.
      *(O chão é achado por traço para baixo; se o nível tiver piso em altura
      inesperada, é aqui que aparece.)*
- [ ] Nada **pisca** onde duas superfícies se encontram. *(Z-fighting já
      aconteceu neste projeto, com a barra de vida.)*
- [ ] O pet do jogador e os inimigos **são visíveis** e distinguíveis do chão.

## MUNDO-02 — O painel diz onde você está

- [ ] O painel mostra **seu pet, nível e experiência**.
- [ ] Mostra os **cinco atributos** numa linha, com os rótulos em português.
- [ ] Mostra **quantos adversários** existem e a **distância em metros** do
      mais próximo.
- [ ] A linha do adversário fica **amarela** quando ele se aproxima, e volta ao
      cinza quando você se afasta. *(A cor é o aviso que se lê sem ler.)*
- [ ] Sem pet na coleção, o painel **diz isso** em vez de ficar calado.
- [ ] Julgamento: o painel ajuda, ou é ruído por cima do jogo?

## TREINO-01 — Os campos existem e chamam

- [ ] Há **cinco discos coloridos** em volta do ponto de partida, um por
      atributo (vermelho musculatura, âmbar personalidade, verde camuflagem,
      azul voo, marrom subsolo).
- [ ] Dá para **vê-los do ponto de partida**, ou ao menos achar um andando um
      pouco. *(Se for preciso procurar às cegas, o raio de 1800 está errado.)*
- [ ] Nenhum deles fica **cinza** — cinza é a cor do "atributo desconhecido", e
      significa campo mal configurado.
- [ ] O disco **não empurra** o jogador: dá para andar por cima dele.

## TREINO-02 — Treinar acontece e se vê

- [ ] Ao entrar num campo, o painel diz `treinando <Atributo> — N% do próximo
      ponto`, e a porcentagem **sobe**.
- [ ] Ao fechar 100%, o **atributo no painel do mundo sobe em 1**.
- [ ] Ao sair do campo e voltar, o progresso **não voltou a zero**.
      *(O resto acumulado atravessa as visitas — sem isso, andar seria punido.)*
- [ ] Pular dentro do campo **não interrompe** o treino.
- [ ] Julgamento: **seis segundos por ponto** é curto demais, longo demais, ou
      certo? É chute meu (`FTrainingFieldRules::SecondsPerPoint`), escolhido
      para ser testável numa sessão — não para ser equilibrado.

## TREINO-03 — A especialidade do treinador

- [ ] Dentro de um campo em que você **não** é especialista, o painel oferece:
      `você pode se especializar em X — N vaga(s), e a escolha NÃO se desfaz`.
- [ ] `bs.Especializar` no console **funciona** e anuncia na tela.
- [ ] Depois de especialista, o treino **naquele atributo** mostra
      `(especialista: +50%)` e rende visivelmente mais rápido.
- [ ] Em **outro** atributo, rende normal. *(A especialidade não vale para
      tudo.)*
- [ ] Depois de **duas**, uma terceira é recusada **com o motivo dito**.
- [ ] **O teste que mais importa:** especializar-se, depois **ganhar
      experiência numa batalha**, e voltar ao campo — a especialidade
      **continua lá**. *(Foi o defeito que quase entrou: gravar a coleção
      apagava o treinador. Se falhar aqui, L-048 voltou.)*
- [ ] Julgamento: **duas de cinco** é escasso demais? E a **falta de troca**
      incomoda? As duas coisas são decisão sua, e estão marcadas em aberto.

## ATRIB-01 — O pet cresce, e dá para ver

- [ ] Ao fim de uma batalha, aparece `Faísca: Musculatura 12 → 14` e afins,
      com o **antes e o depois**.
- [ ] Batalha sem ganho nenhum **diz isso** em vez de ficar calada.
- [ ] O ganho **bate** com o que o painel do mundo mostra depois.
- [ ] Musculatura sobe por **dano causado**, não por atacar o vazio.
- [ ] Julgamento: **ver o pet mudar é interessante?** É a pergunta que a fatia
      1 existia para responder.

## ATRIB-02 — Golpe trancado

- [ ] Um golpe que o pet ainda não alcança aparece como
      `🔒 Explosão (exige Musculatura 12)` — **visível e inerte**, não sumido.
- [ ] Clicar nele **não faz nada**, e não trava a escolha do turno.
- [ ] Os outros golpes continuam funcionando, **no índice certo**. *(Escolher
      o terceiro golpe precisa usar o terceiro, não o que sobrou.)*
- [ ] Ao cruzar o requisito, aparece `Faísca desbloqueou Explosão!` — **uma
      vez**, na batalha em que aconteceu, e **não** em todas as seguintes.
- [ ] Julgamento: o requisito visível vira **objetivo**, ou vira frustração?

## ATRIB-03 — O acaso no combate

- [ ] `desviou por reflexo` aparece no feed quando acontece — frase
      **diferente** de `esquivou`, que é a esquiva que você escolheu.
- [ ] Um pet com reflexo alto desvia **às vezes**, não sempre. *(Teto de 25%.)*
- [ ] O dano do mesmo golpe **varia** entre turnos.
- [ ] Um pet agressivo bate **mais constante** — e não mais forte.
- [ ] **Nada some sem explicação.** Dano que desaparece sem linha no feed é o
      modo de falhar que este projeto mais paga caro.
- [ ] Julgamento: **25% de esquiva** e **±20% → ±5%** são chute meu. Perder por
      causa de um número parece justo, ou parece roubo?

## CAMPO-01 — Tamanho de campo variável

- [ ] Numa grade 3x3, seu pet começa em **(0,1)** e o oponente em **(2,1)** —
      nas bordas opostas, não colados no meio.
- [ ] Mudando `GridColumns`/`GridRows` em `Config/DefaultGame.ini` (ex.: 5x5),
      o tabuleiro **muda de tamanho** e a câmera **ainda enquadra tudo**.
- [ ] Num campo **retangular** (ex.: 4x6), o tabuleiro fica **centrado** na
      clareira — não meia casa para fora.
- [ ] Num campo maior, as arenas de `Config/Arenas.json` deixam de ser
      aplicadas (elas são 3x3) e o campo abre **neutro** — o esperado, não um
      defeito.
- [ ] Julgamento: campo maior torna o movimento mais interessante, ou só mais
      lento?

---

## Como me devolver o resultado

Marque o que passou, escreva o que viu no que falhou, e mande
`Saved/BattleDebug.txt` junto. **Item que reprovar é para ficar escrito aqui** —
o `OBJECTIVE.md` pede o resultado no próprio roteiro, inclusive o que reprovar.

# Roteiro de verificação — Atributos, Treino, Tipos e o Mundo (M8)

**Status:** **não verificado ainda.**
**Atualizado em 30/08/2026** duas vezes: primeiro com os tipos de dois eixos, a
magia de atributo e a paleta; depois com o carregamento, a água, os dois mapas
e os obstáculos derrubáveis (seções CARGA, ÁGUA, MAPA, DERRUBAR).

Cobre tudo o que M8 acrescentou em 2026-08-29 e 30. Os testes provam que os números
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

---

## TIPO-01 — Escola e elemento se leem na tela

O tipo virou um par: `Natural/Fogo`, `Fisica/Terra`, `Psiquica/Agua`. A
**silhueta** diz a escola (malha) e o **elemento** (tombo); a **cor** diz o
elemento de novo, como canal rápido.

- [ ] Os dez pets do catálogo aparecem com adornos **visivelmente diferentes**
      entre escolas: cristal angular (Física), chama alta (Natural), orbe
      pequeno (Psíquica).
- [ ] **A pergunta que decide o desenho:** dois pets da MESMA escola e
      elementos diferentes — por exemplo `Natural/Fogo` contra `Natural/Agua` —
      dá para distinguir **sem olhar a cor**? O tombo do adorno deveria bastar
      (fogo ereto, água deitada).
      *Se não der, o canal de silhueta falhou e a cor está carregando sozinha —
      que é exatamente o que dois testes reprovaram em 30/08.*
- [ ] Um pet de `Fisica/Terra` (Estalagmite) e um de `Psiquica/Fogo`
      (Alfarrábio) não se confundem em nada.
- [ ] O painel de abertura ainda diz o tipo e a vantagem antes da primeira
      escolha.
- [ ] Julgamento: **sete tipos são demais para ler no meio de um turno?**

## TIPO-02 — O Mágico deixou de ser especial

Ele tinha 150% contra os três tipos naturais de uma vez. Agora o eixo da escola
é suave (120/80) e o do elemento é o alto (150/50).

- [ ] Alfarrábio (`Psiquica/Fogo`) contra Faísca (`Natural/Fogo`) mostra
      vantagem **modesta** — não o "super efetivo" de antes.
- [ ] Alfarrábio contra Zunido (`Fisica/Planta`) mostra **desvantagem**.
- [ ] Julgamento: o Mágico ainda parece forte demais? Ele é frágil de
      propósito (30 de defesa, 90 de vida) — a compensação é essa.

## MAGIA-01 — Magia que muda atributo

A escola psíquica muda a luta em vez de encerrá-la. Golpes de efeito batem
fraco de propósito: `Concentração` tem 35 de poder contra os 150 de um
`Colapso`.

- [ ] Usar `Concentração` (Vigília) escreve no feed
      `Vigília concentrou-se: ataque +40%`.
- [ ] E o **ataque seguinte causa visivelmente mais dano** que sem ela.
      *É a ligação inteira: sem isso o bônus é um número guardado que não
      aparece em lugar nenhum.*
- [ ] `Peso da Mente` escreve `enfraqueceu X: velocidade −35%` — frase de
      **enfraquecer**, não a mesma de subir.
- [ ] `Fissura Arcana` (Alfarrábio) derruba a defesa, e o `Grimório` seguinte
      dói mais.
- [ ] Ao fim do turno, aparece `ataque voltou ao normal`. **Não pode sumir
      calado.**
- [ ] Usar a mesma magia duas vezes no turno **não dobra** o bônus.
- [ ] O mesmo golpe usado como **Atacar** (e não Magia) não aplica efeito
      nenhum.
- [ ] Julgamento: gastar um slot para mudar atributo **vale a pena**, ou é
      sempre melhor bater? Se for sempre melhor bater, a escola psíquica não
      existe na prática — e os números precisam mudar.

## PALETA-01 — Criatura é viva, terreno é terra

Corrigido em 30/08: os campos de treino ficaram **dessaturados** para não
disputarem com os pets.

- [ ] Um pet de **Planta** parado no disco de **camuflagem** se destaca dele.
      *Antes eram dois verdes a cinco pontos de distância.*
- [ ] Um pet de **Água** sobre o disco de **voo**: idem.
- [ ] Os cinco discos continuam **distinguíveis entre si** depois de
      apagados — perder saturação não pode ter custado a diferença entre eles.
- [ ] Julgamento: o mundo ficou **sóbrio demais**? A troca foi deliberada —
      os pets são a coisa mais viva na tela — mas o preço é real.

---

## CARGA-01 — A espera passou a ter tela, e a tela passou a ter número

O mundo **não** é montado ao dar play: ele depende do pawn e roda no Tick, com
repetição. Antes, o jogador nascia num mundo cinza e depois levava um engasgo.

- [ ] Ao dar play, aparece a tela de carregamento **antes** de o mundo aparecer
      — e não o mundo pela metade primeiro.
- [ ] Ela mostra o passo em que está (`Conferindo a assinatura dos pets…`,
      `Plantando a mata…`) e a porcentagem **sobe**.
- [ ] Ela **sai sozinha** quando o mundo está pronto, e o que aparece já está
      completo — mata, sol, campos e adversários de uma vez.
- [ ] **O número:** anote o que o painel mostra —
      `carregamento: cenário ___ ms, encontros ___ ms` — e o que o log diz em
      `[carregamento] malhas pesadas prontas em ___ ms`.
      *É a medição que decide o próximo conserto: se o peso está no disco
      (malhas), na mata (cenário) ou na verificação de assinatura. Sem isso,
      qualquer otimização minha é palpite.*
- [ ] Julgamento: a espera ficou **tolerável**, ou ainda é longa mesmo com
      tela? Tela não encurta espera — só a torna explicável.

## CARGA-02 — Falha diz o motivo

- [ ] Renomeando o espelho em `DefaultGame.ini` para um caminho que não existe,
      a tela **fica** e mostra o motivo em laranja — não gira para sempre nem
      some deixando um mundo quebrado.
- [ ] Desfazendo, volta ao normal.

## ÁGUA-01 — O limite se lê de longe

- [ ] Há uma **faixa de água** em volta da ilha, visível de dentro do mapa.
- [ ] Andando até a borda, você **para na margem** — e vê a água antes de
      parar. *Não pode haver parede sentida em terreno seco: a barreira é
      invisível e a razão é visível, nessa ordem.*
- [ ] Não existe **vão** por onde escapar. Tente contornar a borda inteira
      empurrando contra ela. *O anel é um polígono de 24 caixas; vão de um
      centímetro é vão que o jogador encontra.*
- [ ] O guarda de queda **não dispara mais** por andar até o fim do mundo —
      quem não alcança a borda não cai dela.
- [ ] A água é escura e apagada, e a terra se destaca contra ela.

## MAPA-01 — O minimapa acompanha o olhar

- [ ] Há um minimapa no canto superior direito.
- [ ] Ao girar a câmera, o mapa **gira junto**: o que está à sua frente fica
      sempre em cima.
      *É o item que mais importa aqui. O teste da projeção passa nos quatro
      pontos cardeais nos dois modos — se estiver errado NA TELA, o defeito é
      de desenho e não de matemática, o que estreita muito a busca.*
- [ ] Os **cinco campos** aparecem nas cores dos atributos, e andar até um
      deles faz o ponto crescer e ficar centrado.
- [ ] Os **adversários** aparecem, e todos da mesma cor — o mapa diz que há
      alguém ali, não quem.
- [ ] Quem está **fora do alcance** simplesmente não aparece, em vez de ficar
      colado na borda.

## MAPA-02 — O mapa completo é para planejar

- [ ] **M** abre e fecha o mapa completo, e o minimapa continua lá.
- [ ] O completo tem o **norte fixo**: girar a câmera **não** o gira.
      *É de propósito — mapa que gira enquanto se olha para ele não se
      memoriza.*
- [ ] Dá para ver a ilha inteira, com a água como moldura, e localizar os
      cinco campos de uma vez.
- [ ] Com o mapa aberto, ainda dá para andar (ou não — anote o que acontece).
- [ ] Julgamento: com o mapa completo, **dá para planejar uma rota** entre dois
      campos? Se não der, ele é decoração.

## DERRUBAR-01 — Árvore e pedra caem

- [ ] O **botão esquerdo** golpeia. Contra uma árvore, o painel mostra
      `golpe: N de dano, falta M`.
- [ ] Golpeando de novo, a árvore **cai** e some da tela.
- [ ] Golpear o vazio diz `golpe no vazio` — não fica em silêncio.
- [ ] **Capim e flor não caem**, e o golpe os ignora: mirando grama com uma
      árvore perto, é a árvore que apanha.
- [ ] **Pedra aguenta mais** que tronco caído.
- [ ] A árvore derrubada **não volta**, e nenhuma OUTRA árvore some junto.
      *É o risco da implementação: instâncias derrubadas encolhem em vez de
      serem removidas, justamente para o índice não deslocar. Se sumir a
      vizinha, foi isso.*
- [ ] **Treinar musculatura faz diferença:** anote quantos golpes um tronco
      leva antes e depois de subir musculatura num campo.
      *É o ciclo fechando — treino do mundo sentido no mundo.*
- [ ] Julgamento: derrubar é **satisfatório** ou é trabalho? Hoje não há som,
      nem animação, nem queda — a árvore simplesmente some.

## DERRUBAR-02 — O que eu decidi sozinho

- [ ] O golpe do mundo **não atinge os adversários** que andam pelo mapa: eles
      continuam sendo resolvidos na arena.
- [ ] Julgamento, e é decisão sua: era isso que você queria com "destrutíveis
      pelos usuários", ou a intenção incluía **bater nos inimigos no mundo**?
      *Se incluía, isto é metade do caminho — combate fora do tabuleiro muda o
      que o jogo é, e merece spec antes de código.*

## CUBOS-01 — O andaime saiu

- [ ] Os **blocos coloridos** do teste de streaming não aparecem mais.
- [ ] Aparece um aviso laranja dizendo quantos foram removidos e que eles
      devem sair do MAPA.
      *O aviso é chato de propósito: paliativo silencioso vira permanente.*
- [ ] Depois de apagá-los no editor (World Outliner → filtro `Cube` → apagar →
      salvar), o aviso **some sozinho** — e aí me avise para eu remover o
      código (B-010).

## Como me devolver o resultado

Marque o que passou, escreva o que viu no que falhou, e mande
`Saved/BattleDebug.txt` junto. **Item que reprovar é para ficar escrito aqui** —
o `OBJECTIVE.md` pede o resultado no próprio roteiro, inclusive o que reprovar.

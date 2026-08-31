# Roteiro de verificação — o que precisa de olho

**Status:** não verificado ainda.
**Escrito em 30/08/2026**, substituindo os 103 itens de
`atributos-treino-e-mundo.md`. **Atualizado em 31/08** com o mapa por
descoberta e com fantasma, luz e dreno — tudo isso nasceu depois da primeira
versão, e nada disso foi visto por ninguém.

Aquele roteiro misturava duas coisas: o que uma máquina consegue conferir e o
que só uma pessoa consegue. A metade automatizável virou teste e sonda, e pedir
que você a repetisse à mão era gastar o seu tempo com o que já está coberto.

**O que sobrou aqui tem uma coisa em comum: nenhuma máquina responde.** Cor,
espaço, movimento, e a pergunta que nenhum teste faz — *isso é bom de jogar?*

## O que a máquina já responde (e por isso saiu daqui)

| Antes era item do roteiro | Agora responde |
|---|---|
| "o ator aparece na tela" | `Tools/audit_visible_actors.sh` — ator que cria malha e não atribui nenhuma. O defeito de **três atores invisíveis** deste projeto tinha forma fixa |
| ganho de XP, progresso de treino, recusa da terceira especialidade | bateria (464 testes) |
| a poça não deixa submergir; o gelo derrete; a lama dá em três desfechos | `BattleSim.Terrain.*` |
| a arena nasce do lugar; a pedra não bloqueia casa inicial | `BattleSquare.Arena.FromWorld.*` |
| o físico não acerta o fantasma; a luz revela; o dreno cura | `BattleSim.Traits.Incorporeo.*`, `BattleSim.Moves.Drenar.*` |
| andar revela o entorno; a marca apaga ao repetir | `BattleSquare.World.Discovery.*`, `.Pins.*` |
| todo pet de água alaga; as doze combinações existem | testes do catálogo |
| texto do jogador é traduzível; núcleo sem float | sondas |

**Isso não quer dizer que estão certos** — quer dizer que estão *conferidos*.
Um teste verde prova que a regra roda, nunca que ela é boa.

---

## 1. O mundo, antes de qualquer coisa

- [ ] Ao abrir o nível **antes do Play**, não há mais **blocos coloridos** —
      eles saíram do mapa em 30/08. *(Era o andaime do teste de streaming.)*
- [ ] Ao dar Play, há **sol e céu**, e não a luz azul chapada do padrão da
      engine.
- [ ] **O chão existe e segura**: andar até o inimigo mais distante sem cair.
      *(O paliativo que apagava os cubos estava apagando os 25 pisos junto —
      mesma malha. Isto é o que confirma que parou.)*
- [ ] A mata **não flutua** nem afunda no piso.
- [ ] Nada **pisca** onde duas superfícies se encontram.
- [ ] O seu pet e os inimigos são **visíveis** e se distinguem do chão.
- [ ] A serra do horizonte **fecha o fundo** — não há céu encostado no chão.
- [ ] Julgamento: o mundo parece um **lugar**, ou um cenário de teste?

## 2. O terreno se lê sem explicação

É a parte nova, e a que mais depende do olho: cinco terrenos precisam ser
distinguíveis **sem material próprio**, só por altura, cor da etiqueta e forma.

- [ ] **Água funda** e **poça** se distinguem à primeira vista. *(A poça reluz
      logo acima do solo; a funda afunda o pé.)*
- [ ] A **lama** se distingue das duas: ela afunda **abaixo** do chão seco.
- [ ] O **gelo** parece **sólido** — o pet pisa em cima, não dentro.
- [ ] Na grade desenhada, as etiquetas dizem `ÁGUA FUNDA`, `POÇA (rasa)`,
      `GELO [2]`, `LAMA (escorrega?)` — e o número do gelo **conta para baixo**
      a cada ação.
- [ ] Julgamento: cinco terrenos são **demais para ler** no meio de um turno?

## 3. A arena é o lugar onde você estava

Isto substituiu o sorteio de `Arenas.json` e nunca foi visto.

- [ ] Topar com um inimigo **perto da água** abre uma arena **com água**.
- [ ] Topar com um inimigo **no meio da mata** abre uma arena **com casas
      bloqueadas** — troncos e pedras.
- [ ] Topar num **campo de treino** põe casa de **bônus** no tabuleiro.
- [ ] A batalha **começa** em todos os casos. *(Se a pedra caísse numa casa
      inicial e não fosse removida, a arena não abriria — o sintoma seria
      encostar no inimigo e nada acontecer.)*
- [ ] Julgamento: dá para **reconhecer** o lugar onde você estava, ou a arena
      parece outro lugar qualquer?
- [ ] Julgamento: isso muda **como você anda** pelo mundo — passou a escolher
      onde brigar?

## 4. O gelo e a lama, jogando

Use a barra de botões (canto inferior esquerdo) e `bs.ControlOpponent 1` para
provocar os casos em vez de esperar o sorteio.

- [ ] Congelar uma casa: o feed diz **`cobriu a casa de gelo — dura N ações`**.
- [ ] Quem está no gelo **tenta andar e não sai do lugar**, e o feed diz
      `escorregou`.
- [ ] O gelo **derrete sozinho** e o feed anuncia no que a casa virou.
- [ ] Gelo sobre **água funda** derrete de volta em **água funda** — o rio não
      vira poça.
- [ ] Andar na lama dá em **três coisas diferentes** ao longo de algumas
      tentativas: escorregar, atravessar devagar, atravessar firme.
- [ ] Julgamento: a lama é **tensa** (aposta que vale) ou é **injusta**
      (punição sem leitura)? *É a pergunta mais importante deste roteiro —
      é o único acaso de movimento no jogo.*
- [ ] Julgamento: congelar vale um slot? A recompensa é o que o outro deixa de
      fazer, e isso é difícil de **sentir**.

## 5. O painel e o mapa

- [ ] O painel do mundo mostra pet, nível e os cinco atributos, com rótulos em
      português.
- [ ] A linha do adversário **fica amarela** quando ele se aproxima. *(A cor é
      o aviso que se lê sem ler — é o ponto do recurso.)*
- [ ] O minimapa **gira com o olhar**: o que está à sua frente fica em cima.
- [ ] O mapa completo **não gira** ao girar a câmera.
- [ ] Julgamento: o painel **ajuda**, ou é ruído por cima do jogo?

## 5b. O mapa por descoberta

Nasceu depois deste roteiro, e nada aqui foi visto.

- [ ] Ao começar, o mapa está **escuro** — não mostra o mundo inteiro.
- [ ] Andar **abre manchas**, e elas são manchas mesmo: não um rastro fino de
      migalhas atrás dos seus passos.
- [ ] A fronteira do escuro **corresponde a onde você andou** — sem pedaços
      pintados adiante nem buracos pretos no que você acabou de atravessar.
- [ ] Fechar e reabrir o jogo **mantém** o que você descobriu.
- [ ] Os traços do terreno se distinguem: **mata, clareira, margem, água,
      serra**. E o adversário laranja **salta** por cima deles.
- [ ] A **legenda** do mapa completo bate com as cores do mapa, e diz que o
      escuro é o que você ainda não viu.
- [ ] `bs.Marcar` põe uma marca onde você está; **marcar de novo ali apaga**.
- [ ] `bs.Marcar perigo` e `bs.Marcar destino` saem em cores diferentes, e a
      legenda as explica.
- [ ] As suas marcas aparecem **mesmo em região escura** — você marcou, você
      sabe.
- [ ] Julgamento: descobrir o mapa dá **vontade de explorar**, ou é só uma
      barra de progresso disfarçada?

## 6. Os pets e os tipos

- [ ] Dois pets da **mesma escola** e elementos diferentes se distinguem por
      **cor** E por **inclinação** — não só por cor. *(A inclinação existe para
      quem não distingue matiz.)*
- [ ] Um golpe trancado aparece como **`🔒 Nome (exige Atributo N)`**, e clicar
      nele não trava o turno.
- [ ] Julgamento: **quatro escolas e seis elementos** são demais para ler no
      meio de um turno? *(Eram três e quatro; o espectro acrescentou uma de
      cada lado.)*
- [ ] Julgamento: o requisito visível vira **objetivo**, ou vira frustração?

## 6b. Fantasma, luz e dreno

O mais novo de tudo, e o de balanço mais arriscado. Use `bs.ControlOpponent 1`
para jogar os dois lados em vez de esperar o sorteio cair no caso desejado.

- [ ] O pet de **Fantasma** se distingue dos outros por **cor E inclinação** —
      e o de **Luz** também, sem se confundir com o de Fogo.
- [ ] Um golpe **físico** contra o fantasma **erra**, e o feed diz que errou.
- [ ] Uma **magia** contra o fantasma **acerta**. *(É o que impede o fantasma
      de ser invencível: todo pet tem Magia.)*
- [ ] **Atravessar** aparece na barra só para o fantasma; para os outros o
      botão recusa dizendo o motivo, em vez de não fazer nada.
- [ ] Atravessando, ele **passa por dentro do tronco** — e o tronco **continua
      em pé** depois.
- [ ] **Iluminar** revela o outro, o feed anuncia, e o golpe físico **passa a
      acertar** naquele slot.
- [ ] Iluminar também **desfaz camuflagem** de um pet que não é fantasma.
- [ ] Depois do slot, ele volta a estar escondido. *(A luz é jogada, não estado
      permanente.)*
- [ ] Um golpe com **dreno** mostra a linha `drenou N de vida de …`, e a barra
      de vida de quem bateu **sobe**.
- [ ] Julgamento: enfrentar um fantasma **sem** pet de luz é interessante, ou é
      só frustrante? *É a pergunta mais importante desta seção — a imunidade ao
      físico foi a decisão de maior risco de todo o sistema.*
- [ ] Julgamento: a luz é **jogável por si**, ou só serve quando o outro lado
      trouxe fantasma?
- [ ] Julgamento: drenar dá **peso** ao turno, ou vira o golpe óbvio sempre?

## 7. Números que são chute meu

Nenhum tem base — foram escolhidos para o sistema existir, e só jogar decide.

- [ ] **Um terço** para cada desfecho da lama.
- [ ] **50%** de velocidade perdida ao atravessar devagar.
- [ ] **2 e 4 slots** de congelamento (Friagem e Nevasca).
- [ ] **Dois slots** para a poça virar lama, e mais dois para a lama secar.
- [ ] **25%** de esquiva por reflexo e **±20%** de variação de dano.
- [ ] **Seis segundos** por ponto de treino.
- [ ] **200%** da luz contra o fantasma. *(É o número mais alto da tabela — o
      resto do jogo usa 150.)*
- [ ] **50%, 45% e 40%** de dreno nos golpes do Véu, do Afogado e da Bruma.
- [ ] **Um slot** de duração da revelação por Iluminar.
- [ ] **800 unidades** de região de descoberta, e o **3x3** que uma passada
      revela.
- [ ] **24 marcações** de teto no mapa.

---

## Como me devolver o resultado

O painel **se grava sozinho** em `Saved/BattleDebug.txt` a cada mudança — não
depende de tecla, de foco nem da área de transferência. É o caminho mais curto:
me mande o arquivo.

Para o que é visual, uma **captura de tela** vale mais que descrição: foi assim
que os blocos no editor apareceram, e foi a pergunta que desenterrou o
paliativo apagando o chão.

**Item que falhar não precisa de diagnóstico seu.** Diga o que viu; achar a
causa é meu trabalho, e descrever uma causa suposta já custou rodadas a este
projeto — `Atacar Esquerda` virou "ele foi para a esquerda" numa investigação
real.

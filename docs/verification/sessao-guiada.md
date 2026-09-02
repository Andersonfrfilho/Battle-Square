# Sessão guiada — 02/09/2026

**Um percurso só, em ordem de custo de setup — não por feature.** Isto substitui
a versão de 31/08 (ampliando, não jogando fora nada dela) e junta tudo que
tinha caixa aberta em `o-que-precisa-de-olho.md` mais quatro roteiros que
nasceram depois dela e nunca foram rodados: `esconderijos.md`,
`corrente.md`, `legibilidade-do-combate.md`, `skills-por-pet.md`.

**Não diga a causa, diga o que viu.** Descrever causa suposta já custou
rodadas aqui — `Atacar Esquerda` virou "ele foi para a esquerda" e a
investigação foi atrás do defeito errado. Item sem observação sua eu considero
ok.

## Antes de tudo

- Feche o Editor se estiver aberto. Reabra sempre com **`-log=BattleSquare.log`**
  (CLAUDE.md) — um arquivo de log só, em vez de caçar o mais recente.
- O painel de depuração **se grava sozinho** em `Saved/BattleDebug.txt` a cada
  mudança. Não transcreva a tela à mão — é assim que "Atacar Esquerda" virou
  "ele foi para a esquerda". No fim, mande esse arquivo.
- Teclas: **M** mapa completo · **F7** troca o jogador controlado · **F9**
  copia o painel · **F10** limpa · **C/V/B** camuflar/voar/submergir na hora de
  escolher ação.
- Console: `bs.Marcar [tag]`, `bs.ControlOpponent 1`, `bs.ShowBattleDebug 0`,
  `bs.Especializar`, `bs.SkyDay 8.15`.
- Barra de botões (canto inferior esquerdo): copiar, limpar, trocar de
  jogador controlado, **"Controlar também o jogador 2"**, Camuflar/Voar/Submergir.
  Todo clique aparece no painel (`clique: Atacar`) — se o clique aparece e nada
  muda, o defeito é depois do clique; se nem o clique aparece, o widget não
  está recebendo.

**Cada passo cita de onde veio** (`arquivo.md:linha`), pra você não precisar
confiar só neste arquivo. **Quem marca a caixa de origem é quem jogou** — ao
terminar, vá marcar em `o-que-precisa-de-olho.md`, `esconderijos.md`,
`legibilidade-do-combate.md` e `skills-por-pet.md`; este arquivo não guarda
estado de "feito".

**Ficaram de fora, de propósito:** `construcao-do-mundo.md` e
`fluidos-em-jogo.md` — os dois já têm a coluna de resposta preenchida à mão
(inclusive com "não entendi" numa linha), ou seja, já foram jogados e
respondidos direto no arquivo deles. Repetir aqui seria gastar sua rodada de
novo no que você já fez.

## Tempo e carregamentos

| | Sessão 1 — Fundação | Sessão 2 — O que nenhum olho viu |
|---|---|---|
| Tempo estimado | ~60 min | ~65 min |
| Carregamentos de nível | 2 (`BattleScreen`, depois o mundo aberto) | 0 novo — continua do fim da Sessão 1 (ou reabra o mundo, se for outro dia) |
| Risco | médio — combate, arena e mundo já rodaram em produção antes | **alto — mapa por descoberta, corrente, gelo/lama, esconderijos, skills por pet e fantasma/luz/dreno nunca foram vistos por ninguém** |

Se só der pra rodar uma: **rode a Sessão 2**. É ela que cobre o que o
`ROADMAP.md` chama de maior risco aberto do projeto.

---

# Sessão 1 — Fundação

## Bloco 0 — Legibilidade do combate

**Setup:** Editor → abrir o nível `BattleScreen` → Play. A batalha abre direto,
sem mundo aberto (`legibilidade-do-combate.md`, setup).

- [ ] A barra de vida fica sobre cada pet, não atravessando o corpo. (`legibilidade-do-combate.md:17`)
- [ ] Ela encolhe pela **esquerda**. (`legibilidade-do-combate.md:18`)
- [ ] Gradiente verde → vermelho. (`legibilidade-do-combate.md:19`)
- [ ] Some junto com o pet derrotado. (`legibilidade-do-combate.md:20`)
- [ ] Dá pra ler quem está perdendo sem parar pra comparar. (`legibilidade-do-combate.md:21`)
- [ ] Atacar um oponente que esquivou: o feed diz que foi esquivado. (`legibilidade-do-combate.md:27`)
- [ ] Depois de ler, dá pra concluir **por que** o ataque não fez nada. (`legibilidade-do-combate.md:28`)
- [ ] As frases aparecem no ritmo da reprodução, não tudo de uma vez. (`legibilidade-do-combate.md:29`)
- [ ] A cor distingue quem agiu. (`legibilidade-do-combate.md:30`)
- [ ] `bs.ShowBattleDebug 0`: o painel de depuração some, e **o feed continua**. (`legibilidade-do-combate.md:34`)

**Sentimento:** cinco linhas de feed bastam, ou a frase importante rola pra
fora cedo demais? (`legibilidade-do-combate.md:38`)

## Bloco A — Mundo, os primeiros segundos

**Setup:** feche o Editor, reabra o projeto normalmente e aperte Play no mundo
aberto. **Faça isto ANTES de andar** — o item do mapa escuro só vale na
primeira vez.

- [ ] Antes do Play, **sem blocos coloridos** no mapa. (`o-que-precisa-de-olho.md:36`)
- [ ] Ao dar Play, há **sol e céu**, não a luz azul chapada padrão. (`o-que-precisa-de-olho.md:38`)
- [ ] O chão existe e segura: ande até o inimigo mais distante sem cair. (`o-que-precisa-de-olho.md:40`)
- [ ] A mata **não flutua** nem afunda no piso. (`o-que-precisa-de-olho.md:43`)
- [ ] Nada **pisca** onde duas superfícies se encontram. (`o-que-precisa-de-olho.md:44`)
- [ ] Seu pet e os inimigos são visíveis e se distinguem do chão. (`o-que-precisa-de-olho.md:45`)
- [ ] A serra fecha o horizonte — sem céu encostado no chão. (`o-que-precisa-de-olho.md:46`)
- [ ] **Antes de dar um passo:** olhe o minimapa — ele começa **escuro**. (`o-que-precisa-de-olho.md:112`)

**Sentimento:** o mundo parece um lugar, ou um cenário de teste? (`o-que-precisa-de-olho.md:47`)

## Bloco B — Andar: colisão, painel, mapa em movimento, treino

- [ ] Esbarra em tronco/pedra na mata, não atravessa. (herdado da sessão anterior — sem caixa correspondente em roteiro ativo hoje)
- [ ] A beira d'água **barra** a passagem. (herdado — mesma observação acima)
- [ ] O painel (canto superior direito) mostra pet, nível e os cinco atributos, com rótulos em português. (`o-que-precisa-de-olho.md:100`)
- [ ] A linha do adversário fica **amarela** quando ele se aproxima. (`o-que-precisa-de-olho.md:102`)
- [ ] Andar **abre manchas** no minimapa — manchas mesmo, não um fiapo atrás dos seus passos. (`o-que-precisa-de-olho.md:113`)
- [ ] A fronteira do escuro corresponde a onde você andou de fato — sem pedaço pintado adiante nem buraco preto no que você acabou de atravessar. (`o-que-precisa-de-olho.md:115`)
- [ ] Os traços do terreno se distinguem: mata, clareira, margem, água, serra; o ponto laranja do adversário salta por cima deles. (`o-que-precisa-de-olho.md:118`)
- [ ] Entrar num campo de treino colorido mostra progresso, e o atributo sobe ao fechar 100%. (`o-que-precisa-de-olho.md:100` região "5. O painel e o mapa"; regra de bolso é 6s por ponto — `o-que-precisa-de-olho.md:179`)

## Bloco C — Mapa completo e marcações

**Setup:** apertar **M**.

- [ ] O minimapa **gira** com o olhar. (`o-que-precisa-de-olho.md:104`)
- [ ] O mapa completo **não gira** ao girar a câmera. (`o-que-precisa-de-olho.md:105`)
- [ ] A legenda do mapa completo bate com as cores, e diz que escuro = ainda não visto. (`o-que-precisa-de-olho.md:120`)
- [ ] `bs.Marcar`: aparece uma marca onde você está. (`o-que-precisa-de-olho.md:122`)
- [ ] `bs.Marcar` de novo no mesmo lugar: **apaga**. (`o-que-precisa-de-olho.md:122`)
- [ ] `bs.Marcar perigo` / `bs.Marcar destino`: cores diferentes, explicadas na legenda. (`o-que-precisa-de-olho.md:123`)
- [ ] Suas marcas aparecem mesmo em área ainda escura. (`o-que-precisa-de-olho.md:125`)

**Sentimento:** descobrir o mapa dá vontade de explorar, ou é uma barra de
progresso disfarçada? (`o-que-precisa-de-olho.md:127`)

## Bloco D — O encontro e a arena

Repita perto da água, dentro da mata, e num campo de treino.

- [ ] Perto da água: encostar no inimigo abre arena **com casas de água**. (`o-que-precisa-de-olho.md:67`)
- [ ] Dentro da mata: arena **com casas bloqueadas** (tronco/pedra). (`o-que-precisa-de-olho.md:68`)
- [ ] Num campo de treino: **casa de bônus** no tabuleiro. (`o-que-precisa-de-olho.md:70`)
- [ ] A batalha **sempre começa**, nas três vezes. (`o-que-precisa-de-olho.md:71`)
- [ ] Água funda e poça se distinguem à primeira vista (a poça reluz perto da superfície; a funda afunda o pé). (`o-que-precisa-de-olho.md:54`)
- [ ] As etiquetas da grade dizem `ÁGUA FUNDA` e `POÇA (rasa)`. (`o-que-precisa-de-olho.md:58` — gelo e lama ficam pra Sessão 2)

**Sentimento:** dá pra reconhecer o lugar onde você estava, ou a arena parece
outro lugar qualquer? (`o-que-precisa-de-olho.md:74`)

## Bloco E — Combate geral, tipos, feed

- [ ] Escolher três ações e confirmar: o feed narra em português. (herdado da sessão anterior)
- [ ] O pet **desliza** até a casa nova, não teleporta. (herdado)
- [ ] O indicador de fase conta (`reproduzindo fase 3 de 11`). (herdado)
- [ ] Golpe trancado aparece como `🔒 Nome (exige Atributo N)`, e clicar nele **não trava** o turno. (`o-que-precisa-de-olho.md:135`)
- [ ] Dois pets da mesma escola, elementos diferentes: se distinguem por **cor** e por **inclinação**, não só por cor. (`o-que-precisa-de-olho.md:132`)
- [ ] `bs.ControlOpponent 1` (ou F7): você joga os dois lados, e a barra mostra as ações do jogador 2. (herdado)
- [ ] Vencer devolve ao mundo, com XP, e sem cair. (herdado)

**Sentimento:** quatro escolas e seis elementos são demais pra ler no meio de
um turno? (`o-que-precisa-de-olho.md:137`)

## Bloco F — Fronteira: fechar e reabrir

- [ ] Feche o jogo e abra de novo: o que você descobriu no mapa **continua lá**. (`o-que-precisa-de-olho.md:117`)

Esta reabertura já serve de ponto de partida pra Sessão 2 — não é preciso
reabrir duas vezes.

## Bloco Ω1 — Outros julgamentos da Sessão 1

- [ ] Alguma frase do feed soa como mensagem de erro em vez de narração? (`legibilidade-do-combate.md:39`)
- [ ] O painel do mundo ajuda, ou é ruído por cima do jogo? (`o-que-precisa-de-olho.md:106`)
- [ ] Reconhecer a arena mudou como você anda pelo mundo — passou a escolher onde brigar? (`o-que-precisa-de-olho.md:76`)
- [ ] O requisito visível do golpe trancado vira objetivo, ou vira frustração? (`o-que-precisa-de-olho.md:140`)
- [ ] Calibre de olho: os 6 segundos por ponto de treino (`o-que-precisa-de-olho.md:179`) e as 800 unidades / 3×3 de descoberta por passo (`o-que-precisa-de-olho.md:184`) pareceram certos, rápidos ou lentos demais?
- [ ] Calibre de olho: o teto de 24 marcações no mapa (`o-que-precisa-de-olho.md:186`) — só relevante se você marcou bastante.

---

# Sessão 2 — O que nenhum olho viu (maior risco)

**Setup:** continue do mundo já aberto no fim da Sessão 1. Se for outro dia,
reabra o projeto e aperte Play de novo — só o item do "mapa escuro" (Bloco A)
não se repete, o resto segue igual.

## Bloco G — Corrente: no mundo, na arena, na balsa

**Setup:** ache um rio (a cor da água e a linha `terreno:` do painel ajudam).

- [ ] No rio, descendo: o painel mostra `pisando: vau em agua doce (passo X%)` e `corrente: N% rumo (...) — voce vai A FAVOR`, em **verde**. (`corrente.md:20-27`)
- [ ] Subindo, sem sair da água: a mesma linha em **laranja**, `CONTRA`, e o `passo` da linha de cima **cai**. É a queda do `passo` que prova que a corrente é aplicada, não só lida. (`corrente.md:29-36`)
- [ ] Atravessando margem a margem, perpendicular: `de traves`, cinza, `passo` igual ao de água parada. (`corrente.md:38-42`)
- [ ] Saindo pra terra: a linha `corrente:` **some inteira** — não fica dizendo "nenhuma". (`corrente.md:44-48`)
- [ ] Numa batalha montada sobre um rio (encoste num inimigo perto da água): as casas de corrente mostram **seta ciano** com a força na etiqueta, ex. `(1,1) ÁGUA FUNDA →40`. (`corrente.md:54-63`)
- [ ] Setas maiores onde a corrente é mais forte; água parada sem seta nenhuma. (`corrente.md:65-67`)
- [ ] Mandar o pet andar **contra** a seta numa casa de corrente forte: ele escorrega ou sai menos do que pedido, e o painel narra. **Se ele for empurrado numa direção que a seta não aponta, é defeito grave de direção — reporte na hora.** (`corrente.md:69-77`)
- [ ] Ache uma travessia de **balsa**: ida e volta duram tempos **diferentes**, e ela **nunca para** de avançar, mesmo contra a corrente mais forte. (`corrente.md:83-91`)

*(Este roteiro é só fato, por desenho — sem pergunta de sentimento própria.)*

## Bloco H — Gelo e lama, jogando

**Setup:** `bs.ControlOpponent 1` (ou "Controlar também o jogador 2" na
barra) — provoque os casos em vez de esperar o sorteio. (`o-que-precisa-de-olho.md:81`)

- [ ] A lama se distingue das duas águas: afunda **abaixo** do chão seco. (`o-que-precisa-de-olho.md:56`)
- [ ] O gelo parece **sólido** — o pet pisa em cima, não dentro. (`o-que-precisa-de-olho.md:57`)
- [ ] A etiqueta `GELO [2]` conta **para baixo** a cada ação. (`o-que-precisa-de-olho.md:58-60`)
- [ ] Congelar uma casa: o feed diz `cobriu a casa de gelo — dura N ações`. (`o-que-precisa-de-olho.md:84`)
- [ ] Quem está no gelo tenta andar e **não sai do lugar**, feed diz `escorregou`. (`o-que-precisa-de-olho.md:85`)
- [ ] O gelo **derrete sozinho**, e o feed anuncia no que virou. (`o-que-precisa-de-olho.md:87`)
- [ ] Gelo sobre **água funda** derrete de volta em água funda, não em poça. (`o-que-precisa-de-olho.md:88`)
- [ ] Andar na lama, repetidas vezes: **três desfechos diferentes** (escorregar, atravessar devagar, atravessar firme). (`o-que-precisa-de-olho.md:90`)

**Sentimento:** a lama é tensa (aposta que vale) ou injusta (punição sem
leitura)? É a pergunta mais importante deste roteiro — é o único acaso de
movimento no jogo. (`o-que-precisa-de-olho.md:92`)

## Bloco I — Esconderijos: camuflar, voar, submergir, trombada

**Setup:** teclas **C** (camuflar) / **V** (voar) / **B** (submergir) na hora
de escolher ação. Pra trombada, use "Controlar também o jogador 2" e mire os
dois pets na mesma casa.

- [ ] Camuflar: nem físico nem magia acertam; no slot seguinte, o ataque erra "se revelando". (`esconderijos.md:31`)
- [ ] Voar: físico erra, magia acerta mais forte, casas de dano não disparam. (`esconderijos.md:33`)
- [ ] Submergir: nada acerta, casas de dano não disparam, e no slot seguinte não pode mover nem atacar. (`esconderijos.md:34`)
- [ ] Oponente voando: seu pet olha pra **cima**. (`esconderijos.md:40`)
- [ ] Oponente submerso: olha pra **baixo**. (`esconderijos.md:41`)
- [ ] Oponente camuflado: o olhar fica na última casa conhecida — **nunca** aponta pro lugar de verdade. (`esconderijos.md:42`)
- [ ] Cada postura tem frase própria no feed, não um genérico "assumiu postura". (`esconderijos.md:48`)
- [ ] Os dois indo pra mesma casa: nenhum entra, os dois tomam dano de trombada. (`esconderijos.md:63`)
- [ ] Andar pra cima de quem está parado: mesma coisa. (`esconderijos.md:64`)
- [ ] Trocar de casas (os dois se movendo um pro lugar do outro) continua funcionando — ninguém fica coabitando. (`esconderijos.md:65`)
- [ ] Quem esquivou não se fere na trombada; quem defendeu se fere menos. (`esconderijos.md:66`)
- [ ] O feed diz que "trombaram", não um "movimento bloqueado" genérico. (`esconderijos.md:67`)

**Sentimento:** depois de jogar as três posturas, dá pra dizer quando usar
cada uma? (`esconderijos.md:36`)

## Bloco J — Skills por pet

- [ ] O painel diz, ao começar: `<pet> (tipo X) tem N skill(s) própria(s)`. (`skills-por-pet.md:34`)
- [ ] A barra mostra `SUAS SKILLS — <nomes>`, botão só pro que o pet TEM; um pet sem skill própria diz isso explicitamente, não fica vazio. (`skills-por-pet.md:35`)
- [ ] A barra do jogador 2 só mostra as skills **daquele** pet. (`skills-por-pet.md:38`)
- [ ] Um pet de tipo diferente oferece skill diferente. (`skills-por-pet.md:39`)
- [ ] O bot **nunca** voa com um pet que não voa — zero "alçou voo" pra tipo sem a skill. (`skills-por-pet.md:48`)
- [ ] Em casa **seca**, submergir falha, e o feed diz exatamente `tentou mergulhar, mas não há água nesta casa`. (`skills-por-pet.md:63`)
- [ ] A grade **mostra** onde é água (azul, `ÁGUA`) — a regra não pode ser descoberta só perdendo. (`skills-por-pet.md:66`)
- [ ] Numa casa de água: submergir funciona, e a imunidade vale. (`skills-por-pet.md:68`)
- [ ] Dá pra planejar: andar até a água num turno, submergir no seguinte. (`skills-por-pet.md:69`)
- [ ] As outras posturas (defender/esquivar/camuflar/voar) **não** dependem de terreno. (`skills-por-pet.md:70`)
- [ ] Com o catálogo apagado: painel avisa e todo pet cai pros 6 comandos universais, sem travar. **Pule se não houver como forçar isso sem editar `Config/`.** (`skills-por-pet.md:43`)

**Sentimento:** precisar achar água antes de submergir torna a arena parte da
decisão, ou é só mais um passo? (`skills-por-pet.md:72`)

## Bloco K — Fantasma, luz e dreno

⚠️ **Pode estar fora de alcance.** A versão anterior desta sessão registrou
que o espelho de pets (25/08) não tinha fantasma, luz nem dreno — depende do
backend rodando e do worker regerando o espelho. Explore alguns encontros; se
em nenhum aparecer um pet Fantasma ou Luz, marque este bloco como
**BLOQUEADO** (não como reprovado) no seu retorno — é dependência de outra
parte do pipeline, não um "não" do jogo.

**Setup:** `bs.ControlOpponent 1` — jogue os dois lados, não espere o sorteio
cair no caso desejado. (`o-que-precisa-de-olho.md:144`)

- [ ] O pet de Fantasma se distingue dos outros por **cor e inclinação**; o de Luz também, sem se confundir com o de Fogo. (`o-que-precisa-de-olho.md:147`)
- [ ] Golpe **físico** contra o fantasma **erra**, feed diz que errou. (`o-que-precisa-de-olho.md:149`)
- [ ] **Magia** contra o fantasma **acerta**. (`o-que-precisa-de-olho.md:150`)
- [ ] "Atravessar" aparece na barra só pro fantasma; pros outros, o botão recusa dizendo o motivo. (`o-que-precisa-de-olho.md:152`)
- [ ] Atravessando, ele passa por dentro do tronco, e o tronco continua em pé depois. (`o-que-precisa-de-olho.md:154`)
- [ ] "Iluminar" revela o outro, feed anuncia, e o físico passa a acertar naquele slot. (`o-que-precisa-de-olho.md:156`)
- [ ] Iluminar também desfaz camuflagem de um pet que não é fantasma. (`o-que-precisa-de-olho.md:158`)
- [ ] Depois do slot, ele volta a estar escondido — a luz é jogada, não é estado permanente. (`o-que-precisa-de-olho.md:159`)
- [ ] Um golpe com dreno mostra `drenou N de vida de …`, e a barra de quem bateu **sobe**. (`o-que-precisa-de-olho.md:161`)

**Sentimento:** enfrentar um fantasma **sem** pet de luz é interessante, ou só
frustrante? A pergunta mais importante desta seção — a imunidade ao físico foi
a decisão de maior risco de todo o sistema. (`o-que-precisa-de-olho.md:163`)

## Bloco Ω2 — Outros julgamentos da Sessão 2 + números que são chute

- [ ] Congelar vale um slot, dado que a recompensa (o que o outro deixa de fazer) é invisível? (`o-que-precisa-de-olho.md:95`)
- [ ] O erro "se revelando" soa igual a um erro de mira, ou dá pra distinguir? (`esconderijos.md:49`)
- [ ] Camuflar/submergir todo slot é forte demais mesmo custando a ação seguinte? Voar compensa, dado que a magia dói 50% a mais? (`esconderijos.md:55,57`)
- [ ] A trombada dói o bastante pra desestimular, sem virar o jeito mais barato de causar dano? (`esconderijos.md:68`)
- [ ] Enfrentar um pet Água parece um time diferente de enfrentar um Planta? Uma skill por tipo é pouco ou suficiente? (`skills-por-pet.md:53,55`)
- [ ] A luz é jogável por si, ou só serve quando o outro trouxe fantasma? Drenar dá peso ao turno, ou vira o golpe óbvio sempre? *(só se o Bloco K não ficou bloqueado)* (`o-que-precisa-de-olho.md:166,168`)
- [ ] Calibre de olho: 1/3 pra cada desfecho da lama, 50% de lentidão ao atravessar devagar, 2 e 4 slots de congelamento (Friagem/Nevasca), 2 slots poça→lama e mais 2 lama→seco. (`o-que-precisa-de-olho.md:174-177`)
- [ ] Calibre de olho: 25% de esquiva por reflexo e ±20% de variação de dano. (`o-que-precisa-de-olho.md:178`)
- [ ] Calibre de olho: 200% da luz contra o fantasma (o número mais alto da tabela) e 50/45/40% de dreno no Véu/Afogado/Bruma. *(só se o Bloco K não ficou bloqueado)* (`o-que-precisa-de-olho.md:180,182`)
- [ ] Calibre de olho: 1 slot de duração da revelação por Iluminar. *(só se o Bloco K não ficou bloqueado)* (`o-que-precisa-de-olho.md:183`)

---

## Como devolver o resultado

Mande `Saved/BattleDebug.txt` — ele se grava sozinho, não depende de tecla,
foco ou área de transferência. Pro que é visual, uma **captura de tela** vale
mais que descrição.

**Item que falhar não precisa de diagnóstico seu.** Diga o que viu; achar a
causa é trabalho de quem for consertar — descrever uma causa suposta já
custou rodadas neste projeto.

**Depois de jogar:** vá marcar as caixas nos roteiros de origem
(`o-que-precisa-de-olho.md`, `esconderijos.md`, `legibilidade-do-combate.md`,
`skills-por-pet.md`) — este arquivo é o percurso, não a fonte da verdade.
Todo item que falhar **vira teste antes de virar conserto** (CLAUDE.md): o log
encontra o defeito, o teste impede que ele volte.

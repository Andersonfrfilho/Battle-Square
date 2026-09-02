# Fundura no traçado — tarefas

Ordem obrigatória: **mede, depois muda.** Em 26–27/08 consertei por hipótese três
vezes seguidas neste projeto; cada correção estava certa isoladamente e nenhuma
era comprovadamente *a* causa. F1 existe para que F2 não seja a quarta.

Todas as tarefas nascem `[⛔]` no objetivo. Abrir é do usuário.

---

## F1 — MEDIR a resolução e o degrau, antes de tocar no gerador
> 🤖 Modelo: `sonnet` — é medição, não desenho

**O cano que já existe:** `Tools/bake_island.sh` já assa e já valida as seções;
`Saved/IslandMap.json` já sai com `curso`, `corredeira` e `fundo`;
`IslandBakedPlan::RiverSampleCount()` já vale **41** e `HeightGridSide()` **180**.
Nada a construir — a tarefa é ler o que o assado já diz.

Três números que ainda não existem em lugar nenhum, e sem eles F2 e F4 são
adivinhação:

1. **O degrau da rocha em cada uma das 13 quedas.** `BedrockHeightAt` no alto
   menos embaixo, nas mesmas duas posições que `PlungePoolDepthUnits` usa. A
   suspeita a confirmar ou derrubar: `fundo = queda × 0,55` com fundo de 16 a 64
   implica quedas de **29 a 116** unidades — se for isso, a rocha é lisa onde a
   cachoeira cai, e P8 é relevo.
2. **A distribuição do passo entre pontos consecutivos de curso**, por curso, não
   só global. O global já está medido (mínimo 65, mediana 67, **máximo 1 198**), e
   é o máximo que decide: trecho raso mais curto que o passo local é invisível.
3. **Quantos dos 137 cursos têm largura quase constante** e ainda assim
   atravessariam diferente se a fundura variasse. É esse conjunto que prova que a
   feature acrescenta informação; se ele for vazio, a fundura por ponto não muda
   nada e a feature está errada.

*Dependências:* nenhuma.

*Verificação:* `./Tools/bake_island.sh` e depois a medição sobre
`Saved/IslandMap.json`, no mesmo padrão dos números que já estão na spec.

*Aceite:* **os três números aparecem**, escritos em
`docs/verification/fundura-no-tracado.md`. Não há alvo a bater aqui — medição com
alvo é opinião com número.

*Contrapeso obrigatório:* nada é editado em `Source/`. Se F1 mexer em código, ela
deixou de ser a medição de referência de F2.

---

## F2 — A fundura por ponto entra no assado, e quem a decide é o GERADOR
> 🤖 Modelo: `opus` 🧠 — mexe no gerador e cria a fonte de verdade

**O cano que já existe, e é por isso que esta task é curta:** `FBakedRiver` já
carrega **cinco** vetores paralelos por ponto (`HalfWidthUnits`, `bIsRapids`,
`FluidByPoint`, `FlowDirectionByPoint`, `FlowStrengthByPoint`), o laço de
`BakeInto` já os preenche com uma linha cada, e `IslandMapDumpTest.cpp` já tem o
bloco `"corredeira"` como precedente comentado de marca por ponto no JSON. O
sexto vetor é a sexta linha do mesmo laço.

A fundura sai de `FreshWater`, por **progresso** — `FunduraAtProgress(Curso,
Onde)`, na forma dos `*AtProgress` que já existem. Nunca por raio: parametrizar
traço por raio é o defeito de espinha de peixe que §3 das regras de mapa proíbe.

⚠️ **Ausência não se marca com valor fora de faixa.** Fundura `-1` para "não há
água aqui" vaza — foi assim que uma vila nasceu fora da ilha neste projeto (§7).
Quem não tem água responde por função de pertinência, como `bHasLake` já faz.

*Dependências:* F1 (os três números), e a decisão **2** do usuário (número
contínuo ou faixa nomeada) — sem ela não há tipo para o vetor.

*Verificação:* `./Tools/bake_island.sh && ./Tools/run_tests.sh BattleSquare.IslandBakedPlan`

*Aceite:* **o mesmo curso responde funduras diferentes em progressos
diferentes**, e o vetor tem exatamente `RiverSampleCount()` entradas. Um vetor
com o valor repetido 41 vezes é a fundura por rio com nome novo — não conta.

*Contrapeso obrigatório:* `IslandBakedPlan.RiversMatchThePlan` afirma o
comprimento dos vetores contra `RiverSampleCount()`; o vetor novo entra nessa
afirmação **no mesmo commit**. Vetor paralelo que ninguém mede o comprimento é o
vetor que um dia sai desalinhado dos outros cinco, e o sintoma aparece longe.

---

## F3 — A estimativa privada do traçado MORRE
> 🤖 Modelo: `opus` 🧠 — é a task da invariante 4

`TrailLayout.cpp` tem hoje `constexpr float FunduraSobreLargura = 0.065f` e
`FunduraEm(ponto)` em namespace anônimo, e é ela que decide as **56** travessias
da carta. Ela passa a **ler** o assado.

**Esta task não é opcional nem posterior:** enquanto as duas existirem, existem
duas verdades sobre a mesma água — e a nova é a que ninguém está chamando, que é
exatamente a forma de L-033 (a regra certa, testada, e viva em um só dos dois
caminhos). A constante morre no mesmo commit em que a leitura nasce.

Some junto o comentário do cabeçalho de `WaterFooting.h` que ainda afirma *"A
LARGURA decide"* — o `.cpp` já lê as travessias e guarda a medição de por que. E
some o *"Poço de cachoeira é FURO"* de `FreshWater.h`, que é a leitura errada da
literatura escrita dentro do código.

*Dependências:* F2.

*Verificação:* `grep -rn "FunduraSobreLargura" Source/` **não acha nada**, e
`./Tools/run_tests.sh BattleSquare`.

*Aceite:* o grep fora de `/Tests/` acha **quem LÊ** a fundura do assado, e não
acha ninguém que a estime. Uma fundura que só o teste consulta não é uma fonte de
verdade — é uma função morta com teste verde.

*Contrapeso obrigatório:* `ChartConformance.CrossingsMatch` afirma **30 vaus, 25
balsas, 0 pontes, 56 ao todo** e vai ficar vermelho se a fundura reclassificar
travessia. Isso é o teste fazendo o trabalho dele. **Ou os números continuam, ou
eles mudam com a decisão 3 do usuário registrada junto** — nunca se afrouxa a
afirmação para o teste passar. Afrouxar transforma o teste que protege a feature
no teste que não protege nada.

---

## F4 — O poço deixa de ser PRATO: a rocha ganha degrau na queda
> 🤖 Modelo: `opus` 🧠 — mexe na camada mais baixa

**Por que a consulta não resolve, com a conta:** a meia-largura do poço é
`max(MeiaCalhaDoRio() × 1,15 ; fundo × 1,6)`. O primeiro termo vale **886** e o
segundo, para fundo de 16 a 64, vale 25,6 a 102,4 — **o piso ganha sempre**, e é
por isso que os 13 poços medem exatamente 886. Escrever `FunduraDoPocoEm(ponto)`
hoje devolveria o mesmo número em todo o raio.

Portanto a mudança é no **relevo**: a queda precisa de degrau na rocha para o poço
ter fundo. É a camada de baixo (§14: rocha → água → região → relevo → trilhas →
solo), e a de baixo **nunca pergunta à de cima** — o leito é anterior à vila.

⚠️ **A armadilha, e ela já cobrou:** o "dez vezes" da literatura é razão de
velocidade de **erosão**, vertical sobre lateral, não a forma do buraco. O poço é
mais **largo** que fundo, e isso está certo. Um teste afirmando "mais fundo que
largo" já reprovou código certo neste projeto.

*Dependências:* F1 (o degrau medido), F2 (a fundura por ponto), e a decisão **4**
do usuário (se a queda ganha degrau, e quanto).

*Verificação:* `./Tools/bake_island.sh && ./Tools/run_tests.sh BattleSquare`

*Aceite:* **a fundura do poço no centro difere da fundura na borda**, nos 13
poços. O teste afirma **variação dentro do mesmo poço** — nunca proporção entre
largura e fundura, que é a fonte lida errado.

*Contrapeso obrigatório:* mexer na rocha muda tudo que fica em cima dela. A
bateria inteira roda, não só os testes de água: `ChartConformance` afirma 5
templos, 4 ruínas, 7 cemitérios, 2 poços que dão água, 137 cursos, 23 trilhas,
158 galerias. **Ciclo e cascata têm mais de uma aresta** — consertar uma e testar
não basta, e foi assim que eu dei um ciclo de planos por resolvido com metade
dele de pé.

---

## F5 — Passou da CINTURA, nada
> 🤖 Modelo: `opus` 🧠 — muda a regra de quem anda

A pergunta de P7, e ela só existe depois de F2: `WaterFooting` decide pela
fundura **no ponto**, e acima da cintura o passo deixa de ser passo.

Hoje a tabela é `PassoEmTerra 1,0`, `PassoNoVau 0,55`, `PassoNoFundo 0,25`, com
`EmpurraoDaCorrente 0,8` e `PisoContraACorrente 0,2` por cima. A cintura entra
nessa tabela — **uma tabela só**, nunca uma segunda ao lado.

⚠️ **§6 antes de culpar a regra:** se a cintura não aparecer em lugar nenhum do
mapa, o primeiro suspeito é a resolução, não o limiar. F1 já mediu o passo
máximo (**1 198**), e um trecho raso mais curto que ele é invisível para uma
amostra por ponto.

*Dependências:* F2, e a decisão **1** do usuário (onde fica a cintura — as três
âncoras medidas discordam: 100 do a-pé, 88 da cápsula, 94 do maior vau de hoje).

*Verificação:* `./Tools/run_tests.sh BattleSquare.WaterFooting`

*Aceite:* **o MESMO rio, de largura praticamente igual nos dois pontos, atravessa
a pé num trecho e obriga a nadar noutro.** Um rio raso e um rio fundo respondendo
diferente não prova nada: a largura deles também difere, e a fórmula velha
acertaria igual.

*Contrapeso obrigatório:* `WaterFootingTest` tem seis testes escritos contra a
regra ANTIGA — `TheFordAndTheDeepDiffer`, `DryLandStaysDry`,
`TheFordComesFromTheCrossings`, `UpstreamCostsMore`, `SidewaysIsNeither`,
`NeverStopsTheWalker`. Cada um **vira** o teste da regra nova. Deixar os dois
verdes faz a bateria provar duas regras contraditórias, e a bateria passa a não
provar nada. E `NeverStopsTheWalker` continua valendo: nadar é passo lento, nunca
passo zero — jogador travado na água não é regra, é defeito.

---

## F6 — A fundura na TELA
> 🤖 Modelo: `sonnet`

**O cano que já existe:** `BattleSquareGameMode.cpp`, chave **748**, já imprime
`pisando: %s em %s (passo %.0f%%)` com `WaterFooting::DebugName` e o nome do
fluido. Falta o número da fundura ao lado.

Sem isso, "a água passou da cintura" é dedução a partir da velocidade — e foi
exatamente esse ciclo (usuário joga → descreve → eu leio o log → deduzo) que
custou horas neste projeto. A regra da casa é: o que foi implementado aparece na
tela.

Texto do jogador é `LOCTEXT` com argumentos **nomeados**. O painel de depuração
não é texto do jogador e continua como está.

*Dependências:* F5.

*Verificação:* `./Tools/audit_localizable_text.sh` e roteiro em
`docs/verification/fundura-no-tracado.md`.

*Aceite:* andando do seco para o fundo, **o número na tela sobe e a linha diz
quando a cintura passou**. Fundura que ninguém vê é fundura que ninguém confere —
e é o instrumento de medição mais eficiente que este projeto tem.

*Contrapeso obrigatório:* a chave 748 é **fixa**, para a linha se atualizar no
lugar. Virar `-1` empilha uma linha por quadro e come o painel de 12 linhas.

---

## O que estas tarefas NÃO fazem

- **Não decidem onde fica a cintura.** F5 espera o número; as três âncoras
  medidas estão na spec, e elas discordam. Número de produto é do dono.
- **Não mudam a carta por conta própria.** Se a fundura reclassificar travessia,
  `ChartConformance` fica vermelho e a tarefa **para** — não afrouxa a afirmação.
  Números novos na carta são gabarito novo, e gabarito é decisão de produto
  (é a mesma parada da P10).
- **Não põem cavernas atrás das cachoeiras.** Isso é P9, e é falta de
  proximidade, não de consulta: a caverna mais próxima está a 2 710 unidades de
  uma queda de raio 886, três vezes longe demais.
- **Não escondem nada da carta.** P10 continua fechada.
- **Não inventam segunda tabela de fundura.** Uma fonte, no gerador. Se aparecer
  uma segunda em qualquer commit desta feature, a feature falhou pelo motivo que
  ela existe para consertar.
- **Não autoram asset.** Fundura aparece por texto e por número.

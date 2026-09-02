# Mundo por biomas — tarefas

## O que JÁ existe, medido

A spec descreve um mundo que, na maior parte, **já está construído**. Antes de
qualquer tarefa abaixo, isto foi conferido lendo código — não a spec de novo:

- **A ilha e a costa** — `Source/BattleSquare/Public/Environment/IslandGeography.h`.
  `LandRadiusUnits()` é a fonte ÚNICA do raio; `Config/DefaultGame.ini` já tem
  `WorldIslandRadiusUnits=140000.0` (os 1,4 km da decisão final da spec) e
  `WorldCoastShape=ComEncaixe` — a costa já sabe encaixar com a de outra ilha.
  `CoastDockCount()`/`CoastDockBearingDegrees()` já dão as docas de embarque.

- **Um bioma por ilha, em 5 setores** — `IslandGeography::SectorCount=5`,
  `SectorAt()`, `BiomeOfSector()` e `IslandBiome()`. O comentário do próprio
  código já explica o porquê: "cada instância do servidor é uma ilha, e a
  ilha inteira é do mesmo bioma — é o que dá 6 km² a cada um, em vez dos
  0,39 km² que a fatia de pizza deixava." A decisão "uma ilha, um bioma" da
  spec **já está implementada**, não é mais decisão.

- **Relevo com altura de verdade** — `GroundHeightAt`, `NaturalGroundHeightAt`,
  `BedrockHeightAt`, `GroundSlopeAt`, mais o vulcão (`VolcanoCenterUnits`,
  `VolcanoRingUnits`, `VolcanoHeatRadiusUnits`, `VolcanoScorchedRadiusUnits`) e
  o planalto (`PlateauHeightUnits`, `BluffInnerRadiusUnits`/
  `BluffOuterRadiusUnits`, `IsOnBluffRamp` — garante que todo barranco tem
  rampa, nunca é parede pura).

- **Custo de trajeto, já pronto para trilha E para cansaço** —
  `TravelCostBetween`, `UphillCostWeight()`, `DownhillCostWeight()`. O
  comentário do código diz por quê existem juntos: "quem TRAÇA a trilha e
  quem COBRA o cansaço precisam concordar." Isso serve tanto a esta feature
  quanto a `montaria-e-trilhas`.

- **A região de descoberta já é fração do raio** —
  `Source/BattleSquare/Private/World/WorldDiscovery.cpp`:
  `RegionSizeUnits() = LandRadiusUnits() / 25`, exatamente a fórmula que a
  spec pediu na seção "Uma ilha, uma aventura, um mapa."

- **Vilas, cidade e fronteira — layout, distâncias e serviços** —
  `Source/BattleSquare/Public/World/RegionLayout.h` já tem `ESettlementKind`
  com os cinco papéis da spec (`VilaInicial`, `VilaDaAcademia`,
  `VilaDoMercado`, `CidadeGrande`, `PostoDeFronteira`, este último já
  comentado como "a porta que só abre para quem venceu o ranking").
  `RegionLayout.cpp` já posiciona tudo como **fração do raio**
  (`FracaoDasVilas=0.29`, `FracaoDaCidade=0.50`, `FracaoDaOrlaDoPosto=-0.45`),
  e o comentário confirma: "com o raio de 1,4 km elas dão cerca de 400 m,
  700 m e 950 m — o que a spec pediu." `VillageLayout::PlanFor(Kind)` já é o
  MOLDE puro variando por tipo de assentamento que a spec pedia.
  `SettlementEconomy::Offers/PricePercent/PayoutPercent` já é a tabela de
  serviço por tipo (cura de graça só em Casa, `PremioDeRanking` só onde há
  Arena). `RegionLayoutTest.cpp` já testa prédio obrigatório por tipo e a
  autossuficiência da Cidade Grande.

- **Trilha já evita o vulcão** — `TrailLayout.cpp` já exclui todo ponto
  dentro de `VolcanoScorchedRadiusUnits()` do centro do vulcão, com teste em
  `TrailLayoutTest.cpp` afirmando que nenhuma trilha corta a rocha queimada.

- **Desastres já existem, determinísticos** —
  `Source/BattleSquare/Public/Environment/WorldEvents.h`: terremoto, furacão
  e tsunami como funções puras de (semente, lugar, hora), sem estado
  salvo — o pré-requisito de "cidade abandonada por desastre" já está de pé.

- **Flora por bioma já existe** — `BiomeFlora::FitsBiome`/`AllowedSpecies`
  evita "geleira com carvalho, floresta com agulha de pedra de vulcão."

- **Ruína já é cenário, mas GENÉRICO** — `EGroundUse::Ruina` (mais
  `Cemiterio`/`CemiterioEsquecido`) já existe em `LandUseLayout.cpp` e
  `WorldBudget.cpp`, com orçamento por bioma (ex.: deserto tem 7 ruínas,
  pântano 6, geleira 2 — comentário: "deserto guarda ruína: a areia cobre e
  devolve, e nada apodrece"). **Mas é decoração de ruína antiga, sem ligação
  com desastre, com assentamento vivo, nem com o elemento Fantasma.**

- **Os oito elementos já existem no catálogo** — `Config/PetTypes.json` já
  declara `Fogo, Agua, Planta, Terra, Fantasma, Luz, Ar, Raio`. Não é preciso
  estender o eixo de elemento para cobrir a distribuição bioma→elemento que a
  spec pede — a tabela já tem tudo.

- **O pet carregado já sabe seu tipo, e o encontro já sabe o lugar** —
  `FLoadedPetRecord::Type` (`PetDataLoader.h`) guarda o tipo assinado (ex.
  `"Natural/Fogo"`), `FPetTypeIdentity::Parse(...)` já separa escola de
  elemento, e `FEncounterMatchParams::EncounterLocation` (world position) já
  chega em `FEncounterMatchAssembler` — as duas peças para filtrar espécie por
  bioma já existem, só falta a ligação.

- ⚠️ **Achado, não tarefa daqui:** `.specs/project/ROADMAP.md` (linha ~192)
  já registra uma duplicação **não verificada** conhecida pela casa: duas
  frentes decidiram, no mesmo dia, "a arena veste o bioma de onde o encontro
  aconteceu." Qualquer tarefa que pinte por bioma (MB3 abaixo) precisa
  primeiro achar essa fonte existente e **reusá-la**, nunca abrir uma terceira
  — é exatamente o padrão de L-032/L-033.

## MB1 — O Portão só abre pra quem venceu o ranking

> 🤖 Modelo: `sonnet`

*Depende de:* `RegionLayout::ESettlementKind::PostoDeFronteira` (layout já
existe), `SettlementEconomy::Offers(ESettlementKind, ESettlementService::
PremioDeRanking)` (já existe, mas só confere se a Cidade Grande tem Arena —
não lê resultado nenhum).

**O que falta, medido:** `Portao` hoje é só uma caixa cinza
(`Village.cpp::CorDoPredio`, `VillageLayout.cpp` linha ~204). Não existe, em
lugar nenhum do código (`grep` por `IsOpen|Unlock|Gate|Aberto|Trancad` cruzado
com `portao|fronteira|ranking` não achou nada), nenhuma função que trave ou
destrave a passagem. O comentário do próprio enum ("a porta que só abre para
quem venceu o ranking") é hoje só comentário.

**Recorte desta tarefa, e por que é menor que "sistema de ranking":** não
existe placar, pontuação nem UI de ranking em lugar nenhum do jogo — construir
isso é outra frente. Esta tarefa constrói só o **portão**: uma função pura
`bool HasWonRegionalRanking(RegionId)` apoiada num único booleano por região
no estado do mundo (sem tabela de pontos), e o Posto de Fronteira consultando
essa função antes de deixar passar.

*Aceite:* com o booleano da região em falso, um pet que tenta atravessar o
Posto de Fronteira é barrado e a tela diz o motivo (`FBattleDebugScreen`); ao
virar o booleano para verdadeiro, o MESMO ponto passa a deixar passar, sem
recompilar nada além do estado.

*Contrapeso obrigatório:* nenhuma outra passagem da ilha (vila–vila,
vila–cidade) ganha o mesmo travamento — só o Posto de Fronteira, porque é o
único que a spec amarra ao ranking. Testar que atravessar entre vilas comuns
continua livre.

*Verificar com:* teste novo em `Tests/` afirmando "região sem ranking vencido
bloqueia o Posto; região com ranking vencido libera o MESMO Posto" — mesma
região, mesmo ponto, dois estados.

---

## MB2 — O pet selvagem muda com o bioma

> 🤖 Modelo: `sonnet`

*Depende de:* `FPetTypeIdentity::Parse` (já existe), `FLoadedPetRecord::Type`
(já existe), `IslandGeography::BiomeAt`/`IslandBiome` (já existe),
`FEncounterMatchParams::EncounterLocation` (já chega ao assembler).

**O que falta, medido:** `EncounterMatchAssembler.cpp` não cita a palavra
"Biome" nenhuma vez (`grep` vazio). `AvailablePets` chega pronto de fora sem
nenhum filtro por lugar — um pet de Vulcão aparece igual na Geleira.

A tabela bioma→elemento já está decidida pela spec ("As quatro ilhas, e por
que essas"): Floresta e Ruínas = Planta+Fantasma; Pântano e Mangue =
Água+Planta; Geleira = Luz+Água; Planalto e Penhascos = Ar+Luz; Vulcão e
Cavernas = Fogo+Terra.

*Aceite:* o MESMO catálogo de pets, rodado com `EncounterLocation` na
Floresta, produz uma lista de espécies **diferente** da mesma chamada rodada
com `EncounterLocation` no Vulcão — e nenhuma das duas lista um elemento fora
da tabela do próprio bioma.

*Contrapeso obrigatório:* `EncounterLocation == FVector::ZeroVector` (o caso
"batalha sem mundo" — teste, tela de batalha aberta direto) **não filtra
nada** — continua caindo no catálogo de sempre, porque não há bioma nenhum
para consultar.

*Verificar com:* teste novo comparando a lista filtrada nos dois biomas
extremos (Floresta vs. Vulcão) e um teste do caso `EncounterLocation` vazio.

---

## MB3 — A vila veste o bioma

> 🤖 Modelo: `sonnet` (🧠 se a checagem da duplicação ROADMAP apontar
> desenho de paleta novo, não reuso)

*Depende de:* `ScenaryPalette` (já existe, fonte única de cor de cenário),
`IslandGeography::ClimateAt`/`IslandBiome` (já existe).

**O que falta, medido:** `Village.cpp::CorDoPredio` devolve a MESMA cor por
tipo de prédio, sem olhar bioma nenhum — o Mercado é roxo na Geleira e no
Vulcão igual. Isso contradiz direto a decisão da spec ("vila é molde, muda
material e paleta por bioma").

**Antes de escrever uma linha:** confirmar, lendo código (não perguntando),
se a duplicação registrada em `ROADMAP.md` ("a arena veste o bioma") já
resolveu esse tipo de pintura em algum lugar reutilizável. Se sim, a vila
CHAMA essa mesma fonte — nunca cria uma segunda tabela cor×bioma
(L-032/L-033).

*Aceite:* a MESMA função de construção de vila, chamada com bioma Geleira e
com bioma Vulcão, pinta o MESMO prédio (ex. Mercado) com cores
mensuravelmente diferentes — e a arena e a vila, no mesmo bioma, concordam
(mesma fonte, mesma cor).

*Contrapeso obrigatório:* a identidade do prédio continua legível — trocar a
paleta por bioma não pode fazer dois tipos de prédio diferentes ficarem da
mesma cor dentro do MESMO bioma (perderia a distinção que `CorDoPredio` já
dava).

*Verificar com:* teste novo comparando a cor do mesmo `EVillageBuilding` nos
5 biomas — nenhum par de biomas pode empatar.

---

## MB4 — Cidade abandonada, depois do desastre

> 🤖 Modelo: `opus` 🧠 — é a única tarefa que liga dois sistemas que hoje não
> se falam (assentamento vivo e evento de mundo), decisão estrutural.

*Depende de:* `WorldEvents` (terremoto/furacão/tsunami, já existe, puro),
`RegionLayout`/`VillageLayout` (já existe).

**O que falta, medido:** nada liga hoje um evento de `WorldEvents` a uma
mudança de estado de um assentamento. `EGroundUse::Ruina` é decoração solta
(existe em TODO bioma, até deserto e geleira, como "ruína antiga") — não é
"esta vila específica sofreu um desastre e foi abandonada." O elemento
Fantasma existe no catálogo (`Config/PetTypes.json`) mas não há, em código,
nenhum lugar que o associe a um assentamento abandonado.

*Aceite:* uma vila que sofre um `WorldEvents::Tsunami`/`Hurricane` de
magnitude acima de um limiar muda de estado observável (prédios danificados
na tela, serviços que `SettlementEconomy::Offers` deixam de oferecer) — e a
MESMA vila, sem o evento, continua oferecendo os serviços de sempre.

*Contrapeso obrigatório:* o desastre não pode apagar retroativamente o save
de quem já visitou a vila — mesma disciplina da invariante "save antigo
carrega" de `itens-e-biologia`.

*Verificar com:* teste novo que aplica um `WorldEvents` sintético (seed e hora
fixos) sobre uma região e afirma a mudança de `SettlementEconomy::Offers`
antes/depois.

---

## O que estas tarefas NÃO fazem

- **Não decidem quantas vilas o jogador vê ao nascer**, nem o tamanho final
  do mundo — a spec já marca isso como pergunta em aberto, e aqui continua
  sendo.
- **Não constroem placar nem UI de ranking.** MB1 lê um booleano por região;
  o sistema de pontuação que alimentaria esse booleano é outra frente.
- **Não decidem se pets de região são exclusivos ou só mais comuns.** MB2
  assume EXCLUSIVO (lista fechada por bioma) porque é o que a tabela da spec
  descreve; trocar para "mais comum, não exclusivo" é decisão do usuário e
  muda a forma do filtro.
- **Não criam pet novo, nem asset novo.** Onde uma tarefa cita Fantasma, é o
  elemento que já existe no catálogo — nenhuma espécie nova nasce aqui.
- **Não mexem no traçado nem no gabarito.**

## Decisões que são do usuário

1. **Quantas vilas o jogador vê ao nascer, e onde ele nasce** — a spec deixa
   isso em aberto; hoje o layout de região já sabe posicionar tudo, falta
   decidir o ponto de entrada do jogador.
2. **Pets de região são EXCLUSIVOS do bioma ou só mais COMUNS nele?** Muda
   inteiramente a forma de MB2 (lista fechada vs. peso de sorteio).
3. **Quantas ilhas existem ao mesmo tempo, e como uma se liga à outra.** Hoje
   `IslandGeography` modela **uma** ilha por instância de servidor —
   `CoastDockCount`/`CoastDockBearingDegrees` já preparam encaixe físico da
   costa, mas nada no código hoje decide o que acontece quando o jogador
   atravessa essa doca. Se o produto quer as quatro ilhas jogáveis dentro da
   mesma sessão, isso é infraestrutura de outra escala — fora do que MB1–MB4
   cobrem.
4. **O limiar de magnitude que abandona uma vila (MB4)** — a spec não dá um
   número, e não deveria: é decisão de dificuldade/ritmo, não de engenharia.

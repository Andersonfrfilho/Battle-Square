# Mundo vivo — tarefas

**Regra de fatiamento desta feature:** a mata e o corte (MV1–MV5) são o que a
spec já decidiu como CONTEÚDO; envelhecer pet e coletar recurso (MV6–MV7) têm
pergunta aberta do usuário e por isso não têm aceite ainda — a task existe
para não desaparecer da lista, não para ser executada hoje.

O ciclo de cada tarefa é o da casa: ler a task → procurar o cano que já existe
→ escrever o teste e o contrapeso → implementar → build → bateria → auditorias
→ grep fora de `/Tests/` → pôr na tela → commit → próxima.

---

## MV1 — A IDADE DO MUNDO existe, e ela não para quando o jogo fecha
> 🤖 Modelo: `opus` 🧠 — atravessa a fronteira cliente/servidor pela primeira vez nesta feature

**O que a medição achou:** existe um relógio de DIA (`WorldTimeOfDay`,
`Source/BattleSquare/Public/Environment/WorldTimeOfDay.h`) e ele é lido por
`SceneLighting::GetHour()`/`GetElapsedHours()`
(`Source/BattleSquare/Public/Environment/SceneLighting.h`). Os dois são
membros comuns (`float CurrentHour = 12.0f; float ElapsedHours = 0.0f;`), sem
`UPROPERTY(SaveGame)` e sem linha em nenhum `USaveGame` — **zeram a cada boot
e a cada PIE.** "Idade do mundo" — o que a spec pede para calcular o tamanho
da árvore — não existe hoje em lugar nenhum do projeto: `rg -c
"WorldAge|IdadeDoMundo" Source/` devolve zero.

Um relógio que reseta ao fechar o jogo não serve: a árvore que cresceu ontem
teria seu crescimento apagado ao reabrir. A idade do mundo só significa algo
se **continuar contando mesmo com ninguém jogando** — exatamente como uma
árvore de verdade.

**A forma mais simples que atende isso não é um contador que avança:** é UMA
data de nascimento do mundo, gravada uma vez, e a idade é sempre "agora menos
essa data" — subtração pura, sem processo tickando, sem estado para divergir
entre dois clientes. `posse-no-servidor` (PS6) já abre exatamente essa porta:
"o jogo pode falar HTTP, e o núcleo continua sem alcançá-lo" — a idade do
mundo entra por ela, não por uma nova.

**Dependências:** `posse-no-servidor` PS6 (fronteira HTTP fora do núcleo) —
pode nascer antes se for lida a partir de um endpoint próprio, mas reaproveitar
a fronteira já desenhada evita uma segunda forma de o jogo falar com o
servidor.

**Verificação:**
```bash
cd apps/api-battle-pets && bunx drizzle-kit generate && bun test
./Tools/build_editor.sh && ./Tools/run_tests.sh
rg -n "WorldAge" Source/ apps/api-battle-pets/src/
```

*Aceite:* duas leituras da idade do mundo, feitas em dois dias de calendário
diferentes sem que ninguém jogue entre elas, devolvem números DIFERENTES — e
isso é precisamente o que `SceneLighting::ElapsedHours` não faz hoje, porque
zera a cada boot.

*Contrapeso obrigatório:* backend fora do ar não impede abrir o jogo — a
leitura falha para um valor de fallback visível ("idade do mundo:
desconhecida"), nunca para zero silencioso, que pareceria um mundo
recém-nascido.

---

## MV2 — A árvore cresce com a idade do MUNDO, não com a dela própria
> 🤖 Modelo: `sonnet`

**O que a medição achou:** `ForestBackdrop::BuildForest`/`BuildRegion`
(`Source/BattleSquare/Public/Environment/ForestBackdrop.h`) plantam a mata
inteira a partir de uma **semente** — determinístico, recalculado sempre que
o pedaço de mundo volta a existir, nada gravado. Não existe hoje nenhum eixo
de idade: a escala de cada árvore é fixa no momento da montagem, não importa
se o mundo "existe" há um dia ou há um ano.

A spec decide que o tamanho é **semente + idade do mundo** — não idade da
árvore individual, porque duas árvores plantadas no mesmo instante (a mesma
semente) crescem sempre JUNTAS. Dar a cada árvore seu próprio relógio criaria
uma segunda fonte de idade para a mesma pergunta, e é o defeito que
L-032/L-033 já documentaram noutro contexto.

**Dependências:** MV1.

**Verificação:**
```bash
./Tools/build_editor.sh && ./Tools/run_tests.sh
./Tools/audit_determinism.sh
```

*Aceite:* o MESMO ponto de mata (mesma semente), medido com a idade do mundo em
dois valores diferentes, produz duas escalas diferentes — hoje produz sempre a
mesma, porque não existe o terceiro argumento.

*Contrapeso obrigatório:* a POSIÇÃO e a ESPÉCIE de cada árvore continuam
vindo só da semente — a idade do mundo só entra na escala. Recalcular posição
também apagaria o "a mesma semente planta a mesma mata" que `ForestBackdrop`
já garante hoje.

---

## MV3 — Cortar deixa marca: a árvore derrubada NÃO volta sozinha amanhã
> 🤖 Modelo: `opus` 🧠 — novo estado compartilhado, novo módulo de servidor

**O que a medição achou:** dentro de `BattleSim`, `FBattleState` já resolve
exatamente este problema para o terreno de uma arena —
`CellCountdown`/`CellRevertsTo`/`SetTemporaryTerrain`
(`Source/BattleSim/Public/Battle/BattleState.h`): a base continua sendo a que
o layout calcula; só a EXCEÇÃO (para onde reverter, e quantos passos faltam)
é lembrada. O contador desce por TURNO DE BATALHA
(`BattlePhaseResolution.cpp`), e morre com a arena — não dá para reusar esse
código, mas o PADRÃO (base calculada + exceção com prazo) é exatamente o que
esta task pede, só que em tempo real e fora do `BattleSim`.

Fora da batalha não existe nada parecido: cortar uma árvore hoje não deixa
rastro nenhum em lugar nenhum do projeto (a mata é 100% recalculada da
semente, por `ForestBackdrop`), e o mundo é COMPARTILHADO — a marca do corte
precisa estar no servidor, não no save de quem cortou, senão o segundo
jogador que passa pelo mesmo lugar vê a árvore que o primeiro derrubou.

**Dependências:** MV1 (idade/relógio real), `posse-no-servidor` PS1/PS8
(tabela + escrita por fila — cortar é evento com efeito no mundo, não precisa
esperar resposta para o jogador continuar jogando).

**Verificação:**
```bash
cd apps/api-battle-pets && bunx drizzle-kit generate && bun test
./Tools/build_editor.sh && ./Tools/run_tests.sh
```

*Aceite:* cortar uma árvore, fechar o jogo, e reabrir depois de o PRAZO
configurado (MV4) ter passado — ela voltou sozinha. Reabrir ANTES do prazo —
continua cortada. Sem exceção com prazo, hoje ela nunca sai (não existe
estado nenhum) ou sempre volta (recalculada da semente): os dois são o mesmo
defeito visto de lados opostos.

*Contrapeso obrigatório:* terreno permanente (nenhum corte pendente) nunca
grava exceção nenhuma — a tabela cresce só com o que É diferente da semente,
nunca com o mundo inteiro. Gravar toda árvore intacta transformaria a mata
inteira em estado de servidor, que é o problema que "base calculada" existe
para evitar.

---

## MV4 — O prazo de rebrota é NÚMERO DE CONFIGURAÇÃO, nunca literal em C++
> 🤖 Modelo: `sonnet`

**O que a medição achou:** o precedente já existe duas vezes no projeto —
`ScenaryClimate::ConfiguredClimate()` lê `DefaultGame.ini`, seção
`[/Script/BattleSquare.MountainRange]`; `PetBiologyCatalog`/`ItemCatalog` leem
`Config/*.json`. Um prazo de rebrota escrito como literal em C++ obrigaria
recompilar o jogo inteiro para ajustar um único número de equilíbrio — e é
exatamente esse recompilar que a torneira de `mae-natureza` existe para
evitar (ver `mae-natureza/tasks.md`, MN5).

**Dependências:** MV3.

**Verificação:**
```bash
./Tools/build_editor.sh && ./Tools/run_tests.sh
```

*Aceite:* mudar o número em `DefaultGame.ini` (seção
`[/Script/BattleSquare.WorldAge]`, chave `TreeRegrowthDeadlineHours`) muda o
tempo de rebrota sem recompilar — o teste lê dois valores diferentes do
arquivo e confere dois prazos diferentes.

---

## MV5 — Idade do mundo e corte pendente aparecem na TELA
> 🤖 Modelo: `sonnet`

**Por que esta task existe:** regra do `CLAUDE.md` da raiz — "toda vez que
implementar comportamento observável em jogo, mostre na tela o que está
acontecendo." MV1–MV4 mudam uma escala e uma data que hoje ninguém vê; sem
esta task, a árvore cresce e o corte se apaga sozinho e ninguém no projeto
consegue notar sem ler o banco de dados diretamente.

**Dependências:** MV1, MV3.

**Verificação:**
```bash
./Tools/build_editor.sh && ./Tools/run_tests.sh
rg -n "Key ?= ?7[0-9][0-9]" Source/
```

*Aceite:* `FBattleDebugScreen::Show` ganha uma linha com chave nova (não
740–750, já usadas pelo painel do mundo e pelo relógio) mostrando a idade do
mundo e, se houver corte pendente por perto, quanto falta para a rebrota.

---

## MV6 — Pets envelhecem — BLOQUEADA, decisão do usuário
> 🤖 Modelo: `sonnet` (quando destravada)

**Por que está bloqueada:** a spec deixa três perguntas em aberto sobre
envelhecimento de pet, e nenhuma tem resposta: a idade tem FIM (o pet pode
morrer de velho)? o tempo passa com o jogo FECHADO, para o pet? o cuidado do
dono é ATIVO (uma ação) ou PASSIVO (só o tempo)? Sem essas três respostas não
existe aceite possível — inventar um critério aqui seria conteúdo decidido
por quem escreve a task, e a instrução da casa é clara: conteúdo é decisão do
usuário (ver `GOAL.md`, "Decisões que são do usuário").

**O que já dá para reaproveitar quando destravar:** MV1 já resolve "o tempo
passa com o jogo fechado" PARA A MATA (é subtração de data, sempre atual) —
se a resposta para pet for a mesma, a infraestrutura é a mesma tabela, só um
`genesis_at` por pet em vez de um só para o mundo.

**Dependências:** decisão do usuário; se destravada, MV1.

*Aceite:* nasce junto com a resposta — não existe hoje.

---

## MV7 — Coleta de recursos, bosque e comerciante — BLOQUEADA, decisão do usuário
> 🤖 Modelo: `sonnet` (quando destravada)

**Por que está bloqueada:** a spec decide a REGRA (corte livre em bosque
plantado, regulado em mata selvagem) mas deixa em aberto o CONTEÚDO que a
regra precisa para existir: quem é o dono de uma plantação, se a ferramenta
usada importa, e se coletar exige o pet por perto. `GuardaFlorestal`,
`Comerciante` e qualquer variante em português não têm NENHUMA ocorrência em
`Source/` hoje (`rg -c "GuardaFlorestal|Comerciante|Merchant" Source/` = 0) —
não é um sistema incompleto, é um sistema que ainda não começou.

**Dependências:** decisão do usuário; MV1–MV4 (o padrão de exceção-com-prazo
que MV3/MV4 constroem para árvore é o mesmo que pedra e barro vão precisar).

*Aceite:* nasce junto com a resposta — não existe hoje.

---

## O que estas tarefas NÃO fazem

- **Não decidem envelhecimento de pet nem conteúdo de coleta.** MV6 e MV7
  existem para não desaparecer da lista, não para serem executadas — ver
  "Decisões que são do usuário" no `GOAL.md`.
- **Não tocam o terreno temporário do `BattleSim`.** `CellCountdown`/
  `CellRevertsTo` continuam sendo só da arena; o padrão é imitado, o código
  não é reusado, e nenhuma task aqui adiciona uma dependência nova de
  `BattleSquare` para dentro de `BattleSim`.
- **Não escolhem onde exatamente o backend mora.** Assume-se estender
  `apps/api-battle-pets` pelo padrão de `ownership/`/`pet/`, por ser o que já
  existe — se for serviço novo, é decisão de arquitetura a confirmar antes de
  codar (`code-standart.md` §2: "caminho incerto: parar e perguntar").
- **Não implementam a leitura do lado de `mae-natureza`.** MV3/MV4 guardam e
  configuram o prazo de rebrota; quem LÊ esse estado para decidir corrigir a
  torneira é `mae-natureza` (MN6), não esta feature.
- **Não fazem a árvore desaparecer da malha de streaming.** `streaming-de-mundo`
  ainda não existe ("não existe nenhum nível persistente" — medido no próprio
  `spec.md` dela); a mata continua recalculada por `ForestBackdrop` como
  hoje, só que agora consultando idade e exceção antes de escolher escala.

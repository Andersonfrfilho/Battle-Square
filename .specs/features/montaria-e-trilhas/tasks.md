# Montaria e trilhas — tarefas

## O que JÁ existe, medido

A spec cita um bloqueio explícito: "sem altura de chão, não dá para cobrar
cansaço de subida." Isso não é mais verdade — foi medido lendo
`IslandGeography.h` e `.cpp` diretamente, não a spec:

- **Altura e inclinação do chão já existem** —
  `IslandGeography::GroundHeightAt`, `NaturalGroundHeightAt`,
  `BedrockHeightAt`, `GroundSlopeAt`. O comentário do próprio código já
  amarra isso ao propósito de montaria: foram escritas para "destravar o
  cansaço de subida" antes mesmo desta spec existir.

- **Custo de trajeto já existe, e já separa subida de descida** —
  `TravelCostBetween`, `UphillCostWeight()`, `DownhillCostWeight()`. O
  comentário explica o motivo de estarem juntas num único lugar: "quem TRAÇA
  a trilha e quem COBRA o cansaço precisam concordar" — exatamente a
  discórdia que esta spec teria criado se cada sistema inventasse sua própria
  conta.

- **Barranco já garante que dá para chegar a pé** —
  `BluffInnerRadiusUnits`, `BluffOuterRadiusUnits`, `BluffRampAngleDegrees`,
  `BluffRampHalfWidthDegrees`, `IsOnBluffRamp` — nenhum planalto é parede
  pura, sempre tem rampa. Isso é o piso sob qualquer regra de "só monta certo
  pet passa aqui."

- **Trilha já traça caminho mais barato e evita perigo** —
  `TrailLayout.cpp` já usa o custo de trajeto para traçar, e já exclui a
  rocha queimada do vulcão (`VolcanoScorchedRadiusUnits`), com teste em
  `TrailLayoutTest.cpp`. O traçado que MT-alguma-coisa precisaria não nasce
  do zero — herda isso.

- **Disciplina de proporção do lote já existe como precedente** —
  `VillageLayout::PlotHalfExtentUnitsFor(Kind)` já varia por tipo em vez de
  ser um número fixo. É o mesmo padrão que a largura de trilha (se vier a
  precisar de uma) deveria seguir: fração de alguma coisa que já existe, não
  metro fixo.

- **O jogador no mundo já tem UM pet identificado** —
  `WorldStatusReadout.h`: `FWorldStatusSnapshot::OwnedPet` (um só, não uma
  lista com seleção). O comentário do arquivo diz por quê: "fora da batalha o
  jogador andava por um mundo sem saber quem era o seu pet." Isso simplifica
  a montaria: não é preciso construir "escolher pet ativo" como
  pré-requisito, o mundo já sabe qual é o pet do jogador.

**O que continua genuinamente sem existir, medido:**

- Nenhum campo de tamanho/peso no pet (`PetDataLoader.h` grepado —
  só `FLoadedPetMove` e `Type`; nada de `Size`/`Tamanho`/`Weight`/`Peso`).
- Nenhum conceito de montar (`WorldExplorerCharacter.h` grepado — só
  `WalkSpeedBeforeSprint`/`SprintSpeedMultiplier`; nada de mount/fly/swim).
- Nenhum sistema de fadiga/stamina em lugar nenhum do código fora de batalha.

## MT1 — Andar montado usa o custo de trajeto que já existe

> 🤖 Modelo: `sonnet`

*Depende de:* `IslandGeography::TravelCostBetween`,
`UphillCostWeight`/`DownhillCostWeight` (já existem, puras).

**O que falta, medido:** `WorldExplorerCharacter.h` só tem
`WalkSpeedBeforeSprint` e `SprintSpeedMultiplier` — não existe velocidade
"montado." A conta de custo de ladeira já existe e está pronta para virar
velocidade: ela é a MESMA função que a trilha usa para decidir o caminho mais
barato.

*Aceite:* o MESMO trecho de ladeira, andado a pé e depois montado, produz uma
velocidade EFETIVA maior montado — e a proporção entre subida e descida
continua sendo a de `UphillCostWeight`/`DownhillCostWeight` (não uma segunda
conta inventada para montaria).

*Contrapeso obrigatório:* o cálculo de custo que a trilha já usa
(`TrailLayoutTest.cpp`) continua dando o mesmo resultado depois desta tarefa —
nenhum ajuste em `TravelCostBetween` pode mudar o traçado já testado.

*Verificar com:* teste novo comparando velocidade efetiva a pé vs. montado no
mesmo par de pontos, em subida e em descida.

---

## MT2 — Cansaço de quem carrega alguém em cima

> 🤖 Modelo: `sonnet` (os números exatos de limiar são decisão do usuário —
> ver seção final; a forma da conta é execução padrão)

*Depende de:* MT1 (usa a mesma função de custo).

**O que falta, medido:** nenhum estado de fadiga existe fora de batalha hoje.

*Aceite:* subir a MESMA ladeira repetidamente, montado, acumula um valor de
cansaço observável na tela (`FBattleDebugScreen`) que cresce mais rápido na
subida do que na descida — na mesma proporção que
`UphillCostWeight`/`DownhillCostWeight` já usam para custo, não uma tabela
paralela.

*Contrapeso obrigatório:* cansaço é um valor NOVO, isolado — não pode
compartilhar a mesma barra que HP de batalha nem zerar ao entrar em combate.
Testar que uma batalha não altera o cansaço acumulado no mundo.

*Verificar com:* teste que sobe e desce a mesma ladeira em sequência e afirma
que o cansaço acumulado varia com a inclinação, não é constante por metro
andado.

---

## MT3 — Peso de quem monta não trava, cansa mais

> 🤖 Modelo: `sonnet`

*Depende de:* MT2 (fadiga precisa existir antes de ter um multiplicador).

**O que falta, medido:** nenhum campo de peso/tamanho existe em
`PetDataLoader.h` hoje — é dado novo.

*Aceite:* o MESMO trajeto, com um pet leve e com um pet pesado montados,
acumula fadiga em ritmos diferentes — nunca um dos dois fica IMPOSSÍVEL de
percorrer, só mais cansativo.

*Contrapeso obrigatório (mandatório, citado na spec):* peso NUNCA bloqueia
passagem — só multiplica o cansaço. Teste negativo obrigatório: o pet mais
pesado do catálogo ainda consegue completar qualquer trajeto que o mais leve
completa, só mais devagar/cansado.

*Verificar com:* teste comparando fadiga acumulada por dois pets de peso
diferente no mesmo trajeto, e teste afirmando que nenhum trajeto fica
inacessível por peso.

---

## MT4 — Nem todo pet pode ser montado

> 🤖 Modelo: `sonnet` — dado novo com default retrocompatível é execução
> padrão; a REGRA de quem monta é decisão do usuário, não desta tarefa

*Depende de:* nenhuma outra MT — pode ser feita em paralelo com MT1–MT3, mas
precisa existir antes de MT1 poder checar "este pet pode ser montado."

**O que falta, medido:** nenhum campo de montabilidade existe em
`FLoadedPetRecord` hoje.

*Aceite:* um pet marcado como montável no dado pode ser escolhido para
montaria; o MESMO fluxo tentado com um pet não marcado é recusado, com a tela
dizendo por quê — e um pet de save ANTIGO, sem o campo novo, carrega com um
default explícito (nunca crasha, nunca vira "montável" por acidente).

*Contrapeso obrigatório:* dado antigo sem o campo continua carregando sem
erro — teste de retrocompatibilidade obrigatório, no mesmo padrão que
`itens-e-biologia` já exige para inventário salvo antes da feature existir.

*Verificar com:* teste de carga de um registro de pet SEM o campo novo,
afirmando o default; teste de carga COM o campo, afirmando o valor lido.

---

## MT5 — O pet montado aparece na tela, sempre

> 🤖 Modelo: `sonnet`

*Depende de:* MT1 (precisa existir "andar montado" para ter o que desenhar).

**O que falta, medido:** nenhum ator de montaria existe — não há o que
auditar ainda, mas a lição já é conhecida: este projeto teve TRÊS atores
nascerem sem malha atribuída e passarem em toda bateria de teste (pet,
inimigo do mundo, o próprio jogador). Esta tarefa nasce já citando essa
lição, não descobrindo-a de novo.

*Aceite:* o pet montado é visível na tela ao lado/sob o personagem, com cor
própria — e o teste verifica a ATRIBUIÇÃO de malha e cor no construtor do
ator novo, não só que o ator existe.

*Contrapeso obrigatório:* o personagem a pé (sem montaria) continua visível
exatamente como hoje — a tarefa não pode regredir a malha do
`ACharacter` já corrigida.

*Verificar com:* teste que constrói o ator de montaria e afirma
`GetStaticMeshComponent()->GetStaticMesh() != nullptr` (ou equivalente), no
mesmo padrão dos testes que já existem para pet e inimigo do mundo.

---

## O que estas tarefas NÃO fazem

- **Não criam veículo mecânico.** A spec já rejeita isso — montaria é pet,
  não carro.
- **Não geram trilha emergente por pisoteio**, nem estrada pavimentada, nem
  viagem rápida pela trilha — a spec rejeita as quatro, e nenhuma tarefa
  acima as reabre.
- **Não implementam voar nem submergir no MUNDO.** Esses verbos já existem
  dentro de `BattleSim` (batalha); trazê-los para fora da batalha, se um dia
  fizer sentido, é outra frente — MT1–MT5 tratam só de andar montado no chão.
- **Não decidem os números finais de cansaço, peso ou velocidade** — a forma
  da conta é entregue (reusa `TravelCostBetween`), os números concretos são
  decisão do usuário (ver abaixo).
- **Não mexem em `IslandGeography`, `TrailLayout` nem em `mundo-por-biomas`.**
  São objetivos irmãos.

## Decisões que são do usuário

1. **Quais pets podem montar, por tamanho ou por regra?** MT4 constrói o
   campo e o comportamento de recusa; QUEM entra nessa lista é decisão de
   produto, não de engenharia.
2. **O cansaço se recupera sozinho, e a que taxa?** Sem essa decisão, MT2
   entrega só o acúmulo — recuperação fica registrada como pendente, não como
   suposição.
3. **Cansaço é a MESMA barra que HP de batalha, ou uma barra própria?** Esta
   lista de tarefas RECOMENDA barra própria (MT2, contrapeso obrigatório),
   mas a palavra final é do usuário.
4. **Voar por cima da trilha ignora o custo do relevo, ou paga alguma
   conta própria?** Fora do escopo de MT1–MT5 (que tratam de montaria
   terrestre), mas é a pergunta natural que vem depois.
5. **Nadar em superfície no mundo usa o mesmo verbo `Submergir` da batalha,
   ou é outra mecânica?** Levantado pela spec, não respondido aqui.

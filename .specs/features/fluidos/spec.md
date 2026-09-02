# Fluidos — a substância como eixo próprio

**Escrito em 02/09/2026.** Motivado por: *"precisamos bem separar os fluidos,
alguns itens farão diferença entre eles, poderes, skills."*

## A medição que originou esta spec

O jogo já sabia responder **quanta água há numa casa**. Não sabia responder
**de que ela é feita.**

| onde | o que existia | o que faltava |
|---|---|---|
| `ECellProperty` | `Water`, `ShallowWater`, `Ice`, `Mud` — fundura e estado, com cadeia de secagem testada | a substância |
| `BattleState` | `RequireTerrainForSkill(Submergir, Water)` — **uma água só** | submergir na lava passava |
| `EScenaryRole` | `FreshWater`, `SwampWater`, `SwampMud`, `CaveWater`, `LavaGlow` | eram só **cores** |
| mundo | água doce, termal, do mar, de pântano, de caverna, lava | nenhuma diferença de regra |

Seis fluidos existiam no mundo e a diferença entre eles vivia na paleta. Um
poder não tinha como distinguir um do outro.

## A decisão: DOIS EIXOS, independentes

- **Fundura** (`ECellProperty`) — quanta água há. Já existe, funciona, tem
  cadeia de secagem e testes. **Não foi alterada.**
- **Substância** (`EFluidKind`) — de que ela é feita. É o eixo novo.

Independentes de propósito, e é isso que os torna úteis: fundura diz se cabe
nadar, substância diz o que acontece com quem nada. Uma poça de lava e um rio
de lava são a mesma substância em funduras diferentes, e um poder que resiste a
uma resiste à outra.

## Onde mora, e por quê

No **`BattleSim`**, porque skill é regra de combate. Isso impõe a restrição que
molda o resto: **sem float, sem sorteio, sem relógio.** A densidade vai em
**partes por mil**, inteiro — água doce = 1000, e tudo se lê contra ela. Por
mil e não por cento porque a diferença entre água doce e salgada é de 2,5%, e
ela desapareceria arredondada.

## Os campos, e o que cada um habilita

| campo | para quê | novo ou migrado |
|---|---|---|
| `DensityPerMille` | o que boia sobre o quê — a balsa | novo |
| `bAllowsSubmerge` | Submergir deixa de valer na lava | migra `RequireTerrainForSkill` |
| `DamagePerTurn` | estar na lava custa | novo |
| `bIsWater` | um poder vale para "água" sem enumerar as cinco | novo |

`bIsWater` é o que impede a próxima skill de nascer com uma lista de fluidos —
lista à qual a sexta água ficaria de fora, calada.

## O que o registro NÃO faz

- **Não mexe em `ECellProperty`.** A cadeia gelo → poça → lama → seco está
  testada e é de outro eixo.
- **Não decide escorregão nem atraso.** Isso é estado da casa (gelo, lama), e
  já mora em `TerrainSlipPercent`/`TerrainSlowPercent`.
- **Não mapeia casa → fluido ainda.** Hoje o registro responde *sobre* um
  fluido; ninguém ainda diz que a casa X tem o fluido Y. Isso é a próxima
  tarefa, e é ela que liga o eixo novo ao tabuleiro.

## Os números, e por que têm referência de fora

Densidade real em partes por mil: água doce 1000, salgada 1025, termal 988
(quente é menos densa), pântano 1080, lama 1500, lava 3100.

Ter referência do mundo real é o que impede a tabela de virar gosto. Quando um
deles for ajustado por equilíbrio, a mudança fica **visível justamente por
divergir da referência** — e isso é informação, não ruído.

## O caso que só a densidade pega

Uma pedra afunda na água e **boia na lava**, porque a lava é mais densa que
ela. Um booleano `bFloats` no corpo erraria isso sempre, e erraria calado. É a
razão de o que boia depender dos **dois lados**.

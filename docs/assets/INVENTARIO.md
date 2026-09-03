# Inventário de assets — o que o jogo promete e o que ele veste hoje

Levantado em 02/09/2026, por medição do código e das specs.

**O que este documento é:** a lista do que já foi prometido em tela e hoje
aparece como primitiva da engine, cor ou texto. **Quantos e quais** é derivado
das specs; **como cada um se parece** continua sendo decisão do usuário, e este
arquivo não decide nada disso.

**Por que ele existe:** a frase "Não autora asset" aparece em **dez** GOALs,
sempre como fronteira correta da feature — foi ela que permitiu construir e
testar a lógica sem esperar arte. O que faltava era alguém **somar a conta**.
Sem esta lista, "quanto falta de arte" é palpite.

---

## 1. A medição de hoje

| | |
|---|---|
| `.uasset` no `Content/` | **154** — dos quais **148** são `MI_StreamingCell_*` gerados por código |
| Assets realmente autorados | **6** |
| Malhas do jogo inteiro | **4 primitivas da engine**: `Cube`, `Sphere`, `Cylinder`, `Cone` |
| Materiais | `BasicShapeMaterial` e `WorldGridMaterial` |
| Papéis de cenário já nomeados | **28** (`EScenaryRole`) — todos pintados por cor |
| Classes de ator | **26** |
| Áudio | **zero** — nenhuma linha de código, nenhuma menção em GOAL |
| Partículas / Niagara | **zero** |
| Animação | **zero** — `APetView` já declara um `USkeletalMeshComponent`, sem malha e sem `AnimInstance` |

Os seis autorados: `WBP_BattleActionSelector`, `BP_WorldExplorer`, `IA_Move`,
`IA_Look`, `IMC_Traversal`, `IslandBaked`. Nenhum deles é arte: são interface,
input e dado assado.

---

## 2. Quem NÃO precisa de malha

Cinco atores constroem a geometria por código, com `UProceduralMeshComponent`.
Eles pedem **material**, não malha:

`ATerrainMesh` · `ARiverMesh` · `ATrailMesh` · `ACrossingMesh` · `AAqueductMesh`

---

## 3. A tabela de substituição

Cada linha é uma dívida: o ator existe, funciona e é testado — vestindo uma
primitiva.

| Ator | Veste hoje | Representa |
|---|---|---|
| `APetView` | Esfera, Cubo, Cone, Cilindro | **o pet** — a forma varia por porte |
| `APetOwnerView` | Cilindro, Esfera, Cone | o treinador ao lado da arena |
| `AWorldExplorerCharacter` | Esfera + Cubo | **o jogador** no mundo |
| `AWorldEncounterActor` | Esfera | o adversário que dispara batalha |
| `ABattleArena` | Cubo | o tabuleiro e suas casas |
| `AVillage` | Cubo | vila e construções |
| `AGroundUseActor` | Cubo, Cilindro, Cone, Esfera | uso do solo (lavoura, pasto, cemitério) |
| `AWorldTrainingField` | Cubo estático + instâncias | os cinco campos de treino |
| `AFerryActor` | Cubo | o convés da balsa |
| `ACaveSystem` | Cubo + Cone | pedra e estalactite |
| `AWalkableMountain` | Cilindro + Cubo | corpo do monte e patamar |
| `AMountainRange` | Cone | a serra ao fundo |
| `AForestBackdrop` | Cilindro + Cubo + Esfera | tronco, galho e copa |
| `AVolcano` | Cilindro + Cubo + Esfera | disco, bloco e bafo |
| `AWorldBoundaryWater` | Cilindro | o mar que fecha o mundo |
| `AAuroraCurtain` | Cubo | a fita da aurora |
| `ABattleDebugHUD` | HUD desenhada | todo o painel de estado |

---

## 4. Os 28 papéis de cenário

Já nomeados em `EScenaryRole`, hoje resolvidos só por cor. Cada um é um slot de
material — e, num jogo acabado, de textura:

`GroundCover` `Undergrowth` `Accent` `DeadWood` `Rock` `ForestTree` `CanopyTree`
`MountainRock` `MountainSnow` `ClimbableRock` `MountainTrail` `CaveRock`
`CaveFloor` `DesertSand` `DesertRock` `GlacierIce` `VolcanicRock` `LavaGlow`
`BeachSand` `SwampMud` `SwampWater` `AshPlume` `CaveWater` `WetSand`
`WaterFoam` `FreshWater` `AuroraVeil` `AuroraCrown`

Duas dívidas explícitas aqui, escritas nos próprios GOALs:
**a casa de lava veste o material da ÁGUA** até alguém dar outro a ela.

---

## 5. A dívida já escrita nas features

As dez exclusões, na palavra de cada GOAL:

- *"A casa de lava veste o material da água até alguém…"* (duas features)
- *"O material de lava continua sendo o da água."* (duas features)
- *"A lâmina de água na frente da boca é decisão do usuário"*
- *"Ponte e poço aparecem por malha procedural e cor"*
- *"Pet a mais aparece pela malha e cor que `APetView` já atribui"*
- *"Fundura aparece por texto e por número."*
- *"Item aparece por texto no painel."*
- *"O achado aparece por texto no painel."*

---

## 6. O que as decisões de 02/09 acrescentaram

Nenhum destes tem ator, malha nem cor hoje:

| vem da decisão | o que passou a ser preciso |
|---|---|
| 6 | **pets aquáticos**, plantas aquáticas, degraus e itens no fundo do poço |
| 54 | **templos**, e o gesto de orar |
| 22, 23, 27 | cartaz de procurado, **prisão**, e o que se vê de um bandido |
| 20, 25 | **mercado-negro** como lugar, e as organizações criminosas |
| 37, 45 | montaria, e o pet que ajuda a descansar |
| 18 | **placas** de sinalização — *pendente de decisão* |
| M6 (`a-carta-muda-uma-vez`) | ponte de **bloco, madeira e destruída** — três estados |

---

## 7. As categorias com ZERO

Não é "pouco": é nenhum, e nenhuma spec pede.

| categoria | situação |
|---|---|
| **Som** | zero. Sem efeito, sem trilha, sem ambiência. Nenhum GOAL menciona |
| **Animação** | zero. O componente esqueletal do pet existe vazio, esperando |
| **Partículas / VFX** | zero. Fogo, água, poeira e explosão são cor de malha |
| **Ícones de interface** | zero. O painel é texto desenhado |
| **Interface autorada** | um widget (`WBP_BattleActionSelector`); a barra de botões é Slate por código |

---

## 8. O que é derivável e o que é seu

**Derivável (está aqui):** quantos slots existem, quais atores os pedem, e onde
no código a primitiva é atribuída.

**Seu (não está aqui, e não deve estar):** estilo, direção de arte, se pet é
bicho realista ou forma estilizada, quantas espécies, se haverá som, e a ordem
em que a arte entra.

**Uma coisa a decidir cedo, porque muda o resto:** hoje toda cor vem de
`ScenaryPalette`, um lugar só. Enquanto a arte for cor, isso se sustenta. No
momento em que entrar textura, esse ponto único vira o lugar por onde o material
é escolhido — ou vira a segunda fonte de verdade, que é o defeito L-032 que este
projeto já pagou três vezes.

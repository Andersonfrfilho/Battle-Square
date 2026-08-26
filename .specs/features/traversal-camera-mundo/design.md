# Traversal e Câmera de Mundo — Design

**Spec:** `.specs/features/traversal-camera-mundo/spec.md`
**Status:** Draft — aguarda aprovação

---

## O problema de testabilidade que governa o design

Traversal é a feature mais difícil de testar headless de todo o M5, e vale dizer por quê antes de decidir qualquer coisa: **movimento de personagem na Unreal mora dentro do `UCharacterMovementComponent`**, que depende de física, colisão, gravidade e tempo real. Testar "o pawn andou 300uu para frente" num teste `nullrhi` é testar a engine, não o nosso código — e é exatamente o tipo de teste frágil que DP-streaming-05 e AD-004 já ensinaram este projeto a recusar.

A saída é a mesma de sempre neste repositório: **separar a decisão do efeito.** O que é nosso é decidir *para onde* mover, dado o input e a orientação da câmera. Quem move é a engine. A parte nossa é aritmética pura e vai para uma função sem `UWorld`, sem ator e sem tempo — testável de verdade. A parte da engine é configuração, e o que se testa dela é que **está configurada como o design manda**, não que a Unreal funciona.

## DP-trav-01: O pawn de jogador é um `ACharacter`

**Decisão:** `AWorldExplorerCharacter` (novo, `BattleSquare/World/`), derivado de `ACharacter`.

**Razão:** `ACharacter` já traz cápsula de colisão, `UCharacterMovementComponent` com gravidade e caminhada sobre o chão, e o contrato de `AddMovementInput` que a engine inteira espera. `ADefaultPawn` (o que o `DebugRoutePawn` usa) voa e atravessa cenário — é ótimo para uma régua determinística e péssimo para P1/critério 2.

**Consequência aceita:** a cápsula anda sem animação. Animação é conteúdo, declarado fora de escopo — o roteiro manual assume uma cápsula deslizando, e isso não invalida nada que a feature promete.

## DP-trav-02: A conversão input → direção é função pura

**Decisão:** `FWorldTraversalMotion::ComputeMoveDirection(const FWorldTraversalMotionParams&)`, estática e pura. Recebe o eixo de input 2D (frente/lado) e o `FRotator` da câmera; devolve a direção **no espaço do mundo**, normalizada.

Regras que ela implementa, todas testáveis:
- só o **yaw** da câmera entra na conta. Pitch e roll são descartados — senão olhar para baixo empurraria o personagem para dentro do chão, que é um bug clássico e silencioso.
- entrada diagonal é **normalizada**: andar na diagonal não pode ser mais rápido que andar reto.
- entrada nula devolve vetor nulo, sem normalizar (normalizar zero é indefinido, e devolver lixo aqui viraria deriva — P1/critério 3).

**Razão:** é a única parte do traversal que é decisão nossa. Isolada, ela é testada com números exatos; dentro do `ACharacter`, só seria testável ligando a engine inteira.

## DP-trav-03: A câmera é um braço de mola configurado por constantes nomeadas

**Decisão:** `USpringArmComponent` + `UCameraComponent` no `AWorldExplorerCharacter`, com:
- `TargetArmLength = CameraArmLengthUnits` (constante nomeada)
- `bDoCollisionTest = true` — é isto, e só isto, que satisfaz P1/critério 2 da câmera: o braço encolhe quando há geometria no caminho
- `bUsePawnControlRotation = true` no braço, e **false** na câmera — a rotação vive num lugar só
- limites de pitch do controlador em constantes nomeadas (`CameraPitchMinDegrees`/`CameraPitchMaxDegrees`)

**Razão:** o `USpringArmComponent` já resolve seguimento, atraso e colisão de câmera — reimplementar seria assinar embaixo de bugs que a engine já corrigiu. O que é nosso aqui é a **escolha dos números**, e por isso eles são constantes nomeadas (§16 do `code-standart.md`), não literais espalhados.

**O que se testa:** que o rig está montado e configurado com esses valores. Não se testa "a câmera não atravessou a parede" — isso é a engine, e é item do roteiro manual.

## DP-trav-04: O personagem gira para onde anda, a câmera não o arrasta

**Decisão:** `bOrientRotationToMovement = true` no movimento, `bUseControllerRotationYaw = false` no ator.

**Razão:** sem isso, o personagem fica travado apontando para onde a câmera olha e "anda de lado" — que é o comportamento de shooter, não de explorador. A combinação escolhida é a de terceira pessoa clássica: a câmera orbita livre, o corpo vira na direção do movimento.

## DP-trav-05: Encontros reusam o componente existente, sem exceção

**Decisão:** `AWorldExplorerCharacter` recebe um `UEncounterDetectionComponent` como subobjeto padrão. Zero código de detecção novo.

**Razão:** a feature anterior já resolveu detecção e transição, com 9 testes. Um segundo caminho aqui seria o não testado — e é exatamente o erro que `colecao-e-captura` evitou ao não reimplementar captura.

## DP-trav-06: Input via Enhanced Input, com os assets fora do C++

**Decisão:** o C++ declara ponteiros para `UInputMappingContext` e `UInputAction` (`TObjectPtr`, `EditDefaultsOnly`) e implementa os handlers; os **assets** (IMC/IA) são criados no Editor e atribuídos ao Blueprint do personagem.

**Razão:** é o padrão que a própria Epic recomenda (`unreal-best-practices`), e mantém a decisão de teclas fora de recompilação. Como consequência honesta: **um asset ausente não pode virar crash** — os handlers checam antes de usar, e a ausência degrada para "não anda", nunca para queda.

## DP-trav-07: O que é automatizável, e o que não é

| Verificação | Automatizável? | Como |
|---|---|---|
| Direção de movimento a partir de input + yaw da câmera | ✅ Sim | Teste headless puro sobre `FWorldTraversalMotion`, com números exatos |
| Diagonal normalizada, input nulo sem deriva, pitch descartado | ✅ Sim | Mesmo teste puro |
| Rig de câmera montado e configurado (comprimento, colisão, pitch) | ✅ Sim | Teste headless sobre o personagem spawnado — lê propriedades, não comportamento |
| Personagem carrega o componente de encontro e a transição é a mesma | ✅ Sim | Teste headless, reusando as classes da feature anterior |
| Colisão real com cenário, gravidade, câmera não entrar na parede | ❌ Não | É a engine. Roteiro manual |
| "A câmera é confortável", "andar é gostoso" | ❌ Não | Julgamento humano. Roteiro manual, no mesmo espírito de PRES-06/07 |

---

## O que muda

- **`FWorldTraversalMotion`** (novo) — função pura de direção
- **`AWorldExplorerCharacter`** (novo) — `ACharacter` + spring arm + câmera + `UEncounterDetectionComponent`
- **Nível `WorldStreamingTest`** — ganha um `AWorldExplorerCharacter` posicionado, e chão para ele pisar

## O que NÃO muda

- **`BattleSim`:** nenhuma linha.
- **`UDebugRouteMoverComponent` e o `DebugRoutePawn`:** nenhuma linha, nenhum teste alterado (P2). Os dois roteiros manuais anteriores continuam válidos.
- **`UEncounterDetectionComponent`, `UWorldBattleTransitionService`, `UWorldEncounterFlow`:** consumidos como estão.
- **As três sondas:** continuam válidas e limpas. `World/` continua sem calcular atributo — traversal é geometria, e geometria não é recálculo de dano (a atenção de L-022 segue valendo).

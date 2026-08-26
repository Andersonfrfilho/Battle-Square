# Traversal e Câmera de Mundo — Tarefas

**Design:** `.specs/features/traversal-camera-mundo/design.md`
**Status:** Draft — aguarda aprovação
**Escopo:** `BattleSquare` (C++, T1–T3) e Editor/`Content/` (T4). `BattleSim` não é tocado.

---

## Plano de Execução

### Fase 1 — Movimento, C++ puro
> 🤖 Modelo: `sonnet`

```
T1
```

### Fase 2 — Personagem e câmera (depende da Fase 1)
> 🤖 Modelo: `sonnet` — **T2 é 🧠** (rig de câmera + rotação: a combinação errada dá "andar de lado" de shooter)

```
T2 → T3
```

### Fase 3 — Nível e verificação
> 🤖 Modelo: `sonnet` para T4/T5 · `haiku` para T6

```
T4 → T5 → T6
```

---

## T1 — `FWorldTraversalMotion`

**Arquivos:** `Source/BattleSquare/Public/World/WorldTraversalMotion.h`, `.../Private/World/WorldTraversalMotion.cpp`, teste em `.../Private/Tests/WorldTraversalMotionTest.cpp`.
**O que fazer:** `ComputeMoveDirection` puro — input 2D + `FRotator` da câmera → direção no mundo, normalizada, usando **só o yaw** (DP-trav-02).
**Testes:** frente com yaw 0 → +X; frente com yaw 90 → +Y; lado com yaw 0 → +Y; diagonal é normalizada (comprimento 1, não √2); input nulo → vetor nulo; pitch e roll da câmera não mudam o resultado.
**Pronto quando:** `Automation RunTests BattleSquare.World.WorldTraversalMotion` passa, sem `UWorld` em nenhum teste.

---

## T2 — `AWorldExplorerCharacter` 🧠

**Arquivos:** `Source/BattleSquare/Public/World/WorldExplorerCharacter.h`, `.../Private/World/WorldExplorerCharacter.cpp`.
**O que fazer:** `ACharacter` com `USpringArmComponent` + `UCameraComponent` (DP-trav-03), `bOrientRotationToMovement` e `bUseControllerRotationYaw` conforme DP-trav-04, um `UEncounterDetectionComponent` como subobjeto (DP-trav-05), e constantes nomeadas para comprimento do braço e limites de pitch.
**Pronto quando:** compila e um teste headless confirma o rig: braço existe com o comprimento da constante, `bDoCollisionTest` ligado, câmera não usando rotação de controlador, orientação pelo movimento ligada, yaw de controlador desligado, e o componente de encontro presente.

---

## T3 — Input via Enhanced Input

**O que fazer:** ponteiros `EditDefaultsOnly` para `UInputMappingContext` e dois `UInputAction` (mover, olhar), registro do contexto no `BeginPlay` e binds em `SetupPlayerInputComponent`, com os handlers usando `FWorldTraversalMotion` (T1). Asset ausente **nunca** vira crash (DP-trav-06).
**Pronto quando:** compila; um teste headless confirma que montar o personagem **sem** nenhum asset de input atribuído não crasha nem no `BeginPlay` nem no setup de input.

---

## T4 — Personagem no nível `WorldStreamingTest`

**Como:** via `unreal-mcp`. **Aplicar L-024** (salvar a cada passo) e **L-025** (Editor fechado antes de qualquer build).
**O que fazer:** posicionar um `AWorldExplorerCharacter` no nível e garantir chão sob ele; o `DebugRoutePawn` continua onde está, intocado.
**Pronto quando:** o personagem existe no nível, salvo, e o `DebugRoutePawn` continua presente e sem alteração.

---

## T5 — Roteiro de verificação manual

**Arquivo:** `docs/verification/traversal-camera-mundo.md`.
**O que fazer:** documentar os itens não-automatizáveis de DP-trav-07 — colisão e gravidade reais, câmera não entrando na parede, streaming acompanhando o personagem, e o julgamento humano de conforto.
**Pronto quando:** o roteiro existe e é executável sem o contexto desta sessão.

---

## T6 — Regressão completa

**Pronto quando:**
- [ ] `Automation RunTests BattleSquare` — Success == total (102 + os novos), Fail == 0
- [ ] `Automation RunTests BattleSim` — 52 Success, Fail == 0, zero linha tocada
- [ ] As três sondas — todas `exit 0`
- [ ] **L-020 + L-025 aplicadas:** Editor fechado, `Binaries/` limpo, rebuild real depois de `probe_isolation.sh`

# Handoff — `adocao-de-arte` em DUAS sessões

> Aberto em 04/09/2026. Lido por quem entra: **leia a seção da sua trilha e só
> ela**. As duas trilhas foram desenhadas para não se cruzarem.

## A restrição que decide tudo

**O MCP exige a Unreal ABERTA. O `build_editor.sh` exige a Unreal FECHADA.**
As duas não convivem nesta máquina. Por isso a divisão é por RECURSO, não por
quantidade de caixa — e **só uma trilha pode tocar a Unreal**.

| | TRILHA A — Unreal | TRILHA B — Backend |
|---|---|---|
| Editor | **ABERTO** o tempo todo | **nunca toca** |
| Ferramenta | MCP (`ProgrammaticToolset`) | `bun test` |
| Build C++ | ❌ não roda (editor aberto) | ❌ não precisa |
| Caixas | AR3, AR5, AR6, AR7, AR8 | AR4 |

## TRILHA A — a Unreal (a sessão que já está com o MCP conectado)

- **AR3** — cenário como StaticMesh (~730): Medieval Builder (226), Fantasy
  Props (94), Pirate (143), Kenney (391), + os 11 props que a AR2 separou
  (`Boat`, `Dock_*`, `Lure_*`, `FishingRod_*`).
- **AR5** — apontar os sete papéis em `[/Script/BattleSquare.Art]` (é edição de
  `.ini`, não pede build).
- **AR6/AR7** — Blueprint por pet + Animation Blueprint (`BlueprintTools`).
- **AR8** — a vila veste o Medieval Builder.

**Regras da trilha A:**
- Import em LOTE pelo `ProgrammaticToolset`. Um a um estoura o orçamento.
- **Antes de cada lote, conferir colisão de nome** contra o que já existe
  (invariante 21). Já custou três quase-sobrescritas, uma delas na cadeia de
  evolução do Dragon.
- Ignorar pastas `fbx(unity)` — duplicatas.
- Separar `Characters/` (esqueletal) de `Assets/` (props, estáticos).

## TRILHA B — o backend (a outra sessão)

- **AR4** — o gerador casa os modelos importados com os 115 pets do catálogo.

**Como a trilha B trabalha SEM a Unreal:** a trilha A exporta a lista de assets
para `.specs/handoffs/assets-importados.json`. A trilha B lê o ARQUIVO, nunca o
MCP. Se o arquivo estiver velho, ela pede um novo — não abre o editor.

**Arquivos da trilha B (ninguém mais toca):**
- `apps/api-battle-pets/src/pet/species/**`
- `apps/api-battle-pets/src/pet/seed/**`

**Regras da trilha B:**
- `bun test` é o único gate. **Não rodar `build_editor.sh`** — derrubaria a
  trilha A.
- O gerador **nunca chuta elemento**: o que não classificar vai para a lista de
  decisão do usuário.
- Não escrever tabela de BIOMA no gerador — `BiomeEncounterFilter` (C++) já é a
  fonte, e bioma→pet já sai de graça pelo elemento (L-032).

## Git — como não colidir

As duas trilhas mexem em pastas DISJUNTAS, então `main` serve para as duas:

1. **Commitar cedo e frequentemente**, com escopo estreito.
2. **`git pull --rebase` antes de cada push.** Como os arquivos não se cruzam,
   o rebase é limpo.
3. **Ninguém edita `.specs/features/adocao-de-arte/GOAL.md` fora da sua caixa** —
   marcar só a própria. É o único arquivo compartilhado, e é uma linha por vez.

## O que fica para o FIM, com o editor FECHADO

- **AR9** (`bs.MalhaDeOnde` por papel) muda C++ e **precisa de build**. Ela é a
  última, e roda quando a trilha A terminar e a Unreal for fechada.
- Bateria C++ completa e as sete auditorias: mesma janela.

## Decisões do usuário que ainda travam parte da AR4

Os modelos sem elemento — e os dois que mais importam, porque **têm evolução**:
**Alpaking** (alpaca-rei: Terra ou Planta?) e **Armabee** (abelha blindada: Ar ou
Planta?). Sem resposta, eles ficam fora do catálogo; nunca com elemento chutado.

---

## Achado da AR7 (04/09, medido) — o AnimBP NÃO sai pelo MCP

**Duas descobertas estruturais, ambas por medição:**

### 1. Quatro famílias de rig, e 128 esqueletos onde deviam ser 4

Medido com `get_bone_names` em amostras:

| família | ossos | estrutura |
|---|---|---|
| **Flying** (Dragon, Alpaking, +15) | 18 | **idênticas** |
| **Blob** (Cat, PinkBlob, +15) | 7 | **idênticas** |
| **Big** (Dino 59, Bunny 66, +14) | variável | núcleo igual, extras diferem (orelhas) |
| **KayKit** (Knight, +9) | 24 | outro rig (`Rig_Medium`, minúsculas) |

**O defeito é do import da AR1/AR2:** todas as malhas entraram com
`skeleton: None`, então cada uma criou o SEU esqueleto — 128 no total. Um
Animation Blueprint é amarrado a UM esqueleto, logo isso exigiria 128 AnimBPs, e
mata o "um AnimBP serve várias criaturas".

**A correção existe e foi PROVADA:** reimportar passando
`skeleton: {refPath: <esqueleto da família>}` liga a malha ao esqueleto
compartilhado (testado: `SK_TESTE_Alpaking` → `SK_Dragon_Skeleton`, 18 ossos, ok).
Flying e Blob são consolidáveis com segurança (ossos idênticos); **Big NÃO é** —
ligar o Bunny ao esqueleto do Dino perderia as 8 orelhas.

### 2. `BlueprintTools.create` não cria AnimBlueprint — e TRAVA o editor

`create` com `asset_type=/Script/Engine.AnimBlueprint` falha: a fábrica de
AnimBlueprint exige `TargetSkeleton`, que o `create` genérico não passa. Na
segunda tentativa (com a pasta já criada) ele abriu um **diálogo modal** pedindo
o esqueleto — e **diálogo modal trava a thread de jogo, que é onde as chamadas
MCP rodam**. O editor ficou sem responder até alguém fechar a janela à mão.

⚠️ **Regra nova para esta trilha: nunca chamar `BlueprintTools.create` com um
tipo cuja fábrica peça parâmetro** (AnimBlueprint é o caso conhecido). O custo
não é o erro — é o editor travado, que derruba a trilha inteira.

**Consequência para a AR7:** o Animation Blueprint precisa ser criado **à mão no
editor** (Content Browser → Animation → Animation Blueprint → escolher o
esqueleto), ou por Python do editor (que o sandbox do MCP não alcança). Depois de
existir, o MCP CONSEGUE editá-lo (`create_node`, `connect_pins`,
`write_graph_dsl`, `compile_blueprint`).

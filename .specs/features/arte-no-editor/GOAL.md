# OBJETIVO — Arte no editor (TRILHA A: a Unreal)

> **Uma de duas trilhas paralelas.** A outra é `arte-no-catalogo` (TRILHA B).
> Elas foram desenhadas para NÃO se cruzarem — leia
> `.specs/handoffs/2026-09-04-arte-duas-sessoes.md` se precisar do porquê.

## A regra que separa as trilhas

**O MCP exige a Unreal ABERTA; o `build_editor.sh` exige ela FECHADA.** Não
convivem. Esta trilha fica com a Unreal e **nunca compila C++**; a trilha B
nunca abre o editor. Só assim as duas rodam ao mesmo tempo.

> 🤖 **Modelo: `sonnet`** — o trabalho é volume mecânico (montar lote, conferir
> colisão, chamar o MCP, repetir), com aceite verificável a cada passo.
> **Exceção: a AR7 é 🧠** — máquina de estados e o retargeting (um AnimBP para
> várias criaturas) é decisão estrutural. Subir para `fable`/`opus` nela, e
> voltar depois.

## O objetivo, numa frase

O CENÁRIO e os PETS ganham corpo dentro do editor — import, Blueprint e
Animation Blueprint —, tudo por MCP, sem clique manual.

## O que já está feito

- **AR1/AR2 fechadas:** 128 SkeletalMesh importadas (75 monstros, 43 peixes, 10
  personagens KayKit) com 1.108 AnimSequence. Zero falhas.
- A lista está em `.specs/handoffs/assets-importados.json` — é o insumo da
  trilha B.

## PRONTO é isto, e nada menos

- [⛔] **AR3** — o CENÁRIO como StaticMesh (~730): KayKit Medieval Builder (226),
      Fantasy Props (94), Pirate Kit (143), Kenney fantasy-town (167),
      mini-dungeon (30), city-kit (41), Nature (29), + os 11 props que a AR2
      separou (`Boat`, `Dock_*`, `Lure_*`, `FishingRod_*`)
- [⛔] **AR5** — os sete papéis apontados em `[/Script/BattleSquare.Art]`, e
      nenhum caindo na primitiva (é edição de `.ini`, não pede build)
- [⛔] **AR6** — Blueprint por pet: `BlueprintTools.create` + `set_parent`,
      `SkeletalMeshComponent` com o modelo, e **compilado**
- [⛔] **AR7** 🧠 (`fable`/`opus`) — o Animation Blueprint: Idle e Walk ligados. As animações já vêm
      nomeadas por ação (Idle, Walk, Punch, Headbutt, HitReact, Death)
- [⛔] **AR8** — a VILA veste o Medieval Builder: os 12 prédios de
      `EVillageBuilding` deixam de ser caixa colorida
- [⛔] Nenhum import com colisão de nome silenciosa
- [⛔] Um commit por caixa, com o motivo

## As regras desta trilha (todas pagas por medição)

1. **Import em LOTE**, pelo `ProgrammaticToolset` (`execute_tool_script`). Um a
   um estoura o orçamento de contexto.
2. **Conferir COLISÃO de nome antes de cada lote** (invariante 21). Já foram
   três quase-sobrescritas, e uma delas — `SK_Dragon` do Animated Monster —
   teria quebrado a cadeia de evolução `Dragon`/`Dragon_Evolved`.
3. **Ignorar pastas `fbx(unity)`** — são duplicatas do mesmo modelo.
4. **`Characters/` é esqueletal; `Assets/` é prop estático.** Misturar cria lixo.
5. **Prop não é bicho.** `Boat`, `Dock_*`, `Lure_*`, `FishingRod_*` vão como
   StaticMesh. A lista viva desses tokens mora no gerador da trilha B.
6. **Nome de asset não tem espaço** (`Manta ray` → `SK_MantaRay`).
7. **NÃO rodar `build_editor.sh`** — derrubaria a si mesma e à trilha B.

## O que fica FORA desta trilha

- **AR4** (casar modelo com o catálogo de pets) é da trilha B.
- **AR9** (`bs.MalhaDeOnde` por papel) muda C++ e precisa de build: fica para o
  fim, com o editor FECHADO, depois que as duas trilhas terminarem.
- A bateria C++ e as sete auditorias: mesma janela da AR9.

## Como continuar

1. `list_toolsets` tem de responder — se der `ConnectionRefused`, a Unreal está
   fechada; abra-a e reconecte o MCP.
2. Continuar da primeira caixa aberta. Não recomeçar, não replanejar.
3. `git pull --rebase` antes de cada push (a trilha B também commita em `main`;
   as pastas são disjuntas, então o rebase é limpo).

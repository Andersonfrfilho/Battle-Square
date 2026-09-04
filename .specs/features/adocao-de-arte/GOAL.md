# OBJETIVO — Adotar a arte (o mundo deixa de ser primitiva)

> **Reescrito em 04/09/2026**, depois que a medição derrubou a premissa do GOAL
> anterior. Ele dizia que o import era físico e do usuário; **é falso**: o MCP
> importa (`SkeletalMeshTools.import_file`, `StaticMeshTools.import_file`), e os
> 50 monstros do Ultimate Monsters já entraram por ele, com esqueleto, física e
> 513 animações. O agente faz o arco inteiro.

## O objetivo, numa frase

Os ~20 atores do jogo param de ser cubo e cilindro e passam a vestir os modelos
CC0 já baixados — **sem editar C++**, porque o slot dirigido por dado
(`a-malha-vem-de-fora`) já existe e a adoção é troca de config.

## O que JÁ está pronto (medido, não suposto)

- **A ponte de dado:** `ScenaryPalette::MeshPathForRole(papel, fallback)` — override
  de config por papel, primitiva de fallback. Verde sem pacote nenhum.
- **Os 16 packs extraídos** em `ArtSource/` (fora de `Content/`, gitignorado):
  **1.121 FBX**, todos CC0.
- **50 monstros JÁ IMPORTADOS** em `/Game/Quaternius/Monsters`: 50 SkeletalMesh,
  50 Skeleton, 50 PhysicsAsset, **513 AnimSequence**. Zero falhas.
- **O gerador de espécies** (`model-mapping.pure.ts`, 16 testes): modelo →
  elemento → família → cadeia de evolução, corrigido pelos nomes reais.
- **6 cadeias de evolução** já identificadas: Dragon, Alpaking, Armabee, Glub,
  Goleling, Mushnub (o pack traz os pares `X`/`X_Evolved`).

## PRONTO é isto, e nada menos

- [x] **AR1** — os packs baixados e extraídos, e os 50 monstros importados
- [⛔] **AR2** — o RESTO das criaturas importado como SkeletalMesh, com animação:
      Cute Animated Monsters (21), Cute Fish (52), Animated Fish (7), Animated
      Monster (4), KayKit Skeletons (32), KayKit Adventurers (70). Nomes
      desambiguados quando repetem entre pastas — importar por cima em silêncio
      é o defeito que a medição do Ultimate Monsters já pegou
- [⛔] **AR3** — o CENÁRIO importado como StaticMesh: KayKit Medieval Builder
      (226 — os 12 prédios da vila), Fantasy Props (94), Pirate Kit (71+72),
      Kenney fantasy-town (167), mini-dungeon (30), city-kit (41), Nature (29)
- [⛔] **AR4** — o gerador roda sobre os assets IMPORTADOS (via
      `AssetTools.find_assets`, não sobre nomes de arquivo) e casa modelo com os
      115 pets do catálogo. O relatório curto do que ele não soube classificar
      vai ao usuário — **ele nunca chuta elemento**
- [⛔] **AR5** — os sete papéis de cenário apontados em
      `[/Script/BattleSquare.Art]`, e a tela confere que NENHUM caiu na primitiva
- [⛔] **AR6** — os PETS ganham corpo: Blueprint por pet (`BlueprintTools.create`
      + `set_parent`), `SkeletalMeshComponent` com o modelo, e **compilado**
- [⛔] **AR7** — o Animation Blueprint: Idle e Walk ligados. As animações já vêm
      nomeadas por ação (Idle, Punch, Headbutt, HitReact, Death), e elas casam
      com o combate que já existe
- [⛔] **AR8** — a VILA veste o Medieval Builder: os 12 prédios
      (`EVillageBuilding`) deixam de ser caixa colorida
- [⛔] **AR9** — na tela: `bs.MalhaDeOnde` diz, por papel, se veio do PACOTE ou
      da primitiva — "adotei tudo?" vira pergunta com resposta
- [⛔] Bateria completa verde (o número só sobe a partir de **991**)
- [⛔] As sete auditorias limpas
- [⛔] Um commit por task, cada um com o motivo — não só o quê

Enquanto qualquer caixa estiver aberta, o objetivo **continua**.

## Decisões que são do USUÁRIO (e não travam o resto)

1. **O elemento dos 15 sem pista** — Tribal, Ninja, Alien, Alien_Tall, Dog, Cat,
   Slime, PinkBlob, GreenBlob, GreenSpikyBlob e, os mais importantes,
   **Alpaking** e **Armabee** (têm evolução, não têm elemento). O gerador os
   reporta; enquanto não houver resposta, eles ficam de fora do catálogo — nunca
   com elemento chutado.
2. **AR10, a criatura própria** (decisão 69): o pet definitivo, inspirado em
   animal mas com poder. Os monstros importados são o stand-in que faz o jogo
   ter cara hoje; trocar depois é apontar a config para outro asset.

## Invariantes (as de `corrente` + quatro desta feature)

18. **Uma fonte de verdade para o VISUAL.** Malha entra por `ScenaryPalette`, ao
    lado da cor. Nunca uma segunda tabela cor×malha (L-032).
19. **Componente criado não é componente visível.** Todo ator que trocar de malha
    mantém o teste que verifica a ATRIBUIÇÃO — e com asset externo isso vale
    mais: asset que não carrega tem de cair na primitiva, nunca em nulo.
20. **O jogo fecha verde SEM o pacote.** A primitiva é fallback permanente. Se
    uma caixa exigir asset novo para a BATERIA passar, ela virou outra feature.
21. **Nome que repete entre pastas é modelo DIFERENTE.** Medido no Ultimate
    Monsters: `Alien` existe em `Big` e em `Blob`. Importar sem desambiguar
    sobrescreve em silêncio — e silêncio é o modo de falhar que este projeto
    mais paga.

## O que este objetivo NÃO faz

- **Não escolhe o estilo por você.** Os packs já foram decididos (decisão 70).
- **Não autora arte nova.** A criatura própria é AR10, decisão sua.
- **Não abre áudio nem VFX.** Zero medido; nenhum GOAL os pediu (o Kenney tem
  áudio CC0, e é o maior buraco do inventário — mas é outra frente).
- **Não mexe no traçado, no gabarito, nem na lógica de jogo.**

## Como continuar (se o contexto for compactado)

1. Ler este GOAL e `docs/assets/ADOCAO-QUATERNIUS.md`.
2. **O editor tem de estar ABERTO** — o MCP só escuta com ele de pé.
   `list_toolsets` deve devolver `BlueprintTools`, `SkeletalMeshTools`.
3. **Import em lote é pelo `ProgrammaticToolset`** (`execute_tool_script`): ele
   agrupa muitas chamadas num script só. Um a um estoura o orçamento.
4. **Antes de qualquer build: FECHAR a Unreal** (ela segura os dylibs), rodar
   `./Tools/build_editor.sh`, e só então reabrir.
5. Continuar da primeira caixa aberta. Não recomeçar, não replanejar.

# OBJETIVO — Adotar a arte (o pacote, não o slot)

> **Aberto em 04/09/2026**, quando a camada de adoção ficou pronta. Sucede
> `a-malha-vem-de-fora` (o SLOT, fechado): aqui entra o PACOTE. As decisões de
> conteúdo (qual estilo) são do usuário — registradas em `docs/assets/PRONTOS.md`.

## O objetivo, numa frase

O jogo troca as primitivas da engine por modelos de um pacote CC0 (Quaternius, o
recomendado), **sem tocar em C++** — preenchendo `[/Script/BattleSquare.Art]` e
importando os assets, com a primitiva sempre de fallback.

## O que mudou em 04/09: o MCP da Unreal CONECTOU

O plugin `ModelContextProtocol` esta vivo (editor escutando em `localhost:8000`),
e o agente enxerga os toolsets. Isso REDIVIDE o trabalho desta feature:

| toolset | serve a |
|---|---|
| `BlueprintTools` (53 ferramentas: `create`, `set_parent`, `create_node`, `connect_pins`, `write_graph_dsl`, `compile_blueprint`) | AR5 — o Blueprint do pet e o Animation Blueprint |
| `ActorTools`, `PrimitiveTools` | componentes nos atores |
| `StaticMeshTools`, `SkeletalMeshTools` | AR2/AR3/AR5 — malha estatica e a esqueletal riggada |
| `AssetTools` | achar/inspecionar os assets importados |
| `MaterialTools`, `MaterialInstanceTools` | material por papel |
| `ConfigSettingsToolset` | AR2 — escrever `[/Script/BattleSquare.Art]` |
| `AutomationTestToolset` | rodar a bateria pelo MCP |

**So o AR1 continua sendo fisico** (baixar e importar os `.fbx`): o MCP EDITA
assets que existem, nao baixa modelos da internet. **Do AR2 em diante o agente
faz por MCP** — sem clique manual.

A ARQUITETURA ja estava pronta e testada (`ScenaryPalette::MeshPathForRole`,
`ArtAdoptionTest`); agora o braco para aplica-la tambem esta.

## O que JÁ está pronto (a ponte, fechada em a-malha)

- `ScenaryPalette::MeshPathForRole(papel, fallback)` — override de config por
  papel, primitiva de fallback. Verde sem pacote (invariante 20).
- `[/Script/BattleSquare.Art]` — seção vazia e comentada, chaves prontas.
- `docs/assets/ADOCAO-QUATERNIUS.md` — o passo a passo.
- `ArtAdoptionTest` — sem config → primitiva; com → o asset; vazio → primitiva.

## PRONTO é isto, e nada menos

- [⛔] **AR1** *(USUARIO — fisico)* — os packs Quaternius baixados (CC0) e
      importados para `Content/Quaternius/`. **DUAS METADES, destinos diferentes
      (decisao 69):**
      - **CENARIO = arte FINAL.** *Ultimate Nature Pack* + *Stylized Nature
        MegaKit* (arvore, arbusto, flor, capim, rocha). Arvore e arvore — o
        mundo e natural mesmo, e o pacote resolve de vez.
      - **PETS = apenas STAND-IN.** *Ultimate Animated Animal Pack* e de
        ANIMAIS REAIS; o pet e criatura com poder (69), inspirada em bicho mas
        diferente dele. O pack entra so para provar o encanamento
        (SkeletalMesh + AnimBP) e para nao ficar cilindro na tela — **nunca
        como design final**. Mais perto do alvo, se quiser stand-in com cara de
        monstro: *Animated Alien Pack* e *Animated Dinosaur Pack*.

- [⛔] **AR2** *(AGENTE via ConfigSettings + tela)* — os sete papéis de cenário (`ForestTree`, `CanopyTree`, `Rock`,
      `DeadWood`, `Undergrowth`, `Accent`, `GroundCover`) apontados no
      `[/Script/BattleSquare.Art]`, e a tela confere que NENHUM caiu na primitiva
- [⛔] **AR3** *(AGENTE via MCP + C++)* — os atores que hoje pedem primitiva por PAPEL passam a chamar
      `MeshPathForRole` (ForestBackdrop e cenário), provado pelo teste de
      atribuição de cada um (invariante 19)
- [⛔] **AR4** *(AGENTE — auditoria em C++/script)* — uma auditoria de ADOÇÃO: para cada chave preenchida em
      `[/Script/BattleSquare.Art]`, o asset EXISTE e carrega — chave apontando
      para asset ausente REPROVA (o "adotado mas invisível" pego cedo)
- [⛔] **AR5** *(AGENTE via BlueprintTools + SkeletalMeshTools)* — os PETS (`APetView`) ganham um modelo riggado (STAND-IN, decisao 69) —
      e como eles são RIGGADOS, isto pede um Animation Blueprint (Idle/Walk),
      um passo além da malha estática. Decisão de escopo: estático primeiro,
      animado depois
- [⛔] **AR6** *(AGENTE)* — na tela: `bs.MalhaDeOnde` (já existe) passa a listar, por papel,
      se veio do PACOTE ou da primitiva — "adotei tudo?" vira pergunta com resposta
- [⛔] **AR7** *(USUARIO — decisao de conteudo/arte)* — a CRIATURA PROPRIA: o
      pet definitivo, inspirado em animal mas com poder e peculiaridade (69). O
      stand-in do AR5 sai quando esta entrar. Caminhos: encomendar arte, modelar,
      ou achar um pacote de MONSTROS (nao de fauna). O encanamento ja estara
      pronto — trocar e apontar a config para outro asset.
- [⛔] Bateria completa verde (o número só sobe a partir de **991**)
- [⛔] As sete auditorias limpas (a de adoção, AR4, é a oitava quando existir)
- [⛔] Um commit por task, cada um com o motivo

Enquanto qualquer caixa estiver aberta, o objetivo **continua**.

## Invariantes (as de `corrente` + três desta feature)

18. **Uma fonte de verdade para o VISUAL.** Malha entra por `ScenaryPalette`,
    ao lado da cor. Nunca uma segunda tabela cor×malha (L-032).
19. **Componente criado não é componente visível.** Todo ator que trocar de
    malha mantém o teste que verifica a ATRIBUIÇÃO — e agora, com asset externo,
    o teste vale mais: asset que não carrega tem de cair na primitiva, nunca em
    nulo.
20. **O jogo fecha verde SEM o pacote.** A primitiva é o fallback permanente. Se
    uma caixa exigir asset novo para a BATERIA passar, ela virou outra feature —
    o pacote é do runtime do usuário, não do teste.

## O que este objetivo NÃO faz

- **Não escolhe o estilo por você.** Quaternius é a recomendação do `PRONTOS.md`;
  trocar por Synty/Fab é decisão sua e muda só os caminhos.
- **Não baixa nem importa** — isso é você, na sua Unreal.
- **Não abre áudio nem VFX.** Zero medido, nenhum GOAL os pediu.
- **Não mexe no traçado, no gabarito, nem na lógica de jogo.** Só de onde vem a
  malha.

## Como começar (quando reabrir)

1. Ler este GOAL, `docs/assets/ADOCAO-QUATERNIUS.md`, `docs/assets/PRONTOS.md`.
2. **Confirmar o MCP vivo**: o editor tem de estar ABERTO (o plugin so escuta com
   ele de pe). `list_toolsets` deve devolver `BlueprintTools` etc. Se der
   `ConnectionRefused`: abrir a Unreal e reconectar o MCP (`/mcp` em sessao
   interativa).
3. AR1 e do usuario (baixar/importar os `.fbx`). Do AR2 em diante, o agente
   executa por MCP. Continuar da primeira caixa aberta, sem replanejar.
4. **Antes de qualquer build**: FECHAR a Unreal (ela segura os dylibs). Depois
   `./Tools/build_editor.sh` e so entao reabrir — abrir primeiro faz a Unreal
   tentar compilar sozinha, que e o "could not be compiled" de 04/09.

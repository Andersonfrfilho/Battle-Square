# Adotar um pacote de arte — o passo a passo

> O slot dirigido por dado (a-malha-vem-de-fora) esta PRONTO. Adotar um pacote e
> agora **trocar dado**, nao editar C++ ator por ator. Este guia e o caminho.

## A escolha: Quaternius (CC0)

Recomendado no `PRONTOS.md`: personagens, **animais e monstros riggados e
animados** (Idle/Walk/Run/Jump/Death), 150+ modelos, FBX/OBJ/Blend, **CC0** (uso
comercial, sem credito). E o que mais casa com o projeto — `APetView` e o slot
obvio para os bichos.

Fonte: https://quaternius.com (ou o mirror itch.io do autor). Filtrar **CC0**.

## Os passos (o que so voce pode fazer — precisa dos binarios)

1. **Baixar** os packs Quaternius (Animais, Natureza, Rochas).
2. **Importar** para a Unreal, em `Content/Quaternius/` (arraste os FBX; a
   engine gera os `.uasset`). Manter a arvore de pastas: `Trees/`, `Rocks/`,
   `Nature/`, `Animals/`.
3. **Apontar a config**: em `Config/DefaultGame.ini`, secao
   `[/Script/BattleSquare.Art]`, descomentar e ajustar as chaves `Mesh_<Papel>`
   para os caminhos reais dos assets importados (o nome exato do asset entra
   duas vezes: `/Game/.../Tree_01.Tree_01`).
4. **Rodar o jogo** e conferir na tela — se um papel ficou com a primitiva, o
   caminho da config esta errado (o fallback te avisa por nao trocar).

## Por que e seguro

- **Sem config, roda em primitiva** — o jogo nunca depende do pacote para subir
  (invariante 20). Voce adota no seu ritmo, papel por papel.
- **Papel sem asset NUNCA fica invisivel** — `ScenaryPalette::MeshPathForRole`
  cai na primitiva quando a chave falta ou aponta para asset que nao carrega. O
  defeito tres vezes pago (ator sem malha) nao volta pela porta da adocao.
- **Uma fonte de verdade** — malha entra por `ScenaryPalette`, ao lado da cor
  (invariante 18). Nao ha segunda tabela cor×malha.

## O mapa papel -> modelo (sugestao inicial)

| Papel (`EScenaryRole`) | Modelo Quaternius sugerido | Onde aparece |
|---|---|---|
| `ForestTree` | Tree_01 | a mata da arena e do mundo |
| `CanopyTree` | Tree_Tall_01 | o dossel do fundo |
| `Rock` | Rock_01 | pedras da borda, obstaculo |
| `DeadWood` | Log_01 | tronco caido, obstaculo |
| `Undergrowth` | Bush_01 | arbusto |
| `Accent` | Flower_01 | flor, cogumelo |
| `GroundCover` | Grass_01 | capim rasteiro |

Os **pets** (`APetView`) sao o proximo alvo, com os Animals riggados — esses
trazem animacao, e por isso pedem um passo a mais (Animation Blueprint), fora do
escopo do slot de malha estatica.

## Estado atual

A camada esta pronta e testada (`ArtAdoptionTest`): sem config -> primitiva; com
config -> o asset do pacote; override vazio -> primitiva (nunca invisivel). O que
falta e o import binario dos modelos, que so pode ser feito na sua Unreal com os
arquivos em maos.

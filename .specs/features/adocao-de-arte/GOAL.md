# OBJETIVO — Adotar a arte (o pacote, não o slot)

> **Aberto em 04/09/2026**, quando a camada de adoção ficou pronta. Sucede
> `a-malha-vem-de-fora` (o SLOT, fechado): aqui entra o PACOTE. As decisões de
> conteúdo (qual estilo) são do usuário — registradas em `docs/assets/PRONTOS.md`.

## O objetivo, numa frase

O jogo troca as primitivas da engine por modelos de um pacote CC0 (Quaternius, o
recomendado), **sem tocar em C++** — preenchendo `[/Script/BattleSquare.Art]` e
importando os assets, com a primitiva sempre de fallback.

## Por que é uma sessão à parte, e presencial

Esta feature depende de arquivos BINÁRIOS que não existem no repo e não se baixam
por código: `.fbx`/`.uasset` do Quaternius. O import roda na Unreal do usuário,
com os arquivos em mãos. A ARQUITETURA já está pronta e testada
(`ScenaryPalette::MeshPathForRole`, `ArtAdoptionTest`); o que falta é físico.

## O que JÁ está pronto (a ponte, fechada em a-malha)

- `ScenaryPalette::MeshPathForRole(papel, fallback)` — override de config por
  papel, primitiva de fallback. Verde sem pacote (invariante 20).
- `[/Script/BattleSquare.Art]` — seção vazia e comentada, chaves prontas.
- `docs/assets/ADOCAO-QUATERNIUS.md` — o passo a passo.
- `ArtAdoptionTest` — sem config → primitiva; com → o asset; vazio → primitiva.

## PRONTO é isto, e nada menos

- [⛔] **AR1** — os packs Quaternius (Animais, Natureza, Rochas) baixados (CC0) e
      importados para `Content/Quaternius/`, mantendo a árvore de pastas
- [⛔] **AR2** — os sete papéis de cenário (`ForestTree`, `CanopyTree`, `Rock`,
      `DeadWood`, `Undergrowth`, `Accent`, `GroundCover`) apontados no
      `[/Script/BattleSquare.Art]`, e a tela confere que NENHUM caiu na primitiva
- [⛔] **AR3** — os atores que hoje pedem primitiva por PAPEL passam a chamar
      `MeshPathForRole` (ForestBackdrop e cenário), provado pelo teste de
      atribuição de cada um (invariante 19)
- [⛔] **AR4** — uma auditoria de ADOÇÃO: para cada chave preenchida em
      `[/Script/BattleSquare.Art]`, o asset EXISTE e carrega — chave apontando
      para asset ausente REPROVA (o "adotado mas invisível" pego cedo)
- [⛔] **AR5** — os PETS (`APetView`) ganham os modelos de Animais Quaternius —
      e como eles são RIGGADOS, isto pede um Animation Blueprint (Idle/Walk),
      um passo além da malha estática. Decisão de escopo: estático primeiro,
      animado depois
- [⛔] **AR6** — na tela: `bs.MalhaDeOnde` (já existe) passa a listar, por papel,
      se veio do PACOTE ou da primitiva — "adotei tudo?" vira pergunta com resposta
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
2. AR1 é físico (baixar/importar) — feito na Unreal, fora do agente.
3. Do AR2 em diante o agente ajuda: config, wiring por papel, a auditoria de
   adoção (AR4), a tela (AR6). Continuar da primeira caixa aberta, sem replanejar.

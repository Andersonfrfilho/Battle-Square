# Roteiro de Verificação Visual — Apresentação do Combate

**Feature:** `.specs/features/apresentacao-combate/`
**Status:** **não verificado ainda** — nenhum item deste roteiro foi rodado no editor.

Este documento cobre o que a spec marca como P1 e que **não tem teste
automatizado** (ver `tasks.md`, "Cobertura de Requisitos"). O código por
trás de cada item (frustum, uso de token, wiring) já está coberto por
`Automation RunTests BattleSquare` — o que falta aqui é a leitura
estética humana, ou uma propriedade de layout UMG que só existe depois
da autoria visual (DP-08), que este lote de tarefas não cobriu.

Cada item abaixo tem um passo concreto de verificação — nunca "olhar e
ver se está bom". Marcar `[x]` só depois de executar o passo, com a
data e quem verificou.

---

## PRES-04 — Alvo de toque ≥44pt em mobile

- [ ] **Não verificado**

**Passo:** abrir o Blueprint de `UBattleActionSelectorWidget` (após a
autoria visual de DP-08) no UMG Designer, trocar a preview para um
device profile mobile (ex.: iPhone), selecionar cada um dos 6 botões de
tipo de ação e cada um dos botões da roseta de direção, e ler
`Size` no painel de detalhes do `Button`/`SButton`. Confirmar que
largura E altura ≥ 44 (unidade do Designer, que corresponde a pt em
mobile pela configuração de DPI do projeto). Repetir para o botão de
commit/confirmar.

---

## PRES-06/07 — Câmera "bonita" e grade "legível" (estética)

- [ ] **Não verificado**

**Passo:** o código já garante que as 9 casas caem dentro do frustum
(`BattleSquare.BattleArena.SpawnsAndFramesGridInCameraFrustum`, T9).
O que falta é o julgamento humano: colocar um `ABattleArena` num nível
de teste, dar Play in Editor, e confirmar visualmente que:
1. As 9 casas ocupam uma fração confortável do enquadramento (nem
   apertadas na borda, nem minúsculas no centro).
2. As linhas/bordas da grade são distinguíveis a olho no zoom padrão
   de PIE, sem precisar aproximar a câmera do editor.
3. Não há clipping de nenhuma casa contra o near plane da câmera.

---

## PRES-08 — Material fosco de verdade

- [ ] **Não verificado**

**Passo:** o código só garante que `GridMaterial` é configurável (não
há cor hex hardcoded — ver `ABattleArena::GridMaterial`,
`TSoftObjectPtr<UMaterialInterface>`). A textura/material fosco em si
(baixo *Roughness* especular, sem reflexo brilhante, alinhado à direção
de arte Link's Awakening — ver AD-003) precisa ser criado no Content
Browser e atribuído à instância de `ABattleArena` usada em jogo. Abrir
o Material Editor do asset atribuído e confirmar `Roughness` alto
(perto de 1.0) e ausência de reflexão especular visível no preview
esférico do editor de material.

---

## Layout de DP-08 — posição dos 6 botões de tipo + roseta de direção

- [ ] **Não verificado**

**Passo:** no Blueprint UMG de `UBattleActionSelectorWidget`, confirmar
que:
1. Os 6 botões de tipo (`Aguardar`, `Mover`, `Atacar`, `Magia`,
   `Defender`, `Esquivar`) estão todos visíveis sem scroll na
   resolução mínima suportada (definida no `manifest.json` do PWA/
   config mobile do projeto).
2. A roseta/D-pad de 8 direções só aparece quando `CurrentStep ==
   EBattleActionSelectionStep::ChoosingDirection` (bind de
   `Visibility` no Designer, testável clicando `Mover` em PIE e
   observando a roseta aparecer).
3. Um botão de cancelar/voltar está visível durante `ChoosingDirection`
   e chama `CancelPendingSelection` (já exposto por T11).

---

## Tilt-shift aplicado só na arena

- [ ] **Não verificado**

**Passo:** confirmar que o volume ou componente de post-process de
tilt-shift (Depth of Field com foco estreito, ver AD-003/DP-09) está
anexado especificamente à câmera de `ABattleArena` (`ArenaCamera`,
via `PostProcessSettings` no `UCameraComponent` ou um
`APostProcessVolume` com `bUnbound = false` limitado à cena de
batalha) — nunca um post-process global do nível que vazaria o efeito
para o mundo aberto (contradição com AD-003: mundo aberto quer câmera
livre e horizonte visível, sem miniatura). Verificar entrando em PIE
na arena e depois, no mesmo processo, num nível de mundo aberto de
teste (quando existir) — confirmar que o segundo NÃO tem o efeito.

---

## Registro de execução

| Data | Quem | Itens verificados | Resultado |
|---|---|---|---|
| — | — | nenhum ainda | — |

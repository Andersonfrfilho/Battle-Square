# Tarefas — Legibilidade do Combate

> 🤖 Modelo: `sonnet`

## T1 — Testes da narração (antes do código) ✅
Caminho feliz e de falha para cada `EBattleEventType` que o jogador percebe.
**Pronto quando:** os testes existem e falham por ausência de `Describe`.

## T2 — `FBattleNarration` + `FBattleNarrationFeed` ✅
Função pura (DP-leg-01) e anel de linhas de produto, sempre compilado.

## T3 — Barra de vida em `APetView` ✅
Dois cubos da engine (fundo + preenchimento), sem asset autorado, escalados por
`HealthRatio` (DP-leg-03). Some junto com o corpo do derrotado.

## T4 — Feed desenhado no HUD ✅
Rodapé centralizado, fora do gate de depuração. Anúncio do vencedor.

## T5 — Regressão ✅
Bateria completa, três sondas, `BattleSim` sem uma linha tocada.

## T6 — Roteiro de verificação
O que só uma pessoa jogando decide: a frase **explica** a jogada? A barra é
legível a essa distância de câmera?

# Roteiro manual — Fluidos em jogo

**Escrito em 02/09/2026, ao fechar G1–G3.**

## O que já está provado sem você

Não reconte nada disso — a bateria cobra tudo, e são 800 provas:

- o registro separa oito fluidos, e eles **diferem** de verdade (densidade,
  condução, dano, se dá para submergir);
- a casa sabe de que fluido é, e **submergir na lava não vale**;
- a lava **queima** por slot, e voar escapa, e camuflar não;
- a arena **põe fluido nas casas** a partir do mundo, pelo mesmo desempate que
  decide a propriedade;
- o tradutor **liga a condução** para Raio e para mais ninguém;
- o Fogo **resiste** à lava a 50%, e os outros elementos não.

O que sobra é o que teste nenhum vê.

## Antes de abrir

```bash
./Tools/bake_island.sh && ./Tools/build_editor.sh && ./Tools/sync_module_manifest.sh
```

Feche o Editor antes de compilar e reabra com `open -a` — editor lançado em
segundo plano não recebe teclado no PIE.

## O que só o olho pega

| # | Onde | A pergunta |
|---|---|---|
| 1 | qualquer batalha na beira d'água | A etiqueta da casa diz a substância, e ela **só aparece** quando diverge do padrão? (Água doce não deve escrever nada.) |
| 2 | batalha perto do vulcão | A casa sai marcada como **água termal**, e não como água comum? |
| 3 | batalha longe do vulcão | A mesma beira de rio sai **sem etiqueta** — é a prova de que a #2 não está carimbando tudo |
| 4 | uma casa de lava, se aparecer | Ela fica **VERMELHA** antes de alguém pisar? É o único aviso que existe hoje |
| 5 | pisando na lava | A vida cai no fim do slot, e o painel narra o dano como qualquer outro? |
| 6 | um pet de Fogo na lava | Ele perde **metade** do que o outro perdeu na mesma casa? |
| 7 | voando sobre a lava | Não perde nada? |
| 8 | camuflado na lava | **Perde igual** — camuflar não tira ninguém do chão |
| 9 | pet de Raio atacando alguém na água | Um **terceiro** pet na mesma poça leva dano sem ter sido atacado? |
| 10 | o mesmo, com o terceiro numa poça separada por chão seco | Ele **não** leva? |
| 11 | dois pets de Raio na mesma poça | O mais fraco leva só a diferença, e o lançador não leva nada? |
| 12 | pet de Fogo atacando na água | **Nada** conduz — a corrente é do Raio |

## A limitação declarada

**A casa de lava veste o material da ÁGUA.** Material novo exige asset
autorado, e capacidade que espera edição de asset não existe hoje. O nome na
etiqueta e o vermelho são o remendo — se a #4 falhar, a casa mais perigosa do
campo está parecendo a mais inofensiva, e aí é tarefa, não observação.

## Se algo estiver errado

**Escreva o teste antes do conserto.** Nesta sessão a medição apontou uma causa
diferente da impressão **seis vezes** — a regra do vau, o declive das trilhas, a
proximidade da balsa, o resumo por `FName`, o mar que era chão seco, e o
`LoadObject` no coletor que derrubou o processo e escondeu 583 testes.

O painel se grava sozinho em `Saved/BattleDebug.txt`.

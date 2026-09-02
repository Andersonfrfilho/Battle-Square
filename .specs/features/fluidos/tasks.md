# Fluidos — tarefas

**Spec:** `.specs/features/fluidos/spec.md`
**Feito até aqui:** F1 (o registro existe e separa).

---

## F1 — O registro ✅ FEITO

`EFluidKind` + `FFluidTraits` + tabela única em `BattleSim`, com cinco provas.
A que carrega o peso é **`TheFluidsActuallyDiffer`**: uma tabela em que todos
os fluidos têm os mesmos números passa em qualquer teste de leitura e não
distingue nada — que era o estado de onde se partiu.

---

## F2 — A casa sabe de que fluido ela é 🧠
> 🤖 Modelo: `opus` — é o contrato entre o tabuleiro e o registro

Hoje o registro responde *sobre* um fluido, e ninguém diz que a casa X tem o
fluido Y. Sem isto o eixo novo não toca o jogo.

*Decisão em aberto:* o fluido entra em `FBattleState` como campo por casa
(custa memória e entra no hash) ou é derivado do bioma da arena (barato, mas
uma arena não pode ter dois fluidos). **Medir antes de escolher.**
*Aceite:* `Submergir` recusa numa casa de lava marcada como água funda.

## F3 — Poderes que distinguem fluido
> 🤖 Modelo: `sonnet`

Requisito de skill passa a poder pedir substância, não só fundura.
*Aceite:* um poder que exige água funciona nas cinco águas e falha na lava;
um que exige lava falha em todas as águas.

## F4 — O dano de estar dentro
> 🤖 Modelo: `sonnet`

`DamagePerTurn` deixa de ser um número guardado e passa a acontecer.
*Aceite:* um pet que termina o turno na lava perde vida; na água, não.
*Motivo:* enquanto ninguém aplica, a lava é cenário colorido — é L-041.

## F5 — Itens e resistências
> 🤖 Modelo: `sonnet`

O que o usuário chamou de "itens que farão diferença entre eles": proteção
contra um fluido específico.
*Aceite:* com o item, o dano da lava não vem; sem ele, vem.

## F6 — O mundo declara seus fluidos
> 🤖 Modelo: `sonnet`

`WaterFooting` e a paleta passam a consultar o registro em vez de tratarem
toda água como uma. Hoje o mundo tem seis fluidos e trata todos igual.
*Aceite:* a água termal perto do vulcão é reconhecida como termal, não doce.

## F7 — A balsa flutua por DENSIDADE

Fecha o achado #9 da construção do mundo. Hoje a altura da balsa vem da lâmina;
com o registro, ela pode vir do empuxo.
*Aceite:* um corpo mais denso que o fluido não boia nele.

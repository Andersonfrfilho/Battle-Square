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

## F2 — A casa sabe de que fluido ela é ✅ FEITO

**A decisão foi MEDIDA, não escolhida por gosto.** A arena é montada casa a
casa: `FWorldFeatureSample` carrega uma `Location`, e o layout sai de
`CellLayoutIndex(Coluna, Linha)` — casas diferentes recebem propriedades
diferentes do mundo. Derivar o fluido do bioma da arena tornaria impossível
**para sempre** uma batalha na beira do vulcão com água termal de um lado e
lava do outro. Campo por casa, então.

E ele custa quase nada: `CellFluid` é um quarto array paralelo a `CellLayout`,
que já tinha três — o próprio código justifica o padrão ("terreno que passa são
três informações"). Um byte por casa, teto de 225.

**Vazia quer dizer "tudo no padrão".** Uma arena comum não paga um byte por
casa para dizer que água é água; quem tem lava materializa a lista.

O hash soma o fluido **DERIVADO**, nunca o array cru — lista vazia e lista de
zeros querem dizer a mesma coisa, e somando o cru elas dariam assinaturas
diferentes sendo idênticas. Seria dessincronia fantasma, das piores de achar.

`TerrainAllowsSkill` continua sendo o eixo da FUNDURA e não mudou de assinatura
(11 chamadas em teste ficaram intactas); `CellAllowsSkill` é a conjunção dos
dois eixos. Não são duas cópias da regra — são as duas perguntas que a casa
responde.

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

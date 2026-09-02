# Cavernas nas quedas — tarefas

Quatro tarefas. A primeira **mede**, porque o número que decide esta feature —
distância da BOCA até a queda — nunca foi medido: não existe função que dê a
posição da boca no mundo.

Ordem obrigatória. CQ2 sem CQ1 é ajustar constante contra um número que ninguém
tem; CQ4 antes de CQ2 é um teste vermelho por projeto.

---

## CQ1 — A BOCA tem posição no mundo, e a distância dela até a queda aparece
> 🤖 Modelo: `sonnet`

Hoje existe `FBakedCave::CenterUnits` (o centro do retângulo) e
`FCaveGrid::EntranceColumn` (a coluna da borda **sul** por onde se entra). Não
existe a conta que junta os dois. Toda medição feita até agora — inclusive os
**2 710** do BLOQUEIO — mediu **centro** até queda, e o centro de uma gruta de
lado 3 está a até **480** da boca dela (`3 × 240 / 2` mais a casca de 120).

**O cano que já existe:** `CaveSystem::DefaultCellSizeUnits` (240) e
`DefaultShellThicknessUnits` (120) já são a régua de toda caverna, e
`FootprintForSide` já as usa. A posição da boca é a mesma régua aplicada à
coluna de entrada — não é geometria nova, é a que faltava expor.

Junto vem a medição, no mesmo formato do BLOQUEIO: **boca de cada gruta até a
queda mais perto**, mínimo, segundo mínimo e mediana, para as 16 cavernas.

*Aceite:* **o número aparece.** Um relatório com "boca mais próxima: N" e a
mediana, comparáveis lado a lado com os 2 710 / 3 943 do centro. Enquanto a
distância da boca não tiver número, "atrás da cachoeira" não é verificável —
é adjetivo.

*Contrapeso obrigatório:* a função da boca vale para as **três cavernas do
plano** também, e para elas o número medido não muda nada — se mudar, ela está
errada, porque essas três não se mexem nesta feature.

---

## CQ2 — O POÇO DA QUEDA vira exceção declarada: a boca molha, o corpo não
> 🤖 Modelo: `opus` 🧠 — mexe na colocação, que é o gerador

Hoje um argumento só governa a gruta inteira, em `PlanGrottoes()`:

```
MargemDaAgua(Cursos, Centro) > Inflada.ClearanceUnits
```

com `Inflada = ClearanceUnits + FolgaDaGruta`, ou seja **978,8** de chão seco
exigidos ao redor do centro. O poço tem **886** de raio e está em cima da calha,
que tem 770 de meia-largura: nenhum ponto do poço passa.

A regra se separa em duas, e a segunda é a feature:

| o que | regra |
|---|---|
| **corpo** da gruta | continua exigindo os 978,8 de chão seco, como hoje |
| **boca** da gruta | pode cair dentro do **poço de uma queda** — só do poço, e só onde há queda |

**Por que exceção e não folga menor:** baixar `FolgaDaGruta` devolveria as três
grutas com a quina dentro do rio, que é o defeito que a busca existe para não
ter; e valeria em todo curso, não só onde há queda. A exceção é nomeada, tem
lugar (`PlungePoolHalfWidthUnits`, que já existe) e não vaza para o resto da
água.

⚠️ **`HalfWidthAtProgress` não sabe que existe poço.** Ela mistura a calha do rio
com `MeiaCalhaDoLago()` perto do lago e ignora a queda; `PlungePoolHalfWidthUnits`
é função separada e ninguém a consulta na busca. A exceção precisa consultá-la —
não alargar `HalfWidthAtProgress`, que é a largura da água corrente e é lida por
mais gente (vau, balsa, templo de Corrente).

**Esta é a task onde o teste velho VIRA o teste novo.**
`BattleSquare.Environment.FreshWater.GrottoStandsClearOfTheChannel` afirma hoje,
para **toda** gruta e **todo** curso, `Ate > HalfWidthAtProgress + ClearanceUnits`.
Ele passa a afirmar a mesma coisa sobre o **corpo**, com o poço da queda
descontado. Não convive com uma segunda versão: duas afirmações sobre a mesma
regra concordam até a primeira edição, e L-032/L-033 já cobraram isso duas vezes.

*Aceite:* a MESMA gruta que hoje a busca recusa a 900 da queda passa a ser
aceita, e a que tem a **quina** dentro da calha continua recusada. Sem os dois
lados, o aceite não distingue a feature de ter afrouxado a regra.

*Contrapeso obrigatório:* mover grutas muda `PlanUnderwaterLinks()`, que nasce
delas — e a carta afirma **158 galerias**, espelhadas em
`ChartConformance.WaterAndTrailsMatch`. O número velho **vira** o novo, na carta
e no teste, na mesma task e de propósito: divergência entre os dois é a
informação, e deixá-la para depois transforma o teste que protege o traçado num
teste vermelho de origem desconhecida.

*Verificação:*
```bash
./Tools/bake_island.sh
./Tools/run_tests.sh BattleSquare.Environment.FreshWater
./Tools/run_tests.sh BattleSquare.ChartConformance
./Tools/audit_determinism.sh && ./Tools/audit_no_recalculation.sh
```

---

## CQ3 — A boca OLHA para a queda
> 🤖 Modelo: `opus` 🧠 — muda o que o assado carrega

`FCaveGrid::EntranceColumn` é uma coluna da borda **sul**, sempre. Uma gruta
atrás da cachoeira com a boca ao sul e a queda ao norte tem a lâmina de água
pelas costas: a peça existe, a promessa não se cumpre, e nada reprova.

`FCaveGrid` já tem `FMouth{Edge, Along}` com `Edge` 0 sul, 1 norte, 2 oeste,
3 leste — **e `FBakedCave` descarta `ExtraMouths` no assado**. A borda da boca
principal precisa chegar ao assado; hoje só a coluna chega.

**O cano que já existe:** `FBakedCave` é o assado, e `IslandBakedPlanGuardTest`
já cobra que a divergência de parâmetro **nomeie** o parâmetro que mudou. Campo
novo no assado entra por esse caminho, e `UPROPERTY` ausente desserializa como
`0` — que é **sul**, o comportamento de hoje. Assado velho continua válido.

*Aceite:* a gruta escolhida para a lâmina tem a boca voltada para a queda; as
três cavernas do plano continuam com a boca ao sul, byte por byte. Sem a segunda
metade, "a boca olha para a queda" pode ser uma rotação global que ninguém pediu.

*Contrapeso obrigatório:* girar a boca **não** gira o labirinto.
`CaveLabyrinth::Carve` recebe semente de `SeedForPlacement`, que é
`WorldSeed + round(AngleDegrees)`; mudar a boca sem mudar o ângulo tem de deixar
as paredes idênticas — senão esta task muda dezesseis labirintos de carona, e
nenhum teste diria por quê.

⚠️ **Duas grutas no mesmo grau arredondado recebem o mesmo labirinto.** É
verdade hoje, com treze grutas num anel, e mover grutas em CQ2 muda quais
colidem. Se a medição de CQ1 mostrar duas grutas no mesmo grau, isto é achado
para o relatório — não conserto de carona nesta task.

---

## CQ4 — A propriedade, na forma EXISTENCIAL, e a prova na tela
> 🤖 Modelo: `sonnet`

O teste que fecha a feature, na forma que a regra de mapas **§12** manda:

> **EXISTE** queda para a qual há gruta cuja **boca** cai dentro do poço dela.

**Por que existencial, e não "a queda mais perto":** a medição do BLOQUEIO
achou duas cavernas a **2 710** e **2 711** da mesma queda — um empate de uma
unidade. Um teste que refaz a escolha ("a queda mais perto desta gruta é X, logo
a boca está no poço de X") mede o desempate: o gerador elege uma, o teste elege
a outra, e a mensagem não dá nenhuma pista de que o problema é o desempate. Já
aconteceu duas vezes seguidas neste projeto, na regra do cemitério.

E **não** "toda gruta": doze continuam ao lado da queda de propósito, e cobrar
todas congelaria uma decisão de arte dentro de um teste. O teto é parâmetro,
nunca zero (regra de mapas §5).

Junto, a prova na tela: entrar pela lâmina **anuncia**, com
`FBattleDebugScreen::Show` e `LOCTEXT` de argumentos nomeados. Gruta atrás da
cachoeira que não se anuncia é indistinguível de ter errado o caminho — e foi
exatamente assim que oito defeitos ficaram invisíveis em 26–27/08.

*Aceite:* o teste reprova o mundo de **hoje** (nenhuma boca dentro de poço
nenhum) e aprova depois de CQ2 e CQ3. Um teste que já nasce verde não prova
feature nenhuma.

*Contrapeso obrigatório:* roteiro em `docs/verification/cavernas-nas-quedas.md`
dizendo **onde** ir — a queda, o rumo, o que se lê na tela. Sem coordenada, o
roteiro pede que o usuário procure dezesseis cavernas.

*Verificação:*
```bash
./Tools/run_tests.sh BattleSquare
./Tools/audit_localizable_text.sh && ./Tools/gather_text.sh
./Tools/sync_module_manifest.sh
```

---

## Decisões que são do usuário

1. **Quantas quedas ganham gruta na lâmina.** Uma prova a regra; treze fazem de
   "atrás da cachoeira" o normal, e o normal não é segredo. O teste cobra a
   existência, então qualquer número entre 1 e 13 passa — a escolha é de arte.
2. **A gruta da lâmina SUBSTITUI a gruta de lado, ou existe além dela?**
   Substituir mantém 16 cavernas e não move a carta; acrescentar sobe a contagem
   de cavernas e as galerias, e move a carta duas vezes.
3. **O que se vê de dentro.** A lâmina de água na frente da boca é efeito
   visual, e este projeto não autora asset. Pode ficar só no texto do painel,
   como o item ficou.

---

## O que estas tarefas NÃO fazem

- **Não movem as três cavernas do `IslandFeatureLayout::Plan()`.** Elas estão
  onde estão de propósito — *"uma montanha que muda de lugar a cada partida não
  é paisagem"*.
- **Não fazem a gruta perguntar ao USO DO SOLO.** O uso do solo já lê
  `FreshWater::Plan()`, e a pergunta de volta fecha o ciclo que
  `PlanReentryGuard` existe para nomear (regra de mapas §14). O templo de
  Corrente, que já está na margem da primeira cachoeira, continua sem saber da
  gruta e vice-versa.
- **Não alargam `HalfWidthAtProgress`.** Ela é a largura da água corrente e é
  lida por vau, balsa e templo; alargá-la para caber o poço mexeria em três
  regras que ninguém pediu.
- **Não trocam a busca por cálculo.** O lugar da gruta é **procurado** — 16
  distâncias × 12 rumos — e *"onde não houver lugar, não há gruta"* continua
  valendo. Calcular o ponto ignoraria as peças da ilha, os campos de treino e as
  outras grutas, que a busca já confere.
- **Não autoram asset nem efeito de partícula.**

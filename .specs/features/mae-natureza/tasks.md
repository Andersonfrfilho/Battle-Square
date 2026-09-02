# Mãe Natureza — tarefas

**Regra de fatiamento desta feature:** o corretor nasce PURO (MN1–MN5, MN7) e
roda sem depender de nenhum sistema externo estar pronto — só MN6 (repovoar
espécie rara) precisa de censo real, e por isso é a única que espera
`posse-no-servidor` estar no ar.

O ciclo de cada tarefa é o da casa: ler a task → procurar o cano que já existe
→ escrever o teste e o contrapeso → implementar → build → bateria → auditorias
→ grep fora de `/Tests/` → pôr na tela → commit → próxima.

---

## MN1 — Medir o panteão: "rebrota" e "censo" já estão construídos?
> 🤖 Modelo: `sonnet`

**O que a medição achou:** a tabela de deuses do `spec.md` (coluna "o que já
está construído") atribui a Mãe Natureza "rebrota, censo" como coisa que já
existe. A medição diz o oposto: `rg -c "Regrow|Rebrota" Source/` = 0,
`rg -c "Census|Censo" Source/` = 0. Não existe rebrota de recurso nem censo de
população em nenhum arquivo do projeto — a tabela do panteão descreve uma
intenção, não o estado do código, e planejar em cima dela sem medir primeiro
faria MN2 em diante assumir uma base que não existe (a mesma ordem de erro
dos três "pet/inimigo/jogador invisível" do `CLAUDE.md` raiz — o componente
parecia existir).

Esta task não muda `spec.md` — ele fica como registro da intenção original.
A correção mora aqui.

**Dependências:** nenhuma.

**Verificação:**
```bash
rg -c "Regrow|Rebrota" Source/
rg -c "Census|Censo" Source/
```

*Aceite:* o boot do editor (dev-only) mostra, via `FBattleDebugScreen::Show`,
"Mãe Natureza: 0 fontes de rebrota, 0 de censo — panteão desatualizado" —
para a PRÓXIMA pessoa que abrir o projeto não repetir a leitura errada da
tabela.

---

## MN2 — O corretor nasce PURO: censo entra, correção sai, nada mais
> 🤖 Modelo: `opus` 🧠 — modelo de dados novo, decisão estrutural

**O que a medição achou:** o precedente de "função pura, testável sem mundo,
sem tempo, sem banco" já é o estilo da casa — `WorldTimeOfDay`
(`Source/BattleSquare/Public/Environment/WorldTimeOfDay.h`) recebe uma HORA e
devolve fase/brilho/cor, nada mais; `ScenaryClimate`
(`Source/BattleSquare/Public/Environment/ScenaryClimate.h`) faz o mesmo para
clima. `NatureBalance` (proposto em
`Source/BattleSquare/Public/World/NatureBalance.h`, ao lado de
`SettlementEconomy.h`) segue a mesma forma: `FNatureCenso` (o que existe hoje)
+ `FNatureFaixaAlvo` (o que devia existir) → `FNatureCorrecao` (o que muda).

**Dependências:** MN1 (a correção do panteão evita construir sobre uma base
que a medição já provou não existir).

**Verificação:**
```bash
./Tools/build_editor.sh && ./Tools/run_tests.sh
./Tools/probe_isolation.sh
```

*Aceite:* `NatureBalance::Correct(FNatureCenso, FNatureFaixaAlvo)` roda em
teste unitário sem levantar mundo, sem carregar malha, sem abrir o editor —
mesma prova que `WorldTimeOfDay` já tem.

*Contrapeso obrigatório:* `NatureBalance.h`/`.cpp` **nunca** importam símbolo
de `Environment/ForestBackdrop.h` nem de qualquer módulo que leia o mundo.
Isso é o que impede o ciclo de reentrada do §14 de
`geracao-procedural-de-mapas.md`: quem busca o censo e aplica a correção é a
camada de FORA, nunca o próprio corretor — um corretor que se serve sozinho é
um plano consultando outro plano que o consulta de volta.

---

## MN3 — Toda correção é DELATADA — correção sem registro é bug
> 🤖 Modelo: `sonnet`

**Por que esta task existe:** a spec exige que Mãe Natureza "amortece" fora de
vista mas NUNCA em silêncio — o jogador tem que poder achar o registro de toda
intervenção, mesmo que ela não grite. Uma correção aplicada sem log é
indistinguível de um bug de arredondamento; a única diferença visível entre
os dois é o registro.

**Dependências:** MN2.

**Verificação:**
```bash
./Tools/build_editor.sh && ./Tools/run_tests.sh
```

*Aceite:* toda chamada a `NatureBalance::Correct` que produz uma correção
não-nula grava uma linha (entrada, saída, quando) — e um teste que aplica a
correção sem gravar o registro FALHA, provando que o registro é obrigatório,
não incidental.

---

## MN4 — Torneira, nunca balde: teste negativo prova o limite
> 🤖 Modelo: `sonnet`

**O que a spec exige:** Mãe Natureza ajusta a TORNEIRA (prazo de rebrota,
demanda do comerciante, preço, prêmio) e nunca o BALDE (coleção do jogador,
atributo, especialidade). A tabela "ajuste a torneira, nunca o balde" do
`spec.md` só vira regra de verdade quando existe um teste que reprova a
versão errada.

**Dependências:** MN2, `mundo-vivo` MV4 (o prazo de rebrota precisa já ser
número de configuração para ter algo aqui para girar).

**Verificação:**
```bash
./Tools/build_editor.sh && ./Tools/run_tests.sh
```

*Aceite:* `FNatureCorrecao` não tem — nem por acidente — um campo que aponte
para coleção, atributo ou especialidade de pet. Um teste que tenta montar
essa correção não compila, porque o tipo não tem esse campo — não é
validação em runtime, é ausência na assinatura.

---

## MN5 — O preço do assentamento vira NÚMERO DE CONFIGURAÇÃO
> 🤖 Modelo: `sonnet`

**O que a medição achou:** `SettlementEconomy::PricePercent`/`PayoutPercent`
(`Source/BattleSquare/Public/World/SettlementEconomy.h`) são C++ puro,
retornando porcentagem fixa por `(Kind, Service)` — sem `.ini` nem `.json`
por trás, ao contrário do vizinho de padrão `ScenaryClimate::ConfiguredClimate()`,
que já lê `DefaultGame.ini`. Sem essa torneira, Mãe Natureza não tem NADA para
girar na Academia nem no Prêmio de Ranking — a correção existiria só no
papel.

**Dependências:** nenhuma (refatoração do módulo puro existente, seguindo o
precedente já testado de `ScenaryClimate`).

**Verificação:**
```bash
./Tools/build_editor.sh && ./Tools/run_tests.sh
```

*Aceite:* mudar o número em `DefaultGame.ini` muda `PricePercent`/
`PayoutPercent` sem recompilar — mesmo teste-padrão de `mundo-vivo` MV4.

---

## MN6 — Espécie rara migra quando o censo cai — depende do censo existir
> 🤖 Modelo: `sonnet` (a faixa-alvo por ilha ou por região é 🧠 — decisão do
> usuário antes de codar)

**O que a medição achou:** não existe hoje nenhuma contagem GLOBAL de espécie
capturada — a coleção é por conta, por save, nunca agregada entre jogadores. A
correção de migração só tem o que medir depois que `posse-no-servidor` (PS1)
colocar a posse na conta, e depois de existir alguma leitura agregada por
espécie sobre essa tabela.

**Dependências:** `posse-no-servidor` PS1, MN2; decisão do usuário sobre
faixa-alvo por ilha vs. por região.

**Verificação:**
```bash
cd apps/api-battle-pets && bun test
./Tools/build_editor.sh && ./Tools/run_tests.sh
```

*Aceite:* uma espécie com contagem abaixo da faixa-alvo produz uma correção
de migração (peso de encontro sobe onde falta, desce onde sobra) — medido por
`FNatureCenso` real, não por número inventado.

---

## MN7 — Mãe Natureza age devagar e fora de vista — mas o jogador VÊ
> 🤖 Modelo: `sonnet`

**Por que esta task existe:** regra do `CLAUDE.md` da raiz — comportamento
observável aparece na tela. O aviso "definitivo" (guarda florestal, poste da
vila) depende de sistemas que `mundo-vivo` MV7 ainda não construiu (bloqueada
por decisão do usuário); esta task entrega o que já dá para mostrar hoje —
`FBattleDebugScreen::Show`, dev-only — sem esperar a UI de produção existir.

**Dependências:** MN3.

**Verificação:**
```bash
./Tools/build_editor.sh && ./Tools/run_tests.sh
```

*Aceite:* toda correção registrada por MN3 aparece como linha no painel de
depuração no turno em que foi aplicada — quem estiver olhando a tela vê Mãe
Natureza agir, mesmo que o personagem no mundo ainda não veja.

---

## O que estas tarefas NÃO fazem

- **Não implementam aviso de guarda florestal nem poste da vila.** Dependem de
  sistemas que `mundo-vivo` MV7 ainda não tem (bloqueada por decisão do
  usuário) — MN7 entrega o equivalente em painel de depuração.
- **Não decidem se dá para agradar um deus, nem se deuses podem discordar.**
  Ficam em "Decisões que são do usuário" no `GOAL.md`.
- **Não tocam coleção, atributo nem especialidade do jogador.** MN4 é o teste
  que prova isso, não uma promessa em prosa.
- **Não escrevem `mundo-vivo`.** MN6 LÊ o que `mundo-vivo`/`posse-no-servidor`
  gravam; não duplica a escrita.
- **Não corrigem `spec.md`.** MN1 registra a medição em `tasks.md`; a spec
  original permanece como documento histórico da intenção.

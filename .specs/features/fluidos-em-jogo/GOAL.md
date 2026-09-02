# OBJETIVO — Os fluidos chegarem à partida

**Aberto em 02/09/2026. Só fecha quando o critério abaixo for atingido.**

## O objetivo, numa frase

Ligar o eixo da substância — completo, testado, com 111 provas — ao jogo que
alguém joga, porque hoje **nenhuma casa de batalha jamais recebe um fluido**.

## Por que ele existe

Medido, e é uma linha só:

```
$ grep -rn "SetFluidAt|MoveConducts|SetResistPercentFor" \
    Source --include="*.cpp" | grep -v "/Tests/"
(vazio)
```

Nunca há lava, ninguém se queima, nada conduz, nenhum item resiste a nada. É
**L-041**: não faltam elementos de design, falta encarnação.

E quase nada aqui é mecânica nova — são **duas pontas construídas que não se
tocam**. O trabalho é encanamento, e a única decisão de conteúdo é a G3.

## PRONTO é isto, e nada menos

- [x] **G1** — a arena põe fluido nas casas: batalha na saia do vulcão nasce
      com água TERMAL, longe dele com doce, e casa seca sem fluido
- [x] **G2** — o tradutor liga a condução: pet de Raio eletrifica a poça numa
      partida real; pet de Fogo, não
- [x] **G3** — a resistência tem origem (**pergunte antes**: item, traço, ou
      as duas)
- [x] **G4** — a prova na tela, com roteiro do que só o olho vê
- [x] O grep acima deixa de vir vazio (`EncounterMatchAssembler:139`,
      `BattleDataTranslator:157`) — a prova de que o objetivo aconteceu
- [x] Bateria completa verde — **800/800**, zero falhas (era 790)
- [x] As cinco auditorias limpas
- [x] Um commit por task, cada um com o motivo — não só o quê

Enquanto qualquer caixa estiver aberta, o objetivo **continua**.

## Invariantes — violá-las reprova a task, não importa o resto

As dez de `fluidos`, e a décima primeira é a que este objetivo inteiro ensina.

1. **`BattleSim` não tem float.** `audit_determinism.sh` reprova.
2. **Defeito primeiro vira teste, depois conserto.**
3. **Medir, não olhar.**
4. **Uma fonte de verdade por regra.**
5. **Ator sem malha atribuída no construtor não existe na tela.**
6. **Texto do jogador é `FText`** (painel de depuração é `FString`).
7. **Regra nova entra pelo CANO QUE JÁ EXISTE.** Procurar antes de criar
   mecanismo — o dano do fluido foi para o `PendingDamage` que já havia.
8. **O nome do campo bate com o relógio em que ele cai.** `DamagePerTurn` caía
   por slot; virou `DamagePerSlot`.
9. **Todo teste que PROÍBE precisa do teste que PERMITE.** "Submergir falha na
   lava" passaria numa regra que recusasse tudo. Sem o contrapeso, é armadilha.
10. **No hash entra o valor DERIVADO, nunca o array cru.**

11. **REGRA SEM CHAMADOR EM PRODUÇÃO É REGRA QUE NÃO EXISTE.** Uma bateria
    verde não prova que alguém chama aquilo. A prova é o grep FORA de
    `/Tests/`, e ela custa dez segundos:

    ```bash
    grep -rn "MinhaRegraNova" Source --include="*.cpp" | grep -v "/Tests/"
    ```

    Vazio é o defeito. Foi assim que 111 provas verdes conviveram com um eixo
    inteiro que o jogador não alcançava — e é o mesmo padrão que abriu a
    construção do mundo, com 679 testes sobre um mundo invisível.

## O ciclo de cada task

```
ler a task  →  procurar o cano que já existe  →  escrever o teste E o
contrapeso  →  implementar  →  build  →  bateria  →  auditorias  →
grep fora de /Tests/  →  pôr na tela  →  commit  →  próxima
```

Verificação obrigatória:

```bash
./Tools/build_editor.sh
./Tools/audit_determinism.sh && ./Tools/audit_no_recalculation.sh
./Tools/audit_localizable_text.sh && ./Tools/audit_test_helper_names.sh
./Tools/audit_visible_actors.sh
./Tools/sync_module_manifest.sh   # DEPOIS do build
```

Fechar o Editor antes de compilar; reabrir com `open -a`. `timeout` não existe
no macOS — retorna 127 sem executar.

## O que este objetivo NÃO faz

- **Não escolhe a origem da resistência.** É conteúdo, e é do usuário.
- **Não muda o traçado da ilha** nem o registro dos fluidos.
- **Não autora asset.** A casa de lava veste o material da água até alguém
  desenhar um.
- **Não é a T20** da construção do mundo — aquela é a passagem do olho em PIE,
  e continua sendo do usuário.

## Se o contexto for compactado

1. Reler este objetivo e `.specs/features/fluidos-em-jogo/tasks.md`.
2. `git log --oneline -15`.
3. `git status --short --branch`.
4. Continuar da primeira caixa aberta. **Não recomeçar, não replanejar.**

## Se bloquear

Fazer todo o resto que não depende do bloqueio, e então dizer o que travou com
a **medição** que sustenta isso. Reduzir escopo é decisão do usuário.

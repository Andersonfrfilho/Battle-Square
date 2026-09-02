# OBJETIVO — Fechar o eixo dos fluidos (F3, F5, F6, F7)

**Aberto em 02/09/2026. Só fecha quando o critério abaixo for atingido.**

## O objetivo, numa frase

Terminar o que F1, F2 e F4 começaram: o fluido já existe, já mora na casa e já
machuca — falta ele **decidir poderes**, **ser resistido por itens**, **existir
no mundo aberto** e **decidir o que boia**.

## Por que ele existe

O registro separa oito fluidos e o tabuleiro já sabe de qual cada casa é feita.
Mas:

| o que existe hoje | o que ainda não |
|---|---|
| `bIsWater` distingue as cinco águas da lava | nenhum poder pede substância — só fundura |
| a lava cobra 12 por slot | nada no jogo cancela esse dano |
| o mundo tem seis fluidos | `WaterFooting` trata todos como uma água só |
| `FloatsOn` sabe o que boia | a balsa flutua pela lâmina, não por densidade |

**Não falta desenho. Falta o eixo chegar onde ele foi feito para chegar.**

## PRONTO é isto, e nada menos

- [ ] **F3** — poderes distinguem substância: um poder que exige água funciona
      nas cinco águas e falha na lava; um que exige lava falha em todas as águas
- [ ] **F5** — itens de resistência: com o item, o dano da lava não vem; sem
      ele, vem. É o que o usuário pediu desde o começo
- [ ] **F6** — o mundo declara seus fluidos: a água termal perto do vulcão é
      reconhecida como termal, não como doce
- [ ] **F7** — a balsa flutua por DENSIDADE, e um corpo mais denso que o fluido
      não boia nele
- [ ] Bateria completa verde (hoje **770**; o número só sobe)
- [ ] As cinco auditorias limpas
- [ ] Um commit por task, cada um com o motivo — não só o quê

Enquanto qualquer caixa estiver aberta, o objetivo **continua**.

## Invariantes — violá-las reprova a task, não importa o resto

As seis primeiras são as de sempre. As quatro últimas foram **aprendidas nesta
feature**, cada uma com o preço pago escrito ao lado.

1. **`BattleSim` não tem float.** Sem `float`, `double`, `FMath::Rand` nem
   relógio. Densidade vai em partes por mil, inteiro. `audit_determinism.sh`
   reprova.
2. **Defeito primeiro vira teste, depois conserto.** Consertar por hipótese
   custou três rodadas em agosto e mais três nas galerias.
3. **Medir, não olhar.** Impressão formada olhando já apontou a causa errada
   mais vezes que qualquer outra coisa neste projeto.
4. **Uma fonte de verdade por regra.** Duplicar tabela causou L-032 e L-033.
5. **Ator sem malha atribuída no construtor não existe na tela.** Três
   ocorrências.
6. **Texto do jogador é `FText`**, nunca `FString`. (Painel de depuração é
   `FString` — ele não é texto de jogador.)

7. **Regra nova entra pelo CANO QUE JÁ EXISTE.** O dano do fluido foi para o
   mesmo `PendingDamage` do dano de casa, com a mesma guarda e o mesmo evento.
   Um segundo caminho teria as próprias regras de morte e a própria narração, e
   duas delas concordam até a primeira edição. **Antes de criar um mecanismo,
   procurar o que já faz aquilo.**

8. **O nome do campo bate com o relógio em que ele cai.** `DamagePerTurn` caía
   num laço que roda por SLOT, três vezes por turno. Renomeado para
   `DamagePerSlot` — dois custos de terreno em relógios diferentes são um
   defeito esperando, porque o jogador aprende um e erra o outro.

9. **Todo teste que PROÍBE precisa do teste que PERMITE.** "Submergir falha na
   lava" passaria numa regra que recusasse tudo, e o jogo quebraria em
   silêncio. Por isso existe "submergir vale nas cinco águas". Sem o
   contrapeso, o teste é armadilha.

10. **No hash entra o valor DERIVADO, nunca o array cru.** `CellFluid` vazia e
    `CellFluid` de zeros querem dizer a mesma coisa; somando o cru, dariam
    assinaturas diferentes sendo idênticas — dessincronia fantasma.

## O ciclo de cada task

```
ler a task  →  procurar o cano que já existe  →  escrever o teste E o
contrapeso  →  implementar  →  build  →  bateria  →  auditorias  →  pôr na
tela  →  commit  →  próxima
```

Verificação obrigatória:

```bash
./Tools/build_editor.sh
./Tools/audit_determinism.sh && ./Tools/audit_no_recalculation.sh
./Tools/audit_localizable_text.sh && ./Tools/audit_test_helper_names.sh
./Tools/audit_visible_actors.sh
./Tools/sync_module_manifest.sh   # DEPOIS do build
```

Fechar o Editor antes de compilar; reabrir com `open -a`, nunca por shell em
segundo plano. `timeout` não existe no macOS — retorna 127 sem executar.

## O que este objetivo NÃO faz

- **Não muda o traçado da ilha.** Se F6 revelar defeito de traçado, ele vira
  tarefa separada com teste próprio.
- **Não inventa fluido novo.** A lista dos oito é decisão de conteúdo, do
  usuário. Se um faltar, perguntar — não acrescentar.
- **Não autora asset.** A casa de lava veste o material da água hoje, e é
  limitação declarada em `tasks.md`, não defeito a consertar aqui.

## Se o contexto for compactado

1. Reler este objetivo e `.specs/features/fluidos/tasks.md`.
2. `git log --oneline -15` — os commits dizem até onde foi.
3. `git status --short --branch` — conferir branch e limpeza.
4. Continuar da primeira caixa aberta. **Não recomeçar, não replanejar.**

## Se bloquear

Fazer todo o resto que não depende do bloqueio, e então dizer o que travou —
com a **medição** que sustenta isso, não com a impressão. Reduzir escopo é
decisão do usuário, nunca minha.

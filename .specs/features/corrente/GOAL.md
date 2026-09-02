# OBJETIVO — O sentido da corrente

**Aberto em 02/09/2026. Só fecha quando o critério abaixo for atingido.**

## O objetivo, numa frase

Fazer a água deixar de ser **estado** e virar **força**: ela tem para onde ir,
e quem está nela sente.

## Por que ele existe

Hoje a casa molhada muda o passo e nada mais. Atravessar um rio é uma decisão
de **se**, e nunca de **por onde** — porque os dois lados custam igual e nada
carrega ninguém.

E o mais importante: **quase tudo já existe.**

| o que a corrente precisa | onde já está |
|---|---|
| o sentido do fluxo | a polilinha do curso é ORDENADA, da nascente à foz |
| a força | `BedGradientAtProgress` — o declive do leito |
| onde ela é violenta | `IsRapidsAtProgress` |
| a língua para dizer rumo | `EBattleDirection`, oito rumos, que o movimento já fala |
| mover alguém contra a vontade | o escorregão do gelo, e o empurrão da trombada |

**Isto não é inventar física — é carregar até a casa o que o traçado já sabe.**

## PRONTO é isto, e nada menos

- [ ] **C1** — a casa sabe o rumo e a força, assados do traçado; e água parada
      não tem rumo
- [ ] **C2** — a corrente EMPURRA, pelo cano que já move quem escorrega
- [ ] **C3** — subir a correnteza custa mais que descer; de lado, nem um nem
      outro
- [ ] **C4** — a balsa sente a corrente
- [ ] **C5** — o mundo aberto: o mesmo rio nos dois sentidos dá resultados
      diferentes
- [ ] **C6** — a prova na tela: painel e grade dizem o rumo
- [ ] O grep fora de `/Tests/` acha quem PÕE a corrente numa casa
- [ ] Bateria completa verde (hoje **800**; o número só sobe)
- [ ] As cinco auditorias limpas
- [ ] Um commit por task, cada um com o motivo — não só o quê

Enquanto qualquer caixa estiver aberta, o objetivo **continua**.

## Invariantes — violá-las reprova a task, não importa o resto

As onze de `fluidos-em-jogo`, e a décima segunda é desta feature.

1. **`BattleSim` não tem float.** A força da corrente é INTEIRA, e a escala se
   escolhe com a disciplina das partes por mil.
2. **Defeito primeiro vira teste, depois conserto.**
3. **Medir, não olhar.**
4. **Uma fonte de verdade por regra.**
5. **Ator sem malha atribuída no construtor não existe na tela.**
6. **Texto do jogador é `FText`.**
7. **Regra nova entra pelo CANO QUE JÁ EXISTE.** Mover alguém contra a vontade
   dele já acontece duas vezes; um terceiro caminho teria as próprias regras de
   colisão e a própria narração.
8. **O nome do campo bate com o relógio em que ele cai.**
9. **Todo teste que PROÍBE precisa do teste que PERMITE.**
10. **No hash entra o valor DERIVADO, nunca o array cru.**
11. **Regra sem chamador em PRODUÇÃO é regra que não existe.** A prova é o grep
    fora de `/Tests/`, e ela custa dez segundos.

12. **A DIREÇÃO É LIDA DO TRAÇADO, NUNCA DEDUZIDA DE NOVO.** A ordem da
    polilinha já É o sentido do fluxo. Recalculá-lo a partir das posições, do
    raio ou do declive seria uma segunda verdade — e ela concordaria com a
    primeira até a primeira edição, com o rio correndo para trás em algum
    trecho que ninguém olhou. É L-032 aplicada a uma seta.

## O que este objetivo NÃO faz

- **Não decide para onde a eletricidade anda.** A corrente elétrica atravessa o
  meio inteiro; ela não desce o rio. Misturar as duas seria inventar física
  para caber num nome parecido.
- **Não muda o traçado.** O sentido é lido dele.
- **Não faz os outros atributos de casa** que a caminhada pediu — densidade
  por casa, venenoso, condutor por elemento. Estão em
  `.specs/features/fluidos-em-jogo/ACHADOS-G4.md`, e cada um é tarefa própria.

## O ciclo de cada task

```
ler a task  →  procurar o cano que já existe  →  escrever o teste E o
contrapeso  →  implementar  →  build  →  bateria  →  auditorias  →
grep fora de /Tests/  →  pôr na tela  →  commit  →  próxima
```

```bash
./Tools/build_editor.sh
./Tools/audit_determinism.sh && ./Tools/audit_no_recalculation.sh
./Tools/audit_localizable_text.sh && ./Tools/audit_test_helper_names.sh
./Tools/audit_visible_actors.sh
./Tools/sync_module_manifest.sh   # DEPOIS do build
```

Fechar o Editor antes de compilar; reabrir com `open -a`. `timeout` não existe
no macOS — retorna 127 sem executar.

## Se o contexto for compactado

1. Reler este objetivo e `.specs/features/corrente/tasks.md`.
2. `git log --oneline -15`.
3. `git status --short --branch`.
4. Continuar da primeira caixa aberta. **Não recomeçar, não replanejar.**

## Se bloquear

Fazer todo o resto que não depende do bloqueio, e então dizer o que travou com
a **medição** que sustenta isso. Reduzir escopo é decisão do usuário.

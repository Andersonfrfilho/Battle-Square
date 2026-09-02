# OBJETIVO — As pendências registradas

**Aberto em 02/09/2026. Só fecha quando o critério abaixo for atingido.**

## O objetivo, numa frase

Executar as dez pendências que as duas caminhadas manuais deixaram registradas —
tudo o que o usuário pediu e que ainda não tem plano.

## Por que ele existe

As caminhadas de T20 e G4 não acharam defeito nenhum. Mas **a maior parte das
respostas não era verificação: era design novo**, e ele foi registrado em
`ACHADOS-T20.md` e `ACHADOS-G4.md` para não se perder.

Registro sem objetivo é lista que envelhece. Este objetivo existe para essa
lista virar trabalho, ou virar decisão de não fazer — as duas são respostas; o
esquecimento não é.

**Nenhum item aqui é defeito.** Tudo o que foi construído respondeu como
prometido, e há uma única divergência (P1) contra código que existe e está
testado.

## PRONTO é isto, e nada menos

- [ ] **P1** — o aqueduto pode entrar no morro, saindo por túnel; e o teste que
      afirma o contrário vira o teste novo
- [ ] **P2** — a casa ganha atributos, com a medição de o que é do fluido e o
      que é da casa
- [ ] **P3** — a resistência vem da anatomia, e ela também resolve o buraco da
      C2 (resistir a ser carregado)
- [ ] **P4** — efeito de campo inteiro, comparado ao tamanho do campo
- [ ] **P5** — golpe que se desliga sob dano
- [ ] **P6** — a água apaga o fogo
- [ ] **P7** — nadar quando a água passa da cintura
- [ ] **P8** — o poço se lê pela fundura
- [ ] **P9** — cavernas por dentro das cachoeiras
- [ ] **P10** — coisas escondidas, e a carta dizendo o que ela não mostra
- [ ] Bateria completa verde (hoje **809**; o número só sobe)
- [ ] As cinco auditorias limpas
- [ ] Um commit por task, cada um com o motivo — não só o quê

Enquanto qualquer caixa estiver aberta, o objetivo **continua**.

## Invariantes

As doze de `corrente`, sem exceção. E duas coisas que esta lista exige em
particular:

- **P1 muda uma regra que TEM TESTE afirmando o contrário.** Mudar o código sem
  mudar o teste deixaria a bateria verde sobre a regra velha — que é pior que
  vermelho, porque parece prova.
- **P7 e P8 encostam no TRAÇADO.** Por invariante 1, o traçado não muda dentro
  de uma tarefa de construção: se for preciso mexer nele, é tarefa separada com
  teste próprio. **Medir qual dos dois lados precisa mudar antes de escrever.**

## Se um item pedir decisão de conteúdo

**Perguntar, não escolher.** Metade desta lista veio de frases curtas, e uma
frase curta cabe em mais de um jogo. Onde couber em dois, o certo é perguntar —
e enquanto a resposta não vem, fazer os outros itens, que são independentes.

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

## O que este objetivo NÃO faz

- **Não é o objetivo da corrente.** C3 a C6 continuam em
  `.specs/features/corrente/GOAL.md`, e vêm antes: a P3 depende do buraco que a
  C2 deixou.
- **Não cria o sistema de itens.** Ele segue sendo feature própria, e a regra de
  composição já tem prova esperando por ele.
- **Não autora asset.** O material de lava continua sendo o da água.

## Se o contexto for compactado

1. Reler este objetivo e `.specs/features/pendencias/tasks.md`.
2. `git log --oneline -15`.
3. Continuar da primeira caixa aberta. **Não recomeçar, não replanejar.**

## Se bloquear

Fazer todo o resto que não depende do bloqueio, e então dizer o que travou com
a **medição** que sustenta isso. Reduzir escopo é decisão do usuário.

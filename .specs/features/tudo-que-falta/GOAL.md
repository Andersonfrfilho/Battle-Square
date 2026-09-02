# OBJETIVO — Tudo o que falta

**Aberto em 02/09/2026. Só fecha quando o critério abaixo for atingido.**

## O objetivo, numa frase

Fechar **os dois objetivos abertos** e abrir **os dois sistemas** que várias
tarefas já pediram e que nunca existiram.

## Por que ele existe

Este é um objetivo GUARDA-CHUVA, e ele não repete nenhuma tarefa: quem tem dono
continua onde está, porque duplicar uma lista é duplicar uma verdade.

Ele existe para responder "o que falta" com uma coisa só, e para que a ordem e
os bloqueios fiquem escritos em vez de serem relembrados.

## PRONTO é isto, e nada menos

- [x] **Corrente fechada** — `.specs/features/corrente/`
- [~] **Pendências** — P1 a P6 fechadas; **P7, P8, P9, P10 PARARAM**, com a
      medição em `.specs/features/pendencias/BLOQUEIO-P7-P8-P9-P10.md`. Abrir as
      features novas é decisão do usuário.
- [x] **T-ITENS** — feito em `.specs/features/itens-e-biologia/` (I1–I6)
- [x] **T-ANATOMIA** — feito como BIOLOGIA de quatro eixos (B1), na mesma feature
- [x] Bateria completa verde — **858**, zero falhas, zero crash
- [x] As cinco auditorias limpas
- [x] Um commit por task, cada um com o motivo — não só o quê

Enquanto qualquer caixa estiver aberta, o objetivo **continua**.

## Não pare entre tarefas

**Ao fechar uma task, execute a próxima na mesma rodada.** Sem perguntar "sigo?",
sem relatório intermediário, sem esperar confirmação: o objetivo foi aprovado
quando foi aberto, e cada pausa custa uma rodada inteira só para receber de
volta a palavra "pode".

Qual é a próxima não se lembra — se pergunta:

```bash
./Tools/goal_status.sh
```

Ele LÊ os `GOAL.md` de cada feature e diz a primeira caixa aberta. É por isso
que este objetivo não tem a própria lista de tasks: uma segunda lista de caixas
concordaria com a primeira até a primeira edição, e depois estaria mentindo em
silêncio sobre o que já foi feito — que é como uma task fechada volta a ser
refeita depois de uma compactação.

**Só existem três motivos para parar**, e todos os três se dizem com a
medição junto:

1. A task pede **decisão de conteúdo** que é do usuário (quantos slots, o que é
   uma anatomia). Aí: fazer as outras, que são independentes, e perguntar.
2. A task **encosta no traçado** ou **muda o gabarito de aceite** — as três
   previstas abaixo.
3. A bateria ficou **vermelha** e não é falha do teste novo.

Acabar a lista inteira não é motivo para parar de conferir: rodar
`goal_status.sh` de novo, porque uma task pode ter aberto caixa em outra
feature.

## A ordem, e ela não é de valor

1. **C4, C6, P5, P6** — todas têm cano pronto. Rendem sem atrito.
2. **C5** — o mundo aberto, independente do resto.
3. **P4** — a maior, e a única sem nenhuma peça: precisa de um limiar
   comparando potência do golpe com tamanho do campo, e esse número não existe.
4. **P9** — parcial: a queda tem poço, a gruta é peça separada.
5. **T-ITENS** e **T-ANATOMIA** — os dois sistemas. A metade de DENTRO dos dois
   já está pronta e testada; falta a de fora.
6. **P7, P8, P10** — por último, e o motivo está abaixo.

## As TRÊS que vão parar, e isso é previsto

Não é fracasso do objetivo; é o objetivo dizendo antes.

- **P7** (nadar pela cintura) e **P8** (o poço pela fundura) **encostam no
  TRAÇADO**, e por invariante o traçado não muda dentro de uma tarefa de
  construção. **Medir qual lado precisa mudar, e então parar e dizer** — com a
  medição, não com a impressão.
- **P10** (coisas escondidas) **muda o gabarito de aceite**: hoje
  `ChartConformance` exige carta e mundo IDÊNTICOS, e esconder algo reprova.
  Para haver segredo, a carta precisa dizer o que ela **não** mostra — senão o
  teste que protege a feature passa a proibir o design.

## Invariantes

As doze de `corrente`, sem exceção. E a que mais vai pesar aqui:

**Todo teste que afirma a regra VELHA vira o teste da regra NOVA.** Já
aconteceu duas vezes nesta sessão — o aqueduto que nunca entrava no morro, e a
resistência que não podia ser negativa. Nos dois, deixar o antigo verde ao lado
do novo faria a bateria provar duas regras que se contradizem, e uma delas
estaria mentindo em silêncio. **Vermelho avisa; verde errado não.**

## Se um item pedir decisão de conteúdo

**Perguntar, não escolher.** Os dois sistemas novos têm decisões que são do
usuário — quantos slots de item, se ele se perde, o que é uma anatomia. Enquanto
a resposta não vem, fazer os outros itens, que são independentes.

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

- **Não repete tarefa que já tem dono.** C4–C6 e P4–P10 se executam nos
  arquivos delas, e as caixas se marcam LÁ e aqui.
- **Não autora asset.** O material de lava segue sendo o da água.
- **Não decide conteúdo.** Ele pergunta.

## Se o contexto for compactado

1. Reler este objetivo, e depois o `tasks.md` da feature em que se estava.
2. `git log --oneline -15`.
3. Continuar da primeira caixa aberta. **Não recomeçar, não replanejar.**

## Se bloquear

Fazer todo o resto que não depende do bloqueio, e então dizer o que travou com
a **medição** que sustenta isso. Reduzir escopo é decisão do usuário.

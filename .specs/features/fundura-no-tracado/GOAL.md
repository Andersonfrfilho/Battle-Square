> NOTA (04/09, medido): SATISFEITA por a-carta-muda-uma-vez M3 (fundura por
> ponto) + a escarpa (BedrockEscarpmentTest). NAO reconstruir (L-032).

# OBJETIVO — Fundura no traçado

> **As decisões de conteúdo desta feature foram respondidas em 02/09/2026.**
> Elas vivem em `.specs/project/DECISOES.md` — fonte única, não copiar para cá.
> Três itens seguem em aberto, e estão listados no fim daquele arquivo.


**Escrito em 02/09/2026, e nasce FECHADO.** Vem de `pendencias`, que mediu P7 e
P8 e parou nas duas por elas mexerem no gerador — o que uma tarefa de construção
não pode ter dentro.

**Trocar `[⛔]` por `[ ]` ABRE esta feature — é decisão do usuário, e até lá
`./Tools/goal_status.sh` não a elege como próxima.** Esta pasta
ordena antes daquela em ordem alfabética; se as caixas nascessem abertas, ela
roubaria o ponteiro de uma feature que outra sessão está executando agora.

## O objetivo, numa frase

A mesma água, com a mesma largura, atravessa a pé num trecho e obriga a nadar
noutro — porque **a fundura mudou, e é o gerador que diz quanto**.

## PRONTO é isto, e nada menos

> ⚠️ **CINCO DAS SEIS JÁ FORAM FEITAS, em 02–03/09/2026, por
> `a-carta-muda-uma-vez`.** Aquela feature juntou quatro decisões que mexiam no
> mesmo gabarito, e a fundura era uma delas — fazê-la aqui em separado
> reescreveria a carta uma vez a mais, que é exatamente o que a invariante 15
> de lá existe para impedir. **Não reescrever: ler o que está lá.**

- [x] **F1** — os três números medidos: degrau da rocha, passo por curso, cursos de largura constante
      *A única que continua de pé.* A `a-carta-muda-uma-vez` mediu o que ELA
      precisava (os dezesseis da carta, o poço de 30–51, as três âncoras da
      cintura), e não estes três.
- [x] **F2** — a fundura por ponto existe no assado, e quem a decide é o gerador
      → **M3** de `a-carta-muda-uma-vez` (`d4ded94`). `FBakedRiver::DepthUnits`.
- [x] **F3** — a estimativa privada do traçado morre, e o traçado LÊ
      → **M4** (`f1566a3`). `FunduraSobreLargura` não existe mais.
- [x] **F4** — o poço da queda deixa de ser prato: a rocha ganha degrau
      → **M2** (`2cd04a8`), e ela foi **REESCRITA antes de uma linha de código**:
      degrau NA QUEDA recria o ciclo que trava o processo com `abort()`. O que
      se fez foi dar ESCARPA à rocha, e a água — que já procura onde o leito
      despenca — achou degraus maiores sozinha. Ler o comentário da M2 antes de
      mexer aqui.
- [x] **F5** — passou da cintura, nada
      → **M5** (`529e225`). A cintura é 40% da altura de quem pisa, e as três
      âncoras que brigavam pararam de brigar porque nenhuma precisou ganhar.
- [x] **F6** — a fundura na tela
      → **M11** (`5abae34`), com o contrapeso: em terra seca a linha **some**,
      não vai a zero. Roteiro em `docs/verification/a-carta-muda-uma-vez.md`.
- [x] O grep fora de `/Tests/` acha quem LÊ a fundura do assado
      Medido em 03/09: `WaterFooting`, `TrailLayout`, `IslandBakedPlan`,
      `CrossingMesh`, `WorldBoundaryWater`, `FreshWater`, `Volcano`.
- [x] O grep não acha `FunduraSobreLargura` em lugar nenhum
      Medido em 03/09: zero ocorrências em `Source/`.
- [x] Bateria completa verde — o número só sobe
      **898** em 03/09/2026, 0 falhas, 0 crash.
- [x] As cinco auditorias limpas
- [x] `./Tools/bake_island.sh` verde, com a fundura por ponto no JSON
- [ ] Um commit por task, cada um com o motivo — não só o quê
      *Fica aberta de propósito:* os commits existem, mas com o nome da OUTRA
      feature. Quem fechar a F1 fecha esta junto.

Enquanto qualquer caixa estiver aberta, o objetivo **continua**.

## Não pare entre tarefas

`./Tools/goal_status.sh` diz a próxima. Só se para por três motivos, e todos com
a medição junto: decisão de conteúdo que é do usuário (as cinco estão listadas na
spec), encostar no gabarito da carta, e bateria vermelha que não é do teste novo.

## Invariantes

As quatorze que já existem, e duas que esta feature acrescenta:

15. **Uma fonte de verdade sobre a água, e ela é o GERADOR.** O assado carrega, e
    todo mundo lê. Estimativa local que "só resolve aqui" é a segunda verdade, e
    ela concorda com a primeira até a primeira edição — L-032 e L-033 já foram
    pagos, e a estimativa de hoje (`FunduraSobreLargura = 0,065`) argumenta no
    próprio comentário que ela evita o defeito que ela é.

16. **Teste desta feature afirma VARIAÇÃO, nunca proporção.** O "dez vezes" da
    literatura de poço de queda é razão de velocidade de erosão, não a forma do
    buraco. Poço é mais largo que fundo, e está certo. Um teste afirmando "mais
    fundo que largo" já reprovou código certo aqui, e a mensagem dele não dava
    pista nenhuma de que o errado era o teste.

## O que este objetivo NÃO faz

- **Não decide onde fica a cintura.** As três âncoras medidas discordam (100 do
  a-pé, 88 da cápsula, 94 do maior vau de hoje). É número de produto.
- **Não muda a carta por conta própria.** Se a fundura reclassificar travessia,
  `ChartConformance` fica vermelho e a tarefa **para**. Afrouxar a afirmação para
  o teste passar transforma o teste que protege a feature no teste que não
  protege nada.
- **Não põe caverna atrás de cachoeira** (P9) nem **esconde coisa da carta**
  (P10). São features próprias.
- **Não autora asset.** Fundura aparece por texto e por número.

## Se o contexto for compactado

Reler este objetivo, `./Tools/goal_status.sh`, `git log --oneline -15`, e
continuar da primeira caixa aberta. **Não recomeçar, não replanejar.**

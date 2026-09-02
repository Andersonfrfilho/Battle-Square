# OBJETIVO — Fundura no traçado

> **As decisões de conteúdo desta feature foram respondidas em 02/09/2026.**
> Elas vivem em `.specs/project/DECISOES.md` — fonte única, não copiar para cá.
> Onze itens seguem em aberto, e estão listados no fim daquele arquivo.


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

- [⛔] **F1** — os três números medidos: degrau da rocha, passo por curso, cursos de largura constante
- [⛔] **F2** — a fundura por ponto existe no assado, e quem a decide é o gerador
- [⛔] **F3** — a estimativa privada do traçado morre, e o traçado LÊ
- [⛔] **F4** — o poço da queda deixa de ser prato: a rocha ganha degrau
- [⛔] **F5** — passou da cintura, nada
- [⛔] **F6** — a fundura na tela
- [ ] O grep fora de `/Tests/` acha quem LÊ a fundura do assado
- [ ] O grep não acha `FunduraSobreLargura` em lugar nenhum
- [ ] Bateria completa verde — o número só sobe
- [ ] As cinco auditorias limpas
- [ ] `./Tools/bake_island.sh` verde, com a fundura por ponto no JSON
- [ ] Um commit por task, cada um com o motivo — não só o quê

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

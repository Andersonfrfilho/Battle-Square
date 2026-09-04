# OBJETIVO — Crime e recompensa

> **As decisões de conteúdo desta feature foram respondidas em 02/09/2026.**
> Elas vivem em `.specs/project/DECISOES.md` — fonte única, não copiar para cá.
> Três itens seguem em aberto, e estão listados no fim daquele arquivo.


**ABERTA em 04/09/2026, pelo usuário** ("pode seguir para crime-e-recompensa").
Os dois portões que o GOAL exige estão abertos: `posse-no-servidor` FECHOU (o
bloqueio técnico de CR1–CR9 caiu), e as decisões de conteúdo 22–31 de
`DECISOES.md` respondem as perguntas de `tasks.md`. **Spec escrita em
31/08/2026.** Diferente de
`cidades-do-interior`, a medição em `Source/` não achou nada construído por
fora do processo — `Crime`, `Theft`, `Wanted`, `Ownership` e `OwnerId` não
aparecem em lugar nenhum. Este objetivo nasce **bloqueado**, e por dois
motivos ao mesmo tempo: nenhum número de conteúdo foi decidido pelo usuário
(ver `tasks.md`, "Decisões que são do usuário"), e a dependência técnica —
`posse-no-servidor`, que está sendo escrita em paralelo a este mesmo
trabalho — ainda não existe. Toda caixa abaixo começa `[⛔]`, não `[ ]`.

## O objetivo, numa frase

Um pet pode ser roubado numa batalha vencida, a vítima pode recuperá-lo com
ajuda de outro jogador ou da polícia, e o crime deixa rastro — na praça, no
mundo, e na próxima batalha em que o pet roubado aparecer.

## PRONTO é isto, e nada menos

- [x] **CR1** — todo pet do mundo tem um dono explícito (ou "selvagem")
- [x] **CR2** — roubo é consequência de batalha vencida, e é escolha do vencedor
- [x] **CR3** — toda transferência de posse por roubo grava trilha de auditoria
- [x] **CR4** — o poste da praça mostra a lista de procurados
- [x] **CR5** — a recompensa é paga pelo criminoso, nunca do nada
- [x] **CR6** — pet marcado como roubado não pode ser vendido nem trocado
- [x] **CR7** — a marca de procurado sobrevive a desconectar
- [x] **CR8** — o pet roubado se denuncia numa batalha, só para quem viu a lista
- [x] **CR9** — a polícia patrulha, reage e prende
- [x] **CR10** — o pet capturado envelhece, por decisão do usuário primeiro
- [x] `posse-no-servidor` está fechada e CR1 deixou de estar bloqueada por ela
- [ ] Todas as perguntas de "Decisões que são do usuário" em `tasks.md` têm
      resposta registrada antes da tarefa correspondente abrir
- [ ] Bateria completa verde, zero falhas, zero crash
- [ ] As cinco auditorias limpas, incluindo a ausência de PII na trilha de
      auditoria de CR3
- [ ] Um commit por task, cada um com o motivo — não só o quê

**Trocar `[⛔]` por `[ ]` é decisão do usuário, em duas frentes
independentes: fechar `posse-no-servidor` primeiro (bloqueio técnico de
CR1–CR9), e responder as perguntas de conteúdo em `tasks.md` (bloqueio de
CR10, e de detalhes dentro de CR2/CR5/CR9).** Enquanto qualquer caixa
estiver `[⛔]`, `goal_status.sh` pula esta feature.

Enquanto qualquer caixa estiver aberta, o objetivo **continua**.

## Não pare entre tarefas

Depois que as caixas `[⛔]` virarem `[ ]`, `./Tools/goal_status.sh` diz a
próxima. Só se para por três motivos, e todos com a medição junto: decisão
de conteúdo que é do usuário (lista extensa em `tasks.md` — esta feature tem
mais perguntas abertas que qualquer outra do projeto até agora), encostar em
`posse-no-servidor` tentando planejá-la ou implementá-la aqui, e bateria
vermelha que não é do teste novo.

## Invariantes

As doze de `corrente`, mais as duas de `itens-e-biologia`, mais as duas de
`cidades-do-interior`, mais três que esta feature acrescenta:

17. **Nenhuma trilha de auditoria loga PII.** `security.md` §1 é absoluto —
    nem em `debug`. A trilha de CR3 registra IDs opacos, nunca nome, corpo
    de mensagem ou qualquer dado do jogador além do identificador técnico.

18. **Posse é sempre lida de UMA fonte** — a que `posse-no-servidor`
    definir. Criar uma segunda noção de "dono" local para não esperar essa
    spec terminar é o mesmo defeito que L-032/L-033 já custaram por
    duplicar fonte de verdade.

19. **Nenhuma mecânica de assédio nasce sem o contrapeso testado.** CR8 (o
    pet se denuncia em batalha) é a única desta feature com risco explícito
    de virar perseguição entre jogadores reais — o teste do caso negativo
    (quem não tem relação com o roubo não recebe aviso) é tão obrigatório
    quanto o caso positivo.

## O que este objetivo NÃO faz

- **Não implementa `posse-no-servidor`** — apenas depende dela, por nome.
- **Não decide os números e regras de conteúdo** listados em "Decisões que
  são do usuário" de `tasks.md` — isso é conversa com o usuário, não
  suposição de quem executa.
- **Não constrói organização criminosa nem NPCs além da polícia.**
- **Não constrói a Praça/poste inteiros de `cidades-do-interior`** — só o
  mínimo para mostrar a lista de procurados.

## Se o contexto for compactado

Reler este objetivo, `./Tools/goal_status.sh`, `git log --oneline -15`, e
`tasks.md` inteiro (a seção "O que já existe, medido" evita remedir o que já
foi medido — a resposta já é "nada, confirmado por grep"). Verificar primeiro
se `posse-no-servidor` fechou e se alguma pergunta de conteúdo foi
respondida — só então considerar abrir alguma caixa `[⛔]`. **Não recomeçar,
não replanejar, e não tentar decidir sozinho o que está listado em "Decisões
que são do usuário".**

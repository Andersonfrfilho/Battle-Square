# Crime e recompensa — tarefas

## O que já existe, medido (e o que genuinamente não existe)

Diferente de `cidades-do-interior`, aqui a medição não achou trabalho
escondido — achou o vazio que a spec já esperava. Busca por `Crime`, `Theft`,
`Wanted`, `Ownership` e `OwnerId` em `Source/` inteiro: **nada**. Confirmado
também que não existe qualquer marcador de "dono" num pet do mundo — `Pet` é
capturado (`FPetCollectionService::CaptureIfNew`,
`PetCollectionService.cpp:61`) e vira `FOwnedPetInstance` (identidade por
`CatalogId`, `PetCollectionSaveGame.h:15`), mas isso só descreve **o que é
seu**, nunca **de quem é um pet que não é seu**. `APetOwnerView`
(`Battle/PetOwnerView.h`) é a visão do dono DURANTE uma batalha, não um
registro de posse — nome parecido, conceito diferente.

**Isto confirma o bloqueio que a spec já apontava.** A spec
`captura-roubo-e-treinador` está parada porque o conceito de DONO não existe;
a spec `posse-no-servidor` está sendo escrita em paralelo a este mesmo
trabalho para resolver isso pelo lado do servidor (B-005 em `STATE.md`: "a
coleção continua num `USaveGame` local... a posse ainda não é do servidor").
**Esta lista de tarefas depende de `posse-no-servidor` por nome, e não a
planeja.** Toda tarefa abaixo que precisa saber "de quem é este pet" está
marcada bloqueada.

- **Venda de pet no Mercado (`cidades-do-interior`, CI5) NÃO é a mesma coisa
  que roubo.** Venda opera sobre a SUA coleção local. Roubo precisa saber
  que um pet no mundo pertence a OUTRO jogador — é exatamente o que
  `posse-no-servidor` resolve e hoje não existe.
- **O poste da praça (`cidades-do-interior`, CI2/CI7 vizinhos) ainda não
  existe como objeto** — a spec desta feature (linha 104) já prevê ligar a
  lista de procurados a ele: "Ler o poste da praça deixa de ser [...]". Se
  `cidades-do-interior` ainda não tiver o poste construído quando esta
  feature chegar em CR4, a tarefa CR4 constrói o objeto mínimo (texto na
  tela ao entrar no lote da Praça, reaproveitando o gatilho de CI2) — não
  espera a outra feature terminar tudo.
- **Nenhuma trilha de auditoria existe hoje para nenhuma ação sensível** —
  nem precisa existir ainda, porque nenhuma ação sensível (roubo, prisão,
  marcação de procurado) existe. `security.md` §10 exige trilha para toda
  ação sensível; ela nasce junto com a primeira ação real, não antes.

---

## CR1 — Todo pet do mundo tem um dono explícito
> 🤖 Modelo: `opus` 🧠
> **BLOQUEADA por `posse-no-servidor`.** Não iniciar até essa spec definir
> como a posse é representada no servidor — criar `OwnerId` local agora e
> depois migrar para servidor é o mesmo formato de retrabalho que L-032/L-033
> já custaram por duplicar fonte de verdade.

Todo pet vivo no mundo (não só na coleção do jogador) carrega explicitamente
quem é o dono, ou que não tem dono (selvagem). Hoje essa distinção não
existe — um pet no mundo é só um pet.

*Aceite:* dado qualquer pet visível no mundo, existe uma função que responde
"de quem é este" com resposta estável (o mesmo pet responde o mesmo dono em
duas perguntas seguidas) — e "selvagem" é uma resposta válida, não ausência
de resposta.

*Contrapeso obrigatório:* pet do PRÓPRIO jogador, visto por ele mesmo,
responde "seu" — se a função só souber responder sobre pets de outros, a
volta ao próprio pet (ex.: depois de uma reconexão) fica sem dono.

---

## CR2 — Roubo é uma ação de batalha, e só ganha quem vence
> 🤖 Modelo: `opus` 🧠
> Depende de: CR1 (bloqueada).

A spec (linha 36) já recomenda a forma: **"Vencendo o dono — o roubo é
consequência de uma batalha ganha."** Não é ação separada de "furto furtivo"
— é o resultado de vencer uma batalha contra o dono de um pet, escolhido
como consequência declarada da vitória (não automática): o vencedor decide
se rouba ou não.

*Aceite:* o MESMO confronto, uma vez terminando em decisão "roubar" e outra
em decisão "não roubar", produz posse diferente no fim — prova que a escolha
importa, não só a vitória.

*Contrapeso obrigatório:* perder a batalha não pode custar o pet de quem
perdeu além do que a spec decidiu — ver "Decisões que são do usuário" sobre
se o roubo é reversível.

---

## CR3 — Trilha de auditoria para toda transferência de posse
> 🤖 Modelo: `sonnet`
> Depende de: CR1, CR2 (ambas bloqueadas).

`security.md` §10 exige trilha para ação sensível (ator, alvo, IP,
timestamp); §1 proíbe logar PII em qualquer nível. Roubo muda quem é dono de
um pet — é exatamente o tipo de ação que a regra cobre.

*Aceite:* toda transferência de posse por roubo grava um registro com os
IDs opacos das partes (nunca nome, nunca corpo de mensagem) — busca no log
depois do roubo acha o registro, e não acha nenhum campo de PII.

*Contrapeso obrigatório:* o registro sobrevive mesmo se a batalha travar ou
o jogador desconectar no meio — a trilha não pode depender do cliente
terminar a sessão de boa vontade.

---

## CR4 — O poste da praça mostra a lista de procurados
> 🤖 Modelo: `sonnet`
> Depende de: CR2 (bloqueada). Reaproveita o gatilho de prédio de
> `cidades-do-interior` CI2 se ele já existir; senão, cria o mínimo (texto ao
> entrar no lote da Praça) sem esperar CI2 terminar.

A spec (linha 104) descreve a virada: "Ler o poste da praça deixa de ser
[decorativo]" — a lista de procurados vira objeto de jogo, não só texto de
sabor.

*Aceite:* depois de um roubo (CR2), entrar no lote da Praça mostra o pet
roubado e quem o tem — a MESMA informação que existia só em log (CR3) agora
aparece pra qualquer jogador que passe pela praça.

*Contrapeso obrigatório:* praça sem nenhum procurado mostra "sem
procurados", nunca uma lista vazia sem explicação — o mesmo cuidado que
`fallbackMessage` resolve em fluxo de conversa (`conversation-flow.md` §5)
vale aqui: silêncio sem explicação parece bug.

---

## CR5 — Recompensa é paga PELO criminoso, nunca do nada
> 🤖 Modelo: `opus` 🧠
> Depende de: CR1, CR2, CR3 (todas bloqueadas).

A spec propõe recompensa anti-conluio: quem devolve o pet roubado é pago às
custas de quem roubou, não de uma fonte infinita — senão dois jogadores
combinam "eu roubo, você devolve, nós dois lucramos".

*Aceite:* pagar a recompensa DEBITA da carteira/posse de quem roubou (ver
`cidades-do-interior` CI1 para a carteira) — se quem roubou não tiver saldo
suficiente, a tarefa precisa de uma resposta explícita (ver decisão do
usuário abaixo), nunca pagar do nada silenciosamente.

*Contrapeso obrigatório:* devolver o PRÓPRIO pet roubado (o dono recupera
sozinho, sem ninguém mais envolvido) não paga recompensa a ninguém — a
recompensa é para quem RESOLVEU o crime de outro, não para a vítima se
resolvendo.

---

## CR6 — Pet marcado como roubado não pode ser vendido nem trocado
> 🤖 Modelo: `sonnet`
> Depende de: CR1, CR2 (bloqueadas).

Impede que `cidades-do-interior` CI5 (venda no Mercado) aceite um pet com a
marca de procurado — a marca precisa ser visível para QUALQUER sistema que
mexa em posse, não só para o roubo em si.

*Aceite:* o MESMO fluxo de venda que CI5 testa (vender pet capturado) falha
especificamente quando o pet tem marca de procurado, com uma razão visível
("procurado, não pode vender") — não um erro genérico de venda recusada.

*Contrapeso obrigatório:* limpar a marca (devolução, prisão do ladrão, ou o
que a decisão do usuário definir) libera a venda de novo — a trava não pode
ser permanente se a marca em si tem prazo (ver decisões do usuário).

---

## CR7 — A marca sobrevive a desconectar
> 🤖 Modelo: `sonnet`
> Depende de: CR1, CR2 (bloqueadas).

A spec (linha 76) exige: "o procurado **persiste**, e ele reaparece onde
parou." Sem isso, desconectar é forma trivial de lavar um pet roubado.

*Aceite:* marcar um pet, desconectar, reconectar — a marca continua lá, e o
pet reaparece na MESMA posição/estado de quando o jogador saiu, não
reiniciado.

*Contrapeso obrigatório:* reconectar não pode duplicar a marca (uma marca
vira duas registros) — mesmo cuidado de idempotência que `security.md` §6
exige de job com efeito externo.

---

## CR8 — O pet roubado se denuncia numa batalha
> 🤖 Modelo: `opus` 🧠
> Depende de: CR4, CR6, CR7 (todas transitivamente bloqueadas).

A spec (linha 88) chama isto de "o centro do sistema": quando um jogador que
já viu a lista de procurados (CR4) enfrenta quem está com o pet marcado, o
jogo avisa. A própria spec já registra o risco de assédio dessa mecânica
(perseguir alguém só porque o pet dele pisca) — por isso a duração do aviso
precisa ser curta, não persistente.

*Aceite:* dois jogadores em batalha, um com pet marcado — o outro jogador
(SE já tiver visto a lista) recebe o aviso; um terceiro jogador que nunca
viu a lista, na MESMA batalha, não recebe. O aviso desaparece sozinho depois
de um tempo curto, não fica preso na tela.

*Contrapeso obrigatório:* o aviso não pode aparecer para quem NÃO tem
relação nenhuma com o roubo (nem viu a lista, nem é a vítima) — a spec avisa
que isso vira assédio, e é o defeito que o contrapeso testa.

---

## CR9 — A polícia patrulha, reage e prende
> 🤖 Modelo: `opus` 🧠
> Depende de: CR1, CR2 (bloqueadas).

A spec (linha 80) chama a polícia de "o primeiro NPC que ganha [...] história
e roteiro" — é a primeira vez que o projeto tem um NPC com trabalho de
verdade, não decoração de vila. Prisão é o efeito colateral que a spec
recomenda para fechar o ciclo do roubo.

*Aceite:* um jogador com pet marcado, perto de um policial patrulhando, é
identificado e sofre a consequência que a decisão do usuário definir
(prisão, confisco, ou outra — ver seção final); um jogador SEM marca, perto
do mesmo policial, não sofre nada.

*Contrapeso obrigatório:* a prova não pode ser "o policial sempre reage" —
precisa provar também o caso negativo (jogador limpo passa direto), senão
"reage" e "sempre ativo" são indistinguíveis no teste.

---

## CR10 — O pet capturado envelhece, e alguém decide como cuidar dele
> 🤖 Modelo: `opus` 🧠 — **decisão de conteúdo antes de código**
> Depende de: nenhuma técnica — depende só de decisão do usuário.

A spec deixa em aberto se a idade tem fim, se o tempo corre offline e se o
cuidado é ativo ou passivo (ver decisões abaixo). **Não iniciar
implementação até essas três perguntas terem resposta** — qualquer número
escrito agora (idade máxima, taxa de envelhecimento) seria inventado, e a
regra do projeto proíbe inventar número de balanceamento sem medição ou
decisão.

*Aceite (só depois da decisão):* o valor de idade/cuidado aparece
configurável em `DefaultGame.ini`, do mesmo jeito que o tamanho da grade já
vive lá — nunca um número fixo escrito dentro da lógica de envelhecimento.

---

## O que estas tarefas NÃO fazem

- **Não implementam `posse-no-servidor`.** CR1 em diante ficam bloqueadas
  até essa spec (escrita em paralelo) decidir como a posse é representada no
  servidor — implementar posse local aqui e depois portar seria retrabalho
  do mesmo formato que L-032/L-033.
- **Não decidem como o roubo acontece em detalhe** além da recomendação já
  presente na spec (vencer batalha) — duração da marca, se é reversível, e
  quantas vezes um mesmo pet pode ser roubado ficam em aberto.
- **Não constroem organização criminosa, nem NPCs além da polícia.**
- **Não implementam a Praça/poste inteira de `cidades-do-interior`** — só o
  mínimo necessário para mostrar a lista de procurados (CR4); layout,
  direções a outros pontos de interesse são da outra feature.
- **Não decidem se a polícia pode errar** (prender quem não roubou) — a
  spec (linha 164) levanta a pergunta como história possível, não como
  requisito.
- **Não autoram asset de NPC novo** — a polícia usa a mesma convenção de
  mesh primitivo + cor que todo ator do projeto já usa.

## Decisões que são do usuário

- **Como o roubo acontece em detalhe**: só por batalha vencida (recomendado
  na spec), ou existe outra forma? Quantas vezes o mesmo pet pode trocar de
  mão?
- **Duração da marca de procurado** — a spec (linha 159) já avisa que
  "eterna transforma um erro numa pena perpétua". Precisa de um número ou
  regra de expiração.
- **Existe caminho de redenção** para quem roubou (devolver, pagar, cumprir
  pena) além de ser pego pela polícia?
- **Existe organização criminosa** ou o roubo é sempre individual?
- **A polícia pode errar** (prender quem não roubou)? Se sim, existe forma
  de contestar/corrigir?
- **O que a prisão faz de fato** (CR9) — confisca o pet, tranca o jogador
  temporariamente, ou outra coisa?
- **Se quem roubou não tem saldo para pagar a recompensa** (CR5) — a dívida
  fica pendente, é perdoada, ou vira outra penalidade?
- **Idade tem fim?** Se sim, o que acontece quando um pet chega lá.
- **O tempo de envelhecimento corre enquanto o jogador está offline?**
- **Cuidado é ação ativa do jogador ou efeito passivo de tê-lo na
  coleção?**

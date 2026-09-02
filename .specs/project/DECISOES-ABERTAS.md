# As decisões abertas do jogo — lista consolidada

**Montada em 02/09/2026**, lendo as onze features paradas. É a primeira vez que
elas existem numa lista só: até aqui viviam espalhadas, cada arquivo começando
do 1, e por isso não havia como responder "a de número 27".

**A numeração daqui em diante é ESTA.** As respostas de 02/09 vieram numeradas
de 1 a 55 por uma lista que não está no repositório; o casamento abaixo é por
**conteúdo**, e cada linha diz o quanto ela é segura.

| símbolo | significa |
|---|---|
| ✅ | respondida, e o conteúdo casa sem dúvida |
| 🟡 | respondida, mas o casamento é por aproximação — **confirmar** |
| ❓ | o usuário pediu explicação ou sugestão |
| ⬜ | ainda sem resposta |

---

## A — Combate com vários pets (`commit-por-pet`)

**A1. Quantos pets por lado?** ✅ *"poderemos ter vários pets na batalha"* (r1)
→ A CP1 mediu o TETO: a **altura da grade** (3 na grade padrão, até 15 por
eixo). O número dentro disso é seu; ele agora tem onde caber.

**A2. As 3 ações são por pet ou 3 no total?** ✅ *"pode ser dinâmico, poderemos
aumentar no futuro"* (r2)
→ É o que a CP3 implementou: três **por pet**, e `ActionsPerTurn` é constante
num lugar só, então crescer é mudar um número.

**A3. O contrato antigo (commit por LADO) continua aceito na transição?** 🟡
→ Na prática **já respondi executando**: `DuelCommits` e `DuelSlotActions`
traduzem o formato antigo, com prazo de validade escrito no comentário. Se você
quiser recusar o antigo já, elas saem — e aí o fio precisa mandar N commits
desde o primeiro dia.

## B — Fundura da água (`fundura-no-tracado`)

**B1. Onde fica a cintura?** ✅ *"0 → 40% da altura"* (r3)
→ **Vira fórmula, não constante.** As três âncoras medidas (100, 88, e as 30
travessias abaixo de 94) deixam de brigar: a cintura passa a ser 40% da altura
do pet, e um pet miúdo se molha antes que um corpulento na mesma água. Isso é
melhor que qualquer um dos três números fixos.

**B2. A fundura é dado do gerador ou faixa nomeada?** ✅ *"temos a profundidade e
também na propriedade dos personagens"* (r4)
→ Número contínuo por ponto, e a faixa se calcula comparando com a altura de
quem pisa. É a única forma compatível com B1.

**B3. A carta pode mudar de números?** ❓ *"números você diz de elementos?"* (r5)
→ **Não eram elementos.** A pergunta é: hoje o teste da carta afirma **30 vaus,
25 balsas, 0 pontes**. Com fundura de verdade, um rio largo e raso que hoje pede
balsa pode virar vau — e esses números mudam. *Posso mexer nesses números?*
(A sua pergunta sobre catástrofes é outra, e está em E3.)

✅ **RESPONDIDO:** *"claro que precisa; precisa existir pontes sim — pontes de
blocos grandes, pontes de madeira, até mesmo pontes destruídas."*

→ **Isto é maior que permitir a mudança de números.** O teste da carta afirma
hoje **0 pontes**, e o comentário dele diz que *"o zero é uma medição, não uma
ausência de medição"* — ou seja, o traçado nunca produziu ponte nenhuma, e o
teste existe para avisar no dia em que produzir.

Esse dia é agora, e por decisão sua. Três consequências:

1. O traçado passa a **construir ponte**, que ele nunca fez;
2. a carta ganha um número de pontes, e o gabarito muda junto;
3. **material e estado** entram no jogo — bloco, madeira, e **destruída**.

A ponte destruída é a mais interessante das três: ela é uma travessia que
**existe e não serve**, e isso é conteúdo de mapa, não obra. Vira feature
própria (`pontes`), e ela ATRAVESSA `fundura-no-tracado` — as duas mexem em
travessia, e fazer as duas ao mesmo tempo é a única forma de a carta mudar uma
vez só.

**B4. A queda ganha degrau na rocha?** ❓ *"ainda não entendi"*

→ **Reformulando sem jargão:** hoje, embaixo de cada cachoeira, o fundo é
**quase plano** — o buraco tem 886 de largura e só 30 a 51 de fundura. É um
prato, não um poço. Na natureza, água caindo cava um buraco fundo.

A pergunta é: **quero que a rocha tenha um degrau ali**, para o poço ficar
fundo de verdade?

- **Se sim:** dá para mergulhar sob a cachoeira, esconder coisa no fundo, e a
  queda vira lugar. O preço é que a rocha é a camada mais baixa do mundo —
  mexer nela **move a costa, as vilas e as trilhas de tabela**, e a carta muda
  junto (o que a B3 já autorizou).
- **Se não:** a cachoeira continua bonita e rasa, e o mergulho não existe.

**B5. Quem mais passa a ler fundura?** ✅ *"por enquanto isso"* — só o traçado
(o andar). Batalha, pesca e navegação ficam de fora até pedirem.

## C — Cavernas atrás das quedas (`cavernas-nas-quedas`)

**C1. Quantas quedas ganham gruta?** 🟡 *"parametrizável, pensamos nisso
depois"* (r8) — anotado como adiado.

**C2. A gruta da lâmina substitui a de lado ou soma?** ✅ *"soma; e precisa ou
não de ligações ou saídas para outros lugares"* (r12)
→ Soma. E você levantou algo que a spec não tinha: **as grutas se ligam entre
si?** Virou C4.

**C3. O que se vê de dentro?** ✅ *"é efeito visual"* (r13)

**C4. As grutas têm ligação entre si e saídas para outros lugares?** ⬜ *(nova,
levantada por você em r12)*

## D — Cidades, dinheiro e trabalho (`cidades-do-interior`)

**D1. Valor inicial da carteira e como se ganha dinheiro?** ✅ *"o dinheiro é
compartilhado entre os mundos, é uma sociedade econômica; precisamos medir a
quantidade de dinheiro do jogo e a distribuição entre jogadores, npcs e
tarefas"* (r21)
→ Isto é maior que um valor inicial: é **economia com massa monetária
medida**. Vira feature própria.

**D2. Curar custa?** ✅ *"curar é de graça"* (r16)

**D3. Limiar de ranking que abre o Posto de Fronteira?** ✅ *"ganhar campeonato
na cidade; virar líder do centro de treinamento e receber por isso; os desafios
continuam até alguém ganhar de você, inclusive NPC com história própria"* (r15)
→ Também maior que um limiar: é **título defensável**, com renda e NPCs que
evoluem.

**D4. O que machuca/cansa o treinador?** ✅ *"ataques de outros pets e
treinadores, desastres naturais e outros danos"* (r14)

**D5. As vilas e prédios têm nome e placa?** ✅ *"todas têm nome, precisamos de
placas também"* (r18) — e você pediu sugestão de nomes.

**D6. Qual uso do solo vira o primeiro trabalho?** ⬜
**D7. O trabalho tem turno/espera ou é instantâneo?** ⬜
**D8. O Marco de retorno custa dinheiro?** ⬜

## E — Mundo vivo e Mãe Natureza (`mundo-vivo`, `mae-natureza`)

**E1. O tempo corre com o jogador offline?** ✅ *"corre offline, mas vai ter
coisas para fazer"* (r29-31)

**E2. Cuidado é ação ativa ou passiva?** ✅ *"passa, a não ser que você priorize
os pets"* (r33)

**E3. O mundo tem catástrofes?** ✅ *"vamos ter catástrofes"* (r5)
→ Você trouxe isto sozinho, e ele conversa com o abandono de vila (F4).

**E4. Assentamento se compra?** ✅ *"você pode comprar o assentamento, e com
outros"* (r35) — inclusive em sociedade.

**E5. Idade tem fim?** ✅ *"não tem fim"* — o pet envelhece para sempre, sem
morte por idade.

## F — Biomas e ilhas (`mundo-por-biomas`)

**F1. Onde o jogador nasce, e quantas vilas ele vê?** ✅ *"jogadores podem
nascer em outros biomas, então todos devem ser independentes e cheios de
skills"* (r15)

**F2. Pets de região são exclusivos ou só mais comuns?** ✅ *"por elementos do
mapa"* (r53) — o bioma inclina o elemento, e o mapa aumenta atributos.

**F3. Quantas ilhas, e como se ligam?** ⬜ *"vamos decidir ainda; podem ser
ligadas por terra ou ilhas mesmo; hoje todo servidor vê o mapa fixo"* (r51)

✅ **COMPLEMENTADO:** *"pode ser por pontes também"* — e isso amarra com a B3:
a ponte deixa de ser só travessia de rio e passa a poder **ligar ilha a ilha**.

**F4. Limiar de magnitude que abandona uma vila?** ✅ *"por parâmetro,
percentagem, padrão 30%"* (r11)

## G — Montaria e cansaço (`montaria-e-trilhas`)

**G1. Quais pets podem montar?** ✅ *"por tamanho, atributos e skills"* (r44)
**G2. O cansaço se recupera sozinho?** ✅ *"descansando, dormindo, ou médicos
podem ajudar"* (r45)
**G3. Cansaço é a mesma barra que HP?** ✅ *"barra própria"* (r46)
**G4. Voar ignora o custo do relevo?** ✅ *"ignora, mas leva em consideração
temperatura, altitude e gravidade"* (r47)
**G5. Nadar no mundo usa o mesmo verbo da batalha?** ✅ *"todos os golpes
utilizados podem ser usados em batalha; alguns exclusivos, outros comuns"* (r48)

## H — Posse no servidor (`posse-no-servidor`)

**H1. A coleção local vira cache ou desaparece?** ✅ *"fica local, pois vamos
jogar offline"* — o save local continua sendo verdade jogável, e o servidor é a
posse. Some a hipótese de "coleção só no servidor".
**H2. Quem joga offline, o que acontece?** ✅ *"joga o jogo local com NPCs; a
vida acontece normalmente; pode até incrementar jogadores"* (r39)
**H3. Como a conta chega ao jogo?** ✅ *"cadastro via jogo ou portal"* (r40)
**H4. Conta A pedindo o pet de B recebe 403 ou 404?** ❓ *(a resposta dada
descreve a TROCA, que é a H7 — esta pergunta é outra, e menor)*

→ É uma pergunta de segurança, não de jogo: quando alguém pede um pet que não é
dele, o servidor responde **"proibido"** (403, que confirma que o pet existe) ou
**"não encontrado"** (404, que não confirma nada)?

**Minha recomendação: 404.** O 403 conta ao curioso que aquele pet existe, e
isso já é informação. É o padrão para posse por objeto, e não muda nada para
quem joga honesto. Se concordar, respondo por você e sigo.
**H5. Pet abandonado volta a ser selvagem?** ✅ *"selvagem ou adotado"* — as
duas saídas: volta ao mundo, ou outro treinador o adota.
**H6. A coleção tem teto?** ✅ *"quantos quiser, e você salva as informações de
conhecimento e experiência"* (r42)
**H7. Troca entre contas** ✅ *"só se as duas aceitarem; em troca indevida ou
conta hackeada a polícia segura o pet até resolver; verificação de sessão
acontece aqui"* (r41) *(nova, levantada por você)*

## I — Crime e recompensa (`crime-e-recompensa`)

**I1. Como o roubo acontece?** ✅ *"se o pet estiver desmaiado, hipnotizado ou
paralisado"* (r22)
**I2. Duração da marca de procurado?** ✅ *"se for repetitivo você entra na lista
e vira bandido; pode entrar no clã oculto até trocar de roupa e corte de cabelo
e não ser identificado; quem viu o cartaz pode denunciar, dependendo da
moralidade"* (r22)
**I3. Existe redenção?** ✅ *"pode devolver, mas haverá punição; ou caçar
bandidos procurados até chegar na sua recompensa"* (r24)
**I4. Existe organização criminosa?** ✅ *"sim, e elas lutam entre si; vamos ter
ganhos e perdas de quais organizações estão dominando"* (r24, r55)
**I5. A polícia pode errar?** ✅ *"pode ser que coincidentemente um jogador tenha
a mesma característica e pet que o bandido"* (r26)
**I6. O que a prisão faz?** ✅ *"confisca, tranca e multa"* (r27)
**I7. Se o bandido não tem saldo?** ✅ *"dobra o tempo preso; parte do confisco e
da multa vai para o usuário roubado; a recompensa quem paga é o governo"* (r27)
**I8. Vender pet roubado?** ✅ *"pode, mas roubado só no mercado-negro"* (r20)

## J — Segredos e a carta (`segredos-e-a-carta`)

**J1. A carta é o mapa do jogador?** ❓→✅ *"a carta seria o mapa? se sim,
conforme o usuário anda vai abrindo, e pode comprar partes do mapa por
região"* (r9)
→ **São duas coisas hoje**, e a sua resposta as une: a *carta* é o gabarito de
aceite (o mundo tem de bater com ela); o *mapa do jogador* já abre andando
(`WorldDiscovery`). Comprar região é novo. ✅ **Confirmado.**

**J2. A carta conta os segredos?** ✅ *"sim"* (r10)
**J3. Contar ainda deixa ser segredo?** ✅ *"sim"* (r10)
**J4. Quantos segredos, e de que tipo?** ✅ *"lugares escondidos, itens, pets
raros…"* — três tipos. A quantidade fica em aberto, e a `SC1` já prevê que a
carta diga a SOMA do que ela não mostra.

---

## K — A POSSE MUDA DE MÃO POR TRÊS PORTAS

**Levantado por você em 02/09, e é a coisa mais estrutural desta lista.**

Suas respostas descrevem **três** caminhos pelos quais um pet troca de dono, e
eles estavam espalhados por três features diferentes:

| porta | onde estava | o que você decidiu |
|---|---|---|
| **TROCA** | H7, `posse-no-servidor` | só se as duas contas aceitarem |
| **VENDA** | I8 + o Mercado de `cidades-do-interior` | pode vender; **roubado só no mercado-negro** |
| **ROUBO** | I1, `crime-e-recompensa` | só de pet desmaiado, hipnotizado ou paralisado |

### Por que isto é uma coisa só, e não três

As três fazem **exatamente a mesma operação**: tirar o pet de um dono e dar a
outro. Se cada feature implementar a sua, serão três transferências que
concordam **até a primeira edição** — e uma delas vai esquecer a trilha de
auditoria, ou a marca de roubado, ou a verificação de sessão.

É o defeito que L-032 e L-033 já custaram a este projeto por duplicar fonte de
verdade, e a `crime-e-recompensa` já o antecipa na invariante 18: *"posse é
sempre lida de UMA fonte"*.

**Uma transferência só, e as três portas entram por ela.** Cada porta traz a sua
condição — a troca exige aceite dos dois, a venda exige preço, o roubo exige o
pet indefeso —, mas quem move a posse é o mesmo código, com a mesma trilha.

### O que decorre disso, e que você já respondeu

- **A marca de roubado viaja com o pet** (I8): se ela morasse na transferência,
  vender apagaria o roubo. Ela é do PET.
- **A polícia segura o pet até resolver** (H7): a transferência precisa saber
  ficar *pendente*, e não só acontecer ou falhar.
- **Parte do confisco vai para o roubado** (I7): a transferência mexe em
  dinheiro, não só em posse.

### O que ainda falta decidir

**K1. Vender para NPC, para jogador, ou os dois?** ✅ *"pode vender para os
dois"*
→ A transferência precisa aceitar **contraparte de dois tipos**. O NPC compra
sempre (é ele que dá liquidez ao preço tabelado); o jogador compra se quiser, e
aí a venda vira uma troca com dinheiro de um lado.

**K2. Preço livre ou tabelado?** ✅ *"tabelado"*
→ Sai de `SettlementEconomy`, como a regra da casa exige. Preço escrito à mão
numa tarefa de venda recriaria o defeito que o próprio comentário do módulo já
nomeia (o raio da ilha).

⚠️ **Tabelado com NPC comprando sempre é uma torneira de dinheiro**, e ela
alimenta a massa monetária que você mandou medir na D1. As duas se conversam:
quem tabela o preço decide quanto dinheiro entra no mundo por hora.

**K3. O mercado-negro é lugar ou estado?** ✅ *"é lugar"*

→ **E isso faz duas features se encontrarem.** Lugar entra na carta, porque o
gabarito exige que o mundo bata com ela — e um mercado-negro que a carta anuncia
é um mercado-negro **com placa**, que não é mercado-negro.

A saída não é exceção: é a `segredos-e-a-carta`, que você já respondeu. Ela
existe justamente para a carta dizer *"há coisas que eu não mostro"* sem dizer
quais. O mercado-negro vira o **primeiro cliente real** dela:

- é **lugar** (K3), então tem posição no mundo;
- é **escondido** (J4: *"lugares escondidos"*), então a carta o CONTA sem o
  apontar;
- e o mapa do jogador o revela **andando** (J1), como todo o resto.

Sem os segredos, o mercado-negro precisaria de uma exceção no gabarito — e
exceção no gabarito é o começo do gabarito não valer.

**K4. Quantos mercados-negros, e onde?** ⬜ — depende de quantos segredos (J4),
que ficou em aberto.

---

## O que ficou sem par

**Sem resposta na sua lista:** as de número 19, 23, 25, 28, 32, 38, 43, 49, 50 e
54 da numeração antiga — não sei a que perguntas correspondiam.

**Você respondeu "não entendi":** 6, 17, 34 e 52. Sem a lista original, não sei
o que reformular. Se aparecerem, reformulo com contexto.

**Você pediu sugestão:** 7 — *"o que você sugere, como as empresas fazem?"*

**Assuntos que você trouxe e não estavam em spec nenhuma:** economia com massa
monetária medida (D1), título defensável de líder de centro (D3), ligação entre
grutas (C4), troca entre contas com polícia (H7), guerra entre organizações
(I4).

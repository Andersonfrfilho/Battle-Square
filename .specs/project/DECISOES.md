# Decisões do usuário — respondidas em 02/09/2026

**Este arquivo é a fonte única.** As features NÃO copiam as respostas; elas
apontam para cá. Duas cópias concordam até a primeira edição (L-032).

Numeração conforme a pergunta feita. Onde a resposta abriu trabalho que
nenhuma task cobre, está marcado **🆕 ESCOPO NOVO**.

---

## Batalha — `commit-por-pet` (frente ativa)

**1. Quantos pets por lado.** Vários. O número não é fixo: o sistema tem de
aguentar N, e a CP1 mediu o teto de hoje.

**2. Quantas ações por turno.** **Dinâmico, não constante** — hoje três, e o
produto vai aumentar. Portanto o número é configuração, e nenhum teste pode
afirmar "três" como literal.

---

## Água e fundura — `fundura-no-tracado`

**3. Onde fica a cintura.** **Não é número fixo: é fração da ALTURA de quem
atravessa — 0 a 40% da altura é o que se faz a pé.**

> **Medição (02/09/2026):** o `ACharacter` do jogador usa a cápsula padrão,
> meia-altura **88** → altura cheia **176**. 40% de 176 = **70,4 unidades**.
> O limiar de hoje é `WadableDepthUnits() = 100`, que são **56,8% da altura** —
> altura do peito, não da cintura. **A regra nova é mais restritiva**, e
> reclassifica as travessias entre 70,4 e 100. Quantas são exatamente, medir
> na F1: hoje só se sabe que as 30 de vau estão todas abaixo de 94.

**4. Contínuo ou faixa nomeada.** A fundura é dado do mundo **e** propriedade
do personagem: o limiar sai de `0,40 × altura DELE`. Um pet baixo atravessa
menos fundo que o jogador, sem regra nova — só a mesma conta com outra altura.

**5. O gabarito é SAGRADO — e alguns elementos mudam.** `ChartConformance`
continua sendo a autoridade: o mundo não pode divergir dele por acidente.
Mudar uma entrada é **ato deliberado e revisado**, entrada por entrada — nunca
atualização automática do teste para "ficar verde". O mundo é dinâmico, com
âncoras fixas para equilíbrio de onde as coisas nascem, e existem catástrofes.

**6. O poço da cachoeira ganha ROCHA: degraus, e mais.** Além do degrau, o
fundo recebe **outros itens e plantas aquáticas** — 🆕 **e vão existir PETS
AQUÁTICOS**, que hoje não existem em lugar nenhum do projeto.

**7. A fundura passa a ter fonte ÚNICA** (sugestão aceita): vira campo do plano
assado, e traçado, batalha, natação, pesca e bicho leem dali. **A estimativa
`FunduraSobreLargura = 0.065f` morre no MESMO commit em que a leitura nasce** —
duas fontes convivendo é o que causou L-032 e L-033 neste projeto.

---

## Segredos e mapa — `segredos-e-a-carta`

**8. Quantos segredos.** **Parametrizável**, número depois.

**9. 🆕 ESCOPO NOVO — o mapa do jogador (CONFIRMADO).** O mapa **abre conforme
se anda**, e **partes do mapa podem ser COMPRADAS por região**. Feature própria
(névoa de guerra + comércio de cartografia); a SC3 só previa revelar andando.
Não confundir com o gabarito da decisão 5 — são coisas diferentes.

**10. Contar não deixa de ser segredo.** Sim: a contagem pode existir sem
entregar onde.

---

## Cavernas — `cavernas-nas-quedas`

**11. Quantas quedas ganham gruta.** **Por parâmetro, default 30%** das 13.

**12. Soma, não substitui** — a gruta de trás soma à de lado. **E precisa
decidir se ela tem ligação ou saída para outros lugares** (registrado como
pergunta derivada, não respondida).

**13. A lâmina de água é efeito visual.**

---

## Cidades — `cidades-do-interior`

**14. O que machuca o treinador.** Ataques de outros pets, ataques de outros
treinadores, desastres naturais e outros danos.

**15-d. A I.A. SIMULA VOCÊ (03/09).** *"Se você decide ficar na academia, tem
a opção de batalhar ou deixar a I.A. batalhar por você; se está longe, a I.A.
batalha por você com seus pets, simulando você lá, baseando nos seus
movimentos de jogo."* Duas metades:

- **O ESTILO é colhido das batalhas reais**: cada commit do jogador soma no
  perfil (`ActionStyleCounts`), e a I.A. escolhe os TIPOS de ação por roleta
  pesada nesse histórico — quem ataca sempre é simulado atacando. A direção
  continua do bot: o que vale simular é a tendência, não a coordenada de cada
  golpe antigo. Suavização de +1: histórico vazio joga uniforme (estilo nenhum
  é um estilo), e nenhum tipo morre por nunca ter sido usado — estilo é viés,
  não mordaça.
- **AS SEQUÊNCIAS também (03/09, tarde)**: além da tendência, a I.A. aprende
  a ORDEM — "depois de X veio Y", com o início do turno como estado próprio
  ("abro defendendo"). Guardadas esparsas no perfil (matriz achatada quebraria
  o save no primeiro golpe novo — o enum cresce no fim), colhidas do MESMO
  commit da tendência, e compostas por MULTIPLICAÇÃO tendência × sequência —
  o desenho de fase × lugar da fauna. O elo é o que ACONTECEU, não o que se
  quis: Mover que degradou para Aguardar encadeia como Aguardar.
- **PRESENTE também escolhe**: `bs.Desafiar auto` deixa a I.A. jogar com você
  ali — e o desfecho passa pela MESMA função da batalha jogada (prêmio, ponto,
  título, fila): conveniência não muda consequência. Os três caminhos (jogado,
  auto presente, auto ausente) desembocam em `ApplyArenaOutcome`, um só.

**15-c. A DEFESA EM AUSÊNCIA — o posto cobra resposta, não prisão (03/09).**
*"Se ele sair, manda um aviso: qualquer usuário que desafiá-lo, ele pode optar
por batalhar ou deixa no automático."* A tranca da 15-b CAIU: o líder viaja e
desafia onde quiser (vitória fora rende prêmio, nunca segundo título — líder é
de UM centro). O desafiante chega com o dia e ESPERA (fila de um lugar); ao
sair da própria Arena, o aviso diz as duas saídas. `bs.DefesaAutomatica on`
liga o automático — e o automático é o **BattleSim jogando de verdade**, bot
contra bot com a IA que já existe, determinístico pela semente do dia. Empate
mantém o posto (ninguém perde título para um relógio), e falha de dado (espelho
ausente) NUNCA vira derrota: o desafiante volta para a fila.

**15-b. LIDERANÇA DE CENTRO — implementada (03/09), com a leitura registrada.**
Vencer o campeonato da Arena TOMA o título do centro. A frase "não pode sair
nem aceitar desafios até alguém vencê-lo" foi lida assim (leitura minha,
reversível): o posto PRENDE — o líder não desafia OUTRAS arenas enquanto o
tem, e no próprio centro o desafio vira DEFESA contra o desafiante do dia
(seedado pelo dia DE PROPÓSITO: "inclusive NPC" pede fila, não estátua).
Perder a defesa é como o título muda de mão. A renda ("com renda por isso") é
diária, fração do prêmio da tabela, paga NA Arena do posto — só a de hoje,
nunca as perdidas: o posto cobra presença. ⚠️ Aberto e nomeado: título
expirar por defesa NÃO atendida (hoje só batalha muda a mão), e a renda
offline (espera a idade do mundo da `mundo-vivo`).

**15. 🆕 ESCOPO NOVO — especialização e liderança de centro.** As cidades
especializam, MAS todo bioma tem de ser jogável de nascença: **jogadores podem
nascer em qualquer bioma, então todos são independentes e cheios de skills.**
Ganhar o campeonato de uma cidade e de um centro de treinamento **torna o
jogador LÍDER do centro**, com renda por isso — e **ele não pode sair nem
aceitar desafios até alguém vencê-lo**, inclusive NPC. **NPCs têm histórias
próprias e evolutivas.**

**16. Curar é de graça.**

**17. NÃO EXISTE TELETRANSPORTE no jogo.** ⚠️ Isso **invalida a premissa** da
task do "Marco de retorno" em `cidades-do-interior`: ela nasceu supondo viagem
rápida. O Marco vira outra coisa, ou sai.

**18. Toda cidade tem nome.** As **placas** sinalizando posições ficaram
ambíguas: a primeira resposta as pediu, a segunda disse "acredito que não".
**Pendente de uma palavra** — ver o fim do arquivo.

**19. TERRA ganha "Atolar"** (sugestão aceita): prende o alvo na casa por um
turno, sem dano. Reusa lama de casa, barrar movimento e postura — três
mecânicas já provadas. **Mais skills de terra virão depois.**

**20. Comprar e capturar coexistem.** Pets podem ser capturados e vendidos —
**inclusive os roubados, mas roubado só se vende no MERCADO NEGRO.** 🆕

**21. 🆕 ESCOPO NOVO — massa monetária medida.** O dinheiro é **medido e
compartilhado entre os mundos**: é uma sociedade econômica. Precisa existir
medição da **quantidade de dinheiro do jogo** e da **distribuição dele entre
jogadores, NPCs e tarefas.** Isso é maior que "saldo inicial da carteira".

---

## Crime — `crime-e-recompensa`

**22. Como o roubo acontece.** Ataca-se o TREINADOR e toma-se o pet **se ele
estiver desmaiado, hipnotizado ou paralisado**. Repetir põe o jogador na lista
de procurados e o torna bandido — **e abre o clã oculto dos bandidos.** 🆕

**23. Duração da marca.** Dura **até o jogador mudar de aparência** (roupa,
corte de cabelo) a ponto de não ser identificado com o pet. Mas **quem viu o
cartaz reconhece**: jogador tem a OPÇÃO de denunciar; **NPC sempre denuncia,
conforme a moralidade dele.** 🆕 (exige aparência do personagem como dado, e
memória de quem viu o cartaz.)

**24. Redenção.** Devolver é possível, **com punição** — ou **caçar bandidos
procurados** até cobrir a própria recompensa.

**25. Organizações criminosas existem, e LUTAM ENTRE SI** — com ganho e perda
recorrente de domínio entre elas. 🆕

**26. A polícia erra**, quando um jogador coincide em características **e pet**
com o bandido procurado.

**27. A prisão confisca, tranca e multa.** **Parte do dinheiro do confisco e da
multa vai para o usuário roubado**, no valor do que ele perdeu.

**28. Sem saldo para a multa, DOBRA o tempo preso.** A recompensa sempre
existe: **quem paga é o governo.**

**28-b. A polícia ESCALA com quem a vence (04/09).** Cada policial que o
suspeito derrota eleva o nível do PRÓXIMO enviado — o calor sobe a cada
vitória sobre a lei, nunca desce sozinho. O primeiro policial vem no nível do
próprio suspeito (briga justa); a partir daí, cada derrota da lei soma um
degrau à força do reforço seguinte. O calor é contado das DERROTAS, não do
relógio: dois suspeitos com o mesmo histórico enfrentam a mesma polícia. 

**28-c. A polícia anda em DOIS, e o que escala é o TIPO de força (04/09).**
Todo destacamento é uma dupla — nunca um policial solitário. O que o calor
sobe é a PATENTE da força enviada, numa escada de cinco: **ronda → ostensiva →
investigadora → tropa de elite → tropa federal**. Cada derrota da lei sobe um
degrau; a escada TERMINA na federal (topo), mas dali o nível dos pets continua
subindo — federal nunca vira briga fácil. O primeiro destacamento é uma dupla
de ronda no nível do suspeito.

**29–31. O envelhecimento corre offline** — e **a prisão tem o que fazer
dentro.** 🆕

---

## Mundo vivo — `mundo-vivo`

**32. O pet morre de velho.**

**33. O tempo passa para o pet mesmo offline — a não ser que ele seja
CRIOPRESERVADO.** 🆕 (criopreservação não existe hoje.)

**34. O cuidado é ATIVO, e conviver conta.** Dar atenção e **viver junto**
reduz drasticamente o envelhecimento e **faz o pet viver mais**. Cuidar não é
efeito passivo do tempo.

**35. A plantação é do ASSENTAMENTO** — e **o assentamento pode ser comprado,
inclusive em sociedade com outros.** 🆕

**36. A ferramenta importa** no que se colhe.

**37. O pet ajuda a coletar** (não é o jogador sozinho).

---

## Posse — `posse-no-servidor`

**38-b. OFFLINE NORMAL exige o LOCAL (03/09).** *"Lembrando que o jogo deve
rodar normalmente offline."* Isto emenda a 38: a coleção local NÃO desaparece —
ela é o CACHE que mantém o jogo inteiro sem rede (a recomendação da spec, agora
decidida). Offline joga, captura e grava local; ao reconectar, sobe — e **o
servidor manda no empate**. As três decisões restantes da posse (03/09):

- **conta por CONFIG agora, tela de login depois** — token em configuração
  local para desenvolvimento; a tela é feature futura registrada; quem não tem
  conta joga offline normal;
- **recusa por objeto é `403`** — honesto ("existe, mas não é seu"), escolhido
  pelo dono SOBRE a recomendação de 404; o custo assumido: a rota vira oráculo
  de existência de pet, e fica anotado;
- **teto de 500 pets por conta**, como config do servidor — captura além do
  teto recusa com código estável.

**38 (original). A coleção local DESAPARECE** (não vira cache).

**39. Offline joga-se local, com NPCs** — a vida acontece normalmente, e **pode
até haver incremento de jogadores.** 🆕

**40. A conta entra por cadastro**, no jogo ou no portal.

**41. Troca entre contas** exige **autorização das DUAS** (reafirmado). Havendo
relato de troca indevida ou conta invadida, **a polícia segura o pet até
resolver**, e **verificação de sessão acontece aqui** — é onde dá para perceber
que algo está errado. 🆕

*Decisão de engenharia, tomada pelo assistente por ser técnica e não de produto:*
**`404`**. Responder `403` confirma a quem perguntou que aquele pet existe, e
transforma a rota em ferramenta de enumeração de coleção alheia (OWASP API1,
BOLA). O dono legítimo nunca vê `404`, então nada se perde. Reversível se o
usuário preferir o contrário.

**42. Pet abandonado volta a ser selvagem.**

**43. A coleção NÃO tem teto.** E guarda-se **o conhecimento e o ganho de
experiência** de cada um. 🆕

---

## Montaria — `montaria-e-trilhas`

**44. Monta-se por tamanho, atributos e skills.**

**45. O cansaço se recupera descansando, dormindo, acampando e comendo** — e
**alguns pets ajudam na recuperação.** 🆕

**46. Barra própria** (não é a de HP). Confirma a recomendação das tasks.

**47. Voar ignora o custo do relevo**, mas **leva em conta temperatura,
altitude e gravidade.** 🆕

**48. Nadar no mundo usa os mesmos golpes da batalha** — todo golpe usado no
mundo pode ser usado em batalha.

---

## Biomas — `mundo-por-biomas`

**49. O nascimento é aleatório.**

**50. Há pets exclusivos de bioma E pets comuns a vários.**

**51.** *(Adiada pelo usuário: "vamos decidir ainda". Registrado: as ilhas podem
ligar-se por terra ou continuar ilhas; hoje todo servidor vê o mesmo mapa fixo.)*

**52. Vila NÃO se abandona: ela se RECONSTRÓI.** A única exceção é **morrer
toda a população**. ⚠️ Isso **inverte a premissa da MB4**, que procurava um
limiar de magnitude para abandono — o limiar não existe.

---

## Deuses — `mae-natureza`

**53. A faixa-alvo é por ELEMENTOS DO MAPA** (não por ilha nem por região
administrativa). *(MN6 reconciliada 04/09: a
implementacao usa a faixa como config agnostica de escopo, rotulada por
elementos do mapa conforme esta decisao — a resposta "por regiao" da pergunta
cede a decisao ja registrada.)*

**54. Dá para agradar os deuses: existem TEMPLOS, e orar AUMENTA os atributos
do personagem** — confirmado, e vale para os atributos. 🆕

**55. Os deuses discordam — e podem entrar em GUERRA.** 🆕

---

## Ainda em aberto — TRÊS itens

**Respondidas na segunda rodada:** 5, 6, 7, 9, 17, 19, 34, 52, e 41 (a parte de
produto). Restam três, e duas por escolha do usuário:

| # | o que falta |
|---|---|
| 8 | quantos segredos — **adiada pelo usuário** ("pensamos nisso depois") |
| 18 | **placas** no mundo: pedidas na primeira resposta, "acredito que não" na segunda |
| 51 | quantas ilhas e como se ligam — **adiada pelo usuário** ("vamos decidir ainda") |

## Duas premissas que caíram, e as tasks precisam mudar

- **17 mata a viagem rápida.** A task do Marco de retorno em
  `cidades-do-interior` supunha teletransporte. Reescrever ou remover.
- **52 inverte a MB4.** Ela procurava o limiar de magnitude que abandona uma
  vila; não há abandono — há reconstrução, exceto se a população inteira
  morrer. A task passa a ser sobre RECONSTRUÇÃO.

---

## Respostas de 03/09/2026 — `cidades-do-interior`

**56. A carteira começa com 100.** ⚠️ *Delegada a mim* ("quanto vc recomenda?"),
e o número não é redondo por acaso: são **quatro curas na cidade**
(`CuraNaCidade = 25`) e pouco mais que **uma venda de pet** (`CidadePagaPeloPet
= 70`). Dá fôlego para a primeira viagem para fora da vila inicial sem tornar a
primeira venda irrelevante — que é o que 250 ou 500 fariam.

**57. Paga a Arena, o trabalho e o ACHADO.** Batalha comum não paga. Achar
dinheiro no mundo é o item novo, e ele dá motivo para sair da trilha — que é
justamente o que ruína, clareira fechada e mercado-negro precisavam para
existirem como destino.

⚠️ **O achado é conteúdo que nenhuma task previu.** Ele não cabe na CI1: entra
como task própria, depois que a carteira existir para receber.

**58. O Portão do Posto de Fronteira NÃO TRANCA.** *"o usuário deve ter
liberdade para viajar para onde quiser"*.

⚠️ **Isso reescreve metade da CI8.** A caixa dizia "o ranking existe e
**tranca/destranca** o Portão" — a segunda metade cai. O ranking continua
existindo e aparecendo na tela; o que ele deixa de ser é uma cancela.

E isto conversa com a **17**: as duas respostas dizem a mesma coisa por lados
opostos — não se pula distância, e não se é barrado nela. Andar é o custo, e é o
único.

**60. O Marco vira PONTO DE RENASCIMENTO, e quem cai acorda no hospital.**
Resolve o que a decisão 17 deixou aberto: o Marco não teleporta ninguém por
vontade própria — ele marca ONDE se volta quando se perde.

E o lugar é o **Centro de Recuperação**, que casa com a decisão 16: acorda-se
curado, de graça. O prédio que não tinha porta passa a ser o prédio para onde
todo mundo é levado.

⚠️ **Isto reabre a 17 por uma fresta, e a fresta tem nome: morrer de
propósito.** Se acordar levasse de volta à vila natal, morrer seria viagem
rápida de graça — exatamente o que a 17 proíbe, só que pela porta dos fundos.

Leitura minha, e é a mais barata que fecha a fresta: **acorda-se no Centro de
Recuperação MAIS PERTO de onde se caiu**, não no de casa. Assim renascer nunca
poupa caminhada — ele devolve o jogador ao mundo, não ao começo. Se você
preferir que seja sempre a vila natal, é uma linha, e a fresta é aceita de
propósito.

**61. A VIDA DO PET PERSISTE, e mora no registro dele.** *"a vida do pet
persiste, decida você onde mora — pode morar o registro dele nas propriedades
dele"*. Vai para `FOwnedPetInstance`, ao lado da biologia, com o mesmo cuidado
de save antigo: **ausente é "cheio"**, nunca "zero" — um campo novo que
matasse os pets de quem já jogava seria o defeito que o comentário de
`BiologySkin` existe para impedir.

O resto foi delegado, e as escolhas são minhas, nomeadas: guarda-se
**porcentagem** (o teto muda com o nível, e a fração sobrevive à subida); a
vida final da batalha é a que fica; **derrota acorda CURADO** (é a decisão 60
somada à 16 — o hospital cura de graça, e é para lá que se acorda); e batalha
nunca COMEÇA com zero — piso de 1, porque uma luta perdida antes do primeiro
turno não é luta.

**65. CASAS SE VISITAM, E TÊM GENTE DENTRO.** *"jogadores podem visitar casas
e conversar com npcs"*. Isso derruba — de propósito — a regra que o código
carregava: "casa sem função não tem porta". A regra CONTINUA; o que mudou é
que a casa ganhou função: visitá-la e conversar com quem mora. A porta vem
junto, no mesmo commit.

O primeiro morador é o mínimo honesto: cada casa (e cada palafita — casa
sobre a água) tem UM morador fixo, com nome sorteado da geometria do lugar
(nunca do relógio — vizinho que troca de nome a cada visita não é vizinho), e
ele diz UMA coisa VERDADEIRA sobre o mundo: cada dica do repertório aponta uma
mecânica que existe e tem teste. Morador não fala de segredo — a carta conta e
não aponta, e o vizinho tagarela seria a exceção pela porta dos fundos (J4).

🆕 O diálogo FUNDO — história própria, memória, evolução — já era escopo da
decisão 15 ("NPCs têm histórias próprias e evolutivas"); esta é a fundação:
o morador existe, tem nome estável e tem voz.

**Ampliada em 03/09** (*"acredito que casas devam ter portas, ou outros tipos
de portas qualquer coisa"*): a porta ganha CORPO — um vão desenhado na fachada,
virado para a praça, em malha procedural e cor como todo o resto do mundo. O
gatilho invisível continua sendo quem AVISA; a porta desenhada é quem CONVIDA:
prédio com função se lê de fora, sem precisar encostar.

**E emendada duas vezes na mesma conversa:** *"às vezes não é legal você
entrar na casa de todo mundo a hora que quiser"* e *"se não tiver ninguém não
tem porque você ser convidado"*. A casa NÃO é do visitante: o morador tem
JANELA de estar em casa (10 a 16 horas por dia, seedada como o nome — nunca o
dia inteiro, nunca dia nenhum), lida do MESMO relógio do céu. Fora dela,
ninguém atende — e a recusa diz o horário em que ele costuma estar, para a
visita frustrada virar plano em vez de porta morta.

**66. OS MORADORES ANDAM PELA VILA.** *"os npcs podem andar pela vila tbm,
isso é perfeito o que vc propôs dos horários"*. A janela de horário vira dupla:
quando o morador NÃO está em casa, ele está NA RUA — corpo procedural
(cilindro e esfera, roupa seedada como o nome), passeando pela praça com o
MESMO componente de passeio dos encontros. Uma janela só decide as duas
coisas: quem atende a porta e quem se vê na rua — duas fontes da mesma
verdade fariam a vizinha bilocada.

E a conversa acompanha: o morador de rua CUMPRIMENTA quem chega perto, com a
mesma dica que daria em casa — é a mesma pessoa.

**64. A ESCOLA É O LUGAR DE APRENDER.** *"na escola você aprende sobre os
pets, batalhas, aprende golpes e aumenta skill com treinamentos dos seus
pets"*. Três metades, e cada uma tem um destino:

- **aumentar skill treinando** → é o PÁTIO da decisão 63, já de pé;
- **aprender SOBRE pets e batalhas** → a Escola vira o quadro de lições:
  entrar nela mostra os golpes do seu pet — os que ele já alcança e os
  trancados, com o requisito e o campo do pátio que o treina. Nenhum dado
  novo: o registro assinado já carrega `RequiresAttribute`/`RequiresValue`, e
  a regra (`IsMet`) já existia — faltava um lugar fora da batalha que a
  mostrasse;

  **Ampliada em 03/09** (*"mostra estratégia tbm, limitações, terrenos e
  dicas… é bom contra… ruim contra… cuidado com terrenos"*): o quadro ganhou
  a ESTRATÉGIA do tipo — bom contra, ruim contra, e "apanha feio de" (só o
  golpe 150+, porque aviso de tudo é aviso de nada) — lida da MESMA tabela de
  efetividade da batalha; e os avisos de TERRENO, cada frase amarrada ao
  teste de BattleSim que a prova (dano de casa que voar escapa e camuflar
  não, submergir que exige água funda, gelo que nega, lama que é aposta).
  Frase sem teste correspondente não entra: a escola só ensina o que a
  batalha cobra;
- 🆕 **ESCOPO NOVO — "aprender golpes" como mecânica** (golpe que o pet só
  ganha estudando, além do kit do elemento): muda o desenho de desbloqueio,
  conversa com a decisão 19 ("mais skills de terra virão") e é feature
  própria — registrado para não se perder.

**63. A ESCOLA TEM os campos de treinamento — e torneios, e ranking de
treinadores.** *"escolas devem ter os campos de treinamento e até torneios e
ranking de treinadores"*. Resolve a CI4 pelo caminho oposto ao da task: em vez
de a Escola chamar um campo a vinte mil unidades, ela ganha um PÁTIO — os
cinco campos, miúdos, na faixa da clareira atrás dela. `bs.Especializar` e o
treino passam a funcionar ali, pela mesma função de sempre.

🆕 **ESCOPO NOVO — torneios e ranking de treinadores na escola.** Conversa
direto com a decisão 15 (campeonato torna o jogador LÍDER do centro, com renda
e desafios). É feature própria, não uma task desta — registrado para não se
perder.

**67. CONVERSA LIVRE COM O NPC — modelo de fala.** ✅ *Respondida em 03/09:
"podemos na nossa infra ter nosso próprio modelo, para evitar gastos — local
restrita, e se ele tiver online a conversa é mais dinâmica."*

**O lado do JOGO está pronto:** `bs.Falar <texto>` conversa com quem está te
ouvindo (a casa que atendeu, ou o passante mais perto — a visita nunca é
atropelada pelo passante). Sem endpoint configurado, o MODO RESTRITO responde
na hora: o morador admite o limite ("disso eu não sei falar...") e devolve o
que sabe — a história dele, reagindo aos feitos. Com
`NpcDialogueEndpointUrl` no `DefaultGame.ini`, o digesto vai por POST ao
modelo da infra (contrato JSON documentado em `NpcDialogue.h`) e a resposta
volta em personagem — com timeout de 3 s e QUALQUER falha caindo no restrito:
a conversa nunca morre por causa da rede.

**A fronteira que o teste guarda:** o digesto é TUDO que o modelo sabe, e
nenhum segredo entra nele — afirmado no negativo. O que não entra não sai na
fala, por construção: é a regra da carta valendo para o NPC mais eloquente.

~~O que falta é da INFRA~~ **O servidor de referência EXISTE**
(`Server/conversa/`, 03/09): Bun + zod, o contrato do jogo na frente de
qualquer modelo OpenAI-compatível — llama-server local e infra usam o MESMO
código, só muda `MODEL_URL`. Sem estado de propósito (memória é do jogo), erro
rápido de propósito (o restrito é o para-quedas), e o prompt — a única peça
com regra — é a peça com teste: só os fatos do digesto, admitir o que não
sabe, uma a três frases de gente do interior.

**Emendada em 03/09** (*"dê a opção para ele"* + *"e informe das necessidades
e diferenças"*): a escolha é DO JOGADOR — `bs.Conversa` vê, aponta e desliga a
dinâmica em jogo, persistindo por `SaveConfig` (escolha que evapora ao fechar
não é escolha). O endpoint pode ser a infra oficial OU um modelo local em
`localhost` — o jogo não distingue, e é o ponto: o mesmo contrato serve os
dois. As necessidades aparecem na tela ao consultar o modo, e o guia completo
— diferenças, RAM, download, o adaptador do llama.cpp — vive em
`docs/conversa-dinamica.md`.

**É viável, e a forma honesta é esta:** um serviço de diálogo FORA do jogo
(no servidor que `posse-no-servidor` já planeja), chamado quando o jogador
puxa conversa. O prompt leva (a) a FICHA do morador — nome, arco, janela,
quantas visitas — e (b) um DIGESTO de fatos VERDADEIROS do jogo: os feitos do
save, o gabarito da carta, as mecânicas com teste. O modelo responde EM
PERSONAGEM, e a regra de sempre vale dobrada: ele só afirma o que o digesto
contém — fato mecânico falso não entra nem por eloquência.

**O que fica decidido por arquitetura, antes de qualquer código:**
- os ganchos determinísticos CONTINUAM sendo a base: offline joga-se local
  (decisão 39), e conversa que exige internet não pode ser a única conversa;
- **custa dinheiro por conversa** (chamada de modelo) — precisa de teto por
  jogador/dia e de decisão de orçamento, que é do dono;
- precisa de moderação de saída (NPC não xinga, não promete recurso que não
  existe, não aponta segredo — o filtro do digesto cobre o último);
- latência: a resposta chega em segundos, não em quadros — a UI de conversa
  precisa comportar espera.

**Parado aguardando três decisões do dono:** provedor (API externa vs modelo
local pequeno embarcado), orçamento por jogador, e se a v1 é só nos NPCs de
vila ou em todos. Sem essas, implementar seria escolher custo recorrente por
omissão.

**62-b. O FANTASMA tem três moradas.** *"fantasmas podem viver em cemitérios
e lugares bem afastados tbm"* — além da gruta: o cemitério (de vila e
esquecido, os dois assombram) e o fim do mundo (longe de TODO assentamento, em
fração do raio — nunca unidades soltas). Cada morada sozinha basta; fora das
três, ele é o mais deslocado de todos.

**62. A fauna do bioma tem PREDOMINÂNCIA, não exclusividade.** ✅
*Implementada em 03/09* — `EncounterPredominance`: peso do LUGAR multiplicando
o peso da HORA que já existia. Na mata, planta e terra são o comum; o aquático
é "alguns, perto d'água" (amostrado em cruz na margem); o fantasma pesa perto
das grutas; o fogo é o deslocado — raro, NUNCA impossível (nenhum peso é zero,
afirmado em teste sobre todos os biomas e contextos). Biomas sem fauna
desenhada pesam neutro: pesar por palpite seria decidir a fauna do deserto sem
ninguém pedir.

**62 (original).** *"nosso
primeiro bioma é natureza comum, vai ter mais predominância de tipos de pets
comuns, natureza, plantas, insetos, alguns aquáticos, cavernas, noturnos
desses ambientes"*. Os encontros do bioma de mata puxam para o comum e o
natural — planta, inseto — com aquático perto d'água, cavernícola nas grutas e
noturno de noite (o peso por hora JÁ existe: `WorldTimeOfDay::PickSpeciesForPhase`).
⚠️ **É conteúdo de `mundo-por-biomas`/encontros, não de `cidades-do-interior`**
— registrada aqui para não se perder, implementada lá.

**59. O jogador escolhe ONDE trabalhar.** *"ele tem liberdade de escolher"* —
não há um `EGroundUse` eleito.

Leitura, e ela é minha: isto é **um trabalho com três lugares**, não três
trabalhos. A mecânica é uma só, oferecida em Fazenda (8), Criadouro (4) e Pomar
(3) — o que muda de um para outro é **qual atributo do pet facilita**. Assim a
CI9 continua sendo "um trabalho real", como a spec pede, e a liberdade é de
escolha, não de quantidade.


**68. OS RECURSOS DO MUNDO, e cada um vem de onde o mundo já o tem (04/09).**
O que se coleta: **madeira** (árvore/bosque), **pedra** (rocha/montanha),
**frutas** (pomar/árvore), **flores** (clareira/campo), **pets** (encontro), e
mais o que o mundo já dá por bioma — **fibra** (planta/pântano), **cogumelo**
(caverna/pântano, o lado Fantasma), **minério** (vulcão/caverna), **cristal**
(caverna), **mel** (bosque), **peixe** (rio/água), **argila** (margem/pântano),
**sal** (praia), **água** (poço/nascente) e **gelo** (geleira). A lista é fonte
única em código; adicionar recurso é adicionar uma linha, nunca uma tabela nova.

**68-b. A FERRAMENTA decide o que se colhe (decisão 36).** Mão vazia colhe o
fácil (flor, fruta, cogumelo); o resto exige a ferramenta certa — machado para
madeira, picareta para pedra/minério/cristal, vara para peixe, balde para água.
Sem a ferramenta, o recurso aparece mas não rende.

**68-c. O PET AJUDA A COLETAR (decisão 37).** Coletar com o pet por perto rende
mais que o jogador sozinho — o pet é braço, não plateia. Sem pet, ainda se
coleta, só menos.

**68-d. O BOSQUE plantado é do ASSENTAMENTO (decisão 35); a mata selvagem é
regulada.** Cortar livre no bosque que o assentamento plantou; na mata selvagem,
o Guarda Florestal regula (o mesmo padrão exceção-com-prazo de MV3/MV4).
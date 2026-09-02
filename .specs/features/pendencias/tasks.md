# Pendências registradas — tarefas

**Spec:** não há. Cada item aqui nasceu de uma resposta do usuário nas duas
caminhadas manuais, e está em `ACHADOS-T20.md` e `ACHADOS-G4.md` com a citação
que o originou.

**A ordem é de DEPENDÊNCIA, não de valor.** Os primeiros destravam os outros.

---

## P1 — O aqueduto pode ENTRAR no morro ✅ FEITO
> 🤖 Modelo: `opus` — muda uma regra que hoje TEM TESTE afirmando o contrário

> *"pode entrar no morro mas, precisa de sair por túneis ou a estrutura dá a
> volta"*

**É a única divergência contra código que existe e está testado.** Hoje o
envelope garante que a calha nunca entre — e o teste
`DescendsAndNeverEntersTheHill` afirma isso. Mudar a regra sem mudar o teste
deixaria a bateria verde sobre a regra velha.

**MEDIDO antes de escrever:** numa descida reta, o aqueduto 0 fica com 17 dos
25 pontos abaixo do chão e o aqueduto 1 com nenhum. Um "túnel" de 17 em 25
parece cano enterrado — até se lembrar de que **o aqueduto romano corria em sua
maior parte sob a terra**, e o arco é que era a exceção. A referência de fora
decidiu.

O envelope saiu: ele satisfazia só a metade "a estrutura dá a volta", e nunca
precisava de túnel porque inventava altura para escapar do morro. A queda agora
é a DECLARADA pelo traçado, e é ela que impede a obra de fugir para cima.

`DescendsAndNeverEntersTheHill` **virou** `DescendsAndTunnelsThrough` — deixar o
antigo verde ao lado do novo faria a bateria provar duas regras que se
contradizem.

## P2 — A casa ganha ATRIBUTOS 🧠
> 🤖 Modelo: `opus` — é o contrato que P3 e P4 consomem

> *"pode ser específico com as propriedades daquele local: água, doce,
> densidade, velocidade da corrente, venenoso, condutor por elemento com
> potência"*

Hoje a casa tem fundura, substância e (desde a corrente) rumo e força. Falta o
resto ser **medido por casa** em vez de herdado do fluido.

*Decisão a MEDIR:* o que é do FLUIDO (densidade — igual em toda água doce) e o
que é da CASA (venenoso — uma poça específica). Confundir os dois duplicaria a
tabela do registro dentro da grade.

## P3 — Resistência pela ANATOMIA
> 🤖 Modelo: `sonnet`

> *"depende da anatomia dele e da biologia da pele dele"*
> *"pets que têm fraqueza a certos elementos e outros que são imunes"*

Hoje a base vem do ELEMENTO (Fogo aguenta lava a 50%). O pedido é que venha da
criatura. **Não contradiz a G3:** é trocar a fonte do número, e o número já tem
onde morar (`FluidResistPercent`) e como compor (`ComposeFluidResist`).

*Também resolve o buraco da C2:* resistir a ser CARREGADO precisa de um número
por criatura, e hoje não existe nenhum.

## P4 — Efeito de CAMPO INTEIRO
> 🤖 Modelo: `sonnet`

> *"dependendo do tamanho do campo, alguns ataques como eletricidade
> conseguiriam eletrizar todo o campo ou congelar"*

A condução alcança o componente conexo. Isto é outra coisa: um golpe forte o
bastante para tomar o tabuleiro, comparando potência com **tamanho do campo** —
e é o tamanho que impede um golpe fraco de virar arma de área numa arena
pequena.

## P5 — Golpe que se DESLIGA sob dano
> 🤖 Modelo: `sonnet`

> *"golpes desse tipo devem ser desativados se o pet não tiver muito controle,
> pois está tomando dano"*

Concentração interrompida. Tem parente no jogo: a escola Física já *"interrompe
a concentração"* no ciclo de escolas.

## P6 — A água APAGA o fogo
> 🤖 Modelo: `sonnet`

> *"fogo na água não vai ficar pegando fogo"*

Hoje fogo apenas não conduz — a ausência. O pedido é o oposto ativo.
`MoveTerrainEffects` já deixa um golpe mudar a casa, então o cano existe.

## P7 — Nadar quando a água passa da CINTURA
> 🤖 Modelo: `sonnet`

> *"depende o nível da água, se ela ultrapassar a cintura do jogador precisa
> nadar"*

Hoje quem decide o vau é o traçado (as 30 travessias), e não a altura do
personagem contra a fundura. **Mexe no traçado ou na leitura dele — medir qual
antes de escrever.**

## P8 — O poço se lê pela FUNDURA
> 🤖 Modelo: `sonnet`

Hoje é binário: dá água ou não dá. **O traçado não guarda fundura de poço** — e
por isso esta tarefa começa no traçado, que é feature separada por invariante.

## P9 — Cavernas por dentro das cachoeiras
> 🤖 Modelo: `sonnet`

A queda tem poço; a gruta é peça separada, sem ligação com a queda.

## P10 — Coisas ESCONDIDAS, fora da carta
> 🤖 Modelo: `opus` 🧠 — mexe no gabarito de aceite

Hoje `ChartConformance` exige carta e mundo **idênticos**: esconder algo
reprovaria. Para haver segredo, a carta precisa dizer o que ela **não** mostra —
senão o teste que protege a feature passa a proibir o design.

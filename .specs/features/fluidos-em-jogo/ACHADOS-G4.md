# Achados da caminhada — G4 e T20 (segunda leitura)

**Respondido em 02/09/2026**, contra `docs/verification/fluidos-em-jogo.md` e
`docs/verification/construcao-do-mundo.md`.

## O veredito

**Nenhum defeito confirmado, nos dois roteiros.** Tudo o que foi construído
respondeu como prometido.

Confirmados por extenso nos fluidos: a etiqueta só aparece quando diverge do
padrão (1), a água perto do vulcão sai TERMAL (2), camuflar **perde igual** (8),
a poça separada por chão seco **não leva** (10), o modelo de capacidade decide
quem se queima (11), e fogo **não conduz** (12).

## UMA DIVERGÊNCIA de projeto — e é contra o que eu construí

**T20 #11, o aqueduto.** Eu fiz a calha **nunca** entrar no morro: montei um
envelope de trás para a frente, justamente para isso ser impossível. A resposta
foi outra:

> *"pode entrar no morro mas, precisa de sair por túneis ou a estrutura dá a
> volta"*

O que existe hoje está **certo pela regra que eu escrevi** e **errado pela
regra que o jogo quer**. Não é defeito de construção — é decisão de design que
diverge, e por isso vira tarefa em vez de conserto silencioso.

*Aceite quando for feita:* a calha pode atravessar o morro, e onde atravessa há
**túnel** ou **contorno** — nunca pedra maciça cortada pela água.

## UMA PERGUNTA MINHA que não foi entendida

**T20 #13** — *"a linha do deus SOME ao sair do templo"* — recebeu "não
entendi", e a culpa é da pergunta. O que ela cobra: ao **se afastar** de um
templo, o painel deixa de escrever `templo de <deus>`. Se a linha ficasse, ela
diria que você está num lugar que ficou para trás.

**Fica sem verificação até ser reperguntada em português melhor.**

---

# O DESENHO NOVO que as respostas pedem

Nada abaixo é defeito. É superfície de design que as respostas abriram, e ela é
grande — registrada para não se perder, não para ser feita agora.

## A casa precisa de ATRIBUTOS, não de um rótulo

> *"pode ser específico com as propriedades daquele local: água, doce,
> densidade, velocidade da corrente, venenoso, condutor por elemento com
> potência, sentido do movimento"*

Hoje a casa tem **fundura** e **substância**. O pedido é um terceiro nível:
propriedades medidas por casa. A **velocidade e o sentido da corrente** são as
mais consequentes — elas dão direção à água, e água com direção empurra,
carrega e decide para que lado a corrente elétrica anda.

## Efeito de CAMPO INTEIRO

> *"dependendo do tamanho do campo, alguns ataques como eletricidade e nossos
> atributos conseguiriam eletrizar todo o campo ou congelar"*

A condução hoje alcança o **componente conexo**. Isto é outra coisa: um golpe
forte o bastante para tomar o tabuleiro. Precisa de um limiar que compare a
potência do golpe com o **tamanho do campo** — e é o que impede um golpe fraco
de virar arma de área numa arena pequena.

## A resistência vem da ANATOMIA, não do elemento

> *"depende da anatomia dele e da biologia da pele dele"* (F6)
> *"vamos ter pets que têm fraqueza a certos elementos e outros que são imunes,
> que se misturam com o ambiente"* (F5)

Hoje a base vem do **elemento** (Fogo aguenta lava a 50%), porque foi assim que
o Incorpóreo entrou. O pedido é que venha da **criatura** — pele, anatomia. Isso
não contradiz a G3: é trocar a fonte do número, e o número já tem onde morar
(`FluidResistPercent`) e como ser composto (`ComposeFluidResist`).

## Golpe que se DESLIGA sob dano

> *"golpes desse tipo devem ser desativados se o pet não tiver muito controle,
> pois está tomando dano"* (F8)

Concentração interrompida por dano. É regra nova de postura, e ela tem parente
no jogo: a escola Física já *"interrompe a concentração"* no ciclo de escolas.

## Fogo na água se APAGA

> *"fogo na água não vai ficar pegando fogo"*

Hoje fogo simplesmente não conduz — a ausência. O pedido é o oposto ativo: a
água **apaga**. `MoveTerrainEffects` já deixa um golpe mudar a casa, então o
cano existe.

## E as que repetem achados já registrados

- **caverna atrás da cachoeira** (T20 #6) — já em `ACHADOS-T20.md`
- **nadar quando a água passa da cintura** (T20 #8) — já lá
- **coisas escondidas do mapa** (T20 #15) — já lá
- **trilha mais funda que o nível, entrando em caverna** (T20 #10)
- **o poço se lê pela fundura** (T20 #14) — já lá
- **material de lava de verdade** (G4 #4) — a limitação declarada; exige asset

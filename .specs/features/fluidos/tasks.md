# Fluidos — tarefas

**Spec:** `.specs/features/fluidos/spec.md`
**Feito até aqui:** F1 (o registro existe e separa).

---

## F1 — O registro ✅ FEITO

`EFluidKind` + `FFluidTraits` + tabela única em `BattleSim`, com cinco provas.
A que carrega o peso é **`TheFluidsActuallyDiffer`**: uma tabela em que todos
os fluidos têm os mesmos números passa em qualquer teste de leitura e não
distingue nada — que era o estado de onde se partiu.

---

## F2 — A casa sabe de que fluido ela é ✅ FEITO

**A decisão foi MEDIDA, não escolhida por gosto.** A arena é montada casa a
casa: `FWorldFeatureSample` carrega uma `Location`, e o layout sai de
`CellLayoutIndex(Coluna, Linha)` — casas diferentes recebem propriedades
diferentes do mundo. Derivar o fluido do bioma da arena tornaria impossível
**para sempre** uma batalha na beira do vulcão com água termal de um lado e
lava do outro. Campo por casa, então.

E ele custa quase nada: `CellFluid` é um quarto array paralelo a `CellLayout`,
que já tinha três — o próprio código justifica o padrão ("terreno que passa são
três informações"). Um byte por casa, teto de 225.

**Vazia quer dizer "tudo no padrão".** Uma arena comum não paga um byte por
casa para dizer que água é água; quem tem lava materializa a lista.

O hash soma o fluido **DERIVADO**, nunca o array cru — lista vazia e lista de
zeros querem dizer a mesma coisa, e somando o cru elas dariam assinaturas
diferentes sendo idênticas. Seria dessincronia fantasma, das piores de achar.

`TerrainAllowsSkill` continua sendo o eixo da FUNDURA e não mudou de assinatura
(11 chamadas em teste ficaram intactas); `CellAllowsSkill` é a conjunção dos
dois eixos. Não são duas cópias da regra — são as duas perguntas que a casa
responde.

## F3 — Poderes que distinguem fluido
> 🤖 Modelo: `sonnet`

Requisito de skill passa a poder pedir substância, não só fundura.
*Aceite:* um poder que exige água funciona nas cinco águas e falha na lava;
um que exige lava falha em todas as águas.

## F4 — O dano de estar dentro ✅ FEITO

O dano entra pelo **mesmo cano** do dano de casa que já existia: mesmo
acumulador `PendingDamage`, mesma guarda de `IsOffTheGround()`, mesmo evento em
F5. Um segundo caminho teria as próprias regras de morte e a própria narração.

**O campo foi RENOMEADO por causa da medição.** Ele se chamava
`DamagePerTurn`, e o laço onde ele cai roda **por slot** — três por turno. O
dano de casa que já existia (`CellDamageAmount`) é por slot; dois danos de
terreno em relógios diferentes seriam um defeito esperando, porque o jogador
aprenderia o custo de um e erraria o do outro. Agora é `DamagePerSlot`.

Voar escapa (a casa só alcança quem pisa nela); **camuflar não** — quem se
esconde continua em pé no mesmo lugar.

Na tela: a substância aparece na etiqueta da casa **só quando diverge do
padrão** daquela fundura, e o que machuca fica vermelho. Uma casa de lava veste
hoje o material da água, porque material novo exige asset autorado — e sem o
nome, a casa mais perigosa do campo pareceria a mais inofensiva.

## F5 — Itens e resistências
> 🤖 Modelo: `sonnet`

O que o usuário chamou de "itens que farão diferença entre eles": proteção
contra um fluido específico.
*Aceite:* com o item, o dano da lava não vem; sem ele, vem.

## F6 — O mundo declara seus fluidos
> 🤖 Modelo: `sonnet`

`WaterFooting` e a paleta passam a consultar o registro em vez de tratarem
toda água como uma. Hoje o mundo tem seis fluidos e trata todos igual.
*Aceite:* a água termal perto do vulcão é reconhecida como termal, não doce.

## F7 — A balsa flutua por DENSIDADE

Fecha o achado #9 da construção do mundo. Hoje a altura da balsa vem da lâmina;
com o registro, ela pode vir do empuxo.
*Aceite:* um corpo mais denso que o fluido não boia nele.

---

## F8 — A ÁGUA CONDUZ 🧠
> 🤖 Modelo: `opus` — é regra de área nova, e mexe na fronteira do núcleo

**Pedido do usuário:** *"molhar não machuca, mas se o pet for elétrico ele vai
sofrer alterações, pois os poderes elétricos onde estiver molhado conduzem
energia — então se ele soltar um poder molhado, quem estiver no molhado e em um
condutor direto vai sofrer o dano."*

É o primeiro poder que o registro de fluidos torna possível, e o que dá sentido
a `bIsWater` existir.

### A restrição que molda o desenho

`BattleDataTranslator` diz, com todas as letras: **"o núcleo nunca sabe que tipo
existe — o Attack que ele recebe já chega efetivo."** Elétrico é TIPO, e tipo
vive fora do `BattleSim`.

Então a condução **não pode perguntar se o pet é elétrico**. Ela é propriedade
do GOLPE — um `bConduz` por movimento, que o tradutor liga a partir do tipo,
do mesmo jeito que a efetividade já chega pré-multiplicada. O núcleo resolve
área e dano sem nunca aprender o que é eletricidade.

Movimento já carrega dado próprio para dentro do núcleo (`MoveDrainPercents`,
`MoveEffectStats`), então o lugar existe.

### O que o registro ganha

`ConductivityPerMille` — mais um campo, com referência de fora como os outros:
água salgada conduz muito mais que doce; pântano, carregado de minerais, fica
no meio; termal também. Lava conduz (rocha fundida é iônica), e isso importa
porque ela já machuca por outro motivo — as duas coisas somam pelo mesmo cano
(invariante 7).

### A área: CONDUTOR DIRETO

"Quem estiver no molhado **e em um condutor direto**" é uma pergunta de
componente conexo: alaga-se a partir da casa do lançador por casas de fluido
condutor adjacentes. Duas poças que não se tocam são dois circuitos, e quem
está na outra não leva.

**Isto é grafo, não raio de distância.** Uma poça a duas casas de distância mas
ligada por um fio de água leva; uma encostada mas separada por chão seco, não.

*Aceite:*
- lançador molhado em água + golpe que conduz → todos na mesma poça levam;
- alguém numa poça SEPARADA não leva (o contrapeso, invariante 9);
- lançador em chão seco → ninguém leva, nem quem está na água;
- golpe que NÃO conduz na mesma situação → ninguém leva.

### Decisão em aberto

Se o próprio lançador leva o dano que ele conduziu. Fisicamente sim; como jogo,
é o que separa "poder forte" de "poder com preço". **Não decidir sozinho.**

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

## F3 — Poderes que distinguem fluido ✅ FEITO

`SkillFluidRequirement[16]`, paralelo a `SkillTerrainRequirement` — o cano que
já existia (invariante 7). Um poder que só funciona na lava é uma LINHA de
configuração, e não uma exceção dentro da fase, pelo mesmo motivo que `escavar`
não precisou de um `if` no núcleo.

**Os dois eixos são independentes e se combinam.** Um poder pode exigir água
FUNDA *e* que ela seja termal: se a substância declarada apagasse o requisito de
fundura, um poder de água termal funcionaria numa poça.

Zero quer dizer "sem exigência de substância", e aí vale a regra que a fundura
implica — foi assim que submergir passou a recusar a lava na F2, e continua
sendo assim sem ninguém declarar nada.

O poder que exige lava falha em toda água **sem listar quais águas existem**: a
sexta que alguém acrescentar já nasce recusada.

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

## F5 — Itens e resistências ✅ FEITO

**Não existia sistema de itens nem de resistência em nenhum dos dois módulos**
— medido antes de escrever. Isso decidiu a forma: o núcleo carrega o NÚMERO
(`FluidResistPercent`, um por fluido), e de onde ele vem — item, traço da
espécie, bênção de templo — é decisão da camada de cima. É a mesma fronteira
que faz o Attack chegar pré-multiplicado pela efetividade de tipo.

**Porcentagem, não booleano de imunidade.** O jogo já fala em porcentagem em
todo lugar (efetividade, escorregão, atraso), e um booleano fecharia a porta
para "resiste um pouco" sem ganhar nada.

Presa entre 0 e 100: acima de 100 o dano viraria negativo, e curar quem pisa na
lava é o oposto do que a regra promete.

**Resistir a um fluido não protege de outro, nem da brasa** — a casa de dano
não é fluido, e a bota de lava não pode ser armadura geral, senão o primeiro
item do jogo seria o último.

O array tem tamanho fixo (a reflexão não aceita `EFluidKind::Count` como
dimensão), com `static_assert` guardando: o nono fluido não nasce sem lugar
onde ser resistido.

## F6 — O mundo declara seus fluidos ✅ FEITO

O fluido é ASSADO **ponto a ponto** nos rios, e não um por curso: termal é
propriedade da POSIÇÃO, e um rio nasce fervendo na saia do vulcão e chega frio
ao mar. Um fluido por curso decidiria pelo meio do percurso, e um rio que só
passa perto do vulcão sairia frio — quebrando o próprio aceite.

A ordem do mapeamento é decisão: **termal vence o pântano** (a água quente
continua quente correndo por dentro do barro), e o pântano vence a doce.

**DEFEITO ACHADO PELO CONTRAPESO:** a prova de que "onde o pé está seco não há
fluido" pegou as duas perguntas discordando fora da costa — `FluidAt` dizia
água salgada e `At` dizia seco. O painel teria dito "seco em água salgada". O
mar não está no traçado como curso (ele é o que sobra depois que a terra acaba),
então nenhum laço sobre rios o encontraria. Agora fora da costa é FUNDO.

## F7 — A balsa flutua por DENSIDADE ✅ FEITO

Fecha o achado #9 da construção do mundo. A altura vinha da lâmina, e por isso
a balsa flutuaria igual em qualquer coisa — inclusive num fluido menos denso
que ela, onde uma balsa de verdade iria ao fundo.

`ConfigureFor` devolve `false` quando ela não boia, e a travessia deixa de
planejá-la: **uma balsa que afunda não é uma travessia ruim, é uma travessia
que não existe** — diferente de nascer parada no fundo do rio.

O convés é madeira (600 por mil). O caso que só a densidade pega: um convés de
pedra (2700) NÃO boia na água doce e BOIA na lava, porque a lava é mais densa
que ele. Um booleano `bFloats` no convés erraria isso sempre, e erraria calado.

---

## F8 — A ÁGUA CONDUZ ✅ FEITO

A corrente parte de **onde o raio caiu**, não de onde o lançador está. O alvo
direto fica de fora — ele já levou o raio. Condutor direto é **componente
conexo**, vizinhança de quatro.

**Quem gera, aguenta**: a resistência é a CAPACIDADE do pet, não um número
escolhido — e isso responde sozinho se o lançador se machuca.

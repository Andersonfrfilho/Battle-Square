# O sentido da corrente — tarefas

**Spec:** `.specs/features/corrente/spec.md`

---

## C1 — A casa sabe para onde a água corre ✅ FEITO
> 🤖 Modelo: `opus` — é o contrato, e decide a representação

Assar, por ponto de curso, o **rumo** e a **força** — os dois saem do que já
existe: o rumo é a diferença entre pontos consecutivos da polilinha (que já é
ordenada da nascente para a foz), e a força sai de `BedGradientAtProgress`.

Depois, a casa: um campo por casa em `FBattleState`, paralelo a `CellFluid`,
pelo mesmo padrão — **vazio quer dizer água parada**.

**A decisão foi MEDIDA:** oito rumos. Encaixar os 5.480 trechos da bacia em
`EBattleDirection` troca de rumo em 2% dos passos e **vai-e-volta em 0,3%** — o
rio não treme. O erro de encaixe (mediana 12,3°) é irrelevante: o consumidor é
8-direcional de qualquer forma, e não há como empurrar alguém a 12 graus numa
grade de casas.

A força sai de `BedGradientAtProgress` em partes por mil (mediana 56, máximo
329), inteira, pela disciplina das densidades.

## C2 — A corrente EMPURRA ✅ FEITO
> 🤖 Modelo: `sonnet`

Quem está em água corrente é levado, no fim do slot, uma casa rio abaixo — se a
força vencer o que ele é.

**Procurar o cano que já existe** (invariante 7): mover um pet contra a vontade
dele não é novidade — o escorregão do gelo já faz isso, e o empurrão da
trombada também. Um terceiro caminho de movimento involuntário teria as próprias
regras de colisão e a própria narração.

**O limiar não é meu:** a corrente carrega onde ela corre acima de 40 por mil,
que é `FreshWater::RapidsGradient()` — a definição que o traçado já usa para
dizer onde há corredeira. Escolher um número novo criaria duas fronteiras para
a mesma ideia.

**Não há resistência POR PET, e é declarado:** o jogo não tem peso nem firmeza,
e reaproveitar `Defense` (que vai de 0 a 1000) contra uma força em partes por
mil seria comparar escalas que não se falam. Quem resiste é o LUGAR: água mansa
não carrega ninguém. Uma resistência por criatura precisa de um número novo, e
isso é tarefa própria.

**Quem voa escapa; quem SUBMERGE, não** — diferença deliberada em relação ao
dano de casa. O dano é do CHÃO; a corrente é a ÁGUA, e estar submerso é estar
mais dentro dela.

## C3 — Subir a correnteza custa
> 🤖 Modelo: `sonnet`

Andar contra a corrente é mais caro que a favor. É o que faz o sentido virar
decisão de rota em vez de enfeite.

*Aceite:* o mesmo trecho, nos dois sentidos, com custos diferentes.
*Contrapeso:* atravessar de lado não paga nem ganha.

## C4 — A balsa sente a corrente
> 🤖 Modelo: `sonnet`

Ela já é sólida, anda e esbarra. Uma balsa num rio corrente que ignora a água é
uma plataforma sobre trilhos.

*Aceite:* a travessia demora mais contra a corrente que a favor.

## C5 — O mundo aberto
> 🤖 Modelo: `sonnet`

`WaterFooting` diz hoje que a água atrasa. Com sentido, ela atrasa **de um
lado** e ajuda do outro.

*Aceite:* atravessar o mesmo rio nos dois sentidos dá resultados diferentes.

## C6 — A prova na tela
> 🤖 Modelo: `sonnet`

Corrente que ninguém vê é corrente que o jogador descobre sendo empurrado.

*Aceite:* o painel diz o rumo e a força da casa em que se pisa, e a grade
desenhada mostra o sentido. Roteiro com o que só o olho vê.

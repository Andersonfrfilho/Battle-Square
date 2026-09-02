# Fluidos em jogo — tarefas

**Spec:** `.specs/features/fluidos-em-jogo/spec.md`

---

## G1 — A arena põe fluido nas casas 🧠
> 🤖 Modelo: `opus` — é o contrato entre o mundo e o tabuleiro

`FArenaFromWorld` já decide `ECellProperty` **casa a casa** a partir do que há
em volta do encontro. O mundo já sabe de que fluido é cada ponto
(`WaterFooting::FluidAt`, F6). Falta o fluido viajar junto da propriedade.

**Procurar o cano que já existe** (invariante 7): a amostra do mundo
(`FWorldFeatureSample`) já carrega uma `Location`. É por ela que o fluido entra
— não por um caminho novo.

*Aceite:* uma batalha na saia do vulcão nasce com casas de água TERMAL, e uma
longe dele com água doce. E o contrapeso: casa seca continua sem fluido.

## G2 — O tradutor liga a condução
> 🤖 Modelo: `sonnet`

Uma bandeira, no lugar onde o tipo ainda é conhecido. `BattleDataTranslator`
converte "este pet é do elemento Raio" em "os golpes dele conduzem", como já
converte tipo em Attack pré-multiplicado.

*Aceite:* um pet de Raio numa partida real eletrifica a poça; um de Fogo, não.
*Motivo:* sem isto a F8 inteira é inerte — regra completa, testada, e que nunca
acontece.

## G3 — A resistência tem origem  ⏸ AGUARDA DECISÃO
> 🤖 Modelo: `sonnet`

`FluidResistPercent` é sempre zero: nada preenche. **De onde vem a proteção?**

- **Item equipado** — não existe como sistema; criá-lo é feature própria.
- **Traço de espécie** (`EPetTrait`) — existe, e é onde o Incorpóreo mora.
- **As duas** — traço dá a base, item soma.

**PERGUNTAR, não escolher.** Chutar cria um sistema que o usuário desfaz.

*Aceite (qualquer que seja a origem):* um pet com a proteção atravessa a lava
sem se queimar; o mesmo pet sem ela, não.

## G4 — A prova na tela
> 🤖 Modelo: `sonnet`

O que se implementa aparece. A etiqueta da casa já mostra a substância quando
ela diverge do padrão, e o vermelho já marca o que machuca — mas isso nunca
rodou com fluido de verdade numa partida.

*Aceite:* roteiro manual com o que só o olho vê — a casa de lava lida como
perigosa antes de alguém pisar nela, e a corrente contada no painel quando ela
alcança um terceiro pet.

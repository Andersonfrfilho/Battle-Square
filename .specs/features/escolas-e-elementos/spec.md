# Escolas e Elementos — Especificação

**Status:** Aprovado e executado (30/08/2026), escrito DEPOIS do código.
**Depende de:** Escala de Pets e Skills, Golpes por Pet, Desbloqueio de Golpes.

> **Esta spec nasceu atrasada, e isso é um defeito de processo.** O código veio
> primeiro porque o pedido chegou em pedaços — "add os monstros", "balanceia o
> mágico", "temos magia física, elementos da natureza e psíquicas", "magias de
> aumentar e diminuir atributos" — cada um respondido antes do seguinte
> chegar. O desenho só ficou visível por inteiro no fim. Registrado aqui para
> que o **porquê** de cada decisão não se perca, que é o único trabalho que uma
> spec escrita depois ainda faz.

---

## Problem Statement

O jogo tinha três tipos (Fogo, Água, Planta) num ciclo simples. Quatro pedidos
do usuário, em sequência, quebraram esse modelo:

1. **Quatro tipos novos** — Psíquico, Mágico, Inseto, Caverna. Sete tipos
   exigiriam quarenta e duas relações de efetividade, que ninguém decora nem
   equilibra; e sete matizes distintas não cabem sem encostar nas do terreno.
2. **"Balanceia o Mágico"** — ele tinha 150% contra os três tipos naturais de
   uma vez, o que na prática é vantagem contra quase todo o catálogo.
3. **"Magia física, elementos da natureza e psíquicas"** — a taxonomia que o
   usuário queria, com **subdivisões por elemento** (`magia-física-terra`,
   `magia-física-fogo`).
4. **"Magias de aumentar atributos, diminuir, e ativar efeitos"**.

Os quatro se resolvem uns aos outros, e é isso que torna o desenho uma coisa só
em vez de quatro remendos.

## Goals

- [x] O tipo tem DOIS eixos independentes: escola e elemento
- [x] A efetividade sai de duas tabelas pequenas que se compõem
- [x] O Mágico deixa de ser especial **sem número tocado nele**
- [x] Magia que sobe o próprio atributo e derruba o do oponente
- [x] Acrescentar elemento ou escola é UMA linha, sem C++
- [x] Doze tipos distinguíveis na tela, em dois canais independentes

## Out of Scope

| Item | Razão |
|---|---|
| Mais de uma escola por pet | Duplicaria os eixos e o número de silhuetas necessárias; um pet é uma coisa |
| Efeito de atributo que dure entre TURNOS | Faria quem age primeiro vencer por acúmulo |
| "Escavar" para Terra | Exige o requisito de terreno virar DADO da skill; hoje mora no núcleo. Mecânica nova, com spec própria |
| Cor por escola | As matizes seriam três a mais disputando espaço com o terreno; a escola ficou na forma |

---

## Decision Points

### DP-tipo-01 — Um eixo é VERBO, o outro é MATÉRIA

**Decisão: a ESCOLA diz o que a magia daquele pet faz; o ELEMENTO diz contra o
que ela é forte.**

| Escola | O que a magia dela faz |
|---|---|
| Física | dano direto |
| Natural | muda o terreno da casa |
| Psíquica | muda atributos |

Isto não foi escolhido por elegância: **duas das três já existiam**. Dano é o
combate de sempre; mudar terreno é o `terrainEffect` dos golpes. Só a terceira
era nova. O eixo foi descoberto no que o jogo já fazia, não imposto sobre ele.

### DP-tipo-02 — A efetividade se COMPÕE, e os eixos têm pesos diferentes

**Decisão: `escola × elemento ÷ 100`, com elemento em 150/50 e escola em
120/80.**

Duas tabelas de quatro linhas cobrem doze tipos. A matriz equivalente teria 144
células, e ninguém equilibra o que não consegue ler de uma vez.

**O elemento é o eixo alto porque é o intuitivo** — água apaga fogo sem
ninguém precisar decorar. A escola precisa ser aprendida, então pesa menos.

O extremo do jogo saiu de 150 (um par bastava) para 180 (exige vantagem nos
dois eixos), e o piso de 50 para 40.

**Ciclo dos elementos, quatro:** Fogo queima Planta, Planta racha Terra, Terra
bebe Água, Água apaga Fogo. As diagonais são NEUTRAS de propósito — um ciclo de
quatro com diagonais também definidas vira uma matriz que ninguém lembra.

**Ciclo das escolas:** o golpe interrompe a concentração, a mente comanda o
elemento, o elemento corrói a matéria.

### DP-tipo-03 — O Mágico se equilibra por DESENHO, não por número

**Decisão: não existe mais um tipo "Mágico". Existe `Psiquica/Fogo`.**

Ele tinha 150% contra Fogo, Água e Planta. Depois da mudança tem 120 contra um
natural — o eixo suave — e a vantagem dele passou a ser **o que a magia dele
faz**, não quanto ela machuca.

É a diferença entre baixar um número e remover a razão de ele existir. Um ajuste
de 150 para 120 teria produzido o mesmo valor e mantido a pergunta *"por que
este tipo é especial?"* sem resposta.

### DP-tipo-04 — A SILHUETA identifica, a cor agrupa

**Decisão: malha = escola, tombo = elemento, cor = elemento.**

Com três tipos a matiz bastava. Com doze, dois problemas chegam juntos: o olho
não separa doze matizes num bicho em movimento, e doze matizes não cabem sem
encostar nas do terreno.

**A primeira versão errou**, e dois testes antigos a reprovaram citando o
motivo em comentário: *quem não distingue as formas lê a cor, e VICE-VERSA*.
Escola na malha e elemento só na cor deixaria o daltônico sem o elemento
inteiro.

Por isso o elemento ganhou **tombo** — fogo ereto, água deitada, planta aberta,
terra quase rente. Três malhas × quatro inclinações = doze silhuetas únicas. A
cor virou o canal **rápido e parcial**: quatro cores para quatro elementos, e
dois tipos do mesmo elemento partilham a cor de propósito.

> **Achado no caminho:** o tombo da aparência NUNCA chegava à tela — a view
> usava só o do corpo. Agora eles se somam.

### DP-tipo-05 — O SINAL do percentual diz quem sofre

**Decisão: positivo sobe o atributo de quem lançou; negativo derruba o do
oponente.**

Um campo em vez de dois. Não existe subir o atributo do oponente nem baixar o
próprio, então a bijeção é completa e não esconde nenhum caso — e a leitura sai
natural: `+40 de ataque` é ficar mais forte, `−35 de velocidade` é enfraquecer
o outro.

### DP-tipo-06 — UM efeito ativo, e o novo substitui

**Decisão: um por pet, três slots, substituição.**

Empilhar seria dominante: três magias de ataque no mesmo turno dobrariam o
dano, e a escola psíquica venceria por repetição em vez de por escolha.
Substituir mantém a decisão viva — *vale a pena trocar o bônus que já está de
pé?*

**Três slots** é o turno: um slot só nunca chegaria ao golpe seguinte, e a
magia não valeria o turno que custa; mais de um turno faria quem age primeiro
vencer por acúmulo.

**O efeito acontece ANTES do dano no mesmo slot**, porque subir o próprio
ataque precisa valer já naquele acerto e derrubar a defesa do outro precisa
valer contra o dano que vem em seguida.

**Só a Magia carrega efeito.** O mesmo golpe usado como Atacar não aplica nada
— se aplicasse, a diferença entre as duas ações sumiria, e com ela a razão de
existir da escola.

### DP-tipo-07 — O catálogo é DADO; o comportamento é código

**Decisão: `Config/PetTypes.json` declara escolas, elementos e legados. Sem
valor padrão em C++.**

Acrescentar um elemento exigia quatro edições — duas em C++ — e esquecer uma
produzia um tipo pela metade: existe, luta, e sai parecendo genérico.

**Não há tabela padrão embutida no código** para o caso de o arquivo faltar, e
isso é escolha: seria a segunda cópia da mesma lista, e cópias concordam até a
primeira edição (L-032, L-033). Sem o arquivo, o pet sai no ouro-palha do
desconhecido — visível, não silencioso.

`Config/PetSkills.json` foi **apagado** pelo mesmo motivo: era a segunda cópia
da lista de elementos.

**O que continua sendo código, e por quê:** malha de crista (asset novo é
código), `EBattleStat` (cada atributo entra numa fórmula), `ECellProperty` e
`EActionType` (cada um tem regra na resolução). Isso é comportamento com nome,
não dado.

### DP-tipo-08 — Nome antigo continua valendo

**Decisão: `Fogo`, `Magico`, `Inseto`, `Caverna` resolvem para pares.**

Não é gentileza: eles já foram **assinados**, e dado assinado não se reescreve
de fora. Meia-leitura NÃO vale — `Natural/Telepatia` falha no elemento em vez
de virar um tipo que existe pela metade e não bate com nada.

---

## O que ficou provado por teste

| Teste | O que ele impede de voltar |
|---|---|
| `TypeEffectiveness.ShippedChartComposesTwoAxes` | os eixos deixando de se compor, e algum tipo ficando forte contra todos |
| `PetAppearance.EveryTypePairIsTellableApart` | dois dos doze tipos ficando iguais na silhueta |
| `PetAppearance.ShapeIsSchoolAndColorIsElement` | os dois canais voltarem a ser um |
| `PetTypeCatalog.NewElementNeedsNoCode` | acrescentar elemento voltar a exigir C++ |
| `PetTypeCatalog.MissingCatalogHasNoCodeFallback` | a segunda cópia da lista voltar como "padrão" |
| `PetTypeCatalog.SkillsComeFromTheOneCatalog` | o arquivo separado de skills ser recriado |
| `StatEffect.BuffRaisesLaterDamage` | o bônus virar número guardado que não aparece (L-041) |
| `StatEffect.NewEffectReplacesTheOld` | efeito empilhando e dominando o turno |
| `StatEffect.PhysicalAttackCarriesNoEffect` | Atacar e Magia deixarem de ser ações diferentes |
| `StatEffect.EntersStateHash` | dois estados que resolvem diferente com a mesma assinatura |
| `Training.TerrainIsAlwaysDullerThanCreatures` | campo de treino voltar a disputar cor com pet |

## O que NÃO foi verificado

**Nada disto foi visto na tela.** As perguntas que só uma pessoa jogando
responde estão em `docs/verification/atributos-treino-e-mundo.md`, seções
TIPO-01, TIPO-02, MAGIA-01 e PALETA-01. Três delas reprovam um desenho desta
spec se a resposta for não:

- dois pets da mesma escola dão para distinguir **sem olhar a cor**?
- gastar um slot para mudar atributo **vale a pena**, ou é sempre melhor bater?
- o mundo ficou **sóbrio demais** depois de o terreno recuar de croma?

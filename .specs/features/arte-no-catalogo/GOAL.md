# OBJETIVO — Arte no catálogo (TRILHA B: o backend)

> **Uma de duas trilhas paralelas.** A outra é `arte-no-editor` (TRILHA A).
> Elas foram desenhadas para NÃO se cruzarem — leia
> `.specs/handoffs/2026-09-04-arte-duas-sessoes.md` se precisar do porquê.

> 🤖 **Modelo: `fable`** (ou `opus`) — esta trilha é 🧠 do começo ao fim, pelo
> critério do `model-economy.md`. O trabalho central (AR4c) não é tabela: é
> decidir QUAL criatura cabe em qual pet, o que é sabor, coerência de mundo e a
> decisão 69 no meio. A AR4d é modelo de dado novo, e a AR4e é redigir a decisão
> para o usuário. Nada disso tem aceite mecânico.

## ⚠️ A regra que não se quebra

**NÃO ABRA A UNREAL. NÃO RODE `./Tools/build_editor.sh`.**
A trilha A está com o editor aberto e o MCP conectado; compilar derrubaria as
duas. Esta trilha é **TypeScript puro**, e `bun test` é o único gate.

## O objetivo, numa frase

Os 128 modelos importados encontram os 115 pets do catálogo — cada pet ganha o
corpo que lhe cabe, e as cadeias de evolução saem prontas.

## O insumo (leia o ARQUIVO, nunca o MCP)

`.specs/handoffs/assets-importados.json` — 128 SkeletalMesh com nome e pasta,
exportado pela trilha A. Se estiver velho, **peça um novo**; não abra o editor.

## O que já está pronto

`apps/api-battle-pets/src/pet/species/model-mapping.pure.ts` (17 testes):
modelo → elemento → família → cadeia de evolução. Já corrigido por quatro
defeitos que só apareceram com nomes reais.

## PRONTO é isto, e nada menos

- [x] **AR4a** — o gerador lê `assets-importados.json` e classifica os 128:
      elemento, estágio, família. O relatório separa o que ele soube do que
      **não** soube (medido em 04/09: 121 criaturas, 6 humanos, 1 prop; 102 com
      elemento, 19 sem, 1 conflito — `2fde892`)
- [x] **AR4b** — as cadeias de evolução saem nomeadas: as 6 do Ultimate
      (`Dragon`, `Alpaking`, `Armabee`, `Glub`, `Goleling`, `Mushnub`) e as que
      os packs novos trouxerem (7 assinadas pelo autor + 10 de porte — `9147510`)
- [x] **AR4c** — o casamento com o catálogo: cada um dos 115 pets de
      `PET_CATALOG_SEED` recebe (ou não) um modelo, pelo ELEMENTO. Pet sem
      modelo é resposta válida e fica registrada — nunca um modelo chutado
      (**medido: o seed tem 23 pets, não 115.** 20 vestidos, 3 sem modelo com
      motivo: Zunido, Candeia, Farol — `758f142`)
- [x] **AR4d** — o resultado vira dado consumível: um mapa
      `catalogId → asset` que a trilha A vai apontar nos Blueprints
      (`.specs/handoffs/pets-modelos.json`, chave = **nome** do pet: o C++
      identifica por `Name` e o catalogId só nasce no seed — `e3501df`)
- [x] **AR4e** — a lista curta do que precisa de decisão do USUÁRIO, com o
      porquê de cada um (10 itens em `decisoes` do mesmo JSON, derivados do dado)
- [x] `bun test` verde (o número só sobe a partir de **109**) — 171
- [x] Um commit por caixa, com o motivo

## As invariantes desta trilha

1. **O gerador NUNCA chuta elemento.** Sem pista, `undefined` e vai para a lista
   de decisão. Chutar encheria o catálogo de erro silencioso.
2. **Não escrever tabela de BIOMA aqui.** `BiomeEncounterFilter` (C++) já liga
   bioma→elemento, e bioma→pet sai de graça pelo elemento. Uma segunda tabela é
   L-032, e há um teste ESTRUTURAL que reprova.
3. **Um modelo sozinho não é evolução** — cadeia exige dois estágios ou mais.
4. **Prop não é bicho.** A lista `NON_CREATURE_TOKENS` é a fonte; cada token
   dela veio de um arquivo real que quase entrou como criatura.
5. **Todo defeito novo vira TESTE**, com o nome REAL que o causou.

## Arquivos desta trilha (a trilha A não toca)

- `apps/api-battle-pets/src/pet/species/**`
- `apps/api-battle-pets/src/pet/seed/**`

## Decisões do USUÁRIO que travam parte da AR4c

Respondidas em 04/09 e gravadas em `element-decisions.constant.ts` (por
FAMÍLIA, com quem decidiu e o motivo — o gerador aplica por cima do nome):

- **Bee / Armabee → Ar** ("inseto voa"). Consequência: **Zunido** é
  `Fisica/Planta` no seed e fica sem modelo até o seed mudar — decisão de seed.
- **Alien → Raio, Ninja → Fantasma, Tribal → Terra** — provisórias do gerador,
  a pedido ("vc decide por enquanto"). Marcadas `provisoria`; trocar é uma linha.
- **Cat, Dog, Slime, PinkBlob, GreenBlob, GreenSpikyBlob → Comum** — tipo novo
  pedido pelo usuário. Existe no gerador; **no motor ainda não** (item
  `motor-Comum` do JSON, com a receita: entrada em `PetTypes.json` + linhas em
  `TypeEffectiveness.json`, exige build → trilha A).

Ainda abertas (6 itens em `decisoes`):
- **Alpaking** — o insumo não traz cor por malha; pedir à trilha A um export
  da cor-base, e aí decidir Terra ou Planta.
- **Luz** — o usuário espera outro pack. Pesquisa de 04/09: nenhum pack
  gratuito com criatura de Luz ou Raio pelo nome (Quaternius, KayKit,
  poly.pizza); ver o relatório da sessão.
- **Fish, Birb, Cactoro sem Adulto** — não há Blender nesta máquina e o editor
  é da trilha A; a rota viável é um Adulto derivado por escala, declarado em dado.

## Como continuar

1. Ler este GOAL e o insumo em `.specs/handoffs/assets-importados.json`.
2. Continuar da primeira caixa aberta. Não recomeçar, não replanejar.
3. `git pull --rebase` antes de cada push.

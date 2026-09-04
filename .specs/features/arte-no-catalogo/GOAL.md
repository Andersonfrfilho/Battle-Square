# OBJETIVO — Arte no catálogo (TRILHA B: o backend)

> **Uma de duas trilhas paralelas.** A outra é `arte-no-editor` (TRILHA A).
> Elas foram desenhadas para NÃO se cruzarem — leia
> `.specs/handoffs/2026-09-04-arte-duas-sessoes.md` se precisar do porquê.

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

- [⛔] **AR4a** — o gerador lê `assets-importados.json` e classifica os 128:
      elemento, estágio, família. O relatório separa o que ele soube do que
      **não** soube
- [⛔] **AR4b** — as cadeias de evolução saem nomeadas: as 6 do Ultimate
      (`Dragon`, `Alpaking`, `Armabee`, `Glub`, `Goleling`, `Mushnub`) e as que
      os packs novos trouxerem
- [⛔] **AR4c** — o casamento com o catálogo: cada um dos 115 pets de
      `PET_CATALOG_SEED` recebe (ou não) um modelo, pelo ELEMENTO. Pet sem
      modelo é resposta válida e fica registrada — nunca um modelo chutado
- [⛔] **AR4d** — o resultado vira dado consumível: um mapa
      `catalogId → asset` que a trilha A vai apontar nos Blueprints
- [⛔] **AR4e** — a lista curta do que precisa de decisão do USUÁRIO, com o
      porquê de cada um
- [⛔] `bun test` verde (o número só sobe a partir de **109**)
- [⛔] Um commit por caixa, com o motivo

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

Modelos com evolução mas sem elemento — sem resposta, ficam fora do catálogo:
- **Alpaking** / `Alpaking_Evolved` — alpaca-rei: Terra ou Planta?
- **Armabee** / `Armabee_Evolved` — abelha blindada: Ar ou Planta?

E os sem evolução: Tribal, Ninja, Alien, Alien_Tall, Dog, Cat, Slime, PinkBlob,
GreenBlob, GreenSpikyBlob.

## Como continuar

1. Ler este GOAL e o insumo em `.specs/handoffs/assets-importados.json`.
2. Continuar da primeira caixa aberta. Não recomeçar, não replanejar.
3. `git pull --rebase` antes de cada push.

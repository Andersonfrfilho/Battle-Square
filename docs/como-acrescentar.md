# Como acrescentar coisas ao jogo

Cada seção diz **onde mexer** e, quando é em código, **por que não dá para ser dado**.
A regra que organiza tudo: *dado é o que varia; código é o que decide.*

---

## Um ELEMENTO novo (Vento, Metal, Sombra…)

**Uma linha**, em `Config/PetTypes.json`:

```json
{ "name": "Vento", "color": [0.75, 0.90, 0.95], "tiltDegrees": -40, "skills": ["voar"] }
```

E as relações dele em `Config/TypeEffectiveness.json`, no eixo `elements`.

- **`color` E `tiltDegrees` são os dois canais de silhueta.** Quem não distingue
  matiz lê a inclinação, e vice-versa. Declarar só um deixa metade dos jogadores
  sem a informação — dois testes pegaram isso em 30/08/2026.
- **O tombo precisa ser diferente de todos os outros**, e a cor também.
  `ShippedCatalogLoads` compara todos contra todos e reprova se dois se
  encostarem.
- `skills` aceita `voar`, `submergir`, `camuflar`, ou lista vazia.

Nenhuma linha de C++. `NewElementNeedsNoCode` prova isso declarando um elemento
que não existe em lugar nenhum do código e exigindo que ele chegue à tela.

## Uma ESCOLA nova

**Uma linha**, no mesmo arquivo:

```json
{ "name": "Alquimia", "crest": "Antena", "crestScale": [0.05, 0.05, 0.52] }
```

Mais as relações em `TypeEffectiveness.json`, eixo `schools`.

`crest` escolhe entre as malhas que existem: `Chama`, `Cristal`, `Orbe`,
`Antena`, `Ponta`, `Folha`, `Barbatana`, `Orelha`. **Uma malha nova é código** —
ver abaixo.

## Um GOLPE ou uma MAGIA

**Não mexe em nada do jogo.** Golpe vem do backend assinado. Em
`apps/api-battle-pets/src/pet/seed/pet-catalog.seed.ts`:

```ts
{ name: 'Rajada', power: 90, terrainEffect: 'none',
  requiresAttribute: 'flight', requiresValue: 8,
  effectStat: 'speed', effectPercent: -30 }
```

- `terrainEffect` — o que o golpe deixa na casa: `none`, `water`, `damage`.
- `requiresAttribute` / `requiresValue` — o que o pet precisa TER para usá-lo.
- `effectStat` / `effectPercent` — magia de atributo. **O sinal diz o alvo:**
  positivo sobe o seu, negativo derruba o dele.

Quatro golpes por pet, e a **ordem é o índice** que viaja no commit: trocar a
ordem depois de o pet existir muda a jogada de quem já o tinha.

## Um PET novo

Uma entrada em `pet-catalog.seed.ts`, com `type` na forma `Escola/Elemento`.
A semente é idempotente por nome, e passa pelo mesmo schema Zod que valida uma
requisição HTTP — dado escrito à mão erra, e é aqui que o erro aparece.

## Uma ARENA nova

Uma entrada em `Config/Arenas.json`. `columns`/`rows` são opcionais (3x3 quando
ausentes), e o layout precisa ter exatamente `columns × rows` casas.

---

## O que é CÓDIGO, e por quê

Estas quatro coisas parecem dado e não são. Cada uma é **comportamento com
nome** — pôr no JSON criaria uma entrada que o jogo aceita e não sabe executar.

| O quê | Onde | Por que não é dado |
|---|---|---|
| **Malha de crista** | `EPetCrestShape` + `APetView::CrestMeshFor` | Asset novo é código. O JSON escolhe entre as que existem. |
| **Atributo de magia** | `EBattleStat` + `FPetState::GetEffectiveStat` | Cada atributo entra numa fórmula diferente. Um quinto nome no JSON não teria fórmula. |
| **Propriedade de casa** | `ECellProperty` + as fases | Cada terreno tem uma regra na resolução: água habilita submergir, dano fere, bloqueada recusa movimento. |
| **Skill** | `EActionType` + `FPetSkillCatalog::SkillFromName` | Cada skill é uma postura com regra própria no núcleo. |

Ao acrescentar qualquer uma delas: o valor novo vai **no fim do enum**. Os
valores existentes viajam no hash do estado e no traço de eventos, e inserir no
meio reinterpreta o que já foi gravado.

---

## Antes de dar por pronto

1. **Existe pet de catálogo que use isso?** Ramo que o dado nunca exercita é
   ramo que nunca apareceu na tela (L-045) — e a bateria fica verde do mesmo
   jeito.
2. **Aparece na tela?** Regra que só se descobre perdendo não conta como
   entregue.
3. `./Tools/audit_test_helper_names.sh && ./Tools/sync_module_manifest.sh` antes
   da bateria — manifesto defasado faz teste novo sumir **sem a contagem cair**
   (L-046).

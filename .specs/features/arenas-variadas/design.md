# Arenas Variadas — Design

**Spec:** `.specs/features/arenas-variadas/spec.md`
**Status:** Draft — aguarda aprovação
**Escopo:** `BattleSim` (núcleo — layout e sua consulta durante resolução) **e** `BattleSquare` (nomeação/autoria de arena, tradução para o array plano que o núcleo consome). Ao contrário de Escala de Pets e Skills, esta feature muda `BattleSim` de verdade — dito explicitamente desde a spec.

---

## Onde o layout vive, e por quê

**Decisão (DP-arena-01):** `FBattleState` ganha um campo novo:

```cpp
// BattleTypes.h — BattleSim
enum class ECellProperty : uint8
{
	None = 0,
	Blocked,
	Damage,
	Buff
};

// BattleState.h — BattleSim
UPROPERTY()
TArray<uint8> CellLayout; // ECellProperty empacotado, tamanho GridSize*GridSize (9), índice = Row*3+Column
```

**Por que dentro de `FBattleState`, não um parâmetro à parte de `ResolveTurn`:** o layout é **dado de partida**, no mesmo sentido que `Pets` e `Random` já são — precisa viajar junto com o estado para sobreviver a serialização, replicação e reconexão (a mesma razão AD-004 já deu para `FBattleRandom` morar dentro do estado). Um parâmetro separado obrigaria todo consumidor de `ResolveTurn` (`ABattleArena`, `UBattleTurnCoordinator`, `FBattleBalanceSimulator` — três lugares, depois de M2/M3) a se lembrar de passar o layout toda vez, e a esquecer um deles seria um bug silencioso do mesmo tipo que L-019 acabou de expor.

**`ComputeHash()` inclui o layout.** Ele já detecta dessincronia cliente/servidor comparando estado; um layout que diverge entre as duas pontas é exatamente o tipo de bug que esse hash existe para pegar.

**Tamanho fixo (9), não dinâmico:** `GridSize` já é parametrizável em `IsInsideGrid` mas hoje sempre 3 em todo o código — layout de 9 células fixas é consistente com o estado atual do projeto, e mudar para tamanho variável é Out of Scope (spec.md).

---

## Como cada propriedade entra na resolução — reaproveitando o que já existe

### Bloqueio (F3 — Movimento)

Em `BattlePhaseMovement.cpp`, `CollectIntent` já calcula `Intent.bInsideGrid = IsInsideGrid(DestColumn, DestRow)`. A mudança:

```cpp
Intent.bInsideGrid = IsInsideGrid(DestColumn, DestRow)
	&& State.CellLayout[DestRow * 3 + DestColumn] != static_cast<uint8>(ECellProperty::Blocked);
```

Uma casa bloqueada vira exatamente o mesmo caminho de "fora da grade" — `EmitBlocked` já existe, já emite `MovimentoBloqueado`, zero evento novo (ARENA-03, satisfeito por construção, não por esforço extra).

### Dano (F3 — mesmo passo do movimento, ao fim de cada slot)

**Decisão (DP-arena-02):** dano de casa é avaliado **ao fim de cada slot**, não só ao fim do turno — consistente com o fato de que a resolução já acontece por slot (3x por turno). Um pet que fica parado 3 slots seguidos numa casa de dano toma 3 aplicações, não 1.

Adição em `ApplyMovement`, depois dos passos existentes: para cada pet vivo, consulta `CellLayout` na posição ATUAL dele (depois do movimento daquele slot, ou a mesma de antes se ele não se moveu) e, se for `Damage`, soma ao `PendingDamage` — **sem emitir evento próprio**. F5 (`ApplyResolution`) já emite `DanoAplicado` para qualquer `PendingDamage > 0`, não importa a origem (combate ou casa) — é assim que ARENA-04 fica satisfeito sem duplicar caminho de dano (Goals: "nunca um caminho de dano paralelo"). Morte por dano de casa emite `PetMorreu` pelo mesmo código de sempre — zero mudança em F5.

**Morte simultânea preservada (edge case da spec):** como o dano de casa soma no MESMO `PendingDamage` que o combate usa, um pet que leva dano de casa E de combate no mesmo slot morre pela soma dos dois, aplicados juntos em F5 — a garantia de "ninguém morre primeiro" que já existe para combate se estende de graça.

### Buff (F4 — Combate)

**Decisão (DP-arena-03, resolvendo a pergunta "Attack ou Defense" da spec):** buff é um único percentual, aplicado **contextualmente** — fortalece o `Attack` de quem ataca a partir da casa, e fortalece a `Defense` de quem está na casa quando é alvo. Um único pet, numa única casa de buff, fica mais forte nos dois papéis — não dois buffs separados.

Em `BattlePhaseCombat.cpp::ComputeDamage`, que já ajusta `EffectiveDefense` por postura (`Defendendo`), ganha o mesmo tipo de ajuste por casa:

```cpp
const bool bAttackerBuffed = CellLayout[Attacker.Row * 3 + Attacker.Column] == ECellProperty::Buff;
const bool bTargetBuffed = CellLayout[Target.Row * 3 + Target.Column] == ECellProperty::Buff;

const int32 EffectiveAttack = bAttackerBuffed ? (Attacker.Attack * CellBuffPercent) / 100 : Attacker.Attack;
const int32 DefenseFactorPercent = bTargetBuffed
	? (bTargetDefending ? CellBuffPercent * DefendingDefenseFactorPercent / 100 : CellBuffPercent)
	: (bTargetDefending ? DefendingDefenseFactorPercent : 100);
```

**Nunca persiste em `FPetState`** — o bônus existe só no cálculo daquele hit, exatamente como "Defendendo" já funciona. Sair da casa no slot seguinte remove o efeito automaticamente, porque a consulta é sempre pela posição atual (ARENA-07, satisfeito por construção).

`CellDamageAmount` e `CellBuffPercent` são constantes nomeadas (DP-arena-03), mesmo padrão de `MinDamage`/`AttackDamageMultiplierPercent`, vivendo junto delas em `BattlePhaseCombat.cpp`/um novo `BattleArenaConstants.h` em `BattleSim`.

---

## Fronteira de módulo — layout é dado plano, nunca JSON dentro de `BattleSim`

**Decisão (DP-arena-04):** `BattleSim` só conhece `TArray<uint8>` (valores de `ECellProperty` empacotados) — nunca sabe o que é JSON, nunca importa o módulo `Json`. Isso preserva AD-011 (dependências mínimas do núcleo) exatamente como `FTypeEffectivenessTable` preservou AD-012 para tipo de pet.

A autoria/nomeação de arena (ARENA-08, P2) vive inteiramente em `BattleSquare`:

```cpp
// BattleSquare — nova classe, mesmo padrão de FTypeEffectivenessTable
class BATTLESQUARE_API FArenaLayoutCatalog
{
public:
	static bool LoadFromJson(const FString& FilePath, FArenaLayoutCatalog& OutCatalog);
	bool GetLayoutByName(const FString& ArenaName, TArray<uint8>& OutLayout) const;
private:
	TMap<FString, TArray<uint8>> Layouts;
};
```

`ABattleArena::BeginBattle`/`ABattleSquareGameMode::HandleRoomReady` (Combate Online/Sala e Pareamento, já existentes) ganham o layout como mais um parâmetro na montagem do `FBattleState` inicial — mesmo ponto onde `FBattleDataTranslator` já monta `Pets`.

---

## Limite de Ferramenta

| Item | Verificável headless por mim | Como |
|---|---|---|
| Bloqueio de movimento por propriedade de casa | ✅ sim | teste de Automation, resolvedor real |
| Dano de casa acumulando em `PendingDamage`, aplicado em F5 | ✅ sim | idem |
| Morte simultânea (dano de casa + combate no mesmo slot) | ✅ sim | idem |
| Buff contextual (Attack ao atacar, Defense ao defender) | ✅ sim | idem |
| `ComputeHash()` muda quando o layout muda | ✅ sim | teste de Automation puro |
| `FArenaLayoutCatalog::LoadFromJson` | ✅ sim | mesmo padrão de `TypeEffectivenessTableTest` |
| Regressão de `BattleSim`/`BattleSquare` existentes | ✅ sim | bateria completa, disciplina de sempre |
| Arena "parece justa"/"legível" jogando de verdade | ❌ não | mesma categoria de julgamento humano das features anteriores |

---

## Riscos

| Risco | Gravidade | Mitigação |
|---|---|---|
| Esquecer de propagar `CellLayout` num dos 3 consumidores de `ResolveTurn` (`ABattleArena`, `UBattleTurnCoordinator`, `FBattleBalanceSimulator`) | média — mesma classe de bug de L-019 | layout vive DENTRO de `FBattleState`, não como parâmetro extra — elimina a classe de erro por construção, não por disciplina |
| Buff de Defense combinando com "Defendendo" (postura) produzir número inflado demais | baixa | fórmula explícita acima combina os dois multiplicadores por percentual, mesmo padrão inteiro já usado; valor de `CellBuffPercent` é ajustável sem mudar a fórmula |
| Montagem de partida posicionar pet numa casa bloqueada por erro de configuração (edge case da spec) | média | validação explícita na montagem (`BattleSquare`, não `BattleSim`) — falha alto e claro, nunca reposiciona silenciosamente |

---

## Modelo recomendado por fase (a definir em tasks.md)

| Etapa | Modelo |
|---|---|
| `ECellProperty`, `CellLayout` em `FBattleState`, `ComputeHash` | `sonnet` |
| Bloqueio em `ApplyMovement` | `sonnet` |
| Dano de casa (acumula em `PendingDamage`) | `sonnet` — **🧠 morte simultânea combinando dano de casa e combate é fácil de testar incompleto** |
| Buff contextual em `ComputeDamage` | `sonnet` — **🧠 combinar buff com "Defendendo" na mesma fórmula é fácil de inverter a ordem dos percentuais** |
| `FArenaLayoutCatalog` (`BattleSquare`) | `sonnet` |
| Fiação com `ABattleArena`/`ABattleSquareGameMode` | `sonnet` |

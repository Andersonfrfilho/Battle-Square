# Níveis, Experiência e Evolução — Design

**Spec:** `.specs/features/niveis-experiencia-evolucao/spec.md`
**Status:** Draft — aguarda aprovação
**Escopo:** módulo `BattleSquare`. `BattleSim` não é tocado — bônus de nível é aplicado na montagem de partida, mesmo padrão de `TranslateMatchup` (Escala de Pets e Skills).

**Ressalva herdada de Coleção e Captura, reafirmada aqui (B-005, `STATE.md`):** esta feature usa o mesmo mecanismo de save local — correto para Standalone/host, não para dois jogadores remotos numa sala. Não é resolvida nesta feature (depende de M7).

---

## Decision Points resolvidos

### DP-nivel-01: gancho de XP — método irmão de `CheckForCapture`, mesmo ponto de invocação

**Decisão:** `ABattleArena::GrantExperienceIfOwned(const TArray<FBattleEvent>& Trace)`, chamado nos mesmos dois pontos que `CheckForCapture` (logo após `EvaluateOutcome`, tanto em `HandlePlayerCommitted` quanto em `HandleCoordinatorTurnResolved`). Não compartilha a varredura do trace com `CheckForCapture` — são dois métodos pequenos, cada um lendo o mesmo evento `BatalhaEncerrada` uma vez, custo desprezível (um trace tem dezenas de eventos, não milhares).

**Por que não fundir num método só:** `CheckForCapture` decide sobre o pet do OPONENTE; `GrantExperienceIfOwned` decide sobre o pet do JOGADOR LOCAL. São coisas semanticamente diferentes que hoje têm regras de resultado diferentes (captura só em vitória; XP em qualquer resultado, quantidades diferentes) — juntar os dois numa função só criaria um método fazendo duas coisas com condicionais cruzados, mais difícil de testar isoladamente.

### DP-nivel-02: valores — constantes nomeadas

```cpp
namespace BattlePetProgressionConstants
{
	constexpr int32 ExperienceForWin = 50;
	constexpr int32 ExperienceForLoss = 10;   // nunca zero — jogar sempre rende algo
	constexpr int32 ExperienceForDraw = 25;

	constexpr int32 MaxLevel = 10;
	// XP cumulativo necessário para o nível N: (N - 1) * ExperiencePerLevel.
	// Linear de propósito — v1 não precisa de curva sofisticada; ajustar
	// aqui não muda a lógica, só o número.
	constexpr int32 ExperiencePerLevel = 100;

	// Bônus percentual de atributo POR NÍVEL acima de 1. Nível 1 = 100%
	// (sem bônus, catálogo puro — NIVEL-09).
	constexpr int32 AttributeBonusPercentPerLevel = 5;
}
```

Nível 10 (teto) com 5%/nível dá +45% em `Attack`/`Defense`/`Speed`/`MaxHealth` no máximo — perceptível sem quebrar o balanceamento que Escala de Pets e Skills já mede.

### DP-nivel-03: bônus aplicado por um serviço novo, depois da tradução

**Decisão:** `FPetProgressionService::ApplyLevelBonus(FPetState& State, int32 Level)` — função pura, ajusta os 4 atributos por percentual inteiro. Chamada pelo mesmo lugar que hoje monta `FBattleState` a partir do catálogo (`ABattleSquareGameMode::HandleRoomReady`), depois de `FBattleDataTranslator::TranslateMatchup`, antes de `AssembleMatchForRoom`.

```cpp
class BATTLESQUARE_API FPetProgressionService
{
public:
	// Nível → XP acumulado necessário (cumulativo). Nível 1 = 0.
	static int32 ExperienceRequiredForLevel(int32 Level);

	// Aplica XP a uma instância, processa TODOS os níveis ganhos de uma
	// vez (NIVEL-06), nunca ultrapassa MaxLevel (NIVEL-07).
	static void GrantExperience(FOwnedPetInstance& Instance, int32 Amount);

	// Deduz o nível atual a partir do Experience acumulado — não
	// armazenamos "Level" solto em FOwnedPetInstance para não ter dois
	// números que podem descolar um do outro; nível é sempre DERIVADO.
	static int32 GetLevel(const FOwnedPetInstance& Instance);

	// Bônus de atributo por nível — pura, sem side effect, testável sem
	// nada além dos dois argumentos.
	static void ApplyLevelBonus(FPetState& State, int32 Level);
};
```

**Por que nível é derivado de XP, não um campo próprio:** `FOwnedPetInstance` já tem `Experience`; adicionar `Level` como campo separado criaria uma fonte dupla de verdade (o que acontece se alguém atualizar um e esquecer do outro?). `GetLevel` recalcula a partir de `Experience` sempre — barato (é um loop de no máximo `MaxLevel` iterações) e elimina a possibilidade de os dois divergirem.

---

## Arquitetura

```
ABattleArena::HandlePlayerCommitted / HandleCoordinatorTurnResolved
       │  (já existente)
       ▼
BattleOutcome::EvaluateOutcome(...)
       │
       ├──▶ CheckForCapture(Trace)              (Coleção e Captura, já existente)
       └──▶ [NOVO] GrantExperienceIfOwned(Trace)
                  │  acha o CatalogId do pet do JOGADOR LOCAL
                  │  (PresentationsByPetId, já retido)
                  ▼
            FPetCollectionService::LoadCollection → acha a instância
                  │
                  ▼
            FPetProgressionService::GrantExperience(Instance, XP)
                  │
                  ▼
            regrava a coleção inteira (mesmo slot, save local)


ABattleSquareGameMode::HandleRoomReady
       │  (já existente, Sala e Pareamento Simples)
       ▼
FBattleDataTranslator::TranslateMatchup(...)
       │
       ▼
[NOVO] para cada pet: se CatalogId está na coleção local,
       FPetProgressionService::ApplyLevelBonus(PetState, GetLevel(Instance))
       │
       ▼
AssembleMatchForRoom(...)   (já existente)
```

---

## Limite de Ferramenta

| Item | Verificável headless por mim | Como |
|---|---|---|
| `ExperienceRequiredForLevel`, `GetLevel` (derivação a partir de XP) | ✅ sim | função pura |
| `GrantExperience` processa múltiplos níveis de uma vez, nunca passa do teto | ✅ sim | idem |
| `ApplyLevelBonus` — nível 1 sem bônus, nível > 1 com bônus correto | ✅ sim | idem |
| `ABattleArena::GrantExperienceIfOwned` credita o pet certo (jogador local, não oponente) | ✅ sim | teste de Automation, resolvedor real |
| Pet não capturado não gera XP fantasma | ✅ sim | idem |
| Fiação em `HandleRoomReady` aplicando o bônus na montagem real | ✅ parcial | testável via `AssembleMatchForRoom` (caminho já testável sem tocar o espelho de pets, como em Sala e Pareamento) |
| Progressão "parecer certa" jogando várias partidas de verdade | ❌ não | mesma categoria de julgamento humano de sempre |

---

## Riscos

| Risco | Gravidade | Mitigação |
|---|---|---|
| B-005 (save local não distingue jogadores remotos) se agravar com XP | baixa-média | já registrado, não piora nem melhora com esta feature — mesma limitação, mesmo mecanismo |
| `GetLevel` ficar caro se chamado repetidamente num loop maior | baixa | `MaxLevel` = 10, loop trivial; se a tabela crescer muito no futuro, trocar por busca binária é troca localizada, não redesenho |
| Bônus de nível desbalancear Escala de Pets e Skills (tipo + nível empilhados) | média | `FBattleBalanceSimulator` (já existente) pode rodar com instâncias de nível variado para medir — não é tarefa nova, é reuso da ferramenta |

---

## Modelo recomendado por fase (a definir em tasks.md)

| Etapa | Modelo |
|---|---|
| `FPetProgressionService` (XP, nível, bônus) | `sonnet` — **🧠 processar múltiplos níveis de uma vez sem passar do teto é fácil de errar em off-by-one** |
| `ABattleArena::GrantExperienceIfOwned` | `sonnet` |
| Fiação em `HandleRoomReady` | `sonnet` |

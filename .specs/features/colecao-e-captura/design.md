# Coleção e Captura — Design

**Spec:** `.specs/features/colecao-e-captura/spec.md`
**Status:** Draft — aguarda aprovação
**Escopo:** módulo `BattleSquare` inteiro. `BattleSim` não é tocado — captura é decidida a partir do trace já produzido (`BatalhaEncerrada`), nunca influencia a resolução do turno.

---

## Decision Points resolvidos

### DP-colecao-01: save local — `USaveGame` da Unreal

**Decisão:** `UPetCollectionSaveGame : public USaveGame`, salvo/carregado via `UGameplayStatics::SaveGameToSlot`/`LoadGameFromSlot` (módulo `Engine`, já dependência de `BattleSquare`). Nome do slot é constante nomeada (`PetCollectionSlotName`), mesmo padrão de toda constante deste projeto.

```cpp
USTRUCT()
struct FOwnedPetInstance
{
	GENERATED_BODY()

	// Identidade de captura — o id do REGISTRO DE CATÁLOGO (FLoadedPetRecord::Id),
	// nunca o Type (spec.md, edge case: dois pets do mesmo tipo são capturas
	// independentes).
	UPROPERTY()
	FString CatalogId;

	UPROPERTY()
	FString Name;

	UPROPERTY()
	FString Type;

	// Gancho para a próxima feature (Níveis, Experiência e Evolução) —
	// existe aqui, começa em 0, não fazemos nada com isto ainda além de
	// somar no caso P2 (vitória redundante).
	UPROPERTY()
	int32 Experience = 0;
};

UCLASS()
class BATTLESQUARE_API UPetCollectionSaveGame : public USaveGame
{
	GENERATED_BODY()
public:
	UPROPERTY()
	TArray<FOwnedPetInstance> OwnedPets;
};
```

### DP-colecao-02: onde o gancho de captura se conecta — `ABattleArena`, junto de `EvaluateOutcome`

**Decisão:** logo depois de `BattleOutcome::EvaluateOutcome` (o mesmo ponto corrigido em L-019), `ABattleArena` varre o trace resultante por `BatalhaEncerrada`. Se `Value == LocalPlayerSide` (vitória do jogador local), chama `FPetCollectionService::CaptureIfNew` com os dados do pet OPONENTE (o outro lado).

**Por que aqui, e não em `UBattleResultWidget`:** o widget já consome `BatalhaEncerrada` para exibir resultado (Apresentação do Combate), mas ele é puramente de leitura/exibição — não é dele decidir persistir nada. `ABattleArena` já é quem orquestra a resolução do turno (`HandlePlayerCommitted`/`HandleCoordinatorTurnResolved`), então é o lugar natural para mais uma consequência de "a batalha terminou", sem introduzir um observador novo do trace.

### DP-colecao-03: identidade da instância — `FOwnedPetInstance`, referenciando `CatalogId`

Já resolvido acima — `FOwnedPetInstance` é um tipo novo (não reaproveita `FLoadedPetRecord`/`FPetPresentationInfo` diretamente), porque ele representa algo semanticamente diferente: não é "um pet do catálogo", é "a posse de um jogador sobre um registro de catálogo", com campos que só fazem sentido nesse contexto (`Experience`).

**Consequência que exige uma mudança pequena em código já existente:** `FPetPresentationInfo` (`BattleDataTranslator.h`) ganha um campo `CatalogId` (o `FLoadedPetRecord::Id` original, preservado através da tradução) — sem isso, no momento em que a batalha termina, `ABattleArena` não tem como saber qual registro de catálogo o pet oponente representa (a fronteira do núcleo já apagou essa informação de `FPetState`, de propósito, desde AD-012). `ABattleArena` passa a reter o `TArray<FPetPresentationInfo>` recebido em `BeginBattle` (hoje descartado depois de `SpawnPetViews`), para poder consultar por `PetId` quando o trace de fim de batalha chegar.

### DP-colecao-04: "o jogador venceu" — mesma convenção de `UBattleResultWidget`

**Decisão:** `ABattleArena` ganha um `uint8 LocalPlayerSide = 0` (mesma convenção — Side 0 é sempre o jogador local, já usada implicitamente em `HandlePlayerCommitted`/`ConfigureNetworkedOpponent`/`UBattleResultWidget::ApplyBattleEndedEvent`). Nenhuma lógica nova de "quem sou eu" — só nomeia o que já era verdade.

---

## Arquitetura

```
ABattleArena::HandlePlayerCommitted / HandleCoordinatorTurnResolved
       │  (já existente, corrigido em L-019)
       ▼
BattleOutcome::EvaluateOutcome(Result.NextState, Result.Trace)
       │
       ▼
[NOVO] ABattleArena::CheckForCapture(Result.Trace)
       │  varre por BatalhaEncerrada; se Value == LocalPlayerSide,
       │  acha o PetPresentationInfo do lado OPOSTO (pelos
       │  PresentationsByPetId retidos em BeginBattle)
       ▼
FPetCollectionService::CaptureIfNew(SlotName, FOwnedPetInstance{...})
       │  carrega o save, checa CatalogId, adiciona se novo, salva
       ▼
UPetCollectionSaveGame (USaveGame, slot local)
```

### `FPetCollectionService`

```cpp
class BATTLESQUARE_API FPetCollectionService
{
public:
	// Retorna true se uma instância NOVA foi adicionada (captura real);
	// false se o CatalogId já estava na coleção (nenhuma duplicata).
	static bool CaptureIfNew(const FString& SlotName, const FOwnedPetInstance& Instance);

	// Testável sem tocar disco de verdade — recebe/devolve o save já
	// carregado, quem chama decide se persiste (ver Limite de Ferramenta).
	static bool CaptureIfNewInMemory(UPetCollectionSaveGame* SaveGame, const FOwnedPetInstance& Instance);

	static TArray<FOwnedPetInstance> LoadCollection(const FString& SlotName);
};
```

**Por que separar `CaptureIfNew` (toca disco) de `CaptureIfNewInMemory` (pura):** a lógica que decide "é novo ou não" é testável sem `UGameplayStatics`/disco real — mesma disciplina de Limite de Ferramenta já usada no projeto inteiro (headless primeiro, I/O real por último).

---

## Limite de Ferramenta

| Item | Verificável headless por mim | Como |
|---|---|---|
| `CaptureIfNewInMemory` — pet novo captura, pet repetido não duplica | ✅ sim | função pura, `UPetCollectionSaveGame` construído em memória no teste |
| `ABattleArena::CheckForCapture` identifica o pet oponente corretamente | ✅ sim | teste de Automation, resolvedor real, `BatalhaEncerrada` real |
| Derrota/empate não captura | ✅ sim | idem |
| `SaveGameToSlot`/`LoadGameFromSlot` persistindo de verdade em disco | ⚠️ parcial | `UGameplayStatics::SaveGameToSlot` funciona headless (não depende de GPU/GUI), mas grava no diretório de save real do usuário — teste precisa de um slot de teste dedicado e limpar depois, mesmo padrão de fixture já usado (`PetMirrorFixture`, `TypeEffectivenessFixture`) |
| Coleção "parecer certa" numa tela de verdade | ❌ não | mesma categoria de julgamento humano de sempre — sem UI ainda (Out of Scope) |

---

## Riscos

| Risco | Gravidade | Mitigação |
|---|---|---|
| `ABattleArena` reter `Presentations` indefinidamente crescendo memória entre partidas | baixa | array pequeno (2 entradas hoje, 1 pet por lado), sobrescrito a cada `BeginBattle` novo |
| Save corrompido travar o carregamento da coleção | média | `LoadCollection`/`CaptureIfNew` tratam `LoadGameFromSlot` retornando `nullptr` como "coleção vazia", nunca crash — mesmo padrão de `FPetDataLoader`/`FTypeEffectivenessTable` |
| Testes de save real poluindo o slot de save de desenvolvimento do usuário | média | slot de teste com nome dedicado (`PetCollectionTestSlot`, nunca o slot de produção `PetCollection`), limpo ao fim do teste |

---

## Modelo recomendado por fase (a definir em tasks.md)

| Etapa | Modelo |
|---|---|
| `FOwnedPetInstance`, `UPetCollectionSaveGame` | `sonnet` |
| `FPetCollectionService` (captura, carga) | `sonnet` |
| `FPetPresentationInfo::CatalogId` + retenção em `ABattleArena` | `sonnet` |
| `ABattleArena::CheckForCapture`, fiação com `HandlePlayerCommitted`/`HandleCoordinatorTurnResolved` | `sonnet` — **🧠 identificar o lado OPOSTO ao vencedor é fácil de inverter** |

# Sala e Pareamento Simples — Design

**Spec:** `.specs/features/sala-e-pareamento/spec.md`
**Status:** Draft — aguarda aprovação
**Escopo:** módulo `BattleSquare`, subpasta `Net/` (mesma do Combate Online). Primeira feature do projeto a introduzir `AGameModeBase`/`APlayerController` — até aqui, todo o combate era testado sem nenhum dos dois (`ABattleArena` sozinho, `UBattleTurnCoordinator` chamado direto).

---

## Princípio que governa este design: repetir a separação que já funcionou

Combate Online separou **lógica pura testável** (`UBattleTurnCoordinator`, sem `UWorld`) de **fiação de ator** (`ABattleArena`, que existe e é spawnável, mas cuja lógica de decisão vive fora dela). Este design repete exatamente esse corte:

- **`UBattleRoomRegistry`** — lógica pura de sala (gerar código, entrar, atribuir lado, controlar desconexão/reconexão). Sem `AActor`, sem `UWorld`, tempo sempre injetado — testável headless com `NewObject`, mesmo padrão de `UBattleTurnCoordinator`.
- **`ABattleSquareGameMode`** — fiação fina: `PostLogin`/`Logout` chamam o registro; RPCs do `APlayerController` chamam o registro; quando o registro diz "sala cheia", o GameMode instancia `ABattleArena` e conecta com o que o Combate Online já construiu (`ConfigureNetworkedOpponent`, `UBattleNetCommitComponent`).

Isto não é só estilo — é o que mantém a mesma honestidade de "Limite de Ferramenta": a parte que decide QUANDO uma sala está pronta, QUEM é Side 0/1, e QUANDO um jogador é considerado abandonado é 100% testável sem rede real. A parte que só existe com `AGameModeBase`/`APlayerController` de verdade (login, RPC de criação de sala pela rede) continua precisando do mesmo roteiro manual que `combate-online-rede.md` já documenta — estendido aqui, não duplicado.

---

## Decision Points resolvidos

### DP-sala-01: onde a sala vive — **mesmo processo do servidor de combate**

**Decisão:** `UBattleRoomRegistry` é um objeto dono pelo `ABattleSquareGameMode`, no mesmo processo que já roda `UBattleTurnCoordinator`. Nenhum serviço de lobby separado.

**Razão:** mesmo raciocínio de DP-online-02 (Combate Online) — não existe conta de jogador, não existe necessidade de persistência entre partidas, e o volume (salas efêmeras, 2 jogadores cada) não justifica infraestrutura separada. Um serviço de lobby separado faria sentido no dia em que existir matchmaking real (fila, ranking) — explicitamente fora de escopo aqui e lá.

### DP-sala-02: armazenamento do código — **em memória**

**Decisão:** `TMap<FString, FBattleRoomState>` dentro de `UBattleRoomRegistry`. Nada em disco, nada em banco.

**Razão:** sala é efêmera por definição (spec.md, Out of Scope). Se o servidor reiniciar, as salas ativas somem — comportamento aceitável e, mais que aceitável, **correto**: uma sala sobrevivendo a um restart do servidor sem que a partida em si tenha persistido (`FBattleState` também não é salvo em disco) seria uma meia-persistência inconsistente.

### DP-sala-03: atribuição de lado — **quem cria é sempre Side 0**

**Decisão:** confirmado da spec. Determinístico, sem negociação, sem coin flip.

**Razão:** simplicidade sem custo — nada no jogo depende de qual lado é "melhor" (o tabuleiro 3x3 é simétrico, `FBattleResolver` não favorece nenhum lado). Zero motivo para complicar.

### DP-sala-04: identidade de reconexão — **segredo de sessão por sala+lado, não conta**

**Decisão:** ao entrar numa sala (criando ou juntando), o servidor gera um `FGuid` (`FBattleRoomSideSecret`) e entrega ao cliente via RPC. Para reconectar, o cliente reenvia código de sala + segredo; o registro confirma que bate com o lado desconectado antes de tratar como o mesmo jogador.

**Razão:** a spec marca "reconexão automática sem o código" como fora de escopo, mas ainda assim precisa de **algum** jeito do servidor confirmar "este cliente que voltou é o mesmo jogador que caiu, não um terceiro tentando roubar o lado". Um `FGuid` de sessão é o mínimo que resolve isso sem inventar conta — mesma classe de mecanismo de uma "cookie de sessão", só que sem nada persistente por trás. Se o cliente perder o segredo (ex.: fechou o processo sem salvar nada), ele simplesmente não reconecta como o mesmo jogador — vira uma tentativa de entrar numa sala cheia, tratada como erro normal (SALA-04).

**O que isto NÃO é:** não é autenticação. Qualquer um com o código E o segredo entra como aquele jogador — para uma partida casual e efêmera, isso é proporcional. Ver M7 para quando isso deixar de ser suficiente.

---

## Arquitetura

### `UBattleRoomRegistry` — lógica pura, testável headless

```cpp
USTRUCT()
struct FBattleRoomOccupant
{
    GENERATED_BODY()
    FGuid Secret;
    bool bConnected = true;
    double DisconnectedAtSeconds = 0.0; // só significa algo se !bConnected
};

USTRUCT()
struct FBattleRoomState
{
    GENERATED_BODY()
    FBattleRoomOccupant Side0;
    FBattleRoomOccupant Side1;
    bool bHasSide1 = false; // Side0 sempre existe se a sala existe
    bool bMatchStarted = false;
};

UCLASS()
class BATTLESQUARE_API UBattleRoomRegistry : public UObject
{
public:
    // SALA-01/02: gera código único entre salas VIVAS, cria a sala com
    // Side0 ocupado. Retorna o código e o segredo do criador.
    FString CreateRoom(FGuid& OutCreatorSecret);

    // SALA-03/04: entra numa sala existente com 1 vaga. Distingue
    // "não encontrada" de "cheia" (retorno enum, não bool).
    EBattleRoomJoinResult JoinRoom(const FString& Code, FGuid& OutJoinerSecret);

    // SALA-10: reconexão — código + segredo batendo com um lado
    // desconectado da mesma sala. Marca o lado como reconectado.
    bool TryReconnect(const FString& Code, const FGuid& Secret);

    // SALA-09: marca um lado como desconectado, com o tempo injetado.
    void MarkDisconnected(const FString& Code, uint8 Side, double CurrentTimeSeconds);

    // Chamado periodicamente (tempo injetado) — para cada sala com um
    // lado desconectado há mais que AbandonTimeoutSeconds, dispara o
    // delegate de abandono (quem ouve decide o que fazer — GameMode
    // chama UBattleTurnCoordinator::DeclareAbandonment).
    void CheckAbandonment(double CurrentTimeSeconds);

    DECLARE_MULTICAST_DELEGATE_TwoParams(FRoomReadySignature, const FString& /*Code*/, void* /*Unused — placeholder de assinatura, ver nota*/);
    // Assinatura real definida na implementação: carrega o código da
    // sala; quem ouve (GameMode) consulta o estado e monta a arena.
    FRoomReadySignature OnRoomReady; // SALA-05/06

    DECLARE_MULTICAST_DELEGATE_TwoParams(FRoomAbandonedSignature, const FString& /*Code*/, uint8 /*PresentSide*/);
    FRoomAbandonedSignature OnRoomAbandoned; // SALA-09

    // SALA-11: sala sem nenhum ocupante conectado, e sem chance de
    // reconexão (timeout dos dois lados vencido), é destruída.
    void CheckEmptyRooms(double CurrentTimeSeconds);

private:
    TMap<FString, FBattleRoomState> ActiveRooms;
    static FString GenerateCandidateCode(); // exclui 0/O, 1/I (spec.md)
};
```

### `ABattleSquareGameMode` — fiação fina

```cpp
UCLASS()
class BATTLESQUARE_API ABattleSquareGameMode : public AGameModeBase
{
public:
    ABattleSquareGameMode();

protected:
    virtual void Logout(AController* Exiting) override; // SALA-09: marca desconexão

private:
    UPROPERTY()
    TObjectPtr<UBattleRoomRegistry> RoomRegistry;

    // Ligado a OnRoomReady: monta ABattleArena real, UBattleTurnCoordinator
    // real, chama ConfigureNetworkedOpponent, monta FBattleState inicial
    // com pets reais (FPetDataLoader::LoadVerifiedPets — reaproveitado do
    // Backend de Dados de Pet, SALA-08).
    void HandleRoomReady(const FString& Code);

    // Ligado a OnRoomAbandoned: chama
    // UBattleTurnCoordinator::DeclareAbandonment do lado presente.
    void HandleRoomAbandoned(const FString& Code, uint8 PresentSide);
};
```

`APlayerController` ganha dois RPCs simples (`Server_CreateRoom`, `Server_JoinRoom(FString Code)`) que só repassam para `RoomRegistry` via `GetGameMode<ABattleSquareGameMode>()` — sem lógica própria, mesmo espírito de `UBattleNetCommitComponent::Server_SubmitCommit` (fino, delega).

### Montagem do `FBattleState` inicial (SALA-08)

**Decisão pragmática, registrada honestamente:** sem seleção de time (fora de escopo, spec da Apresentação do Combate), `HandleRoomReady` pega os 2 primeiros pets de `FPetDataLoader::LoadVerifiedPets()` — mesmo padrão que os testes E2E do Backend de Dados de Pet já usam com pets de fixture. Isto é um placeholder deliberado, não uma decisão de produto — o dia em que existir seleção de time, este é o ponto exato que muda.

---

## Limite de Ferramenta

| Item | Verificável headless por mim | Como |
|---|---|---|
| Geração de código (formato, exclusão de caracteres ambíguos, unicidade entre salas vivas) | ✅ sim | função pura, teste de Automation |
| Criar/entrar/cheia/inválida (`CreateRoom`/`JoinRoom`) | ✅ sim | `NewObject<UBattleRoomRegistry>`, sem `UWorld` |
| Atribuição de lado determinística | ✅ sim | idem |
| Desconexão + timeout de abandono + `OnRoomAbandoned` | ✅ sim | tempo injetado, mesmo padrão de `UBattleTurnCoordinator` |
| Reconexão por segredo (aceita/rejeita) | ✅ sim | idem |
| Sala vazia sendo destruída | ✅ sim | idem |
| `ABattleSquareGameMode::HandleRoomReady` monta `ABattleArena` de verdade | ✅ parcial | `CreateHeadlessTestWorld` (já usado em `BattleArenaTest.cpp`) cobre a construção; não cobre `PostLogin` real de um `APlayerController` de rede |
| RPCs `Server_CreateRoom`/`Server_JoinRoom` atravessando uma conexão real | ❌ não | mesma barreira de B-004 (Combate Online) — `ListenServer` funciona, `DedicatedServer` não compila nesta engine |
| Dois processos completando sala→partida→abandono de ponta a ponta | ❌ não | idem |

As duas últimas linhas ❌ estendem exatamente o mesmo roteiro manual de `docs/verification/combate-online-rede.md` — não criam um documento novo, viram uma seção adicional nele (T14 desta feature).

---

## Riscos

| Risco | Gravidade | Mitigação |
|---|---|---|
| Código de sala colidir entre salas vivas | baixa | `GenerateCandidateCode` testado a checar contra `ActiveRooms` antes de aceitar; espaço de códigos grande o bastante (formato definido em T1) para colisão ser rara mesmo sem essa checagem |
| Terceiro "roubar" um lado abandonado sabendo só o código | média | mitigado pelo segredo de reconexão (DP-sala-04) — só isso não é forte contra um atacante determinado, mas é proporcional ao valor em risco (uma partida casual, sem conta) |
| `HandleRoomReady` presumir pets que não existem no espelho local (mirror vazio) | baixa | `FPetDataLoader::LoadVerifiedPets` já tem tratamento de erro explícito (`MissingMirrorFailsExplicitly`, testes existentes) — `HandleRoomReady` só precisa propagar a falha, não a esconder |

---

## Modelo recomendado por fase (a definir em tasks.md)

| Etapa | Modelo |
|---|---|
| `UBattleRoomRegistry` — geração de código, criar/entrar | `sonnet` |
| Desconexão, timeout de abandono, reconexão por segredo | `sonnet` — **🧠 mesma classe de risco de corrida que T6 do Combate Online** |
| `ABattleSquareGameMode`/RPCs de `APlayerController` | `sonnet` |
| Roteiro de verificação (extensão de `combate-online-rede.md`) | `haiku` |

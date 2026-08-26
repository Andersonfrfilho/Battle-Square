# Escala de Pets e Skills — Design

**Spec:** `.specs/features/escala-pets-skills/spec.md`
**Status:** Draft — aguarda aprovação
**Escopo:** módulo `BattleSquare` inteiro. **`BattleSim` não é tocado** — decisão central deste design, explicada abaixo. É a mesma disciplina de fronteira que Combate Online manteve (zero linhas em `BattleSim`).

---

## A decisão que evita mexer no núcleo

DP-escala-01 perguntava: como o tipo do pet entra em `FBattleState`/`FPetState` para o resolvedor consultar, sem violar AD-012?

**Resposta: não entra.** A efetividade de tipo é resolvida **inteiramente fora de `BattleSim`**, no momento em que o `FPetState` é montado (`FBattleDataTranslator`, hoje em `BattleSquare`) — o `Attack` do pet já chega ao núcleo **pré-multiplicado** pela efetividade contra o oponente específico daquela partida. `ComputeDamage` (`BattlePhaseCombat.cpp`) não muda uma linha; ele nunca soube o que é "tipo" e continua sem saber.

**Por que isto funciona hoje, e por que é um trade-off explícito:** com 1 pet por lado (escopo atual — N pets por lado é Out of Scope, B-003), o tipo do único oponente é conhecido no instante em que a partida é montada, e nunca muda durante a partida. "Attack efetivo contra ESTE oponente" é, portanto, um número fixo desde o primeiro turno — pré-calcular é correto, não uma aproximação. **O dia em que N pets por lado chegar** (B-003, expansão futura), um atacante poderá ter mais de um alvo possível com tipos diferentes, e "Attack pré-multiplicado" deixa de fazer sentido — a efetividade precisaria virar uma consulta em tempo de resolução, e aí sim a pergunta de DP-escala-01 (tipo entrando em `FPetState` de verdade) volta à mesa. Registrado aqui para não ser esquecido, não para resolver agora.

**O que isso ganha, comparado a fazer `TypeId` entrar no núcleo:**
- Zero mudança em `BattleSim` — zero risco de regressão nos 44 testes existentes, zero novo símbolo cruzando a fronteira que `probe_isolation.sh` audita.
- AD-012 continua **literalmente** intacto, não só "no espírito" — nenhum dado de tipo atravessa a fronteira do núcleo, nem como `FGameplayTag` nem como inteiro.
- `Tools/audit_determinism.sh` não precisa de nenhuma mudança — a matemática de efetividade é `Attack * Percentual / 100`, inteiro puro, e vive em `BattleSquare`, fora do escopo que o script varre (mas seguiria a mesma disciplina se estivesse dentro).

---

## Decision Points resolvidos

### DP-escala-02: tabela de efetividade — **arquivo JSON local, não backend**

**Decisão:** um arquivo `Config/TypeEffectiveness.json` (par de tipos → percentual), lido uma vez na montagem da partida, parseado com `FJsonSerializer` (já disponível via módulo `Json`, sem dependência nova).

**Por que não o backend de pets (AD-008/AD-014):** a arquitetura de backend externo existe para **conteúdo administrado com ciclo de vida próprio** — 250 pets, cadastrados/editados via API, cada um uma entidade independente. Uma tabela de efetividade de tipo é o oposto disso: um punhado de linhas (N tipos × N tipos), que muda raramente, e que é **configuração de balanceamento do jogo**, não conteúdo administrável por operação. Tratá-la como se fosse conteúdo (nova tabela no Postgres, novo endpoint, extensão do sync sidecar) importaria uma arquitetura pesada para um problema pequeno.

**Custo aceito:** "sem recompilar" (ESCALA-06) vira "editar o JSON e reiniciar o servidor" — não um C++ recompile, que é o que a spec pede. Se um dia a efetividade precisar ser editada em produção sem restart, ISSO é motivo suficiente para promover a tabela ao backend — decisão adiável sem custo de retrabalho grande (a leitura já é isolada em uma função só, `FTypeEffectivenessTable::Load`).

**Formato:**
```json
{
  "types": ["Fogo", "Agua", "Planta"],
  "effectiveness": [
    { "attacker": "Fogo", "defender": "Planta", "percent": 150 },
    { "attacker": "Fogo", "defender": "Agua", "percent": 50 },
    { "attacker": "Agua", "defender": "Fogo", "percent": 150 }
  ]
}
```
Par ausente = neutro (100%) — mesma filosofia de "ausência não é erro" já usada em `GetDirectionDelta` (spec.md, Edge Cases).

### DP-escala-03: ferramenta de simulação em lote — **reaproveita Automation**

**Decisão:** um `IMPLEMENT_SIMPLE_AUTOMATION_TEST` novo, `BattleSquare.Balance.RunBatchSimulation`, que roda N combates internamente (N configurável por uma constante ou parâmetro), agrega estatísticas, e reporta via `AddInfo`/log — não é pass/fail no sentido usual (sempre "passa" se rodar sem crash), é uma ferramenta de medição usando a infraestrutura de execução headless que já existe.

**Por que não um commandlet dedicado:** todo o projeto já roda testes headless via `Automation RunTests` num pipeline provado (`Tools/*.sh`, dezenas de execuções nesta sessão). Um commandlet novo duplicaria esse pipeline sem necessidade — a única coisa que muda é o CONTEÚDO do teste (medir em vez de assertar), não o mecanismo de execução.

### DP-escala-04: multiplicadores — **150%/50%, constantes nomeadas**

`SuperEffectiveDamagePercent = 150`, `ResistedDamagePercent = 50` — mesmo padrão de `AttackDamageMultiplierPercent`/`MagicDamageMultiplierPercent`. Vivem em `BattleSquare` (não em `BattleSim`), porque é ali que a efetividade é aplicada.

---

## Arquitetura

```
Backend de Pets (existente)          Tabela de Efetividade (NOVA)
  FLoadedPetRecord.Type (string)       Config/TypeEffectiveness.json
       │                                      │
       ▼                                      ▼
FBattleDataTranslator::TranslateMatchup(LeftPetRecord, RightPetRecord, EffectivenessTable, ...)
       │  calcula o Attack efetivo dos DOIS lados, um em função do
       │  tipo do outro, ANTES de montar FPetState
       ▼
FPetState (Attack já pré-multiplicado — igual a hoje, um int puro)
       │
       ▼
FBattleResolver::ResolveTurn   ← BattleSim, ZERO mudança
```

### Nova função de tradução

```cpp
// BattleDataTranslator.h — nova, ao lado de TranslatePet (que continua
// existindo para o caso sem efetividade de tipo, ex.: testes antigos).
static void TranslateMatchup(
    const FLoadedPetRecord& LeftSource, const FLoadedPetRecord& RightSource,
    const FTypeEffectivenessTable& EffectivenessTable,
    uint8 LeftPetId, uint8 RightPetId,
    FPetState& OutLeftState, FPetPresentationInfo& OutLeftPresentation,
    FPetState& OutRightState, FPetPresentationInfo& OutRightPresentation);
```

`TranslateMatchup` chama a lógica de `TranslatePet` duas vezes internamente (não duplica código), e depois ajusta `Attack` de cada lado: `EffectiveAttack = BaseAttack * EffectivenessTable.GetPercent(MyType, OpponentType) / 100`.

### `FTypeEffectivenessTable`

```cpp
class BATTLESQUARE_API FTypeEffectivenessTable
{
public:
    static bool LoadFromJson(const FString& FilePath, FTypeEffectivenessTable& OutTable);
    int32 GetPercent(const FString& AttackerType, const FString& DefenderType) const; // 100 se ausente
private:
    TMap<TPair<FString, FString>, int32> Entries;
};
```

Função pura o bastante para testar sem I/O real (constrói a tabela em memória direto no teste, sem depender do arquivo em disco — mesmo padrão de `MakeCoordinatorDuelState()` em outros testes).

---

## Ferramenta de balanceamento — detalhe

```cpp
// Roda N combates entre duas composições fixas, agrega e reporta.
struct FBatchSimulationResult
{
    int32 LeftWins = 0;
    int32 RightWins = 0;
    int32 Draws = 0;
    double AverageTurns = 0.0;
    double AverageDamagePerTurn = 0.0;
};

FBatchSimulationResult RunBatchSimulation(
    const FPetState& LeftTemplate, const FPetState& RightTemplate,
    int32 NumSimulations, uint64 BaseSeed);
```

Cada simulação roda com `BaseSeed + Index` (determinístico, reproduzível — ESCALA-08), usa `FDumbOpponentAI` dos dois lados (não há jogador humano numa simulação de balanceamento), e chama `FBattleResolver::ResolveTurn` real, turno a turno, até `bBattleEnded`.

---

## Limite de Ferramenta

| Item | Verificável headless por mim | Como |
|---|---|---|
| `FTypeEffectivenessTable::GetPercent` (par presente, ausente, neutro) | ✅ sim | função pura, teste de Automation |
| `LoadFromJson` (arquivo válido, ausente, malformado) | ✅ sim | fixture de teste, mesmo padrão de `PetDataLoaderTest` |
| `TranslateMatchup` produz `Attack` efetivo correto para super-efetivo/resistido/neutro | ✅ sim | teste de Automation com `FLoadedPetRecord` de fixture |
| `RunBatchSimulation` é determinístico por seed | ✅ sim | rodar duas vezes com a mesma seed, comparar agregados byte a byte |
| Regressão de `BattleSim` (44 testes) e `BattleSquare` existentes | ✅ sim | bateria completa, disciplina de sempre |
| Efetividade de tipo "parece certa" jogando de verdade | ❌ não | julgamento humano — mesma categoria dos itens ❌ de `apresentacao-combate-visual.md` |

---

## Riscos

| Risco | Gravidade | Mitigação |
|---|---|---|
| `TranslateMatchup` divergir de `TranslatePet` (dois caminhos de tradução, um deles esquecido numa correção futura) | média | `TranslateMatchup` chama a lógica de `TranslatePet` internamente para os campos que não mudam — nunca duplica a cópia de `Attack`/`Defense`/`Speed`/`MaxHealth`, só ajusta `Attack` depois |
| JSON malformado/ausente travar a montagem de partida em produção | média | `LoadFromJson` retorna `false` explicitamente (mesmo padrão de `FPetDataLoader::LoadVerifiedPets`) — quem chama decide: falha alto e claro, nunca silenciosamente neutro |
| Pré-multiplicar `Attack` confundir alguém lendo `FPetState.Attack` esperando o valor "de catálogo" do pet | baixa | documentado no código e aqui; `FPetPresentationInfo` (camada de apresentação) pode expor o `Attack` de catálogo separadamente se um dia a UI precisar mostrá-lo — fora de escopo agora |

---

## Modelo recomendado por fase (a definir em tasks.md)

| Etapa | Modelo |
|---|---|
| `FTypeEffectivenessTable` (carregar, consultar) | `sonnet` |
| `TranslateMatchup` (a peça que decide o `Attack` efetivo) | `sonnet` — **🧠 é fácil inverter atacante/defensor por engano, ou aplicar o percentual no lado errado** |
| Ferramenta de simulação em lote | `sonnet` |
| Fiação com o backend de pets (tipo já vem como string, `FLoadedPetRecord::Type`) | `sonnet` |

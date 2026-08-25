# Backend de Dados de Pet — Verificação de Ponta a Ponta (T20–T22)

**Spec:** `.specs/features/backend-dados-pet/spec.md`
**Data:** 2026-08-25
**Status:** ✅ Verificado — cadeia completa, backend real → servidor real → Postgres real → Unreal real

Este documento não substitui os testes automatizados de cada fase — ele
amarra as evidências já produzidas em T1–T19 numa narrativa de ponta a
ponta, como o `tasks.md` pede para T20–T22. Nenhum resultado aqui foi
simulado: toda linha vem de um comando real, rodado nesta sessão.

---

## T20 — Ponta a Ponta

**Requisito:** backend assina → sidecar verifica e grava → loader lê e reverifica → tradutor produz `FPetState` válido → resolvedor aceita.

### Cadeia comprovada, com a evidência de onde cada etapa foi provada

| Etapa | Provado em | Evidência |
|---|---|---|
| 1. Backend cria e assina um pet | Fase 2 (T5) + Fase 3 (T9) | `POST /v1/pets` real contra Postgres; assinatura Ed25519 verificada por par genuíno backend→C++ |
| 2. Sidecar sincroniza e verifica | Fase 4 (T14) | `runSyncCycle()` real: `{"fetched":6,"written":6,"rejected":0}` contra o backend rodando |
| 3. Espelho local persistido, criptografado | Fase 4 (T12) | `sqlite3` cru falha ao abrir; decifra corretamente só com a chave certa |
| 4. `UPetDataLoader` (C++) lê e reverifica | Fase 5 (T17) | `BattleSquare.PetDataLoader.LoadsAndVerifiesRealSyncedMirror` — `Success`, pet de fixture dedicado carregado com os atributos exatos gravados via API |
| 5. `FBattleDataTranslator` produz `FPetState` | Fase 5 (T18) | `BattleSquare.BattleDataTranslator.CopiesNumericFieldsAndInitializesHealth` — `Success` |
| 6. `FBattleResolver::ResolveTurn` (núcleo) aceita | Fase 5 (T18) | `BattleSquare.BattleDataTranslator.TranslatedStateFeedsResolverCorrectly` — `Success`: Fluffy (Attack=20) ataca Spike, `Result.NextState.Pets[1].Health < RightBattleState.Health` confirmado |

**Conclusão:** as 6 etapas da cadeia têm prova própria, individual, rodada contra sistemas reais (Postgres, servidor Bun, editor Unreal headless) — não há elo simulado ou presumido entre o dado que nasce no backend e o dano que ele causa dentro do núcleo de combate.

---

## T21 — Resiliência a Backend Fora do Ar

**Requisito:** backend inacessível durante sync não afeta partida em andamento nem trava o sidecar.

### Evidência 1 — sidecar sobrevive ao backend caindo no meio de um ciclo (Fase 4, T14)

```
=== Cenário 1: backend fora do ar (porta errada) — não deve travar nem lançar exceção ===
[sync-loop] backend inacessível — mantendo espelho atual, tentando de novo no próximo ciclo:
Unable to connect. Is the computer able to access the url?
{"fetched":0,"written":0,"rejected":0,"backendUnreachable":true}
processo chegou até aqui sem crashar — confirmado
```

O processo do sidecar não morre, não lança exceção não tratada, e o espelho local **não é apagado nem sobrescrito** — próximo ciclo tenta de novo (PETDB-08).

### Evidência 2 — `UPetDataLoader` funciona com backend indisponível, DESDE que o espelho já tenha sincronizado antes

Verificado com o backend **confirmadamente desligado** antes de rodar (checado com `curl` no `/health`, recusado) — não presumido, testado:

```
$ curl -s -m 2 http://localhost:3100/health
backend confirmado fora do ar

$ UnrealEditor ... -ExecCmds="Automation RunTests BattleSquare.PetDataLoader.LoadsAndVerifiesRealSyncedMirror; Quit"
Found 1 automation tests based on 'BattleSquare.PetDataLoader.LoadsAndVerifiesRealSyncedMirror'
Test Completed. Result={Success} Name={LoadsAndVerifiesRealSyncedMirror}
TEST COMPLETE. EXIT CODE: 0
```

O espelho local (gerado em ciclo de sync anterior, quando o backend estava no ar) continua sendo lido normalmente pelo `UPetDataLoader` — a leitura é 100% local (arquivo + chave), nunca toca rede. Isso é a garantia estrutural do design (AD-014/AD-018): a montagem de uma batalha lê exclusivamente do espelho, nunca do backend.

### O que isto NÃO cobre (fora de escopo desta spec)

- Comportamento se o backend cair **durante a primeira inicialização**, com o espelho ainda vazio — esse caminho já está coberto por `MissingMirrorFailsExplicitly` (T17): o servidor de combate recusa iniciar partidas, erro explícito (PETDB-06), não é o mesmo cenário de "partida em andamento".

---

## T22 — Adulteração do Espelho Local

**Requisito:** corromper o espelho local depois de sincronizado, confirmar que o `UPetDataLoader` rejeita e loga.

### Evidência — fixture com 1 registro adulterado direto no arquivo, fora do fluxo de sync

Gerado decifrando um espelho real e alterando 1 campo (`attack: 15 → 515`) de 1 registro diretamente via SQL, sem passar pela assinatura de novo — depois recriptografado com a mesma chave (simula um atacante com acesso ao disco, mas sem a chave privada do backend para forjar uma assinatura nova, ver AD-016).

```
BattleSquare.PetDataLoader.TamperedRecordIsRejectedOthersSurvive — Success

Asserções confirmadas:
- Carregamento prossegue apesar de 1 registro adulterado (chave certa, arquivo decifra)
- Exatamente 1 registro rejeitado (RejectedCount == 1)
- Pet adulterado NÃO aparece entre os carregados
- Outros pets do mesmo espelho carregados normalmente
```

Log emitido pelo `PetDataLoader` (nível `Warning`, não derruba o processo):
```
PetDataLoader: assinatura inválida para o pet '<id>' — descartado, POSSÍVEL VIOLAÇÃO
```

---

## Bateria Final — Todos os Testes de `BattleSquare`

```
Found 9 automation tests based on 'BattleSquare'
BattleSquare.BattleDataTranslator.CopiesNumericFieldsAndInitializesHealth — Success
BattleSquare.BattleDataTranslator.TranslatedStateFeedsResolverCorrectly — Success
BattleSquare.BattleDataTranslator.TwoPetsGetDistinctStableIds — Success
BattleSquare.PetCrypto.DecryptsBufferEncryptedByTypeScriptSidecar — Success
BattleSquare.PetCrypto.VerifiesSignatureFromTypeScriptBackend — Success
BattleSquare.PetDataLoader.LoadsAndVerifiesRealSyncedMirror — Success
BattleSquare.PetDataLoader.MissingMirrorFailsExplicitly — Success
BattleSquare.PetDataLoader.TamperedRecordIsRejectedOthersSurvive — Success
BattleSquare.PetDataLoader.WrongKeyFailsExplicitly — Success
TEST COMPLETE. EXIT CODE: 0
```

Mais **44 testes** de `BattleSim` (Combate Núcleo, feature anterior) — total de **53 testes automatizados** no projeto, todos verificados nesta sessão, nenhum presumido.

---

## Lacunas Conhecidas, Registradas de Propósito (não escondidas)

Ver `.specs/project/STATE.md` para o detalhe completo de cada uma:

- **B-003** — colisão entre aliados (BTL-05, Combate Núcleo) estruturalmente inatingível sob o contrato de v1 (1 pet por lado)
- **AD-013** — desempate por velocidade não implementado (infraestrutura de M3, código morto em v1)
- **L-015** — sonda de isolação original testava um confundidor do SDK do macOS; corrigida
- **M7 (Contas e Moderação)** — banimento persistente de jogador adulterador fica fora de escopo até existir conta de jogador (AD-017)

#!/usr/bin/env bash
# T19 (tasks.md) — estende a sonda de isolação (AD-011/AD-012) para cobrir
# o que a Fase 5 do backend de pets trouxe: SQLite (SQLiteLibrary) e
# OpenSSL (AddEngineThirdPartyPrivateStaticDependencies). BattleSim NUNCA
# deve conseguir referenciar nenhum dos dois — só BattleSquare tem essas
# dependências.
#
# Duas sondas, cada uma testada isoladamente: se QUALQUER uma compilar,
# a fronteira do núcleo foi furada por esta feature.
#
# Removidas mesmo em interrupção (trap) — nunca deixa lixo no núcleo.

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
PROBE_FILE="$PROJECT_DIR/Source/BattleSim/Private/_IsolationProbePetBackend.cpp"
UPROJECT="$PROJECT_DIR/BattleSquare.uproject"
BUILD_SH="/Users/Shared/Epic Games/UE_5.8/Engine/Build/BatchFiles/Mac/Build.sh"

cleanup() {
  rm -f "$PROBE_FILE"
}
trap cleanup EXIT

if [ ! -f "$BUILD_SH" ]; then
  echo "probe_isolation_pet_backend: Build.sh não encontrado em '$BUILD_SH'." >&2
  exit 2
fi

run_probe() {
  local ProbeName="$1"
  local ProbeBody="$2"

  cat > "$PROBE_FILE" <<CPPEOF
// SONDA TEMPORÁRIA — T19 ($ProbeName). Se sobreviver no repositório,
// algo interrompeu o script antes do cleanup. Apagar.
$ProbeBody
CPPEOF

  echo "probe_isolation_pet_backend: sonda '$ProbeName' plantada, compilando (esperado: FALHA)..."
  if "$BUILD_SH" BattleSquareEditor Mac Development -Project="$UPROJECT" -waitmutex > "/tmp/probe_isolation_pet_backend_${ProbeName}.log" 2>&1; then
    echo "probe_isolation_pet_backend: FALHA DA SONDA '$ProbeName' — o build COMPILOU." >&2
    echo "BattleSim consegue referenciar dependência da Fase 5 (SQLite/OpenSSL)." >&2
    echo "Fronteira do núcleo furada. Ver /tmp/probe_isolation_pet_backend_${ProbeName}.log" >&2
    return 1
  fi
  echo "probe_isolation_pet_backend: sonda '$ProbeName' — build falhou como esperado."
  return 0
}

OVERALL_STATUS=0

# NOTA: a sonda original tentava "#include sqlite3.h" cru — e isso deu
# FALSO POSITIVO. macOS embute seu PRÓPRIO sqlite3.h no SDK do sistema
# (/usr/include/sqlite3.h), então o include resolvia sem depender do
# SQLiteLibrary vendorizado do projeto. Descoberto via "clang -H" (rastreio
# de resolução de header) antes de aceitar o resultado como violação real
# — mesma disciplina de L-004/L-007: verificar a causa, não presumir.
run_probe "openssl" '#include <openssl/evp.h>
const void* IsolationProbeOpenSSLRequiresLinking() { return (const void*)EVP_aes_256_gcm(); }' || OVERALL_STATUS=1

# Substitui o teste de sqlite3.h cru por um alvo que NUNCA pode colidir
# com nada do sistema: os próprios headers do BattleSquare (Data/*.h).
# Isto testa a fronteira que realmente importa — BattleSim alcançando
# arquivos de uma feature construída em cima dele, o que violaria a
# direção de dependência de AD-011/AD-012 — sem o confundidor do SDK.
run_probe "battlesquare_headers" '#include "Data/PetDataLoader.h"
void IsolationProbeReachesBattleSquareHeaders() { FLoadedPetRecord Unused; (void)Unused; }' || OVERALL_STATUS=1

if [ "$OVERALL_STATUS" -eq 0 ]; then
  echo "probe_isolation_pet_backend: TODAS as sondas falharam como esperado — fronteira do núcleo intacta após a Fase 5."
else
  echo "probe_isolation_pet_backend: AO MENOS UMA sonda revelou vazamento de dependência para o núcleo." >&2
fi

exit "$OVERALL_STATUS"

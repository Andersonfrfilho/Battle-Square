#!/usr/bin/env bash
# T14 (tasks.md) — sonda automatizada da fronteira do núcleo (AD-011/AD-012).
#
# Planta um .cpp no BattleSim que referencia AActor::StaticClass() — algo
# que só existe se o módulo "Engine" vazar para dentro do núcleo. O build
# TEM que falhar. Se compilar, a fronteira foi furada e este script avisa
# no mesmo dia, não meses depois quando alguém notar que o servidor
# dedicado carregou UWorld sem querer.
#
# A sonda é removida mesmo que o script seja interrompido (trap) — nunca
# deixa lixo de teste no núcleo de produção.

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
PROBE_FILE="$PROJECT_DIR/Source/BattleSim/Private/_IsolationProbe.cpp"
UPROJECT="$PROJECT_DIR/BattleSquare.uproject"
BUILD_SH="/Users/Shared/Epic Games/UE_5.8/Engine/Build/BatchFiles/Mac/Build.sh"

cleanup() {
  rm -f "$PROBE_FILE"
}
trap cleanup EXIT

if [ ! -f "$BUILD_SH" ]; then
  echo "probe_isolation: Build.sh não encontrado em '$BUILD_SH' — ajuste o caminho do engine no script." >&2
  exit 2
fi

cat > "$PROBE_FILE" <<'CPPEOF'
// SONDA TEMPORÁRIA — T14 (probe_isolation.sh). Se este arquivo sobreviver
// no repositório, algo interrompeu o script antes do cleanup. Apagar.
#include "GameFramework/Actor.h"
UClass* IsolationProbeRequiresLinking() { return AActor::StaticClass(); }
CPPEOF

echo "probe_isolation: sonda plantada, compilando (esperado: FALHA)..."
if "$BUILD_SH" BattleSquareEditor Mac Development -Project="$UPROJECT" -waitmutex > /tmp/probe_isolation_build.log 2>&1; then
  echo "probe_isolation: FALHA DA SONDA — o build COMPILOU com a sonda presente." >&2
  echo "Isso significa que o BattleSim consegue referenciar tipos do módulo Engine (AActor)." >&2
  echo "A fronteira do núcleo (AD-011/AD-012) foi furada. Ver /tmp/probe_isolation_build.log." >&2
  exit 1
fi

echo "probe_isolation: build falhou como esperado — fronteira do núcleo intacta."
exit 0

#!/usr/bin/env bash
# a-malha-vem-de-fora MV9 — a malha vem do DADO, e o padrao hardcoded nao VOLTA.
#
# Reprova qualquer literal /Engine/BasicShapes/* em codigo de ator (fora da
# fonte unica ScenaryPalette.cpp e fora de testes). Depois da migracao MV4-MV8,
# a contagem deve ser ZERO — e este script e o que a mantem assim: um FObjectFinder
# novo com o caminho no construtor reprova, apontando arquivo e linha.
set -euo pipefail

RAIZ="$(cd "$(dirname "$0")/.." && pwd)"
cd "$RAIZ"

# A fonte unica PODE ter os caminhos (e o dono deles). Testes tambem podem
# afirmar strings. Todo o resto, nao.
VIOLACOES=$(grep -rn "/Engine/BasicShapes/" Source/BattleSquare --include="*.cpp" 2>/dev/null \
  | grep -v "Environment/ScenaryPalette.cpp" \
  | grep -v "/Tests/" \
  || true)

if [ -n "$VIOLACOES" ]; then
  echo "audit_mesh_from_data: FALHOU — caminho de malha hardcoded fora da fonte unica:"
  echo "$VIOLACOES"
  echo
  echo "Use ScenaryPalette::PrimitiveMeshPath / ColorableBaseMaterialPath — a malha"
  echo "vem do DADO (a-malha-vem-de-fora, invariante 18)."
  exit 1
fi

echo "audit_mesh_from_data: OK — nenhum caminho de malha hardcoded fora da fonte unica."

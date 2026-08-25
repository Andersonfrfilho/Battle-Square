#!/usr/bin/env bash
# T13 (tasks.md, BTL-22) — auditoria anti-recálculo da apresentação.
#
# design.md: a apresentação (BattleSquare) NUNCA recalcula dano, alcance
# ou resultado — ela só lê campos que o núcleo (BattleSim) já preencheu
# no trace (FBattleEvent::Value). Se alguém escreve a fórmula de dano de
# novo dentro de um widget ou ator de cena, o cliente e o servidor podem
# divergir sem que nada acuse erro. Este script falha o build ANTES
# disso, mesmo espírito de audit_determinism.sh e probe_isolation.sh.
#
# Padrões vigiados (ver BattlePhaseCombat.cpp, a fonte real da fórmula):
#   Attack * / * Attack   — multiplicador de ataque recalculado
#   Defense - / - Defense — dedução de defesa recalculada
#   * 1.5                 — o multiplicador de magia (MagicDamageMultiplierPercent)
#                           reaparecendo como literal fora do núcleo
#
# Não é um parser C++ completo — grep disciplinado, mesmo padrão dos
# outros dois scripts desta pasta.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PRESENTATION_DIR="$SCRIPT_DIR/../Source/BattleSquare"

if [ ! -d "$PRESENTATION_DIR" ]; then
  echo "audit_no_recalculation: diretório da apresentação não encontrado: $PRESENTATION_DIR" >&2
  exit 2
fi

FORBIDDEN_PATTERN='\bAttack\b[[:space:]]*\*|\*[[:space:]]*\bAttack\b|\bDefense\b[[:space:]]*-|-[[:space:]]*\bDefense\b|\*[[:space:]]*1\.5\b'

VIOLATIONS=""
while IFS= read -r -d '' FILE; do
  while IFS=: read -r LINE_NUM LINE_CONTENT; do
    TRIMMED="$(echo "$LINE_CONTENT" | sed -E 's/^[[:space:]]*//')"
    case "$TRIMMED" in
      "//"*) continue ;;
    esac
    VIOLATIONS="${VIOLATIONS}${FILE}:${LINE_NUM}: ${LINE_CONTENT}"$'\n'
  done < <(grep -nE "$FORBIDDEN_PATTERN" "$FILE" || true)
done < <(find "$PRESENTATION_DIR" \( -name "*.h" -o -name "*.cpp" \) -not -path "*/Tests/*" -print0)

if [ -n "$VIOLATIONS" ]; then
  echo "AUDITORIA ANTI-RECÁLCULO FALHOU (BTL-22) — fórmula de dano/alcance reaparecendo fora de BattleSim:" >&2
  echo "$VIOLATIONS" >&2
  exit 1
fi

echo "audit_no_recalculation: limpo — nenhuma fórmula de dano/multiplicador recalculada fora de BattleSim."
exit 0

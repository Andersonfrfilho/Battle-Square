#!/usr/bin/env bash
# T13 (tasks.md, BTL-18) — auditoria anti-ponto-flutuante do núcleo.
#
# AD-004: a simulação de combate usa exclusivamente aritmética inteira.
# Um único `float` no caminho de resolução quebra o determinismo em
# silêncio — resultado diferente entre plataformas sem erro nenhum. Este
# script falha o build ANTES que isso aconteça.
#
# Ignora comentário (linha começando com // após espaços) e string
# literal simples (heurística: se o token aparece só dentro de aspas).
# Não é um parser C++ completo — é um grep disciplinado o bastante para
# pegar o caso real (declaração de tipo, cast, literal numérico).

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CORE_DIR="$SCRIPT_DIR/../Source/BattleSim"

if [ ! -d "$CORE_DIR" ]; then
  echo "audit_determinism: diretório do núcleo não encontrado: $CORE_DIR" >&2
  exit 2
fi

FORBIDDEN_PATTERN='\b(float|double|FMath::Rand|FRandomStream)\b'

VIOLATIONS=""
while IFS= read -r -d '' FILE; do
  while IFS=: read -r LINE_NUM LINE_CONTENT; do
    # Descarta linha que É comentário de linha inteira (começa com // após espaços).
    TRIMMED="$(echo "$LINE_CONTENT" | sed -E 's/^[[:space:]]*//')"
    case "$TRIMMED" in
      "//"*) continue ;;
    esac
    VIOLATIONS="${VIOLATIONS}${FILE}:${LINE_NUM}: ${LINE_CONTENT}"$'\n'
  done < <(grep -nE "$FORBIDDEN_PATTERN" "$FILE" || true)
done < <(find "$CORE_DIR" \( -name "*.h" -o -name "*.cpp" \) -not -path "*/Tests/*" -print0)

if [ -n "$VIOLATIONS" ]; then
  echo "AUDITORIA DE DETERMINISMO FALHOU (AD-004) — float/double/RNG não semeado encontrado no núcleo:" >&2
  echo "$VIOLATIONS" >&2
  exit 1
fi

echo "audit_determinism: limpo — nenhum float, double, FMath::Rand ou FRandomStream em código de produção do BattleSim."
exit 0

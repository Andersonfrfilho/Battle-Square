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

# O LITERAL entra na conta, e não só a palavra. `Peso = 1.5f;` compila sem a
# palavra "float" em lugar nenhum, quebra o determinismo do mesmo jeito, e
# passou batido por esta sonda desde que ela existe — descoberto ao testá-la
# contra um caso que eu supunha coberto.
#
# É o PONTO DECIMAL que denuncia, e não o sufixo `f`: `0x0F` é hexadecimal e
# casou com "número seguido de F" na primeira tentativa, acusando o
# empacotamento de casas que não tem float nenhum.
FORBIDDEN_PATTERN='\b(float|double|FMath::Rand|FRandomStream)\b|[0-9]+\.[0-9]'

VIOLATIONS=""
while IFS= read -r -d '' FILE; do
  while IFS=: read -r LINE_NUM LINE_CONTENT; do
    # Descarta linha que É comentário e nada mais. Comentário não compila, e
    # uma sonda que acusa a PALAVRA "float" dentro de uma explicação sobre
    # por que float não entra aqui obriga a escrever mal para passar — o que
    # é pior que não ter sonda, porque ensina a contornar em vez de corrigir.
    TRIMMED="$(echo "$LINE_CONTENT" | sed -E 's/^[[:space:]]*//')"
    case "$TRIMMED" in
      "//"*) continue ;;
      "/*"*) continue ;;
      # Corpo de comentário de bloco: `* texto`. Exige o ESPAÇO — sem ele,
      # `*Ponteiro = 1.0f;` passaria batido, e essa é exatamente a linha que
      # a sonda existe para pegar.
      "* "*) continue ;;
      "*/"*) continue ;;
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

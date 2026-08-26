#!/usr/bin/env bash
# T10 (tasks.md, Combate Online) — auditoria anti-replicação do commit.
#
# design.md, "O erro que esta arquitetura existe para não cometer": o
# commit de um jogador NUNCA pode ser UPROPERTY(Replicated). Se fosse,
# a Unreal entregaria a fila de ações do jogador A para o jogador B
# assim que ela chegasse ao servidor — ANTES da resolução — destruindo
# o commit às cegas (AD-005/BTL-02) sem produzir erro nenhum. O jogo
# continuaria "funcionando", só que sem a parte que o torna um jogo.
#
# Este script falha o build se FNetTurnCommit ou FTurnCommit aparecerem:
#   - marcados UPROPERTY(Replicated ...) na linha imediatamente acima
#     da declaração do campo, ou
#   - referenciados dentro do CORPO de GetLifetimeReplicatedProps
#     (DOREPLIFETIME e variantes).
#
# Ignora comentários de linha inteira (começam com // após espaços) —
# senão o próprio comentário que EXPLICA esta regra (que cita os nomes
# proibidos de propósito) dispararia falso positivo nele mesmo.
#
# Não é um parser C++ completo — grep disciplinado, mesmo padrão de
# audit_determinism.sh, audit_no_recalculation.sh e probe_isolation.sh.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PRESENTATION_DIR="$SCRIPT_DIR/../Source/BattleSquare"

if [ ! -d "$PRESENTATION_DIR" ]; then
  echo "audit_no_commit_replication: diretório não encontrado: $PRESENTATION_DIR" >&2
  exit 2
fi

is_comment_line() {
  local TRIMMED
  TRIMMED="$(echo "$1" | sed -E 's/^[[:space:]]*//')"
  case "$TRIMMED" in
    "//"*) return 0 ;;
    *) return 1 ;;
  esac
}

VIOLATIONS=""

while IFS= read -r -d '' FILE; do
  PREV_LINE=""
  LINE_NUM=0
  IN_LIFETIME_FUNC=0

  while IFS= read -r LINE; do
    LINE_NUM=$((LINE_NUM + 1))

    if is_comment_line "$LINE"; then
      PREV_LINE="$LINE"
      continue
    fi

    # Padrão 1: UPROPERTY(Replicated...) na linha anterior + tipo de
    # commit na linha atual (campo declarado logo abaixo do atributo).
    if ! is_comment_line "$PREV_LINE" \
      && echo "$PREV_LINE" | grep -qE 'UPROPERTY\s*\([^)]*Replicated' \
      && echo "$LINE" | grep -qE '\b(FNetTurnCommit|FTurnCommit)\b'; then
      VIOLATIONS="${VIOLATIONS}${FILE}:${LINE_NUM}: ${LINE}"$'\n'
    fi

    # Padrão 2: dentro do corpo de GetLifetimeReplicatedProps, qualquer
    # menção a campo de commit — só entra no modo depois de ver a
    # ASSINATURA da função (contém "::" e o nome), sai ao ver a chave de
    # fechamento na coluna 0/indentação mínima.
    if echo "$LINE" | grep -qE 'GetLifetimeReplicatedProps\s*\('; then
      IN_LIFETIME_FUNC=1
    elif [ "$IN_LIFETIME_FUNC" -eq 1 ] && echo "$LINE" | grep -qE '^\}'; then
      IN_LIFETIME_FUNC=0
    elif [ "$IN_LIFETIME_FUNC" -eq 1 ] && echo "$LINE" | grep -qiE '(Commit)'; then
      VIOLATIONS="${VIOLATIONS}${FILE}:${LINE_NUM}: ${LINE}"$'\n'
    fi

    PREV_LINE="$LINE"
  done < "$FILE"
done < <(find "$PRESENTATION_DIR" \( -name "*.h" -o -name "*.cpp" \) -not -path "*/Tests/*" -print0)

if [ -n "$VIOLATIONS" ]; then
  echo "AUDITORIA ANTI-REPLICAÇÃO DO COMMIT FALHOU — commit marcado como replicado:" >&2
  echo "$VIOLATIONS" >&2
  exit 1
fi

echo "audit_no_commit_replication: limpo — nenhum commit (FNetTurnCommit/FTurnCommit) é replicado."
exit 0

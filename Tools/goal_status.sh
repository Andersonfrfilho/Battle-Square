#!/usr/bin/env bash
# Diz o que falta, LENDO os GOAL.md de cada feature.
#
# Existe porque a alternativa era o guarda-chuva manter a propria lista de
# caixas — uma segunda verdade, que concorda com a primeira ate a primeira
# edicao. Aqui nao ha o que sincronizar: se a caixa fechou na feature, ela
# fechou aqui.
set -uo pipefail
cd "$(dirname "$0")/.."

PROXIMA=""
for GOAL in .specs/features/*/GOAL.md; do
    FEATURE=$(basename "$(dirname "$GOAL")")
    [ "$FEATURE" = "tudo-que-falta" ] && continue

    TRECHO=$(sed -n '/PRONTO é isto/,/^Enquanto/p' "$GOAL")
    ABERTAS=$(echo "$TRECHO" | grep -E '^- \[ \] \*\*[A-Z]+-?[0-9A-Z-]*\*\*' || true)

    # PAROU nao e fechado. Contar ⛔ como pronto faria este painel dizer "nada
    # aberto" com quatro tarefas travadas — e um painel que esconde o que parou
    # e pior que nenhum painel: ele da a rodada por encerrada.
    PARADAS=$(echo "$TRECHO" | grep -E '^- \[⛔\]' || true)

    if [ -z "$ABERTAS" ] && [ -n "$PARADAS" ]; then
        printf '  \033[31m⛔\033[0m %s — %d parada(s), decisao do usuario\n' \
            "$FEATURE" "$(echo "$PARADAS" | wc -l | tr -d ' ')"
        while IFS= read -r LINHA; do
            printf '      %s\n' "$(echo "$LINHA" | sed 's/^- \[⛔\] //; s/\*\*//g')"
        done <<< "$PARADAS"
        continue
    fi

    if [ -z "$ABERTAS" ]; then
        printf '  \033[32m✓\033[0m %s\n' "$FEATURE"
        continue
    fi

    printf '  \033[33m•\033[0m %s\n' "$FEATURE"
    while IFS= read -r LINHA; do
        printf '      %s\n' "$(echo "$LINHA" | sed 's/^- \[ \] //; s/\*\*//g')"
        if [ -z "$PROXIMA" ]; then
            PROXIMA="$FEATURE: $(echo "$LINHA" | sed 's/^- \[ \] \*\*//; s/\*\*.*//')"
        fi
    done <<< "$ABERTAS"
done

echo
if [ -n "$PROXIMA" ]; then
    printf '  proxima: \033[1m%s\033[0m\n' "$PROXIMA"
else
    printf '  \033[32mnada aberto\033[0m — veja as ⛔ acima antes de dar por encerrado\n'
fi

#!/usr/bin/env bash
# Sonda: todo texto que o jogador LÊ precisa ser coletável para tradução.
#
# Por que é sonda e não teste: a coleta (GatherText) lê o código-FONTE
# procurando as macros LOCTEXT. Em tempo de execução não existe propriedade que
# distinga FText::Format de FString::Printf — tentei, e um teste que finge
# fazer essa distinção é pior que nenhum, porque passa verde enquanto a frase
# some da tradução em silêncio.
#
# O modo de falhar que isto pega: alguém troca FText::Format por
# FString::Printf "porque é mais simples". Nada quebra, a tela continua certa
# em português, e o idioma novo nasce faltando linha.

set -euo pipefail
cd "$(dirname "$0")/.."

VOLTADO_AO_JOGADOR=(
    "Source/BattleSquare/Private/Battle/BattleNarration.cpp"
    "Source/BattleSquare/Private/UI"
)

FALHOU=0

for ALVO in "${VOLTADO_AO_JOGADOR[@]}"; do
    [ -e "$ALVO" ] || continue

    # Texto voltado ao jogador construído por Printf, fora de comentário.
    ACHADOS=$(grep -rn "FText::FromString(FString::Printf\|SetText(FText::FromString(TEXT(" "$ALVO" \
        --include="*.cpp" 2>/dev/null | grep -v "^\s*//" || true)

    if [ -n "$ACHADOS" ]; then
        echo "FALHA: texto voltado ao jogador não é coletável em $ALVO"
        echo "$ACHADOS"
        FALHOU=1
    fi
done

# A narração é o caso central: se ela parar de usar LOCTEXT, a sonda inteira
# perde o sentido, então isto é verificado explicitamente.
if ! grep -q "LOCTEXT_NAMESPACE" Source/BattleSquare/Private/Battle/BattleNarration.cpp; then
    echo "FALHA: BattleNarration.cpp perdeu o namespace de localização"
    FALHOU=1
fi

if [ "$FALHOU" -eq 0 ]; then
    echo "audit_localizable_text: texto do jogador é coletável."
fi

exit "$FALHOU"

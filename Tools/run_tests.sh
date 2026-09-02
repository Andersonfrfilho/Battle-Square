#!/usr/bin/env bash
# Roda a bateria (ou o filtro passado) e resume em UMA linha.
#
# O resumo conta o CRASH separado das falhas de proposito: um crash engole em
# silencio todo teste que viria depois em ordem alfabetica, e a contagem menor
# passa por "ainda nao rodou tudo" (B-0FLAKY em STATE.md).
set -uo pipefail
cd "$(dirname "$0")/.."
UE="/Users/Shared/Epic Games/UE_5.8"
FILTRO="${1:-BattleSquare+BattleSim}"
LOG="$HOME/Library/Logs/Unreal Engine/BattleSquareEditor/RunTests.log"

"$UE/Engine/Binaries/Mac/UnrealEditor-Cmd" "$PWD/BattleSquare.uproject" \
  -ExecCmds="Automation RunTests $FILTRO; Quit" \
  -unattended -nopause -nosplash -nullrhi -log="RunTests.log" > /dev/null 2>&1

SUCESSOS=$(grep -ac "Test Completed. Result={Success}" "$LOG" 2>/dev/null)
FALHAS=$(grep -ac "Test Completed. Result={Fail}" "$LOG" 2>/dev/null)
CRASH=$(grep -ac "StaticShutdownAfterError" "$LOG" 2>/dev/null)

echo "sucessos: $SUCESSOS  falhas: $FALHAS  crash: $CRASH"
[ "$FALHAS" -gt 0 ] && grep -a "Result={Fail}" "$LOG" | sed 's/.*Test //' | head -20
[ "$CRASH" -gt 0 ] && echo "ATENCAO: crash engoliu os testes seguintes — a contagem NAO e o total"
exit 0

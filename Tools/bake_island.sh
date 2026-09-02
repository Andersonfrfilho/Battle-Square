#!/bin/bash
# Assa o traçado da ilha: roda todos os planos uma vez, fora do jogo, e grava
# o resultado em `Saved/IslandMap.json`.
#
# É comando ÚNICO de propósito. O despejo já existia como teste de automação,
# mas só era alcançável por uma linha de `UnrealEditor -ExecCmds` que ninguém
# lembra de cor — e um assado que depende de lembrar a linha é um assado que
# envelhece calado (spec: "a guarda contra o assado velho").
#
# Sai diferente de zero se o arquivo não aparecer ou vier faltando seção: um
# assado parcial que passa por bom é pior que assado nenhum, porque o mundo
# construído em cima dele fica sem as peças e nada acusa.

set -uo pipefail
cd "$(dirname "$0")/.."

UE="/Users/Shared/Epic Games/UE_5.8"
ASSADO="$PWD/Saved/IslandMap.json"
UASSET="$PWD/Content/World/IslandBaked.uasset"

# As sete seções são as sete linhas da tabela da spec — os planos que o mundo
# precisa construir. Faltar uma é faltar uma categoria inteira de mundo.
SECOES=(alturas rios trilhas travessias solo cavernas aquedutos)

rm -f "$ASSADO" "$UASSET"

"$UE/Engine/Binaries/Mac/UnrealEditor-Cmd" "$PWD/BattleSquare.uproject" \
  -ExecCmds="Automation RunTests BattleSquare.IslandMap; Quit" \
  -unattended -nopause -nosplash -nullrhi -log > /dev/null 2>&1

if [ ! -f "$ASSADO" ]; then
  echo "bake_island: o assado nao foi gravado em $ASSADO" >&2
  exit 1
fi

FALTOU=0
for SECAO in "${SECOES[@]}"; do
  if ! grep -q "\"$SECAO\"" "$ASSADO"; then
    echo "bake_island: secao ausente no assado: $SECAO" >&2
    FALTOU=1
  fi
done
[ "$FALTOU" -eq 0 ] || exit 1

# O JSON e o retrato para a carta; o `.uasset` e o que o JOGO le. Gravar so um
# dos dois faria carta e mundo divergirem em silencio — que e exatamente o que
# a fonte unica existe para impedir.
if [ ! -f "$UASSET" ]; then
  echo "bake_island: o UDataAsset nao foi gravado em $UASSET" >&2
  exit 1
fi

echo "bake_island: $ASSADO ($(du -h "$ASSADO" | cut -f1)), sete secoes presentes"
echo "bake_island: $UASSET ($(du -h "$UASSET" | cut -f1))"

#!/usr/bin/env bash
# Compila o editor, esperando a vez quando outra sessao esta compilando.
#
# O mutex do UBT as vezes fica ORFAO — sem processo dono — e ai trava todo
# mundo. Remover nesse caso e seguro; remover com build EM CURSO nao e, e foi
# o que eu fiz uma vez por ter posto a checagem e o `rm` na mesma linha, sem
# guarda. O `ps` imprimiu 24 processos e o `rm` rodou do mesmo jeito.
#
# Aqui a checagem DECIDE a remocao, em vez de ser impressa ao lado dela.

set -uo pipefail
cd "$(dirname "$0")/.."
UE="/Users/Shared/Epic Games/UE_5.8"

ocupado() { [ "$(ps aux | grep -cE '[c]lang|[U]nrealEditor|[U]nrealBuildTool|[d]otnet')" -gt 0 ]; }

for TENTATIVA in $(seq 1 20); do
  SAIDA=$("$UE/Engine/Build/BatchFiles/Mac/Build.sh" BattleSquareEditor Mac Development \
    -project="$PWD/BattleSquare.uproject" -NoUBA 2>&1)

  if ! echo "$SAIDA" | grep -q "ConflictingInstance"; then
    echo "$SAIDA" | grep -E "error:|Error:|Result:" | head -20
    echo "$SAIDA" | grep -q "Result: Succeeded" && exit 0 || exit 1
  fi

  if ocupado; then
    echo "outra sessao compilando (tentativa $TENTATIVA) — esperando"
    sleep 60
    continue
  fi

  echo "mutex ORFAO (nenhum processo dono) — removendo e tentando de novo"
  rm -f /tmp/.dotnet/shm/global/UnrealBuildTool_Mutex_*
done

echo "desisti: o mutex seguiu ocupado por 20 minutos" >&2
exit 1

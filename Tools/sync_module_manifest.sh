#!/usr/bin/env bash
# Alinha Binaries/Mac/UnrealEditor.modules com os dylibs que existem de fato.
#
# POR QUÊ (L-025): o UBT às vezes escreve o manifesto apontando para o dylib
# ANTERIOR ao que acabou de linkar (defasagem de um build, herdada de builds
# feitos com o Editor aberto). Quando isso acontece, o UnrealEditor-Cmd carrega
# o binário velho: o teste novo simplesmente não aparece na contagem, sem erro
# nenhum, e o diagnóstico natural ("errei o nome do teste") é o errado.
#
# Roda depois de Build.sh e antes de qualquer Automation RunTests.
set -euo pipefail

BINARIES_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)/Binaries/Mac"
MANIFEST="${BINARIES_DIR}/UnrealEditor.modules"

if [[ ! -f "${MANIFEST}" ]]; then
	echo "sync_module_manifest: ${MANIFEST} não existe — rode Build.sh antes." >&2
	exit 1
fi

changed=0
for module in BattleSquare BattleSim; do
	# O dylib real: o numerado mais recente, ou o sem número se não houver numerado.
	actual="$(ls -t "${BINARIES_DIR}/libUnrealEditor-${module}"*.dylib 2>/dev/null | head -1 || true)"
	if [[ -z "${actual}" ]]; then
		echo "sync_module_manifest: nenhum dylib para ${module} — build incompleto." >&2
		exit 1
	fi
	actual="$(basename "${actual}")"

	declared="$(grep -o "\"${module}\": \"[^\"]*\"" "${MANIFEST}" | sed 's/.*: "//;s/"//')"
	if [[ "${declared}" != "${actual}" ]]; then
		# sed -i '' é a forma do BSD sed (macOS); GNU sed usaria -i sem argumento.
		sed -i '' "s|\"${module}\": \"${declared}\"|\"${module}\": \"${actual}\"|" "${MANIFEST}"
		echo "sync_module_manifest: ${module}: ${declared} -> ${actual}"
		changed=1
	fi
done

if [[ "${changed}" -eq 0 ]]; then
	echo "sync_module_manifest: manifesto já alinhado."
fi

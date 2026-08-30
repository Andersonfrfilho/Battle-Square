#!/usr/bin/env bash
# Sonda: ator com componente visual e SEM malha atribuida.
#
# Este projeto ja teve TRES atores invisiveis, e o padrao foi identico nas tres
# vezes (CLAUDE.md): o componente e criado no construtor, o ator existe, a
# logica roda, a bateria passa inteira — e nada aparece na tela, porque
# ninguem atribuiu a malha.
#
# Nenhum teste de logica pega isso: o ator ESTA la. Quem pegava era o usuario
# olhando, uma rodada de cada vez. E o roteiro de verificacao manual existe em
# grande parte por causa disso — que e trabalho humano gasto num defeito com
# forma fixa.
#
# Nao e um parser: e um grep disciplinado. Arquivo que cria componente de
# malha precisa, em algum lugar dele, ATRIBUIR malha. Falso negativo existe
# (malha vinda de outro arquivo), falso positivo tambem — e por isso a lista
# de dispensa abaixo e explicita e comentada, nunca silenciosa.

set -euo pipefail
cd "$(dirname "$0")/.."

FALHAS=""

while IFS= read -r -d '' ARQUIVO; do
  CRIA=$(grep -cE "CreateDefaultSubobject<U(Hierarchical)?(Instanced)?StaticMeshComponent>" "$ARQUIVO" || true)
  [ "$CRIA" -eq 0 ] && continue

  ATRIBUI=$(grep -cE "SetStaticMesh\(|StaticMesh =|->SetSkeletalMesh\(" "$ARQUIVO" || true)
  [ "$ATRIBUI" -gt 0 ] && continue

  FALHAS="${FALHAS}$(basename "$ARQUIVO"): cria componente de malha e nunca atribui uma"$'\n'
done < <(find Source -name "*.cpp" -not -path "*/Tests/*" -print0)

if [ -n "$FALHAS" ]; then
  echo "FALHA: ator com componente visual e sem malha — ele existe e NAO aparece." >&2
  echo "$FALHAS" >&2
  exit 1
fi

echo "audit_visible_actors: todo ator que cria malha atribui uma."

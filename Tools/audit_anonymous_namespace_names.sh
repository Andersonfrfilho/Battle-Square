#!/usr/bin/env bash
# Nome de namespace ANÔNIMO repetido entre arquivos.
#
# Namespace anônimo NÃO isola em unity build: os arquivos viram uma unidade de
# tradução só, os anônimos se fundem, e dois nomes iguais de tipos diferentes
# viram redefinição.
#
# E o modo de falhar é PIOR que não compilar: enquanto o agrupamento do unity
# build não os juntar, tudo compila — e o que se lê pode ser a variável do
# outro arquivo. Foi assim que três `GOverride` de catálogos diferentes
# conviveram por horas, e um catálogo "vazio" fez um elemento sumir.
#
# `audit_test_helper_names.sh` cobre HELPER DE TESTE. Este cobre a variável de
# produção, que é o mesmo defeito no outro lado da fronteira.
set -uo pipefail
cd "$(dirname "$0")/.."

TEMP=$(mktemp)
trap 'rm -f "$TEMP"' EXIT

# Variável de arquivo dentro de `namespace {`: uma linha indentada com tipo e
# nome que começa em G ou maiúscula, terminada em `= ...;`.
while IFS= read -r ARQUIVO; do
    awk -v arq="$ARQUIVO" '
        /^namespace$|^namespace[[:space:]]*$|^namespace[[:space:]]*\{/ { dentro=1 }
        dentro && /^\}/ { dentro=0 }
        dentro && /^\t(const |static )?[A-Za-z_][A-Za-z0-9_:<>* ]*[ *]([A-Z][A-Za-z0-9_]*)[[:space:]]*=/ {
            linha=$0
            sub(/[[:space:]]*=.*/, "", linha)
            n=split(linha, partes, /[ *&]+/)
            print partes[n] "\t" arq
        }
    ' "$ARQUIVO"
done < <(find Source -name '*.cpp' -not -path '*/Tests/*') > "$TEMP"

REPETIDOS=$(cut -f1 "$TEMP" | sort | uniq -d)

if [ -n "$REPETIDOS" ]; then
    echo "FALHA: nome de namespace anonimo repetido entre arquivos de producao." >&2
    echo "Em unity build eles se fundem, e o que se le pode ser o do outro arquivo." >&2
    echo >&2
    while IFS= read -r NOME; do
        # `grep -P` NAO existe no macOS — ele sai 2 sem procurar, e a
        # auditoria acusaria sem dizer ONDE, que e meia auditoria. awk com
        # comparacao exata de campo faz o mesmo sem depender de PCRE.
        printf '%s: %s\n' "$NOME" \
            "$(awk -F'\t' -v n="$NOME" '$1 == n { printf "%s ", $2 }' "$TEMP")" >&2
    done <<< "$REPETIDOS"
    exit 1
fi

echo "audit_anonymous_namespace_names: nenhum nome de namespace anonimo repetido."

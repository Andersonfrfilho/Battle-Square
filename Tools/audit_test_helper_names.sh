#!/usr/bin/env bash
# Sonda: helper de teste com nome repetido entre arquivos.
#
# O Unreal compila em UNITY BUILD — vários .cpp viram uma unidade de tradução.
# Dois arquivos de teste com `MakePet` em namespace anônimo NÃO dão erro de
# compilação: viram SOBRECARGAS. E uma chamada pode ligar na função do OUTRO
# arquivo, com significado diferente para os mesmos argumentos.
#
# Foi o que aconteceu em 2026-08-29: três `MakePet` coexistiam com assinaturas
#   (PetId, Side, Column, Row)      — movimento
#   (PetId, Side, Health, MaxHealth) — desfecho
#   (PetId, Side, Health)            — resolução
# e `MakePet(1, 0, 0, 0)` no teste de movimento passou a criar um pet com VIDA
# ZERO em vez de posição, derrubando a bateria inteira com index out of bounds.
#
# O defeito era PRÉ-EXISTENTE e ficou latente por semanas: só apareceu quando um
# arquivo novo mudou o agrupamento do unity build. Nada acusava, e o sintoma não
# apontava para a causa.

set -euo pipefail
cd "$(dirname "$0")/.."

DUPLICADOS=$(python3 - <<'PY'
import re, pathlib, collections

por_nome = collections.defaultdict(set)
for caminho in pathlib.Path('Source').glob('*/Private/Tests/*.cpp'):
    texto = caminho.read_text(errors='ignore')

    # Namespace NOMEADO isola de verdade: em unity build os helpers do arquivo
    # ficam qualificados e dois homônimos deixam de ser sobrecargas um do
    # outro. É a correção da armadilha, não um jeito de escapar da sonda —
    # então arquivo que a aplicou sai da conta.
    #
    # A sonda nasceu quando TODO helper vivia em namespace anônimo, e ali o
    # nome era mesmo a única defesa. Continuar exigindo nome único depois da
    # correção transformaria a sonda numa taxa sobre quem a seguiu.
    # ...MAS so quando o arquivo NAO reabre o namespace com `using` no escopo
    # dele. Em unity build os dois `using` ficam visiveis na mesma TU e a
    # chamada volta a ser ambigua — o namespace nomeado deixa de isolar
    # justamente onde precisava. Quem qualifica no ponto de chamada esta
    # protegido; quem poe `using namespace` no topo NAO esta, e a sonda
    # precisa continuar cobrando dele.
    if (re.search(r'^namespace\s+\w+', texto, re.M)
            and not re.search(r'^using namespace\s+\w+\s*;', texto, re.M)):
        continue

    # Funções livres indentadas com um tab: o formato dos helpers em namespace
    # anônimo neste projeto.
    for achado in re.finditer(r'^\t(?:static\s+)?[A-Za-z_][\w:<>\* &]*\s+(\w+)\s*\(', texto, re.M):
        por_nome[achado.group(1)].add(caminho.name)

for nome, arquivos in sorted(por_nome.items()):
    if len(arquivos) > 1:
        print(f"{nome}: {', '.join(sorted(arquivos))}")
PY
)

if [ -n "$DUPLICADOS" ]; then
    echo "FALHA: helper de teste com nome repetido entre arquivos."
    echo "Em unity build eles viram sobrecargas, e a chamada pode ligar na errada."
    echo
    echo "$DUPLICADOS"
    exit 1
fi

echo "audit_test_helper_names: nenhum helper de teste com nome repetido."

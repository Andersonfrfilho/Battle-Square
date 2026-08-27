#!/usr/bin/env bash
# Coleta o texto do jogo para tradução e gera os .locres que o jogo carrega.
#
# Rodar sempre que texto voltado ao jogador for adicionado ou mudar de redação:
# frase que não passou pela coleta não existe para o tradutor, e o idioma novo
# nasce faltando linha sem nada quebrar.
set -euo pipefail
cd "$(dirname "$0")/.."

ENGINE="/Users/Shared/Epic Games/UE_5.8/Engine/Binaries/Mac/UnrealEditor-Cmd"

"$ENGINE" "$PWD/BattleSquare.uproject" \
    -run=GatherText -config="Config/Localization/Game.ini" \
    -unattended -nopause -nosplash || true

# O comando devolve status não-zero por causa do servidor MCP que não sobe em
# modo commandlet; o que decide é o .locres ter nascido.
for CULTURA in pt-BR en es; do
    if [ ! -f "Content/Localization/Game/$CULTURA/Game.locres" ]; then
        echo "FALHA: $CULTURA sem .locres — a tradução não chegaria à tela."
        exit 1
    fi
done

echo "gather_text: manifesto e .locres gerados para pt-BR, en, es."
echo "Traduzir editando Content/Localization/Game/<cultura>/Game.po e rodando de novo."

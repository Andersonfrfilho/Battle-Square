# Conversa dinâmica na SUA máquina — guia do jogador

Os moradores do jogo sempre conversam com você (isso funciona sem instalar
nada). Este guia é para quem quer o **modo dinâmico sem depender de
internet**: um modelo de linguagem rodando no seu próprio computador.

## Antes de começar — o que você precisa

| | |
|---|---|
| Espaço em disco | ~2 a 4 GB (o modelo) + ~60 MB (o servidor da conversa) |
| Memória RAM | ~4 GB livres **além** do que o jogo já usa |
| Internet | só UMA vez, para baixar o modelo |
| Tempo | uns 10 minutos |

**A diferença honesta:** o modelo pequeno conversa bem para vizinho de vila,
mas erra mais que o servidor oficial — pode embaralhar um detalhe aqui e ali.
E ele divide o computador com o jogo: se a sua máquina for apertada, a
conversa fica lenta primeiro. Se nada disso valer a pena para você, o modo
normal continua ótimo.

## Passo 1 — instale o Ollama (o programa que roda o modelo)

Baixe em **https://ollama.com** e instale como qualquer aplicativo.
(Mac, Windows e Linux.)

## Passo 2 — baixe um modelo pequeno

Abra o terminal (no Mac: aplicativo "Terminal") e digite:

```
ollama pull llama3.2:3b
```

Espere o download terminar (é a parte dos ~2 GB). Só precisa fazer uma vez.

## Passo 3 — rode o servidor da conversa

Pegue o executável `conversa-server` (vem junto com o jogo, ou na página de
downloads) e rode no terminal:

```
MODEL_URL=http://localhost:11434/v1/chat/completions MODEL_NAME=llama3.2:3b ./conversa-server
```

Quando aparecer `escutando em http://localhost:8080/fala`, está pronto.
Deixe essa janela aberta enquanto joga.

## Passo 4 — ligue no jogo

Dentro do jogo, abra o console e digite:

```
bs.Conversa http://localhost:8080/fala
```

Pronto — e fica salvo: não precisa repetir na próxima vez que jogar. Agora é
só chegar perto de um morador e conversar:

```
bs.Falar bom dia! como vai a lavoura?
```

## Se algo der errado

| sintoma | causa provável | conserto |
|---|---|---|
| O morador responde "disso eu não sei falar" para TUDO | o servidor da conversa não está rodando, ou a URL está errada | confira a janela do passo 3; `bs.Conversa` sem argumento mostra o modo atual |
| A primeira resposta demora muito | o Ollama está carregando o modelo na memória (só na primeira) | espere — as próximas são rápidas |
| "o modelo nao respondeu" no terminal | o Ollama não está aberto, ou o nome do modelo está errado | `ollama list` mostra os modelos que você tem; o `MODEL_NAME` tem de ser um deles |
| O jogo ficou lento | o modelo está disputando memória com o jogo | use um modelo menor (`ollama pull llama3.2:1b`) ou volte ao modo normal (`bs.Conversa off`) |

## Para desligar

```
bs.Conversa off
```

O modo normal volta na hora — os moradores continuam conversando, do jeito
de sempre.

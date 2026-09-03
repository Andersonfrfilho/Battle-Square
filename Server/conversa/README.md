# conversa-server — o servidor de referência da conversa dinâmica

O contrato do jogo (`NpcDialogue.h`) na frente de **qualquer modelo
OpenAI-compatível**. Um código só serve os dois modos da decisão 67:

- **na infra oficial**, apontando `MODEL_URL` para o vLLM/servidor de modelo;
- **na máquina do jogador**, como companheiro local do llama.cpp.

## Rodar local (o companheiro)

```bash
# 1. o modelo (qualquer .gguf pequeno serve — Llama 3.2 3B, Qwen 2.5 1.5B...)
llama-server -m modelo.gguf --port 8081

# 2. este servidor (traduz o contrato do jogo para a API do modelo)
cd Server/conversa && bun install && bun start

# 3. no jogo
bs.Conversa http://localhost:8080/fala
```

## Rodar na infra

```bash
MODEL_URL=https://modelo.interno:8000/v1/chat/completions \
MODEL_NAME=llama-3.3-70b \
PORT=8080 bun start
```

## O contrato

`POST /fala` com o digesto do jogo → `{ "npcSays": "..." }`.
Digesto malformado → `400`; modelo fora → `502` — e o jogo cai no modo
restrito sozinho, então falhar depressa é o comportamento certo.

## Verificar

```bash
bun test          # o prompt e a fronteira
bun x tsc --noEmit
curl -s -X POST localhost:8080/fala -H 'Content-Type: application/json' -d '{
  "npc": {"name":"Dona Iraci","storyStage":"guardo as sementes","meetings":4,"homeStartHour":14},
  "deeds": {"beatChampion":false,"soldAPet":false,"hasWaterPet":false},
  "facts": ["o Centro de Recuperacao cura de graca"],
  "playerSays": "bom dia!"
}'
```

## O que ele NÃO faz, de propósito

- **Não decide o que o NPC sabe** — o digesto do jogo decide, e segredo não
  entra nele por construção (testado do lado do jogo).
- **Não guarda estado** — memória de conversa é do jogo (visitas no save) ou
  de uma versão futura; um servidor sem estado escala e reinicia de graça.
- **Não fala com o jogador sem o jogo no meio** — a rota valida o digesto
  inteiro, e digesto só o jogo monta.

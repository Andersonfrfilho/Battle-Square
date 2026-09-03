# Conversa dinâmica com os moradores — necessidades e diferenças

O jogo tem **dois modos de conversa**, e a escolha é sua (`bs.Conversa` no
console). Nenhum deles é obrigatório: o restrito funciona sempre, para todo
mundo, sem instalar nada.

## As diferenças

| | **Restrito** (padrão) | **Dinâmica** |
|---|---|---|
| O morador conversa sobre | a história DELE e o que você já fez | qualquer assunto do jogo |
| Improvisa? | não — roteiro que reage aos seus feitos | sim, em personagem |
| Precisa de internet? | **não** | só no modo infra oficial |
| Precisa instalar algo? | **não** | só no modo modelo local |
| Velocidade da resposta | imediata | 1–5 segundos |
| O que ele sabe | a história e as dicas dele | tudo que o jogo lhe conta — e **nada além**: segredo não entra na conversa em modo nenhum |

Se a rede falhar no meio de uma conversa dinâmica, a resposta cai no modo
restrito **na hora** — a conversa nunca morre.

## As necessidades de cada jeito de ligar a dinâmica

### 1. Pela infra oficial (o jeito simples)

- **Precisa:** internet.
- **Não precisa:** instalar nada, nem hardware além do jogo.
- Ligue com: `bs.Conversa <url do servidor oficial>`

### 2. Por um modelo rodando NA SUA MÁQUINA (offline de internet)

- **Precisa:**
  - baixar um modelo pequeno (~1–2 GB — ex.: Llama 3.2 3B ou Qwen 2.5 1.5B,
    quantizados);
  - ~2–4 GB de RAM livres **além** do jogo;
  - rodar um servidor local de modelo (uma linha com o
    [llama.cpp](https://github.com/ggerganov/llama.cpp)):

    ```
    llama-server -m modelo.gguf --port 8080
    ```

- **Não precisa:** internet depois do download.
- **A diferença honesta:** o modelo pequeno conversa bem para vizinho de vila,
  mas não é o modelo grande da infra — respostas mais curtas, menos memória de
  contexto. E ele disputa RAM e CPU com o jogo: numa máquina apertada, a
  conversa fica lenta primeiro.
- Ligue com: `bs.Conversa http://localhost:8080/fala`

> ⚠️ O endpoint precisa implementar o contrato de `NpcDialogue.h`
> (`{npc, deeds, facts, playerSays}` → `{npcSays}`). Um adaptador de uma
> dúzia de linhas na frente do llama-server resolve; a infra oficial já fala
> o contrato nativamente.

### Desligar

`bs.Conversa off` — o modo restrito continua, como sempre.

## Por que o restrito nunca sai

- **Offline joga-se local** (decisão 39): conversa que exige internet não pode
  ser a única conversa.
- Ele é o **para-quedas** da dinâmica: qualquer falha de rede cai nele.
- E ele é honesto: quando não sabe, diz *"disso eu não sei falar"* — e conta o
  que sabe.

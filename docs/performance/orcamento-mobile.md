# Orçamento de Performance — Mobile

**Feature:** `.specs/features/mobile/`
**Criado:** 2026-08-26
**Status dos números:** **todos são ALVO. Nenhum foi medido.** Ver "Como medir" no fim.

Este documento paga uma dívida explícita: `STATE.md` carregava, desde antes de
M5, o Todo *"Definir orçamento de performance de mobile antes de M5 — decisões
de mundo tomadas sem ele são irreversíveis"*. O Todo não foi cumprido a tempo,
e **M5 inteiro foi decidido contra um número provisório** (DP-streaming-01: 512
MB e 60 FPS, com a nota de que eram "decisão de partida, não medição"). Este
documento é onde essa conta chega.

---

## Aparelho de referência

**Gama média de ~4 anos atrás.** Referência concreta: iPhone 11 / Android de
classe Snapdragon 730, **4 GB de RAM**.

**Por que não o topo de linha:** mirar num aparelho que não precisa de
otimização é não ter orçamento nenhum — qualquer coisa passa, e o primeiro
aparelho real reprova.

**Por que não o mais fraco possível:** desenhar para um aparelho que quase
ninguém mais usa custa decisões de escala que empobrecem o jogo para todo mundo,
em troca de um público que já migrou.

Gama média de 4 anos é o aparelho que a maior parte de um público real tem na
mão hoje. É uma escolha de produto, e está aqui escrita para poder ser
contestada com argumento em vez de por gosto.

---

## Os números

| Grandeza | Alvo | Medido | De onde vem o alvo |
|---|---|---|---|
| Memória do processo | **1,5 GB** | *(não medido)* | Dos 4 GB de RAM, o SO e o resto do sistema comem boa parte; 1,5 GB é o teto realista antes de o SO começar a matar o app em segundo plano |
| Streaming de mundo (subconjunto) | **384 MB** | *(não medido)* | Os 512 MB de DP-streaming-01 eram alvo de PC. Em mobile ele **desce** |
| Frame time | **33,3 ms (30 FPS)** | *(não medido)* | 60 FPS em mundo aberto num aparelho de gama média de 4 anos é promessa que não se cumpre. O alvo de 60 continua valendo **em PC** |
| Resolução de render | **até 1280x720**, com escala dinâmica | *(não medido)* | Renderizar na resolução nativa de tela de celular gasta em pixels que ninguém distingue à distância de uso |

### O que muda em relação a M5

**DP-streaming-01 fica explicitamente superado em mobile.** O par (512 MB, 60
FPS) continua sendo o alvo de PC; em mobile o par é (384 MB, 30 FPS). As duas
coisas convivem, e a diferença é o ponto: um orçamento por plataforma, não um
número único fingindo servir aos dois.

### O número que mais provavelmente cai

**Os 384 MB de streaming.** Um mundo com World Partition e HLOD num orçamento
desses é apertado, e é o primeiro lugar onde uma medição real deve discordar do
alvo. Se discordar, a resposta certa **não** é subir o número em silêncio: é
reabrir DP-streaming-02a (tamanho de célula e raio) com o número medido na mão.

---

## Como medir (quando der)

**Isto não pode ser feito hoje.** A máquina não compila para nenhuma plataforma
móvel — ver B-006, B-006b e B-007 em `STATE.md`. Os passos abaixo existem para
que, quando os bloqueios caírem, ninguém precise redescobrir o método.

1. Resolver B-006 (componente IOS pelo Epic Games Launcher) ou B-007 (NDK
   Android), e B-006b (assinatura) se o alvo for iOS.
2. Empacotar `WorldStreamingTest` em Development para o aparelho de referência.
3. Possuir o `WorldExplorer` e percorrer a área de ponta a ponta.
4. Com `stat memory`, `stat unit` e `stat streaming`, preencher a coluna
   **Medido** da tabela acima, anotando aparelho e build.
5. Para cada linha onde medido e alvo discordarem, escrever aqui **qual dos
   dois muda** e por quê. Um alvo que se ajusta a cada medição não é orçamento,
   é legenda.

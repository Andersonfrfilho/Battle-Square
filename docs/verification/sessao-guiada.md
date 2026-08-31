# Sessão guiada — 31/08/2026

Roteiro para uma sessão só, na ordem em que a informação vale mais.
Responda por código (`A3 estranho: ...`). Item sem observação eu considero ok.

**Não diga a causa, diga o que viu.** Descrever causa suposta já custou rodadas
aqui — `Atacar Esquerda` virou "ele foi para a esquerda" e eu fui atrás do
defeito errado.

## Preparação

1. Feche o Editor se estiver aberto.
2. Reabra o projeto normalmente e aperte **Play**.
3. Ao terminar, me mande `Saved/BattleDebug.txt` — ele se grava sozinho.

Teclas: **M** abre o mapa completo · **F7** troca o jogador controlado ·
**F9** copia o painel · **F10** limpa.
Console: `bs.Marcar`, `bs.ControlOpponent 1`, `bs.Especializar`, `bs.SkyDay 8.15`.

## A — Os trinta primeiros segundos

- **A1** — Você aparece de pé, e **não cai** pelo cenário.
- **A2** — Há **sol e céu**, não a luz azul chapada padrão.
- **A3** — Há chão, mata e **serra ao longe** fechando o horizonte.
- **A4** — Não há mais **blocos coloridos** em lugar nenhum.
- **A5** — Seu pet e os inimigos são visíveis e se distinguem do chão.

## B — Andar pelo mundo

- **B1** — Andar até a mata: você **esbarra** em tronco e pedra, não atravessa.
- **B2** — Andar até a beira: há **água**, e ela te barra.
- **B3** — O painel (canto superior direito) mostra pet, nível, cinco atributos
  e a distância do adversário mais próximo.
- **B4** — A linha do adversário **fica amarela** quando ele se aproxima.
- **B5** — Entrar num campo de treino colorido mostra progresso, e o atributo
  sobe ao fechar 100%.

## C — O mapa (tudo novo, nunca visto)

- **C1** — O minimapa começa **escuro**.
- **C2** — Andar **abre manchas** — manchas mesmo, não um fiapo atrás de você.
- **C3** — As manchas têm cores diferentes: mata, clareira, margem, água.
- **C4** — O minimapa **gira com o olhar**.
- **C5** — **M** abre o mapa completo, e ele **não gira** quando você gira a câmera.
- **C6** — O mapa completo tem **legenda**, e as cores dela batem com o mapa.
- **C7** — No console, `bs.Marcar` — aparece uma marca no mapa onde você está.
- **C8** — `bs.Marcar` de novo no mesmo lugar **apaga**.
- **C9** — Ande um pouco, `bs.Marcar perigo` — sai numa **cor diferente**.
- **C10** — Feche o jogo, abra de novo: o que você descobriu **continua lá**.

## D — O encontro e a arena

Faça duas vezes: uma **perto da água**, outra **dentro da mata**.

- **D1** — Encostar no inimigo **começa a batalha** (as duas vezes).
- **D2** — Perto da água, a arena tem **casas de água**.
- **D3** — Na mata, a arena tem **casas bloqueadas** (tronco/pedra).
- **D4** — Nenhuma batalha **deixa de começar**.
- **D5** — A grade desenhada mostra o que cada casa é, com o nome.

## E — O combate

- **E1** — Escolher três ações e confirmar: o feed narra **em português**.
- **E2** — O pet **desliza** até a casa nova, não teleporta.
- **E3** — O indicador de fase conta (`reproduzindo fase 3 de 11`).
- **E4** — Golpe trancado aparece como `🔒 Nome (exige Atributo N)` e clicar
  nele **não trava** o turno.
- **E5** — `bs.ControlOpponent 1`: você joga os dois lados, e a barra mostra as
  ações do jogador 2.
- **E6** — Vencer devolve ao mundo, com XP, e **sem cair**.

## F — Julgamento (só você responde)

- **F1** — O mundo parece um **lugar**, ou um cenário de teste?
- **F2** — Descobrir o mapa dá **vontade de explorar**, ou é barra de progresso?
- **F3** — Reconhecer o lugar na arena muda **como você anda** pelo mundo?
- **F4** — O painel **ajuda** ou é ruído?
- **F5** — O combate é **tenso**? Onde ele fica chato?
- **F6** — O que mais te incomodou, mesmo que pequeno?

## Fora de alcance hoje

O espelho de pets é de 25/08. **Fantasma, luz, dreno, gelo e os pets novos não
existem no jogo** — precisam do backend rodando e do worker regerando o espelho.
Lama pode aparecer sozinha, na margem d'água.

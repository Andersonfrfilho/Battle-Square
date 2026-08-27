# Roteiro de verificação — Legibilidade do Combate

**Status:** **não verificado ainda** — o que está abaixo é exatamente o que
teste headless NÃO decide.

Os testes provam que a frase existe, nomeia quem agiu e mostra o número do
dano. Eles não têm como responder se a frase **explica a jogada** para quem
está aprendendo, nem se a barra é legível a essa distância de câmera. Isso é
julgamento humano, e L-035 diz que julgamento humano é medição, não palpite.

## Como abrir

Editor → nível `BattleScreen` → Play. A batalha abre direto, sem mundo aberto.

## LEG-01 — A barra de vida é legível

- [ ] A barra aparece **sobre** cada pet, não atravessando o corpo.
- [ ] Ela encolhe **pela esquerda**, não pelos dois lados.
- [ ] A cor vai de verde a vermelho conforme cai.
- [ ] Ela some junto com o pet derrotado.
- [ ] Dá para ler quem está pior **sem parar para comparar**.

## LEG-02 — A narração explica a jogada

Jogue um turno atacando um oponente que assumiu postura evasiva.

- [ ] O feed diz que o ataque foi esquivado.
- [ ] Depois de ler, dá para concluir **por que** o ataque não valeu nada.
- [ ] As frases aparecem no ritmo da reprodução, não todas de uma vez.
- [ ] A cor distingue quem agiu.

## LEG-03 — O feed é produto, não depuração

- [ ] Com `bs.ShowBattleDebug 0`, o painel some e **o feed continua**.

## LEG-04 — Julgamento

- [ ] Cinco linhas são suficientes, ou a frase importante sai cedo demais?
- [ ] Alguma frase soa como mensagem de erro em vez de narração?

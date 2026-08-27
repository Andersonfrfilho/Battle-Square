# Legibilidade do Combate

**Status:** Aprovado para execução (2026-08-27)
**Marco:** Fora de marco — dívida aberta pela Interface de Batalha

---

## Problema

O combate ficou **jogável** e continua **ilegível**. O núcleo já distingue seis
desfechos diferentes — `AtaqueAcertou`, `AtaqueErrou`, `Esquivou`, `Defendeu`,
`DanoAplicado`, `PetMorreu` — e **nenhum deles chega ao jogador**. Ele escolhe
três ações, vê duas esferas trocarem de casa, e a partida acaba sem que ele
saiba quem bateu em quem, quanto doeu, ou por que perdeu.

Isso não é enfeite. É o que separa "escolher ações" de **"aprender a jogar"**:
sem ver que um ataque foi esquivado, o jogador não tem como concluir que atacar
um pet que assumiu postura evasiva é desperdício. A regra existe, está testada,
e é invisível — então, para quem joga, ela não existe.

`APetView` já **calcula** `HealthRatio` a cada evento e não o desenha em lugar
nenhum. A informação está pronta e é jogada fora uma linha antes de virar tela.

## Quem se beneficia

O jogador aprendendo as regras, e nós: uma narração legível é também o
instrumento que mostra defeito de resolução — do mesmo jeito que a grade
desenhada teria mostrado na hora que "Baixo" andava para a direita.

## Comportamento esperado

1. **Cada pet mostra sua vida** — barra sobre o corpo, encolhendo do valor
   cheio conforme o dano entra, na cor do lado. Derrotado, some com o corpo.
2. **Cada evento vira uma frase em português**, no feed do combate:
   *"Faísca atacou Brisa e acertou — 12 de dano"*, *"Brisa esquivou"*,
   *"Faísca tentou andar para a esquerda e bateu na borda"*.
3. **O fim da partida é anunciado** com quem venceu, não só com o silêncio da
   fila que não reabre.
4. **O feed é de produto, não de depuração** — compila em Shipping e não some
   com `bs.ShowBattleDebug 0`. O painel de depuração continua existindo ao
   lado, com outra função e outro público.

## Fora de escopo

- Animação de ataque, som, partícula. Aqui se resolve **o que se entende**,
  não o que se admira.
- Autoria de UMG para o feed: ele é desenhado no HUD, como a grade. Trocar por
  widget autorado é melhoria posterior e não muda nada do que o jogador lê.

## Decisões

**DP-leg-01 — A frase é função pura do evento.** `FBattleNarration::Describe`
recebe evento e nomes e devolve texto; não lê estado, não consulta a arena, não
tem efeito. É o que a torna testável sem abrir o editor — e a lição L-035 diz
que o teste que reproduz vem antes de tudo.

**DP-leg-02 — A narração NÃO reinterpreta a regra.** Ela traduz o evento que o
núcleo emitiu. Se a frase precisar decidir se um ataque "deveria" ter acertado,
isso é uma segunda fonte de verdade, e L-032/L-033 mostraram o preço disso.

**DP-leg-03 — A barra de vida não recalcula dano.** Ela lê `HealthRatio`, que
já vem do evento. Nenhuma subtração acontece na apresentação.

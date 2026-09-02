# Roteiro — o turno endereça PET, e o que só o olho vê

Os testes provam que cada ação tem dono e que dois aliados agem no mesmo turno.
**Nada disso prova que o jogador consegue dizer de quem é qual** — e é essa
metade que decide se a feature existe para quem joga.

---

## 1. O clique diz de qual pet é

1. Abrir uma batalha (PIE) e clicar em qualquer ação na barra de botões.

**Esperado**, no painel:

```
clique: Atacar — pet 2
```

- Com **um** pet por lado, o sufixo `— pet N` **não aparece**, e é assim mesmo:
  fila sem dono é estado de montagem, e escrever "pet 0" ali seria responder
  errado a uma pergunta que ninguém fez.

> **Por que esta linha importa mais do que parece:** com um pet por lado,
> `clique: Atacar` já dizia quem agiu. Com dois aliados, a mesma linha deixa de
> dizer — e o defeito que ela existe para diagnosticar ("a ação não mudou
> nada") passa a ter DUAS explicações indistinguíveis: a regra falhou, ou a ação
> foi para o outro pet. Sem o sufixo, a ferramenta de diagnóstico perde a
> capacidade de diagnosticar **sem nada acusar**.

## 2. Dois aliados, duas direções, no mesmo turno

1. Mandar o aliado A para a **esquerda** e o B para a **direita**.
2. Confirmar o turno.

**Esperado:** os dois andam, cada um para o seu lado, e o painel diz qual foi
qual.

> **Se os dois andarem juntos**, o defeito é o de antes desta feature: só o
> primeiro pet vivo do lado recebia a ação. E sem a linha no painel ninguém
> saberia se aquilo foi escolha ou limitação.

## 3. Dois aliados mirando a MESMA casa

Mandar A e B para a mesma casa.

**Esperado:** os **dois** ficam onde estavam, e o painel narra **dois**
bloqueios.

- Não é um passar e outro ficar: a casa disputada iria para quem o laço visitou
  primeiro, e a ordem do array não é garantia de determinismo.

## 4. Pet sem ação escolhida AGUARDA, e o painel diz

Escolher ação só para o aliado A e confirmar.

**Esperado:** B aguarda, **e isso aparece por escrito**. Silêncio faria o
jogador achar que escolheu para os dois.

## 5. A grade distingue os aliados

Com dois aliados no tabuleiro, a grade desenhada mostra **qual** está em cada
casa — não apenas que há alguém do lado 0.

---

## O que este roteiro NÃO cobre

- **Quantos pets por lado o jogo terá.** A CP1 mediu o TETO (a altura da
  grade); o número dentro dele é decisão do dono.
- **A partida em rede com N pets.** O fio carrega o dono e há teste disso, mas
  duas máquinas de verdade é outro roteiro.

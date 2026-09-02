# Roteiro — itens e biologia, o que só o olho vê

Os testes provam que a soma acontece. **Nada disso prova que a informação chega
ao jogador** — e item que ninguém vê é item que ninguém equipa.

---

## 1. A biologia no painel

1. Abrir o mundo (PIE) com um pet na coleção.
2. Ler o painel, canto superior direito.

**Esperado**, logo abaixo dos atributos:

```
biologia: escamosa · corpulento · branquia · patas
```

- Só os eixos **informados** aparecem. Um pet de coleção antiga não tem a
  linha — e é assim mesmo: a ausência não decide nada, então não ocupa altura
  num painel com teto de 12 linhas.

> **Se dois pets do mesmo elemento mostrarem a mesma biologia**, o cadastro é
> que está igual, não o código. A diferença tem de vir do dado.

## 2. Os itens no painel

**Esperado**, quando há o que mostrar:

```
vestindo: Bota de lava · Grampo de rocha
mochila: Unguento termal x3
```

- Aparecem por **nome**, nunca por `bota_de_lava`. Ver o id na tela é vazamento
  da chave interna.
- As duas linhas **somem** quando estão vazias.

## Como conseguir um item

Não há loja, espólio nem recompensa ainda — e sem uma maneira de conseguir um
item, a mochila seria um sistema que ninguém consegue exercitar. Pelo console:

```
bs.DarItem bota_de_lava
bs.Vestir bota_de_lava
bs.Tirar bota_de_lava
bs.Usar unguento_termal
```

`bs.DarItem <id> [quantos]` — sem quantidade é um.

**Toda recusa aparece na tela**, dizendo o motivo provável (`nao vestiu — sem o
item, ou sem slot livre`). Falhar calado faria você clicar de novo achando que
o clique não pegou.

Compilados fora do Shipping, como todas as ferramentas deste projeto: um jogo
publicado onde qualquer um se dá itens não é o mesmo jogo.

## 3. O aceite: a bota faz diferença

1. Com a bota **na mochila** (não vestida), atravessar lava.
2. **Vestir** a bota e atravessar de novo.

**Esperado:** com a bota, o pet aguenta visivelmente mais.

> **É a diferença que é o aceite.** Se os dois forem iguais, a bota está sendo
> lida e não aplicada — e a linha `vestindo:` estaria mentindo.

## 4. O invariante: equipar MOVE, não cria

1. Anotar quantos há na mochila.
2. Equipar. **A mochila diminui em um.**
3. Desequipar. **A mochila volta ao número anterior.**

> Se equipar **não** diminuir a mochila, o item existe duas vezes — e a mesma
> bota vai vestir cinco pets. É o defeito que a decisão do usuário descreve ao
> contrário, e o mais fácil de não notar: tudo funciona, só que de graça.

## 5. O gasto do consumível

Usar um consumível com quantidade 1, duas vezes.

**Esperado:** a primeira funciona, a segunda **falha visivelmente**. Gastar em
silêncio um item que não existe é o jogador achando que usou.

---

## O que este roteiro NÃO cobre

- **Quais itens o jogo terá.** O catálogo nasce com três, que provam as regras;
  o resto é decisão do dono, e será perguntada.
- **Se os números estão certos.** São balanceamento, e estão marcados como tal
  no JSON.

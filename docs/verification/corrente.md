# Roteiro — a corrente, o que só o olho vê

Os testes já afirmam que descer rende mais que subir, que de través não muda
nada e que a correnteza nunca prende ninguém. **Nada disso prova que a
informação CHEGA ao jogador**, e é essa metade que este roteiro cobre.

A distinção não é formalidade: a corrente é a primeira regra deste projeto que
move o jogador **contra a vontade dele**. Uma regra assim, invisível, não se lê
como regra — se lê como defeito.

---

## 1. No mundo: o painel diz de que lado da corrente você está

1. Abrir o mundo (`open -a` no Editor; PIE).
2. Andar até um rio — a cor da água no chão e a linha `terreno:` do painel
   ajudam a achar.
3. Entrar na água e andar **rio abaixo**.

**Esperado:** duas linhas no painel, atualizando no lugar (chave fixa):

```
pisando: vau em agua doce (passo 71%)
corrente: 40% rumo (0.83, -0.55) — voce vai A FAVOR
```

A segunda linha em **verde**.

4. Sem sair da água, virar e andar **rio acima**.

**Esperado:** a mesma linha em **laranja**, dizendo `CONTRA` — e o `passo` da
linha de cima **cai**.

> **É a queda do `passo` que é o aceite da C5.** Se os dois sentidos mostrarem
> a mesma porcentagem, a corrente está sendo lida mas não aplicada — e o rio
> voltou a ser um pântano com desenho de rio.

5. Atravessar de **margem a margem**, perpendicular ao curso.

**Esperado:** `de traves`, em cinza, e o `passo` **igual** ao de água parada.
Este é o contrapeso que importa: atravessar é o movimento que o mapa mais pede,
e cobrar atraso ali castigaria justamente a travessia.

6. Sair para a terra.

**Esperado:** a linha `corrente:` **some**. Ela não fica dizendo "nenhuma" pela
ilha inteira — uma linha sempre presente é uma linha que se aprende a ignorar,
justo onde ela passaria a importar.

---

## 2. Na batalha: a grade aponta

1. Entrar numa batalha cuja arena tenha sido montada sobre um rio (encostar num
   inimigo perto da água).
2. Olhar a grade desenhada no tabuleiro.

**Esperado:** nas casas de água corrente, uma **seta ciano** apontando o rumo, e
o número da força na etiqueta:

```
(1,1) ÁGUA FUNDA →40
```

- A **seta é maior** onde a corrente é mais forte. Setas todas do mesmo tamanho
  significam que a força não chegou ao desenho.
- Casas de água **parada** não têm seta nenhuma.

3. Mandar o pet andar **contra** a seta, a partir de uma casa de corrente forte.

**Esperado:** ele **escorrega** — o painel narra —, e não sai do lugar, ou sai
menos do que se pediu. Andar **a favor** da seta funciona normalmente.

> Se o pet for empurrado para um lado que a seta **não** aponta, o defeito é
> grave e é de direção: a seta e o movimento leem a mesma tabela
> (`GetDirectionDelta`), então discordarem significa que alguém abriu uma
> segunda cópia dela. Foi assim que "Baixo" andou para a direita neste projeto.

---

## 3. A balsa

1. Achar uma travessia de **balsa** (as do traçado que não são ponte nem vau).
2. Cronometrar a ida e a volta — o relógio do próprio olho basta, a diferença
   não é sutil.

**Esperado:** os dois sentidos duram **diferente**, e a balsa **nunca para**:
mesmo na correnteza mais forte, ela continua avançando devagar.

Balsa parada no meio do rio não é uma água difícil — é um jogo travado, e o
jogador não tem como saber a diferença.

---

## O que este roteiro NÃO cobre

- **A força estar certa** para cada rio: isso vem do traçado assado, e quem
  verifica é `bake_island.sh`.
- **Para onde a eletricidade anda.** A corrente elétrica atravessa o meio
  inteiro; ela não desce o rio.

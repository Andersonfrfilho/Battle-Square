# As quatro decisões do usuário — 02/09/2026

Perguntadas antes de escrever uma linha, porque são de conteúdo e não minhas.
Ficam aqui porque decisão que só existe no chat some na próxima compactação, e
a próxima rodada a redecide diferente.

---

## 1. Onde o item mora

> *"nós carregamos os itens na mochila, a não ser que for um item equipado com
> o pet"*

**Dois lugares, e a diferença é o estado do item:**

| lugar | o que é | quem é o dono |
|---|---|---|
| **Mochila** | o item guardado, com QUANTIDADE | o jogador |
| **Equipado** | o item vestido, agindo | aquele pet |

Não são dois sistemas: é o **mesmo item** em dois estados. Equipar TIRA da
mochila; desequipar DEVOLVE. Sem isso, o mesmo item existiria duas vezes e o
jogador equiparia a mesma bota em cinco pets.

## 2. Quantos slots

> *"podemos ter vários espaços"*

**Vários, e o número é configuração, não constante espalhada.** Vive em
`DefaultGame.ini` como o tamanho da grade já vive.

## 3. O item se gasta

> *"depende do item, mas ele pode se gastar, e pode ser usado e aí a quantidade
> é gasta"*

**Duas naturezas, e é o CADASTRO que diz qual:**

| natureza | o que faz | o que acontece ao usar |
|---|---|---|
| **Equipamento** | age enquanto vestido (a bota de lava) | nada — ele fica |
| **Consumível** | age uma vez, na hora | a quantidade cai; zerou, some da mochila |

⚠️ **A quantidade é da PILHA, não do item.** Cinco poções são uma linha da
mochila com `quantidade: 5`, não cinco linhas — senão a mochila de quem junta
coisa vira uma lista que não cabe na tela.

## 4. O que é anatomia

> *"devemos ter propriedade de biologia do pet com várias propriedades"*

**Uma BIOLOGIA com vários eixos**, não um rótulo só. Cada eixo responde por uma
coisa que o mundo já pergunta:

| eixo | valores | responde por |
|---|---|---|
| **Pele** | escamosa, peluda, lisa, couraçada, viscosa | resistência por fluido |
| **Porte** | miúdo, médio, corpulento | firmeza contra a corrente |
| **Respiração** | pulmão, brânquia, ambos | submergir sem se afogar |
| **Apoio** | patas, nadadeiras, asas, rastejante | firmeza e passo na água |

**Por que eixos e não um perfil nomeado:** com perfil, cada criatura nova pede
um perfil novo, e "Réptil de deserto" e "Réptil de pântano" viram duas entradas
que repetem tudo menos uma linha. Com eixos, as combinações nascem do cruzamento
— e é o cruzamento que faz dois pets do MESMO elemento resistirem diferente, que
é o aceite da tarefa.

---

## O que estas respostas NÃO decidem, e vai ser perguntado quando chegar

- Que itens existem (o catálogo em si).
- Quanto cada pele resiste a cada fluido (os números).
- Se a mochila tem limite de tamanho.

Números de balanceamento nascem defensáveis e **medidos contra o elenco**, como
o `PowerPerCellToTakeTheField` da P4 — e ficam marcados como decisão do dono.

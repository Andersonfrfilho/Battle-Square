# Itens e biologia — tarefas

## B1 — A biologia existe e chega à batalha ✅ FEITO
> 🤖 Modelo: `opus` — mexe no cadastro assinado

Catálogo de quatro eixos, coluna tolerada no espelho, biologia no fim do
payload canônico, três formatos aceitos na verificação, soma no tradutor, e a
linha no painel do mundo.

*Aceite:* dois pets do MESMO elemento com biologias diferentes resistem
diferente. **Feito e provado no tradutor**, que é o caminho de produção.

---

## I1 — O item existe como CADASTRO
> 🤖 Modelo: `sonnet`

`Config/Items.json` + `FItemCatalog`, espelhando `FPetBiologyCatalog`. Cada item
tem nome, natureza (**equipamento** ou **consumível**), resistências por fluido
e firmeza.

**Por que espelhar em vez de inventar:** o catálogo de biologia acabou de ser
escrito com leitor único para quatro eixos, override para teste e busca sem
diferenciar caixa. Um segundo catálogo com outra forma daria duas maneiras de
cadastrar coisa neste projeto, e a segunda é sempre a que esquece o override.

*Aceite:* o catálogo carrega, e os nomes de fluido dele **casam com o
registro** — uma segunda grafia seria uma resistência que nunca casa e nunca
acusa.

## I2 — A MOCHILA guarda, com quantidade
> 🤖 Modelo: `sonnet`

`FPetCollectionSaveGame` ganha a mochila: uma lista de `{itemId, quantidade}`.

⚠️ **A quantidade é da PILHA, não do item.** Cinco poções são uma linha com
`quantidade: 5`, não cinco linhas — senão a mochila de quem junta coisa vira
uma lista que não cabe na tela.

*Aceite:* a mochila sobrevive a salvar e carregar, e **save antigo sem mochila
carrega** — campo novo que invalidasse a coleção apagaria os pets de quem já
jogava.

## I3 — EQUIPAR tira da mochila
> 🤖 Modelo: `sonnet`

O pet ganha slots; o número vive em `DefaultGame.ini`, como o tamanho da grade
já vive.

*Aceite:* equipar **diminui** a mochila e desequipar **devolve**; o total nunca
muda. Sem isso o mesmo item existe duas vezes e a mesma bota veste cinco pets.

*Contrapeso obrigatório:* equipar num slot ocupado **devolve o que estava lá**,
e não o apaga.

## I4 — O item SOMA na resistência
> 🤖 Modelo: `sonnet` — a menor de todas

O terceiro argumento de `ComposeFluidResist` deixa de ser `0`.

**Esta task é uma linha, e isso é de propósito:** a função foi escrita com o
argumento do item e com prova por valor injetado justamente para que este dia
não fosse uma descoberta.

*Aceite:* o MESMO pet com a bota atravessa a lava e sem ela, não.

## I5 — O consumível GASTA
> 🤖 Modelo: `sonnet`

Usar um consumível baixa a quantidade; zerou, sai da mochila.

*Aceite:* usar duas vezes um item com quantidade 1 falha na segunda, e a falha
**aparece** — gastar em silêncio um item que não existe é o jogador achando que
usou.

## I6 — O item na TELA
> 🤖 Modelo: `sonnet`

Painel do mundo diz o que está equipado; a barra de botões deixa equipar.

*Aceite:* item que ninguém vê é item que ninguém equipa. Roteiro em
`docs/verification/`.

---

## O que estas tarefas NÃO fazem

- **Não decidem que itens existem.** O catálogo nasce com um item que prova a
  regra (a bota de lava, que é o aceite escrito) e **pergunta** o resto.
- **Não mexem no traçado nem no gabarito.** Nada aqui encosta na carta.

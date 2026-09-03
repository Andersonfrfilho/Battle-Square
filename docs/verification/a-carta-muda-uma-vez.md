# Roteiro — a fundura, a ponte e o que se achou

Os testes já afirmam que a fundura existe por ponto, que a ponte destruída não
deixa passar e que a carta conta o escondido sem apontá-lo. **Nada disso prova
que a informação CHEGA ao jogador**, e é essa metade que este roteiro cobre.

A distinção pesa mais aqui do que de costume: as três coisas desta feature são
invisíveis por natureza. Fundura não tem silhueta, ponte destruída de longe é
uma ponte, e um segredo que ninguém percebe ter achado é indistinguível de não
existir.

---

## 1. A fundura, e a conta que ela permite refazer

1. Abrir o mundo (`open -a` no Editor; PIE).
2. Andar em **terra seca**, longe de qualquer água.

**Esperado:** **nenhuma** linha de fundura no painel.

> Este é o aceite invertido, e é o que importa aqui. Uma linha fixa dizendo
> `fundura: 0` em toda a ilha ensina a ignorá-la justo onde ela passa a
> importar — a mesma lição que a linha da corrente já tinha aprendido.

3. Entrar num **córrego** ou num rio raso.

**Esperado:** a linha aparece, em azul claro:

```
fundura: 62 (cintura 70 — da pe)
```

4. Sem sair da água, seguir para o **curso baixo**, onde o rio alarga.

**Esperado:** o número **sobe**, e em algum ponto a linha vira azul escuro:

```
fundura: 118 (cintura 70 — NADO)
```

> **A cintura ao lado do número é o aceite.** `fundo` sozinho não explica nada;
> `118 de 70` é uma conta que o jogador refaz sem instrumentar coisa nenhuma —
> e é a mesma conta que decide se o pé passa.

5. Voltar para a margem.

**Esperado:** a linha **some**, e não vai a zero.

---

## 2. A ponte, e o material antes da tentativa

1. Achar uma travessia. São **quatro pontes** na ilha: uma de **bloco** e três
   de **madeira**.
2. Chegar até ela.

**Esperado:** a linha `ponte de madeira` (ou `de bloco`), em bege.

3. Se a ponte for **destruída**, a linha é outra, em **vermelho**:

```
ponte de destruida — NAO PASSA
```

> **O aceite é ver isso ANTES de tentar atravessar.** Descobrir empurrando o
> personagem contra ela é a mesma caminhada até lá para descobrir que não
> valia — e é o que a ponte destruída existe para não ser.

4. Num vau ou numa balsa, a linha diz `travessia: vau` / `travessia: balsa`.
5. Sair de perto: a linha **some**.

---

## 3. O escondido, revelado por andar

A carta afirma **0 mostrados, 3 escondidos, 3 ao todo** — e não diz onde. Este
passo é o outro lado do mesmo trato.

1. Andar pelas trilhas, **longe das vilas**.
2. Sair da trilha ao passar por um trecho sem vila por perto.

**Esperado:** ao entrar na mancha, uma linha em âmbar:

```
VOCE ACHOU: mercado-negro (a carta nao mostra este lugar)
```

3. Sair de perto: a linha **some**.

> **O que este passo prova é o par.** A carta continua calada — conferir isso é
> abrir `docs/mundo/carta-ilha-de-mata.html` e ver que a linha **Escondido** diz
> quantos são e nenhuma coordenada. Se algum dia a carta apontar, os dois lados
> deste trato viram um só, e o segredo acaba.

---

## O que este roteiro NÃO cobre

- **Se a fundura está certa** — isso é teste, e está afirmado.
- **Se as pontes estão nos lugares certos** — o gabarito compara mundo e carta.
- **O achado em `LOCTEXT`** — o painel é ferramenta de desenvolvimento,
  compilada fora do Shipping. O texto que o JOGADOR lê quando acha um segredo é
  da `segredos-e-a-carta` (SC4), e não existe ainda.

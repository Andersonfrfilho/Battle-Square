# Desbloqueio de Golpes

**Status:** PROPOSTA — aguarda decisão. Pedida pelo usuário em 2026-08-29.

## O pedido

> "proponha a gamificação de skill com desbloqueios de golpes"

## O que já existe, e o que falta

O pet **já tem quatro golpes** vindos do backend, com poder e efeito de
terreno. O que não existe é **progressão**: os quatro estão disponíveis desde o
primeiro combate, e subir de nível só aumenta atributo.

Isso deixa um buraco: o pet de nível 20 joga igual ao de nível 1, só com
números maiores. **Nada muda de forma** — e é a forma que faz o jogador
reaprender o pet.

## A tensão central, que decide o resto

Golpe é **dado assinado do backend** (DP-golpe-03), e nível é **estado local do
jogador** (`USaveGame`, coleção). Desbloqueio cruza os dois — e é aí que mora o
risco:

- Se o **cliente** decide o que está desbloqueado, ele decide o próprio dano.
  Um save editado libera o golpe mais forte no nível 1, e a assinatura do
  catálogo não protege contra isso, porque o golpe É legítimo — o que foi
  forjado é o direito de usá-lo.
- Se o **servidor** decide, o jogo passa a exigir rede para uma partida
  solo — e hoje jogar sozinho não exige conta nem conexão.

**Não há resposta certa sem uma decisão de produto.** As duas opções abaixo são
honestas; a terceira é a que eu recomendo.

## Três desenhos possíveis

### A — Desbloqueio local, confiando no cliente
Nível do pet libera slots: 1 golpe no nível 1, 2 no 5, 3 no 10, 4 no 15.
- ✅ Funciona offline, sem conta, sem mudar backend.
- ❌ Save editado libera tudo. Aceitável em partida solo; **inaceitável em
  ranqueada**, e AD-017 já diz que adulteração não pode mudar resultado.

### B — Desbloqueio no servidor
A conta guarda quais golpes cada pet tem liberados, e o commit é validado lá.
- ✅ À prova de save editado.
- ❌ Exige conta e rede para jogar — contradiz "a conta é opcional", que
  **você acabou de inverter** ao decidir que conta é obrigatória. Se a
  obrigatoriedade valer, este custo já está pago.

### C — Local, mas o desbloqueio é ASSINADO (recomendado)
O backend assina, junto do pet, **em que nível cada golpe abre**. O cliente
compara com o nível local e libera — mas o *critério* é dado assinado, não
regra do save.
- ✅ Funciona offline; o save só guarda "nível 7", que já é o que ele guarda.
- ✅ Save editado ainda pode mentir o nível — mas isso é o mesmo problema que
  já existe hoje com atributo por nível, e não abre um novo.
- ✅ Nenhuma decisão de rede é forçada agora, e a checagem no servidor pode ser
  acrescentada depois sem mudar o dado.
- ❌ Um campo novo por golpe no payload assinado — a mesma cadeia de cinco
  lugares das fatias anteriores. É trabalho conhecido, não risco.

## Como isso vira JOGO, e não só uma trava

Desbloquear por nível sozinho é só esperar. Três formas de dar decisão:

1. **Escolha na evolução.** Ao subir o nível que libera, o jogador escolhe
   entre dois golpes. O outro fica perdido para aquele pet — e dois pets iguais
   passam a ser diferentes por causa de uma escolha dele.
2. **Golpe por conquista, não por nível.** "Vença 3 batalhas em casa de água"
   libera o golpe que alaga. Isso ensina a mecânica enquanto a exige — e amarra
   o desbloqueio ao terreno, que é o sistema mais novo do jogo.
3. **Ensinar golpe entre pets da coleção.** Um golpe já desbloqueado num pet
   pode ser ensinado a outro do mesmo tipo, uma vez. Dá função à coleção além
   de colecionar.

**Minha recomendação:** C + a forma 2. Conquista é o único dos três que ensina
o jogo enquanto recompensa, e é o que aproveita o terreno mutável que acabou de
existir.

## Perguntas que precisam de resposta

1. **A + B + C:** qual desenho? (recomendo C)
2. Quantos golpes o pet começa com — **um** ou dois? Um faz o segundo
   desbloqueio ser marcante; dois evitam que o começo seja monótono.
3. **Golpe desbloqueado é para o PET ou para a espécie?** Se for para a
   espécie, capturar outro Faísca já vem pronto — e a captura perde peso.
4. Perder um golpe ao escolher outro (forma 1) é **definitivo** ou refazível?
   Definitivo dá peso à escolha e frustra quem errou sem saber.
5. Vale em **ranqueada**? Se sim, o desenho C precisa da checagem no servidor
   antes de a ranqueada existir.

## Fora de escopo

- Golpes novos além dos quatro por pet.
- Comprar desbloqueio.

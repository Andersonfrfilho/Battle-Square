# Desbloqueio de Golpes

**Status:** Aprovado para execução (2026-08-29). Decisões do usuário abaixo.

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

---

## Decisões do usuário (2026-08-29)

**DP-desb-01 — O desbloqueio é da ESPÉCIE.** Liberou em um Faísca, liberou em
todo Faísca. Capturar outro da mesma espécie já vem com o que você conquistou.

> **Consequência que aceito registrar:** a captura perde valor como fonte de
> golpe. Ela passa a valer por atributo, nível e o próprio ato de completar a
> coleção — o que é coerente, mas é uma troca real, não um detalhe. Se depois
> parecer que capturar ficou sem graça, esta é a decisão a revisitar primeiro.

**DP-desb-02 — A SKILL sobe de nível com o uso, e é ela que desbloqueia.** Não
é o nível do pet que abre golpe: é a proficiência na skill (camuflar, voar,
submergir). Quem camufla muito destrava os golpes de camuflagem.

Isso amarra os dois sistemas mais novos do jogo: **a skill que você usa
determina os golpes que você ganha**. E resolve o que a proposta original tinha
de fraco — desbloqueio por nível é só esperar; por uso, é jogar de um jeito.

**DP-desb-03 — O requisito de cada golpe é DADO ASSINADO** (desenho C). O
backend assina, junto do golpe, qual skill e qual nível ele exige. O cliente
compara com a proficiência local e libera — o *critério* nunca é regra do save.

## O que a decisão exige que seja resolvido

**Proficiência sobe com uso EFETIVO, não com repetição.** "Camuflou 50 vezes"
convida a gastar turnos camuflando contra um bot parado. O ganho precisa vir de
uso que fez diferença — camuflar que de fato evitou um golpe, submergir que
aconteceu na água. Caso contrário o desbloqueio recompensa moer, não jogar.

**Proficiência é estado local**, como o nível. Vale a mesma ressalva do
desenho C: save editado pode mentir a proficiência, e isso não abre buraco
novo — é o mesmo que já vale para atributo por nível. Ranqueada, se existir,
precisa da checagem no servidor.

**O jogador precisa VER a proficiência subir.** Uma barra que não aparece é uma
recompensa que ninguém percebe recebendo — e este projeto já pagou caro por
regra invisível. Ao fim da batalha: `Camuflagem 3 → 4` e, quando abrir,
`GOLPE NOVO: Névoa`.

## Fatiamento

**Fatia 1 — proficiência por espécie e skill**, subindo com uso efetivo,
guardada na coleção e VISÍVEL ao fim da batalha. Sem desbloquear nada ainda:
entrega a progressão e o retorno na tela, que é o que dá sentido ao resto.

**Fatia 2 — requisito assinado no golpe** (`unlockSkill`, `unlockLevel`), com a
mesma cadeia de cinco lugares das fatias anteriores.

**Fatia 3 — o golpe trancado não aparece**, e destrava com anúncio.

A ordem importa: a fatia 1 é jogável sozinha e responde a pergunta que decide
tudo — **subir proficiência é divertido?** Se não for, trancar golpe atrás dela
só piora.

## Perguntas ainda abertas

1. Quantos golpes o pet começa com — **um** ou dois? Um faz o primeiro
   desbloqueio ser marcante; dois evitam que o começo seja monótono.
2. Vale em **ranqueada**? Se sim, a fatia 2 precisa da checagem no servidor
   antes de a ranqueada existir.
3. A proficiência tem **teto**? Sem teto, ela vira número que só cresce; com
   teto, o jogador sabe quando terminou aquela skill.

## Fora de escopo

- Golpes novos além dos quatro por pet.
- Comprar desbloqueio.

# Objetivo — Terminar

**Criado:** 2026-08-28, substituindo o objetivo de fechar M1→M7 (cumprido).
**O que este documento é:** o critério de "acabou" e a ordem do que falta.
Não é cronograma, não é lista de desejos: cada item aqui é trabalho identificado,
com o motivo de ainda não estar pronto.

---

## Onde estamos

O roadmap M1→M7 está fechado. O jogo tem, hoje, o **laço completo**: caminhar
pelo mundo, encontrar um inimigo que anda, cair numa arena com botões, escolher
ações que o pet sabe fazer, ver o que aconteceu narrado em português, vencer,
ganhar experiência, e voltar a um mundo que se repõe.

**347 testes**, cinco sondas, Editor e Shipping compilando (medido em
29/08/2026).

**Atualização de 29/08/2026 — M8, Atributos e Treino.** O laço deixou de ser
só "andar, lutar, ganhar XP": o pet tem atributos que crescem pelo uso, golpes
que exigem esses atributos, e o mundo tem CAMPOS DE TREINO — cinco clareiras
coloridas que dão destino ao mapa. O treinador ganhou especialidades escassas,
que multiplicam o treino sem substituir o pet.

O que isso muda no item 1 abaixo: as regras novas nasceram já visíveis, e a
varredura de classe sem chamador (59 públicas, 6 sem uso, todas justificadas)
não achou nenhuma L-041 nova.

O que mudou o método em 2026-08-27/28: **quatro recursos completos e testados
estavam inalcançáveis** porque ninguém os chamava (L-041). A varredura que os
achou virou rotina.

---

## O que falta, em ordem

### 1. Tornar visível o que já funciona — ✅ ESGOTADO do meu lado (2026-08-29)

O padrão mais caro deste projeto é regra que existe e não aparece. Cada item
abaixo é regra pronta, testada, invisível:

- ~~**Efetividade de tipo.**~~ ✅ **Feito em 2026-08-28.** O feed diz "É super
  efetivo!" e "Não é muito efetivo...". O número vem da montagem
  (`FPetPresentationInfo::EffectivenessPercent`) — o MESMO que multiplicou o
  ataque, não um recálculo.
- ~~**Skills no `WBP`.**~~ ✅ **Feito em 2026-08-29 (B-009 resolvido).** Os três
  botões existem no widget, com rótulo e ligados às variáveis do C++.
- ~~**Tipos dos pets.**~~ ✅ **Feito em 2026-08-28.** O painel diz
  `Faísca [Fogo] x Brisa [Planta] (você tem VANTAGEM de tipo)` **antes da
  primeira escolha**, e cada linha de estado mostra nome e tipo em vez de
  "lado N". Sem isso, "é super efetivo" era surpresa, não decisão.

### 2. Verificar o que ninguém verificou

**Doze roteiros nunca rodados.** Testes headless não decidem nada do que está
neles: eles existem justamente para o que só uma pessoa jogando responde.

Ordem por valor, do mais decisivo:

1. **`atributos-treino-e-mundo.md`** — M8 inteiro, escrito em 29/08/2026. É o
   mais decisivo AGORA: cobre tudo o que nunca foi visto na tela, e a pergunta
   que a feature existia para responder (*ver o pet mudar é interessante?*)
2. `encontros-transicao-batalha.md` — o laço completo, ENC-10 a ENC-12
3. `skills-por-pet.md` — a identidade do pet se sente?
4. `esconderijos.md` — as três trocas são distintas?
5. `legibilidade-do-combate.md` — a narração explica a jogada?
6. `arenas-variadas-jogabilidade.md` — a arena parece justa?
7. `escala-pets-skills-balanceamento.md` — a efetividade parece certa?
8. `traversal-camera-mundo.md`, `streaming-de-mundo.md`
9. `apresentacao-combate-visual.md`, `combate-online-rede.md`, `mobile.md`

Os três últimos exigem infraestrutura (dois processos, aparelho) e seguem
bloqueados.

### 3. Decidir o que está em aberto

**Oito specs com decisão pendente.** Duas são ideias novas do usuário, e as
duas estão paradas pela mesma razão — falta uma decisão, não código:

- **Clima no campo** (`clima-no-campo/`) — nove perguntas, sendo as mais
  pesadas: clima por região faz dois jogadores enfrentarem campos diferentes na
  mesma partida; localização é dado pessoal; campo que muda depois do commit faz
  perder por algo imprevisível.
- **Captura, roubo e treinador** (`captura-roubo-e-treinador/`) — o jogo não tem
  o conceito de DONO, e sem ele "sem dono", "selvagem" e "roubar" não têm como
  ser distinguidos.

As outras seis têm PROPOSTAs registradas que nunca foram confirmadas nem
recusadas. Enquanto isso, o comportamento vigente é o da proposta — decisão por
inércia, que é a pior forma de decidir.

### 4. Bloqueios que não são trabalho

Não têm solução por código nesta máquina, e ficam registrados para não serem
confundidos com pendência:

| | O quê |
|---|---|
| **B-006/B-007** | nenhuma plataforma móvel é compilável aqui |
| **B-008** | console exige licença e devkit |
| **B-004** | Server Target não compila com a engine do Launcher |
| **B-001** | escopo de mundo aberto contínuo |
| **B-003** | BTL-05 sem cobertura de execução sob o contrato de v1 |
| **B-005** | coleção local não distingue jogadores em partida de rede |

`FTouchMovementInput` fica sem ligar enquanto B-006/B-007 durarem: ligá-la sem
poder verificar seria entregar trabalho não medido.

---

## O que falta agora (29/08/2026)

**Só o que não é meu.** O item 1 segue esgotado do meu lado, o item 4 é
bloqueio de infraestrutura, e sobram dois:

- **Item 2 — os NOVE roteiros verificáveis.** Nenhum foi rodado. O de M8
  (`atributos-treino-e-mundo.md`) é novo e é o mais decisivo: 40 itens, e o
  julgamento de três números que são chute meu. É o único
  caminho para o que teste nenhum decide, e agora há bem mais a olhar: sol e
  mata no mundo, painel do mundo, campos de treino, golpe trancado, esquiva por
  reflexo. **Nada disso foi visto na tela** — só verificado por build e teste,
  que é exatamente o que deixou três atores invisíveis passarem aqui.
- **Item 3 — as decisões.** Além das oito antigas, M8 acrescentou duas: se o
  treino avança offline (DP-atr-10) e se dá para trocar de especialidade
  (DP-atr-11). As duas estão implementadas na metade REVERSÍVEL, de propósito.

## Critério de "acabou"

1. Nada de regra pronta e invisível (item 1 zerado).
2. Os oito roteiros verificáveis nesta máquina, rodados, com o resultado escrito
   no próprio roteiro — inclusive o que reprovar.
3. Toda PROPOSTA confirmada ou recusada, com o motivo registrado.
4. Bateria verde, quatro sondas limpas, Editor e Shipping compilando.
5. `STATE.md` e `ROADMAP.md` refletindo o que existe — não o que se pretendia.

**O que NÃO conta como pronto:** recurso testado que ninguém alcança (L-041),
regra que só aparece perdendo, e proposta que virou comportamento por inércia.

---

## Como trabalhar até lá

- Item 1 primeiro: é o único que eu resolvo sozinho e o que mais muda o jogo.
- Item 3 em paralelo, perguntando — decisão não bloqueia enquanto houver item 1.
- Item 2 é do usuário, e é o único caminho para o que teste nenhum decide.
- Ao terminar qualquer feature: **quem chama isto no jogo?** e a varredura de
  classe sem uso em produção.

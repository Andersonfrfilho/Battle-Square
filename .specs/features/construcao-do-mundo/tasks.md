# Construção do mundo — Tarefas

**Spec:** `.specs/features/construcao-do-mundo/spec.md`
**Status:** Draft — aguarda aprovação
**Escopo:** módulo `BattleSquare`. O traçado (`FreshWater`, `TrailLayout`,
`LandUseLayout`, `IslandGeography`) **não é alterado** por nenhuma tarefa aqui.

**Gabarito de aceite de toda fase:** `docs/mundo/carta-ilha-de-mata.html`.
O mundo construído tem de bater com a carta — mesma quantidade, mesmos lugares.

---

## A regra que reprova qualquer tarefa desta feature

Toda tarefa que cria ator entrega **três coisas**, e sem as três não está
pronta:

1. malha e cor **atribuídas no construtor** — nunca esperando edição de asset;
2. um teste que verifica a **ATRIBUIÇÃO**, não a existência do componente;
3. uma linha no painel (`FBattleDebugScreen::Show`) dizendo o que construiu.

Isto já falhou três vezes neste projeto (pets, inimigos, o próprio jogador).

---

## Plano de Execução

### Fase 0 — Assar o traçado
> 🤖 Modelo: `sonnet` — **T2 é 🧠** (o formato do assado é decisão estrutural)

```
T1 → T2 → T3 → T4
```

**T1.** Ferramenta de editor (`commandlet` ou teste de automação) que executa
todos os planos e grava o assado. Reaproveita `IslandMapDumpTest`, que já
percorre tudo.
*Aceite:* roda por comando único e escreve o arquivo. Verificação: o arquivo
existe e tem as sete seções.

**T2.** 🧠 O formato do assado: `UDataAsset` com o traçado inteiro. Decisão
estrutural — é o contrato entre o gerador e o mundo, e mudá-lo depois custa
caro.
*Aceite:* carrega em menos de 100 ms; teste que compara campo a campo com o
que os planos devolvem.

**T3.** O hash dos parâmetros dentro do assado (`WorldBudget`, raio da ilha,
forma da costa, semente).
*Aceite:* teste que muda um parâmetro e vê o hash mudar.

**T4.** A guarda na carga: hash diferente **falha alto**, nomeando o parâmetro.
*Aceite:* teste com assado velho; a mensagem diz qual parâmetro divergiu.
*Motivo:* aviso silencioso aqui é pior que nenhum — o mundo passa a ser de uma
configuração que não existe mais e nada quebra.

---

### Fase 1 — O relevo (o chão que tudo mais pisa)
> 🤖 Modelo: `sonnet` — **T5 é 🧠**

```
T5 → T6 → T7
```

**T5.** 🧠 `ATerrainMesh`: malha procedural a partir da grade de alturas do
assado (180×180 hoje).
*Aceite:* o ponto mais alto e o mais baixo batem com `GroundHeightAt` dentro da
tolerância da grade; a malha tem material e cor atribuídos no construtor.

**T6.** O jogador anda sobre ele — colisão e a câmera acompanhando a subida.
*Aceite:* teste que caminha do mar ao barranco e mede a altura a cada passo;
nenhum degrau maior que a casa da grade.

**T7.** A cor conta o terreno: praia, mata, rocha queimada, barranco, cume.
*Aceite:* teste de atribuição por faixa; painel mostra em que terreno o jogador
está.

---

### Fase 2 — A água
> 🤖 Modelo: `sonnet`

```
T8 → T9 → T10 → T11
```

**T8.** `ARiverMesh` a partir de `FreshWater::Plan()` — largura por progresso,
seguindo a polilinha.
*Aceite:* 137 cursos no mundo; largura em cada ponto bate com
`HalfWidthAtProgress`.

**T9.** Lagos, quedas e poços — os pontos que a carta já marca.
*Aceite:* contagem bate com a carta; a queda fica no degrau do terreno.

**T10.** Córregos e fontes (5 + 5).

**T11.** A água **molha**: entrar no rio muda o movimento, e o fundo importa.
*Aceite:* teste que atravessa um vau e um trecho fundo, com resultados
diferentes.

---

### Fase 3 — Trilhas e travessias
> 🤖 Modelo: `sonnet` — **T13 é 🧠**

```
T12 → T13 → T14
```

**T12.** `ATrailMesh` — as 23 trilhas assentadas no relevo, acompanhando a
altura.
*Aceite:* nenhum trecho flutuando nem enterrado; declive medido ≤ 10%, que é o
que o traçado promete.

**T13.** 🧠 As 56 travessias, e cada tipo se comporta diferente: vau se
atravessa a pé, ponte é geometria, barranco exige subida, balsa é interação.
*Aceite:* um teste por tipo.

**T14.** Os 2 aquedutos.

---

### Fase 4 — O uso do solo (o motivo de andar)
> 🤖 Modelo: `sonnet` — **T15 é 🧠**

```
T15 → T16 → T17 → T18
```

**T15.** 🧠 `AGroundUseActor`: um ator que lê `FGroundUsePatch` e se constrói
conforme o uso. Quinze usos, um ator parametrizado — **não quinze classes**.
*Aceite:* os 71 lugares aparecem; cada uso tem forma e cor próprias, atribuídas
no construtor.

**T16.** Fazenda, criadouro, pomar, loja, acampamento, deck, poço.

**T17.** Templo e ruína, com o deus visível no painel ao chegar perto.
*Aceite:* os 5 templos e 4 ruínas; o painel diz de que deus é.

**T18.** Cemitério e cemitério esquecido.

---

### Fase 5 — Conferência contra a carta
> 🤖 Modelo: `sonnet`

```
T19 → T20
```

**T19.** Teste que compara o mundo construído com o assado, elemento a
elemento, e falha nomeando o que faltou.
*Motivo:* é o que impede a construção de silenciosamente perder peças — o modo
de falhar desta feature inteira.

**T20.** Percorrer a ilha em PIE e conferir contra a carta. O que a carta
mostra e o mundo não tem vira tarefa, não observação.

---

## Verificação obrigatória de toda task

```bash
./Tools/build_editor.sh
./Tools/audit_determinism.sh && ./Tools/audit_no_recalculation.sh
./Tools/audit_localizable_text.sh && ./Tools/audit_test_helper_names.sh
./Tools/sync_module_manifest.sh   # DEPOIS do build
```

Bateria completa verde antes de fechar qualquer fase — hoje, **679/679**.

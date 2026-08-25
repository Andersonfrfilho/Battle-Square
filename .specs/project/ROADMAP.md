# Roadmap

**Marco atual:** M1 — Fatia Vertical do Combate
**Status:** Planning

---

## M1 — Fatia Vertical do Combate

**Objetivo:** um combate 1v1 offline, jogável do início ao fim, com as três ações e a resolução simultânea funcionando. É o que prova que o jogo é divertido antes de qualquer investimento em rede, conteúdo ou mundo.

**Critério de conclusão:** dá para jogar uma partida completa contra uma IA burra e sentir a tensão do commit às cegas.

### Features

**Núcleo de Combate** — PLANNED
- Estado de batalha, grade 3x3, ocupação de casas
- Fila de 3 ações por pet, commit e revelação
- Resolução simultânea por slot, em fases
- Movimento, ataque, magia, defesa, esquiva
- Condição de vitória, derrota e empate

**Apresentação do Combate** — PLANNED
- Consumo do trace de eventos para animar o resultado
- Câmera de arena, seleção de ações, indicação de limite (3/3)
- Feedback de vida, dano e ação inválida

**Dados de Pet** — PLANNED
- DataAsset de pet e de skill
- 6 a 10 pets de teste carregados por dados

---

## M2 — Combate Online

**Objetivo:** o mesmo combate, autoritativo no servidor, entre dois jogadores reais.

### Features

**Servidor Autoritativo** — PLANNED
- Simulação rodando no dedicated server
- Cliente envia apenas o commit das 3 ações; servidor devolve o trace
- Timeout de commit e preenchimento automático
- Reconexão com replay do trace

**Sala e Pareamento Simples** — PLANNED
- Código de sala, lobby de dois jogadores
- Tratamento de abandono

---

## M3 — Conteúdo e Balanceamento

**Objetivo:** provar que o sistema aguenta escala de conteúdo sem código novo por pet.

### Features

**Escala de Pets e Skills** — PLANNED
- Tipos, fraquezas e resistências por dados
- Ferramenta de balanceamento e simulação em lote (rodar N combates headless)

**Arenas Variadas** — PLANNED
- Casas com propriedades (bloqueio, dano, buff)

---

## M4 — Progressão e Meta

**Objetivo:** razão para voltar amanhã.

### Features

**Coleção e Captura** — PLANNED
**Níveis, Experiência e Evolução** — PLANNED

---

## M5 — Mundo Aberto Contínuo

**Objetivo:** o salto de escopo. Mundo contínuo com World Partition, exploração, encontros e batalhas instanciadas a partir do mundo.

**Pré-requisito inegociável:** M1 a M3 concluídos e o combate validado como divertido. Começar o mundo antes disso é o cenário de não entregar nenhum dos dois.

### Features

**Streaming de Mundo** — PLANNED
**Encontros e Transição para Batalha** — PLANNED
**Traversal e Câmera de Mundo** — PLANNED

---

## M6 — Plataformas

**Objetivo:** sair do PC.

### Features

**Mobile** — PLANNED (exige orçamento de performance definido desde M5)
**Console** — PLANNED (exige licença de desenvolvedor e devkit)

---

## Considerações Futuras

- Pixel Streaming como demo jogável por link (não como canal de distribuição)
- Modo de batalha avulso em navegador, se a batalha for separável do mundo
- Espectador e replay a partir do trace de eventos — o formato já permite de graça

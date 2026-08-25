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

**Backend de Dados de Pet** — PLANNED (spec escrita, `.specs/features/backend-dados-pet/`)
- Backend próprio (Bun + PostgreSQL + Drizzle), API REST `/v1/pets`, CRUD
- Espelho local sincronizado no servidor de combate — batalha nunca lê a rede diretamente (AD-014)
- 6 a 10 pets de teste cadastrados via API

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

## M7 — Contas e Moderação

**Objetivo:** identidade de jogador persistente entre partidas, pré-requisito para qualquer consequência que precise sobreviver ao fim de uma sessão — banimento, histórico, reputação, progressão de conta.

**Dependência registrada:** AD-017 (backend de dados de pet) já prevê a trilha de auditoria de adulteração de cache no Nível 1 (sessão); banimento persistente (Nível 2) só é implementável quando este marco existir. Não é bloqueante para nenhum marco anterior — é consumidor do que eles já produzem.

### Features

**Conta de Jogador** — PLANNED
**Moderação e Banimento** — PLANNED (consome a trilha de auditoria do backend de dados de pet)

---

## Considerações Futuras

- Pixel Streaming como demo jogável por link (não como canal de distribuição)
- Modo de batalha avulso em navegador, se a batalha for separável do mundo
- Espectador e replay a partir do trace de eventos — o formato já permite de graça

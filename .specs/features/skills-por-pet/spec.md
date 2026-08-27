# Skills por Pet

**Status:** Aprovado para execução (2026-08-27) — correção de rumo do usuário.

## O problema

Camuflar, voar e submergir entraram como ações **universais**: qualquer pet
pode escolher qualquer uma. O usuário corrigiu — elas são **skills**, e só
alguns pets as têm.

Segui o padrão que existia: os seis tipos de ação sempre foram universais, e a
spec de M3 **adiou explicitamente** "skills customizadas por pet". Não havia
modelo de skill no projeto, então três ações novas viraram três ações
universais sem que nada acusasse a diferença.

## Por que isso importa além do rótulo

Skill universal **anula a identidade do pet**. Se todo pet voa, voar não é
característica de ninguém — é só mais um botão. E a escolha do pet, que é o
coração de um jogo de coleção, deixa de ter consequência tática.

## A restrição que decide o desenho

Os registros de pet vêm de um **espelho SQLite assinado** do backend
(`FLoadedPetRecord`, PETDB-10). Acrescentar um campo `skills` ali exige mudar o
backend, o esquema, e o **payload da assinatura** — cadeia longa, e nada disso
é necessário para o jogo já respeitar skills.

## Decisão

**DP-skill-01 — Skills vêm do TIPO, num catálogo local.** Um arquivo de dados
mapeia tipo → skills disponíveis, no mesmo padrão de `ArenaLayoutCatalog` e
`TypeEffectivenessTable`, que já vivem assim. Nenhuma mudança no espelho
assinado, e a regra existe hoje.

Quando o backend ganhar skills por registro, elas passam a **sobrepor** o
catálogo por tipo — o catálogo vira o padrão, não a única fonte.

**DP-skill-02 — Quem recusa é o componente de fila, não a tela.** A tela
esconde o que o pet não tem, mas a recusa mora no `UBattleActionQueueComponent`
(DP-ui-01). Tela que só esconde é tela que um cliente adulterado ignora.

**DP-skill-03 — Os seis tipos originais continuam universais.** Aguardar,
mover, atacar, magia, defender e esquivar são a gramática do combate; skills
são o que distingue um pet de outro. Tornar os seis condicionais mudaria o jogo
inteiro, não só acrescentaria identidade.

**DP-skill-04 — Pet sem entrada no catálogo tem só os seis.** Ausência degrada
para o comportamento de antes desta feature, nunca para crash nem para "todas
as skills".

## Fora de escopo

- Skills por registro individual (depende do backend).
- Skills novas além das três que já existem.
- Aprender skill subindo de nível.

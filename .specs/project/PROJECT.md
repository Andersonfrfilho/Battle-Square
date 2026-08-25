# Battle-Square

**Visão:** Jogo de batalha de pets onde dois jogadores programam três ações em segredo e as assistem resolverem simultaneamente numa arena 3x3, evoluindo depois para um mundo aberto contínuo de exploração e captura.

**Para:** jogadores de battlers táticos e colecionadores de criaturas que acham combate por turnos alternados lento e previsível.

**Resolve:** no turno alternado clássico, você reage ao que já aconteceu. Aqui os dois lados comprometem suas ações às cegas e as veem colidir — a habilidade é ler o oponente, não decorar a ordem de iniciativa.

## Objetivos

- **Combate completo em menos de 90 segundos**, do commit à tela de resultado.
- **Determinismo verificável:** mesma seed e mesmas ações produzem o mesmo resultado, bit a bit, no cliente e no servidor. Divergência é bug bloqueante, não tolerância.
- **250 pets definidos por dados**, com zero linha de código por pet.
- **Direção de arte produzível por uma pessoa:** low-poly fosco, sem PBR complexo, sem texturas de alta resolução.

## Stack

**Núcleo:**
- Motor: Unreal Engine 5
- Linguagem: C++ para simulação, dados e rede; Blueprint para gameplay de apresentação e UI
- Rede: Dedicated Server da Unreal, autoritativo
- Dados: **DataTable** (com CSV como fonte da verdade) para os 250 pets; **DataAsset** para skills; **Gameplay Tags** para classificação, no lugar de enums

**Decisão de fronteira:** a simulação de combate é C++ puro, sem dependência de `UWorld`, `Actor` ou tick do motor. Ela recebe estado + ações e devolve estado + trace de eventos. Isso a torna testável fora do editor e reutilizável no servidor dedicado.

**Camada de dados** (ver AD-008 em STATE.md):

| O quê | Onde | Por quê |
|---|---|---|
| 250 pets | `UDataTable` com struct `FTableRowBase` | Conjunto grande e homogêneo; importa de CSV, então o balanceamento se edita em planilha |
| Skills | `UDataAsset` | Poucas, variadas, com herança; cada uma é um `.uasset` próprio, diffável no git |
| Tipos, fraquezas, estados | `FGameplayTag` / `FGameplayTagContainer` — **na camada `BattleSquare`, nunca no núcleo** | Hierárquico e criável por designer; substitui enum. O módulo `GameplayTags` arrasta `Engine` por dependência pública, então o núcleo usa inteiros próprios e traduz na fronteira (AD-012) |

**O CSV é a fonte da verdade, não o `.uasset`.** `UDataTable` é binário e não dá para revisar em pull request — com CSV versionado e reimportado, o balanceamento de 250 pets vira diff legível.

## Escopo

**v1 (fatia vertical) inclui:**
- Combate 1v1 online, um pet por lado
- Três ações por pet, commit às cegas e revelação simultânea
- Arena 3x3 única
- 6 a 10 pets de teste, já vindos de dados
- Servidor autoritativo com trace de eventos e reconexão

**Explicitamente fora de escopo na v1:**
- Mundo aberto (é o marco 5, e é outro jogo em custo)
- Os 250 pets — o sistema tem que aguentar 250; o conteúdo não é v1
- Mobile, console e Pixel Streaming
- Progressão, coleção, itens, guildas, ranqueada
- Matchmaking real (v1 usa código de sala)

## Restrições

- **Equipe:** pequena, possivelmente solo, sem estúdio de arte dedicado.
- **Técnica:** o desenvolvedor vem de TypeScript e está aprendendo C++/Blueprint. A divisão de responsabilidade (C++ para dados e simulação, Blueprint para apresentação) existe também para amortecer essa curva.
- **Escopo:** mundo aberto contínuo é o maior risco do projeto e é a razão da escolha do motor. Ele não pode começar antes do combate estar provado e divertido.

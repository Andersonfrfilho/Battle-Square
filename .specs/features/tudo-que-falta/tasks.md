# Tudo o que falta — tarefas

**Este arquivo NÃO repete as tarefas que já têm dono.** Duplicar uma lista é
duplicar uma verdade, e cópias concordam até a primeira edição (invariante 4).

O que já tem plano vive onde sempre viveu, e é lá que se executa:

- `.specs/features/corrente/tasks.md` — **C4, C5, C6**
- `.specs/features/pendencias/tasks.md` — **P4 a P10**

Abaixo só o que **não existe em lugar nenhum**: dois sistemas que várias
tarefas já pediram e que nunca foram abertos.

---

## T-ITENS — O sistema de itens 🧠
> 🤖 Modelo: `opus` — é cadastro, save e equipar; três fronteiras de uma vez

**Medido, e dito ao usuário duas vezes antes:** não há cadastro de item, não há
save, não há equipar. `FLoadedPetRecord` não tem o campo, e o save da coleção
também não.

E **a metade de dentro já está pronta há tempo**: `FluidResistPercent` no pet,
e `ComposeFluidResist(DoTraco, DoItem)` com prova por valor injetado — escrita
exatamente para que este dia fosse uma linha, e não uma descoberta.

*Aceite:* um pet com uma bota de lava equipada atravessa a lava sem se queimar;
o mesmo pet sem ela, não. E o item aparece na tela — item que ninguém vê é item
que ninguém equipa.

*Decisões que são do usuário, e por isso se PERGUNTA:* quantos slots, se o item
é por pet ou por coleção, e se ele se perde.

## T-ANATOMIA — O eixo da anatomia 🧠
> 🤖 Modelo: `opus` — mexe no cadastro assinado

> *"depende da anatomia dele e da biologia da pele dele"*

Hoje quem responde por isso é o **elemento**, porque é o que a criatura tem. O
usuário pediu algo mais fino, e o cadastro do pet não tem esse eixo.

**A metade de dentro também já está pronta:** `FootingPerMille` e
`FluidResistPercent` recebem números, e o comentário em
`FPetElementDefinition::FootingPerMille` marca por onde a anatomia entra.

*Cuidado que esta tarefa carrega:* o dado do pet é **assinado**, e nomes antigos
continuam valendo porque recusá-los invalidaria pets que existem. Um campo novo
não pode quebrar isso.

*Aceite:* dois pets do MESMO elemento com anatomias diferentes resistem
diferente. Sem isso, a anatomia é o elemento com outro nome.

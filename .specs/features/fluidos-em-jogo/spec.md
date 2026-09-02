# Fluidos em jogo — do núcleo até a partida

**Escrito em 02/09/2026**, ao fechar o objetivo dos fluidos (F1–F8).

## A medição que originou esta spec

O eixo da substância está completo e testado: registro, casa, poderes,
resistência, dano, mundo, empuxo, condução. **111 testes verdes.**

E então:

```
$ grep -rn "SetFluidAt|MoveConducts|SetResistPercentFor|RequireFluidForSkill" \
    Source --include="*.cpp" | grep -v "/Tests/"
(vazio)
```

**Ninguém, fora dos testes, põe um fluido numa casa.** Toda casa de batalha cai
no padrão da fundura — água doce. Numa partida de verdade: nunca há lava,
ninguém se queima, nada conduz, nenhum item resiste a nada.

É **L-041**, a mesma coisa que abriu a construção do mundo: *não faltam
elementos de design, falta encarnação.*

## O que falta, e o que já existe para ligar

Nada aqui é mecânica nova. São **duas pontas construídas que não se tocam**:

| ponta A (existe) | ponta B (existe) | falta |
|---|---|---|
| o mundo sabe de que fluido é cada ponto (F6) | a arena decide `ECellProperty` casa a casa | passar o fluido junto |
| o pet tem elemento Raio | o golpe tem `MoveConducts` | o tradutor ligar a bandeira |
| `FluidResistPercent` no pet | — | **de onde vem a proteção?** |

A terceira linha é a única que **não** é encanamento: ela pede uma decisão de
conteúdo, e ela é do usuário.

## A restrição que continua valendo

O núcleo **não sabe que tipo existe**. Quem sabe é o tradutor, e é ele que
converte "este pet é Raio" em "este golpe conduz" — do mesmo jeito que o Attack
já chega pré-multiplicado pela efetividade. Nada nesta spec muda isso.

## O que esta spec NÃO faz

- **Não inventa a origem da resistência.** Item equipado não existe como
  sistema; traço de espécie existe. Escolher sozinho criaria um sistema que o
  usuário teria de desfazer.
- **Não muda o traçado da ilha** nem o registro dos fluidos.
- **Não autora asset.** A casa de lava veste o material da água até alguém
  desenhar um; o nome na etiqueta é o remendo declarado.

# Roteiro de verificação — Skills por Pet

**Status:** **não verificado ainda.**

Os testes provam que o catálogo carrega, que a fila recusa e que a IA respeita.
O que eles não decidem: **a diferença entre pets se SENTE jogando?**

## Skills de hoje (`Config/PetSkills.json`)

| Tipo | Skill própria |
|---|---|
| Fogo | voar |
| Agua | submergir |
| Planta | camuflar |

Uma por tipo, de propósito: identidade só existe quando há diferença, e duas
por tipo já começam a diluir isso. Os seis tipos de ação universais
(aguardar, mover, atacar, magia, defender, esquivar) são de todo pet.

## SKILL-01 — O pet mostra o que ele é

- [ ] O painel diz, ao começar: `<pet> (tipo X) tem N skill(s) própria(s)`.
- [ ] Na barra do jogador 2, **só aparecem** as skills daquele pet.
- [ ] Um pet de tipo diferente oferece skill diferente.

## SKILL-02 — A recusa é de regra, não de tela

- [ ] Com o catálogo apagado, o painel avisa e todo pet fica com os seis
      universais — nada de crash, nada de "todas as skills".

## SKILL-03 — O oponente também respeita

- [ ] O bot nunca voa com um pet que não voa. (No feed: nenhuma linha de
      "alçou voo" para um pet de tipo que não tem voo.)

## SKILL-04 — Julgamento

- [ ] Enfrentar um pet Água (submerge) é **diferente** de enfrentar um Planta
      (camufla)? Se a sensação for a mesma, as trocas ainda estão indistintas.
- [ ] Uma skill por tipo é pouco, ou é o suficiente para dar identidade?

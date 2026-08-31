# Cidades do interior

**Escrito em 31/08/2026**, antes de desenhar qualquer coisa.

## A regra que ordena tudo

**Cada lugar da cidade precisa servir a um laço que o jogo JÁ TEM.** Casa que
não serve a nada é cenário com porta — e este projeto já pagou caro por
recursos completos que ninguém alcançava.

Por isso a lista abaixo não é "o que uma cidade de RPG costuma ter". É o que
ESTE jogo está precisando, e que hoje não tem lugar nenhum.

## O que a cidade resolve

| Problema de hoje | Onde ele dói |
|---|---|
| Não existe como curar entre batalhas | Perder vida é permanente até o próximo nível |
| `bs.Especializar` é comando de CONSOLE | A especialidade do treinador é invisível para quem joga |
| A coleção vive no save e não tem tela | Capturar um pet não muda nada visível |
| Tudo fica a 100–190 m e nada diz onde | O jogador anda ao acaso; a cachoeira nunca foi vista |
| Voltar ao centro é caminhada longa | O mapa tem 200 m de raio |

## O que a cidade TEM

Em ordem de valor, e cada linha diz se é regra nova ou só falta lugar.

1. **Casa de repouso** — restaura a vida do pet. Regra nova e pequena, e a
   única coisa que transforma "perdi vida" em decisão em vez de dano
   permanente.
2. **Casa do treinador** — tira `bs.Especializar` do console. A regra existe,
   está testada, e hoje nenhum jogador a alcança: falta o LUGAR.
3. **Abrigo dos pets** — ver a coleção e trocar o pet ativo. O dado existe no
   save desde M4; falta a tela.
4. **Poste de anúncios** — diz onde ficam a cachoeira, os campos de treino e a
   montanha, em direção e distância. Novo, barato, e é o que responde ao "não
   vi" da primeira sessão jogada.
5. **Marco de retorno** — ponto de viagem rápida, e marca no mapa. Sem ele, uma
   ilha de 200 m de raio é caminhada de ida e volta.
6. **Arena da vila** — um oponente mais forte, com motivo para voltar. Reusa a
   arena inteira; não é sistema novo.

## O que NÃO entra agora

Economia, lojas, diálogo ramificado e NPC com rotina. **O jogo não tem nenhum
desses sistemas**, e cada um é uma feature inteira. Uma loja sem economia é uma
casa com balcão.

## Tamanho e forma

O bloco já existe: `RegionResidency`, **6400 unidades = 64 metros de lado**, e
só os nove à volta do jogador vivem.

- A cidade ocupa uma FRAÇÃO do bloco 0,0 — cerca de 20 m — e o resto é mata.
  Cidade que enche o bloco não deixa lugar para a floresta que vem depois.
- Ela precisa se ler **de fora**: quem chega pela mata vê telhado antes de ver
  porta.
- **Uma trilha entra e outra sai.** É o que liga a cidade à floresta em vez de
  a deixar como ilha dentro da ilha.
- O ponto de partida do jogador passa a ser a cidade, não o centro vazio.

## Ordem de construção

1. Bloco 0,0 com a cidade e a mata em volta.
2. Casa de repouso e casa do treinador — as duas que tiram regra do console.
3. Trilhas ligando à floresta.
4. Bloco vizinho, e assim por diante.

## Perguntas em aberto

- **Curar é de graça?** De graça tira o peso de perder vida; cobrar exige
  economia, que não existe.
- **A cidade tem nome, ou é "a vila"?** Nome pede localização de texto.
- **O marco de retorno é imediato ou tem custo?** Viagem rápida de graça pode
  esvaziar a caminhada, que é metade do jogo de mundo aberto.

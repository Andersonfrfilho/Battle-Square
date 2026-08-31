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

## O que a cidade TEM — o que cabe hoje

Cada linha diz se é regra nova ou se é regra EXISTENTE sem lugar.

1. **Centro de Recuperação** — cura o pet, e é onde vive o **abrigo**: ver a
   coleção e trocar o pet ativo. Junta duas coisas que o jogador faz na mesma
   visita, e evita duas portas para um gesto só. A coleção existe no save desde
   M4; falta a tela.
2. **Escola do treinador** — tira `bs.Especializar` do console. A regra existe,
   está testada, e hoje nenhum jogador a alcança: falta o LUGAR.
3. **Academias** — o par URBANO dos campos de treino. E a ideia que mais vale
   aqui é a restrição: **cada cidade ensina só alguns atributos**. É isso que
   faz uma cidade ser diferente da outra, e é o que dá DESTINO ao mapa — hoje
   as cinco clareiras estão todas a 18 metros do centro, e por isso nenhuma é
   uma viagem.
4. **Arena da vila** — um oponente mais forte, com motivo para voltar. Reusa a
   arena inteira; não é sistema novo.
5. **Praça e poste de anúncios** — diz onde ficam a cachoeira, os campos e a
   montanha, em direção e distância. Novo, barato, e é o que responde ao "não
   vi" da primeira sessão jogada.
6. **Marco de retorno** — viagem rápida e marca no mapa. Sem ele, uma ilha de
   200 m de raio é caminhada de ida e volta.
7. **Casas** — cenário, e elas ganham o lugar por uma razão só: a cidade
   precisa se LER de fora. Quem chega pela mata vê telhado antes de porta. Casa
   sem função é aceitável enquanto for silhueta; casa com porta que não abre é
   promessa quebrada, e a porta não deve existir.

## O que exige SISTEMA NOVO — e qual

Nada disto é caro por implementação; é caro por ser conceito que o jogo não
tem. Cada um precisa de decisão antes de código.

**Curar o TREINADOR** exige estado de treinador. Hoje ele tem só
`Specialties` — nenhuma vida, nenhum cansaço. A pergunta que decide: **o que
machuca o treinador?** Sem uma resposta, curá-lo é curar um número que nunca
desce.

**Lojas e compra/venda de pets** exigem ECONOMIA. Não há moeda alguma no
projeto. Uma loja sem economia é uma casa com balcão. E economia é mais que
moeda: precisa de FONTE (de onde vem dinheiro) e de DRENO (no que ele se
gasta), senão inflaciona e os preços deixam de significar.

**Comprar pet colide com CAPTURAR pet**, e a colisão é de desenho, não de
código. Hoje o pet vem de vencer e capturar — é a recompensa do laço inteiro.
Se dá para comprar, ou a captura vira o caminho lento (e ninguém captura), ou a
compra vira decoração cara (e ninguém compra). Vender também tem armadilha:
vender pets capturados transforma o mundo em fazenda de farm.

Uma saída possível, para discutir: o comprador/vendedor **não vende pets
melhores** — ele troca. Você entrega um que tem, recebe um que não tem, e o
valor é a raridade e não a força. Isso mantém a captura como única fonte e dá
sentido a ter repetidos.

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

- **O que machuca o treinador?** Sem isso, o Centro de Recuperação cura metade.
- **De onde vem dinheiro, e no que ele se gasta?** Sem fonte e dreno, não há
  economia — há números.
- **Comprar pet e capturar pet podem coexistir?** Ver a proposta da troca.
- **Quais atributos cada cidade ensina?** É o que faz viajar valer.

- **Curar é de graça?** De graça tira o peso de perder vida; cobrar exige
  economia, que não existe.
- **A cidade tem nome, ou é "a vila"?** Nome pede localização de texto.
- **O marco de retorno é imediato ou tem custo?** Viagem rápida de graça pode
  esvaziar a caminhada, que é metade do jogo de mundo aberto.

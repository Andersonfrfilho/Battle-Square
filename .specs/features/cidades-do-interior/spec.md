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


## A economia: de onde vem, e para onde vai

Decidido em 31/08/2026. **Fonte e dreno nascem no MESMO laço**, e isso não é
elegância — é o que impede a moeda de virar número que só sobe. Economia sem
ralo faz preço nenhum significar coisa alguma.

**vencer na cidade → prêmio e reputação → pagar a academia → mais forte →
vencer onde é mais difícil**

### Fonte: ser o melhor treinador da cidade

A melhor das fontes possíveis porque **não precisa de conteúdo novo**: vitória,
XP, atributos e especialidade já existem. Um ranking por cidade é "vença aqui",
e é o que dá IDENTIDADE a cada cidade — não só prédios diferentes.

E ele resolve sozinho o problema de moer dinheiro. Prêmio por vitória contra
selvagens que reaparecem é impressora de moeda: dá para lutar sem fim, e aí o
preço da academia deixa de doer. **O ranking não é infinito** — sobe-se até o
primeiro lugar, e aquela cidade para de pagar. O jogador é empurrado para a
próxima, que é para onde queremos que ele vá.

### Dreno: a academia cobra

É o que dá preço ao treino rápido, e é por isso que a academia não compete com
os campos: **campo é lento, gratuito e universal; academia é rápida, paga e
restrita** a alguns atributos por cidade.

### Troco: vender o pet capturado

Fica, apertado, e a armadilha é conhecida — se a venda for a renda principal, o
jogo vira fazenda: captura-se para vender, e o pet deixa de ser companheiro
para virar mercadoria. Três amarras:

- O preço sai da **raridade**, não da força — vender o forte não compensa.
- **Preço baixo.** É troco, não salário.
- Alternativa a considerar: entregar ao abrigo rende **reputação** em vez de
  moeda. Aí o repetido tem destino sem virar dinheiro.

### Trabalhos, e os PODERES DO PET neles

O trabalho é a fonte mais cara: cada tipo é conteúdo novo, e conteúdo que não é
batalha é quase outro jogo dentro deste. Começar por UM, ligado ao que existe.

O que o torna valioso aqui é a ideia do usuário: **os poderes do pet ajudam no
trabalho.** Hoje `voar`, `submergir`, `camuflar`, `atravessar` e `iluminar` só
servem dentro do combate — são metade da identidade de um pet, usada em um
terço do jogo.

**A amarra que recomendo, e ela é a diferença entre bom e insuportável: o poder
FACILITA, nunca HABILITA.** Um trabalho que EXIGE submergir tranca o jogador
que não tem pet de água, e aí o pet deixa de ser escolha e vira chave de porta
— o defeito clássico. Com o poder facilitando, quem tem o pet certo faz mais
rápido ou ganha mais; quem não tem, faz assim mesmo.

**Um buraco que isto expõe:** o elemento TERRA não tem skill nenhuma. Fogo voa,
água submerge, planta camufla, fantasma atravessa, luz ilumina — e terra ficou
sem. Num sistema de trabalhos, o pet de terra não contribui com nada, e o
elemento vira o pior de todos por omissão.

## Perguntas em aberto

- **O que machuca o treinador?** Sem isso, o Centro de Recuperação cura metade.
- ~~De onde vem dinheiro?~~ **Decidido:** ranking da cidade é a fonte, academia
  é o dreno, venda é troco. Ver a seção da economia.
- **Que skill o elemento TERRA ganha?** Sem uma, o pet de terra não serve a
  nenhum trabalho.
- **Comprar pet e capturar pet podem coexistir?** Ver a proposta da troca.
- **Quais atributos cada cidade ensina?** É o que faz viajar valer.

- **Curar é de graça?** De graça tira o peso de perder vida; cobrar exige
  economia, que não existe.
- **A cidade tem nome, ou é "a vila"?** Nome pede localização de texto.
- **O marco de retorno é imediato ou tem custo?** Viagem rápida de graça pode
  esvaziar a caminhada, que é metade do jogo de mundo aberto.

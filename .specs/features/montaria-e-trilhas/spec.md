# Montaria e trilhas — o pet é o veículo

**Escrito em 31/08/2026.**

A pergunta era "vamos ter estradas ou trilhas? e veículos?". A resposta que
ordena tudo o resto é do usuário: **alguns pets se montam, e é neles que se
voa e se nada.**

## Por que o pet, e não um veículo

Três motivos, e nenhum deles é gosto:

1. **`voar` e `submergir` JÁ EXISTEM.** Hoje só servem dentro do combate — são
   metade da identidade de um pet, usada num terço do jogo. Montaria os põe no
   mundo sem inventar sistema nenhum.
2. **Veículo mecânico briga com o mundo.** Carroça ou moto numa ilha de pets é
   outro jogo dentro deste, com outra arte, outra física e outra economia.
3. **Reforça a coleção.** Cada pet novo pode mudar como o mapa se atravessa, e
   isso dá valor a capturar sem transformar pet em mercadoria.

## A amarra: FACILITA, nunca HABILITA

É a mesma regra já escrita para os trabalhos, e vale aqui inteira.

**Voar encurta a montanha; não é a única forma de passar.** Quem não voa dá a
volta — e chega. Se voar for o único caminho, o pet deixa de ser escolha e vira
chave de porta, e o elemento vira imposto sobre quem não o tem.

Isto tem consequência de traçado, e ela é obrigatória: **todo destino é
alcançável a pé.** Nenhum assentamento, campo de treino ou recurso pode ficar
atrás de uma parede que só a montaria vence.

## Montar CANSA o pet

**Decidido em 31/08/2026.** É o contrapeso, e sem ele ninguém anda: se voar for
sempre melhor que caminhar, a caminhada deixa de existir e o mapa vira menu de
destinos.

**O pet montado cansa, e cansado ele não luta bem.** Escolher a montaria é
abrir mão do lutador — e é essa troca que faz a decisão ter peso.

O que isso dá de bom, e não estava no pedido:

- **A coleção passa a ter função de EQUIPE**, não só de vitrine. Um pet que
  carrega e outro que luta é a primeira razão real para manter dois.
- **O cansaço liga a montaria ao Centro de Recuperação**, que já existe e já
  cura de graça em casa. A viagem longa tem preço, e o preço se paga no lugar
  que já foi construído para isso.
- **Casa com os pets que envelhecem** (`mundo-vivo`): cansaço é fadiga curta,
  idade é desgaste longo, e as duas se leem no mesmo lugar.

### O que cansa: distância, INCLINAÇÃO e PESO

**Decidido em 31/08/2026.** O cansaço não é uma régua sobre o mapa: subir custa,
e carregar custa.

| fator | efeito |
|---|---|
| **distância** | a base |
| **subida** | custa MUITO mais que o plano |
| **descida** | custa um pouco mais que o plano — nunca menos |
| **peso carregado** | multiplica tudo acima |

**A descida não é de graça, e essa é a decisão que faz a inclinação importar.**
Se descer devolvesse o que subir tirou, toda ida e volta se anularia e o relevo
deixaria de existir para quem viaja — o morro seria só um desenho. Descer custa
pouco, mas custa: é freio, não descanso. (E é verdade no corpo também: quem
desce a serra sente os joelhos, não o fôlego.)

Isso dá ao mapa uma coisa que ele não tinha: **rota**. Contornar o morro passa
a ser uma escolha legítima contra atravessá-lo, e a trilha desenhada vira o
conselho de quem já andou ali — ela vai pelo caminho barato.

### O peso: o que o jogador carrega

**Decidido em 31/08/2026.** Peso multiplica o cansaço, e é o que dá custo à
coleta de pedra, madeira e barro (`mundo-vivo`). Sem ele, coletar é lucro sem
contrapartida, e o mapa vira depósito.

E é aqui que a montaria encontra a função mais bonita dela: **o pet que carrega
é diferente do pet que luta.** O forte carrega mais e cansa menos com carga — e
aí manter dois pets deixa de ser preferência e vira estratégia.

**Duas amarras, e as duas contra o mesmo defeito: virar jogo de inventário.**

1. **Peso nunca BLOQUEIA — ele torna lento e cansa mais.** Limite rígido produz
   a mesma cena em todo jogo que o tem: a pessoa parada no chão decidindo o que
   largar. Isso não é decisão, é contabilidade.
2. **Peso pequeno não conta.** Só a partir de uma carga que se note. Senão cada
   pedra apanhada obriga a pensar, e apanhar uma pedra deve ser um gesto.

### O que isto obriga, e o mundo ainda não tem

**Não existe altura do chão.** A ilha é plana, e as montanhas são atores em
cima dela: nenhuma função responde "que altura tem aqui". Sem isso, "subida
cansa mais" não tem como ser calculado nem testado.

O caminho, e ele é o padrão que já funcionou três vezes neste projeto:
**`IslandGeography::GroundHeightAt(WorldXY)`, pura.** Uma fonte só, consultada
por quem CONSTRÓI o relevo, por quem traça a trilha e por quem cobra o cansaço.

É o que torna "a trilha evita a subida" e "subir a montanha cansa mais que
contorná-la" asserções de milissegundos, em vez de uma caminhada dentro do
jogo. E é o que impede a terceira cópia do relevo — este projeto já pagou por
duas fontes da mesma verdade em L-032, L-033 e num defeito de direção.

**Enquanto `GroundHeightAt` não existir, o cansaço é só distância e peso.** Não
é meia regra: é a regra inteira menos o fator que ainda não tem como ser
medido, e escrever inclinação sobre um mundo plano seria afirmar um número que
sempre daria zero.

### O que precisa ser decidido junto

- **O cansaço se recupera sozinho com o tempo?** Se não, toda viagem termina
  em caminhada de volta ao Centro, e isso é imposto. Recomendo: recupera
  devagar sozinho, e na hora no Centro.
- **Cansaço é o mesmo número que a vida?** Recomendo que NÃO. Vida perdida é
  consequência de luta; cansaço é consequência de distância. Misturá-los faria
  viajar machucar o pet, e ninguém montaria.
- **Quanto cansa?** Proporcional à DISTÂNCIA, não ao tempo montado. Ao tempo,
  ficar parado em cima do pet custaria — e o jogador aprenderia a desmontar
  para conversar, que é atrito sem sentido.

## Trilhas DESENHADAS, não estradas

**Decidido em 31/08/2026.**

**Trilha, não estrada.** Uma faixa sem árvore ligando os assentamentos, com o
chão de cor diferente. Estrada com asfalto e meio-fio implicaria uma civilização
que este mundo não tem.

E ela resolve um defeito relatado jogando: *"não vi esses campos de treino"*,
*"a cachoeira nunca foi vista"*. Hoje se anda ao acaso. **A trilha é a resposta
mais barata para "para onde eu vou", e é anterior a qualquer placa ou marcador
de mapa** — quem vê um caminho no chão segue o caminho.

### Desenhada, e por quê

A trilha é **traçada entre os assentamentos**, reta e previsível. A alternativa
— trilha que NASCE por onde os jogadores passam — é mais bonita, casa com Mãe
Natureza, e fica guardada como ideia. Não agora:

- ela exige gravar passagem por posição, que é estado compartilhado novo;
- e um mapa cujos caminhos dependem do que outros fizeram é impossível de
  verificar headless, que é onde este projeto consegue verificar de verdade.

### O que a trilha É, no código

**A trilha é a mesma pergunta que a clareira, com outra forma.**
`VillageLayout::BlocksPlanting` já responde "aqui não se planta" para as
clareiras; a trilha acrescenta os corredores entre elas. Isso a torna barata, e
mantém **uma fonte de verdade só** para onde a mata não entra — duas listas de
"onde não plantar" concordariam até a primeira edição (L-032).

Do mesmo jeito que o traçado da vila e o da região, ela é **pura**: ligar
pontos e medir distância a um segmento não precisa de `UWorld`, e por isso
"nenhuma trilha atravessa o vulcão" e "toda vila está ligada" se verificam
headless.

### O traçado

- **Estrela a partir da vila inicial**, não malha completa. Casa é o centro de
  tudo, e o primeiro caminho que se aprende é o de sair de casa e voltar.
- **Uma trilha entre a vila da academia e a vila do mercado**, porque a spec da
  região já diz que elas ficam "a quatro minutos de casa, **e uma da outra**".
  Sem ela, ir de uma à outra seria passar por casa — um desvio que a spec não
  quis.
- **Postos de fronteira ligados à cidade grande**, não à vila inicial: é a
  cidade que dá o ranking que abre a porta, e o caminho deve dizer isso.
- **Nenhuma trilha corta a rocha queimada do vulcão.** Mesmo motivo dos
  assentamentos, e há de haver teste.

### Largura

Fração do lote, nunca metros escritos à mão — o defeito que já custou os anéis
das peças da ilha quando o raio cresceu.

Larga o bastante para se ler como caminho de longe; estreita o bastante para a
mata continuar sendo mata. Trilha que vira avenida apaga a floresta que ela
deveria atravessar.

## O que fica de fora, e por quê

| ideia | por que não agora |
|---|---|
| Veículo mecânico | outro jogo dentro deste |
| Trilha que nasce do uso | estado compartilhado novo, e não se verifica headless |
| Estrada pavimentada | implica civilização que este mundo não tem |
| Viagem rápida por trilha | o marco de retorno já é isso, e dois sistemas de encurtar distância se anulam |
| Montaria em qualquer pet | o que faz a montaria valer é ela ser característica de ALGUNS |

## Perguntas em aberto

- **Quais pets se montam?** Recomendo: por TAMANHO, não por elemento — assim
  não há elemento privilegiado, e o pet grande ganha uma vantagem que a batalha
  não lhe dá.
- **Voar por cima é livre ou segue a trilha?** Se for livre, a trilha some para
  quem voa; se seguir, voar é só andar mais rápido. Recomendo livre, aceitando
  que a trilha é para quem anda — que é a maioria do tempo.
- **Nadar exige o quê?** `submergir` é golpe de combate e mergulha; atravessar
  água na superfície é outra coisa. Decidir se são o mesmo verbo.
- **O pet montado aparece na tela?** Se não aparecer, montar é só velocidade
  diferente — e a regra deste projeto é que o que se implementa precisa
  aparecer.

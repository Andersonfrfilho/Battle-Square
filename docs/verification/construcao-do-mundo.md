# Roteiro manual — Construção do mundo

**Escrito em 02/09/2026, ao fechar T1–T19.**
Gabarito: `docs/mundo/carta-ilha-de-mata.html`.

## O que já está provado sem você

Não repita à mão o que a bateria já cobra. **Todas as contagens da carta são
afirmadas em teste** (`BattleSquare.ChartConformance`): 9 bosques, 6 clareiras,
8 fazendas, 4 criadouros, 4 lojas, 6 acampamentos, 3 pomares, 7 decks, 5
templos, 4 ruínas, 7 cemitérios, 2 poços com água, 30 vaus, 25 balsas, 0
pontes, 56 travessias, 137 cursos, 5 córregos, 5 fontes, 23 trilhas, 158
galerias, 2 aquedutos.

E `BattleSquare.WorldMatchesBakedPlan` confere o mundo construído contra o
assado elemento a elemento, nomeando o que faltar.

**Contar coisa na tela é o trabalho que o teste já faz.** O que sobra para
você é o que teste nenhum vê.

## Antes de abrir

```bash
./Tools/bake_island.sh && ./Tools/build_editor.sh && ./Tools/sync_module_manifest.sh
```

Feche o Editor antes de compilar, e reabra com `open -a` — editor lançado em
segundo plano não recebe teclado no PIE.

## O que só o olho pega

Cada item é **uma pergunta com resposta sim ou não**. Um "não" vira TAREFA com
teste próprio, nunca observação solta — é o que a T20 pede.

| #   | Onde                               | A pergunta                                                                           |
| --- | ---------------------------------- | ------------------------------------------------------------------------------------ | ---------------------------------------------------------------------------------------------------------------- |
| 1   | qualquer lugar                     | O chão existe, e você anda sobre ele sem cair através?                               | sim                                                                                                              |
| 2   | andando do mar para dentro         | A cor MUDA ao trocar de terreno, e o painel (`terreno:`) concorda com o que você vê? | sim                                                                                                              |
| 3   | perto do vulcão                    | A rocha queimada é visivelmente outra coisa, e não mata escurecida?                  | sim                                                                                                              |
| 4   | num rio qualquer                   | A água está SOBRE o leito — sem piscar, sem faixa aparecendo e sumindo?              | sim                                                                                                              |
| 5   | seguindo um rio da cabeceira à foz | Ele ENGROSSA?                                                                        | sim                                                                                                              |
| 6   | numa cachoeira                     | Existe um poço embaixo dela, e ele está no degrau do terreno?                        | sim, poderia existir cavernas entrando para dentro de cachoeiras                                                 |
| 7   | entrando na água                   | O painel muda para `pisando: vau` ou `agua funda`, e o andar fica mais lento?        | sim, como se fosse lama ou caminhos muita neve                                                                   |
| 8   | num vau marcado                    | Dá para atravessar a pé sem nadar?                                                   | depende o nivel da agua se ela ultrapassar a cintura do jogador precisa nadar                                    |
| 9   | numa balsa                         | A plataforma está sobre a água, não afundada nem na altura de uma ponte?             | ela bate na ponte pois é um solido no jogo que o fluido é mais leve que a agua                                   |
| 10  | numa trilha                        | Ela acompanha o chão — sem flutuar, sem enterrar — e sobe em curva, não de frente?   | sim sim, trilha não flutua ela pode ser as vezes mais profunda que o nivel atual exemplo entrando em uma caverna |
| 11  | num aqueduto                       | Ele desce, e em nenhum ponto entra no morro?                                         | pode entrar no morro mas, precisa de sair por tuneis ou a estrutura da a volta                                   |
| 12  | chegando num templo                | O painel diz `templo de <deus>`, e o nome MUDA entre templos diferentes?             | sim sim pois vamos ter varias divindades                                                                         |
| 13  | saindo do templo                   | A linha do deus SOME (ela não pode ficar dizendo onde você já não está)              | não entendi                                                                                                      |
| 14  | num poço                           | Dá para ver de longe qual dá água e qual é seco?                                     | depende a fundura do poço                                                                                        |
| 15  | o mapa todo                        | Alguma coisa da carta que você não achou no mundo?                                   | pode ter coisas escondidas do mapa                                                                               |

## Se algo estiver errado

**Escreva o teste antes do conserto.** Consertar por hipótese custou três
rodadas em agosto, três nas galerias, e mais três nesta feature (a regra do
vau, o declive das trilhas, a proximidade da balsa) — em todas, a medição
apontou uma causa diferente da impressão.

O painel se grava sozinho em `Saved/BattleDebug.txt` a cada mudança: não é
preciso transcrever nada da tela.

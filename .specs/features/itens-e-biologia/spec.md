# Itens e biologia — o que são, e por que juntos

Duas features num arquivo porque elas se cruzam num ponto só, e o cruzamento é
o motivo de as duas existirem: **a bota de lava e a pele couraçada somam na
mesma conta.** Separá-las produziria duas somas, e duas somas concordam até a
primeira edição.

O ponto é `FBattleDataTranslator::ComposeFluidResist(traço, biologia, item)`.

---

## A BIOLOGIA — feito

Quatro eixos, em `Config/PetBiology.json`, lidos por `FPetBiologyCatalog`:

| eixo | responde por |
|---|---|
| **pele** | resistência por fluido |
| **porte** | firmeza contra a corrente |
| **respiração** | submergir sem se afogar |
| **apoio** | firmeza e passo na água |

**Eixos, não perfil nomeado.** Com perfil, cada criatura nova pede um perfil, e
"Réptil de deserto" e "Réptil de pântano" viram duas entradas que repetem tudo
menos uma linha. Com eixos, as combinações nascem do **cruzamento** — e é o
cruzamento que faz dois pets do mesmo elemento resistirem diferente, que é o
aceite.

### O cuidado que esta feature carrega: o dado é ASSINADO

O cadastro do pet vem assinado do backend, e nomes antigos continuam valendo
porque recusá-los invalidaria pets que existem. A biologia entra:

1. **numa coluna própria do espelho**, tolerada por ausência como `moves` já é;
2. **no FIM do payload canônico**, depois dos golpes;
3. **só quando o payload a trouxe** — `"biology":{}` acrescentado a um pet que
   não o tinha mudaria o payload dele e invalidaria a assinatura DELE.

A verificação tenta **três** formatos: com biologia, com golpes, e o original. A
do meio existe para o pet que ganhou biologia e ainda não foi reassinado — ele
continua jogando.

Isso **não enfraquece nada**: forjar assinatura exige a chave privada, e um
registro adulterado falha nos três.

---

## OS ITENS — a fazer

As decisões do usuário estão em `DECISOES.md`. O desenho que sai delas:

### Dois estados do mesmo item

| estado | onde | o que carrega |
|---|---|---|
| **na mochila** | do jogador | quantidade da PILHA |
| **equipado** | naquele pet | o slot que ocupa |

Equipar **tira** da mochila; desequipar **devolve**. Sem isso o mesmo item
existiria duas vezes, e a mesma bota vestiria cinco pets.

### Duas naturezas, e o cadastro diz qual

| natureza | ao usar |
|---|---|
| **equipamento** | nada — age enquanto vestido |
| **consumível** | a quantidade cai; zerou, some da mochila |

### Aceite

Um pet com bota de lava equipada atravessa a lava sem se queimar; **o mesmo
pet** sem ela, não. E o item aparece na tela — item que ninguém vê é item que
ninguém equipa.

### O que ainda vai ser perguntado

Que itens existem, quanto cada um dá, e se a mochila tem limite. Números de
balanceamento nascem **medidos contra o elenco**, e ficam marcados como decisão
do dono.

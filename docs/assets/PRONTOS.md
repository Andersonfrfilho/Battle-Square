# Coisas prontas — o que dá para não autorar

Pesquisa de 02/09/2026, companheira de `INVENTARIO.md`. O inventário mediu **o
que falta**; este arquivo pesquisa **o que já existe pronto** e sob que licença.

Uma frase de resumo: **dá, e a licença não é o problema. O construtor é.**

---

## 1. O bloqueio é o construtor, não a loja

Hoje a malha de cada ator está **escrita dentro do construtor C++**:

```cpp
static ConstructorHelpers::FObjectFinder<UStaticMesh> Esfera(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
```

Esse padrão aparece em ~17 atores (`PetView`, `PetOwnerView`,
`WorldExplorerCharacter`, `Village`, `GroundUseActor`, `FerryActor`,
`WorldEncounterActor`, `ForestBackdrop`, `MountainRange`, `WalkableMountain`,
`CaveSystem`, `Volcano`, `AuroraCurtain`, `WorldBoundaryWater`, `BattleArena`).
Consequência prática: **adotar qualquer pacote significa editar C++ ator por
ator**, e o defeito que este projeto já pagou **três vezes** é exatamente ator
com componente visual e **sem asset atribuído** — passa em todo teste de lógica
e não existe na tela.

Então a ordem é essa, e não a inversa:

1. **Primeiro o slot dirigido por dado.** Malha e material por papel, numa
   tabela ou `DataAsset`, do mesmo jeito que `ScenaryPalette` já faz com **cor**
   (28 valores de `EScenaryRole`, um lugar só). Sem isso, cada pacote novo é uma
   rodada de edição de construtor.
2. **Depois o pacote.** Com o slot no lugar, trocar de pacote é trocar dado.

E o aviso do inventário vale aqui em cheio: no momento em que entrar
malha/textura, `ScenaryPalette` **vira o ponto por onde o material é escolhido**
— ou nasce uma segunda fonte de verdade ao lado dela, que é L-032 pela quarta vez.

---

## 2. Fontes, por licença

### CC0 — sem custo, sem crédito, uso comercial liberado

| fonte | o que serve aqui |
|---|---|
| **Quaternius** | personagens, **animais e monstros**, natureza (150+ modelos). Vêm **riggados e animados** (Idle, Walk, Run, Jump, Death) em FBX/OBJ/Blend. É a fonte que mais casa com este projeto: `APetView` é o slot óbvio |
| **Kenney** | SFX CC0 game-ready (cliques, impactos). Fecha o maior zero do inventário pelo menor custo |
| **OpenGameArt / Freesound** | filtrando **CC0** na busca. Freesound é o maior acervo, licença varia por arquivo |
| **Poly Haven** | HDRI e textura CC0 — serve à iluminação (`ABattleSceneLighting`) |

CC0 dispensa crédito. **CC-BY exige linha de crédito e CC-BY-NC está fora** de
qualquer jogo vendido — filtrar é obrigatório, não recomendação.

### Fab (Epic) — gratuito rotativo, e o que se resgata FICA

A Epic torna conteúdo de publishers selecionados **gratuito a cada duas
semanas**, e **o que for resgatado dentro da janela permanece na biblioteca para
sempre**. A **Licença Padrão da Fab permite uso comercial** em jogo, animação e
VFX, **em qualquer engine**, em dois níveis: **Pessoal** e **Profissional**. A
Fab também oferece licenças **Creative Commons** em parte do acervo gratuito.

Isso tem uma consequência operacional que não custa decisão nenhuma:
**resgatar a rotação gratuita é lucro puro de opção** — zero custo, zero
compromisso, e o acervo só cresce. Vale fazer semanalmente independentemente de
qual estilo o jogo vá adotar.

### Pago — quando o que se compra é COERÊNCIA

**Synty POLYGON**: low-poly estilizado, suporte a UE5, pacotes temáticos
(Nature, Town, Fantasy Kingdom, Adventure, Prototype…). Preço visto: pacotes
entre **US$ 29,99 e US$ 249,99**, ou **assinatura a partir de US$ 30/mês** para
a biblioteca.

O que ele resolve e a colcha de CC0 não: **um estilo só para 26 atores de uma
vez**. Juntar cinco fontes CC0 dá cinco estilos — e o inventário mostra que este
mundo é feito de peças que aparecem **juntas na mesma tela** (floresta, montanha,
caverna, vulcão, vila, pet, jogador).

---

## 3. A camada que nenhum pacote de cenário resolve: animação do pet

`Public/Battle/PetView.h:129` declara `TObjectPtr<USkeletalMeshComponent>
CharacterMesh;` — e **não há malha esqueletal, nem `AnimInstance`, nem
`AnimMontage` em lugar nenhum do projeto**. O movimento hoje é deslizamento de
ator, não animação.

- **Quaternius já traz animação** nos bichos — é o caminho de menor atrito.
- **Mixamo** auto-rigga e anima **humanoide**, o que serve ao
  `AWorldExplorerCharacter` (jogador), não a bicho de quatro patas.
  *(não pesquisado a fundo nesta rodada — só registrado como candidato.)*

---

## 4. O maior zero é áudio — e ninguém pediu

O inventário mediu: **zero** `AudioComponent`/`USoundBase`/`SoundCue`, **zero**
Niagara/ParticleSystem, e **nenhum GOAL** mencionando som, áudio, música ou
sonoro. Áudio não está atrasado; **não foi pedido**.

Fechá-lo é o mais barato da lista (Kenney, CC0, sem crédito). Mas **abrir essa
frente é decisão de produto**, e não é minha.

---

## 5. As duas decisões que são suas

1. **Estilo de arte.** É ela que decide CC0-remendado (grátis, estilos mistos)
   contra pacote coerente pago (Synty). Nenhuma medição responde isso.
2. **O jogo é comercial?** Decide o nível da licença Fab (**Pessoal** vs
   **Profissional**) e coloca **CC-BY-NC fora** de qualquer escolha.

---

## Fontes

- https://www.unrealengine.com/fabfreecontent
- https://dev.epicgames.com/documentation/fab/fab-documentation
- https://dev.epicgames.com/documentation/unreal-engine/free-epic-games-content-for-unreal-engine
- https://quaternius.com/
- https://opengameart.org/content/all-cc0-uploader-quaternius
- https://syntystore.com/collections/polygon
- https://www.kenney.nl/assets
- https://freesound.org/

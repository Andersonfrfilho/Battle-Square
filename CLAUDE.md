# Contexto de I.A. — Battle Square

Leia também `.specs/project/STATE.md` (histórico, lições L-001+, blockers) e
`.specs/project/ROADMAP.md` antes de começar.

---

## Regra: o que for implementado precisa aparecer na tela

**Toda vez que implementar comportamento observável em jogo, mostre na tela o
que está acontecendo.** Use `FBattleDebugScreen::Show` (`Debug/BattleDebugScreen.h`).

### Por que esta regra existe

Em 26–27/08/2026, ao dar interface ao combate, **oito defeitos sérios
apareceram — e sete só foram encontrados porque um humano olhou a tela**:

| Defeito | Por que nenhum teste pegou |
|---|---|
| Pets invisíveis (`APetView` sem componente visual) | O ator existia e a lógica passava |
| **Inimigos do mundo invisíveis** (malha nunca atribuída) | Mesmo padrão, um mês depois: andavam e disparavam batalha |
| **O próprio jogador invisível** (`ACharacter` traz malha esqueletal sem asset) | Terceira ocorrência do mesmo padrão, achada auditando em vez de esperando |

**O padrão vale mais que os três casos:** ator que nasce com componente visual
mas SEM asset atribuído passa em todo teste de lógica e não existe na tela.
Componente criado não é componente visível. Ao criar ator novo, atribuir malha
e cor no construtor, e escrever o teste que verifica a ATRIBUIÇÃO.
| Câmera apontada para longe da arena | A batalha rodava, só que fora de campo |
| Pet afundando meio corpo no tabuleiro | Posição correta, offset visual errado |
| "Baixo" andava para a direita | Eixos da grade contra os da câmera |
| Batalha de um turno só | 189 testes verdes, e um deles *afirmava* o defeito |
| Três ações instantâneas | "Animação" sem tempo nenhum |
| Oponente previsível (semente sempre 0) | Determinismo correto, produto errado |

O ciclo "usuário joga → descreve o que viu → eu leio o log → deduzo" custou
horas. Informação na tela encurta para "joga → lê". O usuário é o instrumento
de medição mais eficiente que este projeto tem; **facilite o trabalho dele.**

### Como aplicar

- **Estado que muda a cada turno** → `Key` fixa, para a linha se atualizar no
  lugar em vez de empilhar.
- **Evento pontual** → `Key = -1`, para empilhar.
- **Cor com significado**: um lado numa cor, o outro em outra.
- Nunca custa em Shipping (compilado fora).

### O que fica visível durante a batalha

| Recurso | Para que serve |
|---|---|
| **Grade desenhada no mundo** | cada casa com `(coluna,linha)`, **o que ela é** (ÁGUA azul, DANO vermelho, BÔNUS verde, BLOQUEADA cinza) e quem está nela; casa ocupada fica amarela. Torna posição e direção legíveis — teria mostrado na hora que "Baixo" andava para a direita, e mostra a coabitação sem explicação |
| **Movimento deslizando** | o pet ANDA até a casa nova, e o olhar acompanha durante o percurso. Teleporte não conta a história: quem vê só o antes e o depois não sabe se ele andou, se foi empurrado, nem em que ordem |
| **Indicador de fase** | `reproduzindo fase 3 de 11`, que desfaz a impressão de "fez tudo de uma vez" |
| **Painel de texto** | o que cada lado escolheu e onde cada pet terminou |
| **Skills do pet** | `SUAS SKILLS — Voar`, com botão só para o que ele tem. Skill que não aparece na tela não é característica de ninguém |

### O painel, e por que ele é copiável

`ABattleDebugHUD` desenha as últimas 16 linhas numa caixa fixa no canto
superior direito. É HUD, não UMG, de propósito: não depende de asset autorado,
então funciona em qualquer nível assim que o GameMode o declara.

| Tecla / console | O que faz |
|---|---|
| **F9** | copia o painel para a área de transferência **e** grava `Saved/BattleDebug.txt` |
| **F10** | esvazia o painel |
| `bs.ShowBattleDebug 0` | esconde |
| **F7** | **troca** qual jogador você controla — o bot assume o outro |
| `bs.ControlOpponent 1` | o mesmo, por console |

`bs.ControlOpponent` existe porque verificar trombada, esquiva na trombada ou
camuflagem depende de as duas escolhas serem deliberadas. Esperar o sorteio do
oponente cair no caso desejado é medir a sorte, não a regra.

É ferramenta de **desenvolvimento**: compilada fora do Shipping por `#if`, não
por disciplina. Um jogo publicado onde qualquer um joga pelos dois lados não é
o mesmo jogo.

**F7, e não F8: F8 é a tecla de Eject do PIE**, consumida pelo editor antes de
chegar ao jogo.

As teclas são escutadas por um **ouvinte de pré-input do Slate**, não amarradas
no `InputComponent`. Amarrar no controlador depende de foco e de modo de input,
e foi assim que elas simplesmente não chegaram: o widget de ações tem o foco, o
modo é `GameAndUI`, e a tecla morria calada. O ouvinte vê a tecla antes de
qualquer camada poder engoli-la — e é **caminho único**, porque tratar nos dois
lugares faria F7 alternar duas vezes por toque.

**O painel se grava sozinho** em `Saved/BattleDebug.txt` a cada mudança. F9
virou conveniência, não requisito: o arquivo não depende de tecla, de foco, de
modo de input nem da área de transferência. Quem estiver ajudando lê direto.

Isso veio de a tecla de copiar falhar TRÊS vezes seguidas, cada tentativa
custando uma rodada do usuário. Quando um caminho falha três vezes, o defeito
é a dependência, não o detalhe.

Se uma tecla parecer morta: `bs.LogKeys 1` mostra no painel toda tecla que
chega e por qual camada. Medir antes de consertar.

**Há uma BARRA DE BOTÕES na tela** (canto inferior esquerdo, `FBattleDebugToolbar`):
copiar painel, limpar, **trocar de jogador controlado**, e Camuflar/Voar/Submergir.
Montada em **Slate por código**, sem asset autorado — o `WBP` não pôde ganhar
botões, e capacidade que espera edição de asset é capacidade que não existe
hoje. Clique é o caminho que comprovadamente funciona aqui; as teclas são
conveniência e falharam três vezes em PIE.

**Todo clique de botão aparece no painel** (`clique: Atacar`). Se o clique
aparece e a ação não muda nada, o defeito está depois do clique; se nem o
clique aparece, o widget não está recebendo. Uma rodada, duas respostas.

**Ações do jogador 2 na barra, sempre visíveis.** Escolhe-se ali (tipo, depois
direção quando o tipo pedir) e o turno fecha pelo **botão normal de confirmar
do jogador 1**. Sem ação escolhida, o bot decide por ele, como sempre.

Sem modo para ligar e sem segunda fase: cada etapa a mais era uma chance de o
caminho parecer destrutivo, e foi o que travou o uso três vezes seguidas.

**Todo clique de botão aparece no painel** (`clique: Atacar`). Se o clique
aparece e a ação não muda nada, o defeito está depois do clique; se nem o
clique aparece, o widget não está recebendo. Uma rodada, duas respostas.

O botão **troca quem você comanda**: `Controlando jogador 1 — clique para o
jogador 2`. O bot joga pelo outro. É o que permite experimentar **ações
diferentes de cada lado** sem depender de o sorteio da IA cair no caso desejado.

O rótulo diz o estado ATUAL e o que o clique faz. Só o estado deixaria a pessoa
adivinhando o efeito; só o efeito, adivinhando onde ela está.

Trocar zera as escolhas pendentes: elas eram para o outro pet, e aplicá-las ao
novo produziria uma jogada que ninguém pediu.

**Para os DOIS no mesmo turno** — o caso da trombada, em que ambos precisam
mirar a mesma casa — o botão **"Controlar também o jogador 2"**. Aí o turno pede
duas escolhas, e quando chega a vez do jogador 2 **aparece na barra um painel
com as ações dele**: tipos, direções e "CONFIRMAR turno do jogador 2".

O painel fica visível durante todo o modo duplo, com o cabeçalho dizendo de
quem são as ações. Escondê-lo até a vez do jogador 2 fazia o caminho até ele
depender de um botão que parecia encerrar a batalha.

**O confirmar diz a consequência, não a mecânica:**
`Confirmar jogador 1 → passar ao jogador 2`, e depois
`Confirmar jogador 2 → RESOLVER o turno`. Com o texto genérico "confirmar
turno", ninguém clicava na vez do jogador 1 — e sem clicar, o jogador 2 era
inalcançável.

(`bStartControllingBothSides=True` em `Config/DefaultGame.ini` já abre a batalha
nesse modo.)

O painel é **desenhado**, não é campo de texto: o mouse nunca consegue
selecioná-lo. Por isso a cópia é por tecla — e por isso ela também grava
arquivo, que é o caminho que não depende da área de transferência funcionar.

Altura **fixa**, teto de 12 linhas: painel que cresce a cada linha muda de
tamanho o tempo todo e mesmo assim não rola. As mais antigas saem por cima.

Mensagem que some obriga a ler depressa e a repetir a partida para reler; e
transcrever da tela à mão perde justamente o detalhe que importa — foi assim
que `Atacar Esquerda` virou "ele foi para a esquerda" numa investigação real.
Por isso o painel **persiste** e **copia**.

### O que a regra NÃO autoriza

- Log de PII (`security.md` §1) — nem na tela, nem em arquivo.
- Substituir teste por log. O log encontra o defeito; **o teste impede que ele
  volte**. Todo defeito achado na tela ganha teste antes de ser fechado.

---

## Depurar: teste primeiro, conserto depois

**Diante de um defeito, a primeira ação é escrever um teste que o reproduza —
não um conserto.** Em 26–27/08 consertei por hipótese três vezes seguidas; cada
correção estava certa isoladamente, e nenhuma era comprovadamente *a* causa. Só
parei de errar quando medi.

Ordem de eficácia, do que mais resolveu para o que menos:

1. **Teste headless que reproduz** — respondeu "o gerador avança e o inimigo se
   move" em um ciclo de build, sem envolver o usuário, e ficou como proteção.
2. **Inspecionar o estado vivo** (PIE + consulta ao mundo) — achou a batalha
   acontecendo a um milhão de unidades da câmera.
3. **Instrumentação dirigida no log** — decisiva quando a dúvida é sequência.
4. **O usuário olhando a tela** — insubstituível para o que é visual.

**Sempre abrir o Editor com `-log=BattleSquare.log`**, para haver um arquivo só
a consultar em vez de caçar o mais recente.

---

## Verificação: rode antes de dar qualquer coisa por pronta

```bash
./Tools/audit_determinism.sh && ./Tools/audit_no_recalculation.sh
./Tools/audit_localizable_text.sh   # texto do jogador precisa ser coletável
./Tools/probe_isolation.sh
./Tools/sync_module_manifest.sh   # L-025: manifesto defasado faz teste novo sumir da contagem
```

Depois de `probe_isolation.sh`, **recompile** (L-020) — ela deixa o `.dylib` do
`BattleSim` quebrado.

**Feche o Editor antes de compilar** e reabra com `open -a` (não por shell em
segundo plano): editor lançado em background não recebe teclado no PIE, o que
custou várias rodadas de investigação inútil.

---

## Texto na tela é FText, nunca FString

Todo texto que o **jogador lê** usa `LOCTEXT`/`NSLOCTEXT` com argumentos
**nomeados** (`{Actor}`, não `{0}` — em alemão o objeto vem antes do verbo, e
posicional obriga o tradutor a reordenar o que não é dele).

A coleta lê o **código-fonte** procurando as macros. Por isso não existe teste
de runtime que separe `FText::Format` de `FString::Printf` — tentei escrever um
e ele afirmava uma diferença que a engine não expõe. Quem verifica é
`Tools/audit_localizable_text.sh`.

O modo de falhar é silencioso: alguém troca por `Printf` "porque é mais
simples", nada quebra, o português continua certo, e o idioma novo nasce
faltando linha. Depois de mexer em texto, rodar `./Tools/gather_text.sh`.

Culturas empacotadas: `pt-BR` (nativa), `en`, `es`.

---

## Fronteiras que não se cruzam

- **`BattleSim` é o núcleo determinístico.** Sem float, sem `FMath::Rand`, sem
  relógio. Semente e montagem são decisão da camada de fora.
- **A tela não decide regra** (DP-ui-01). Todo botão encaminha ao
  `UBattleActionQueueComponent`, que já tem a regra e o teste.
- **Uma fonte de verdade por regra.** Duplicar uma tabela ou uma validação foi
  a causa de L-032, L-033 e de um defeito de direção — as cópias concordam até
  a primeira edição.

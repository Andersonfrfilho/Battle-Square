# Decisões do usuário — respondidas em 02/09/2026

**Este arquivo é a fonte única.** As features NÃO copiam as respostas; elas
apontam para cá. Duas cópias concordam até a primeira edição (L-032).

Numeração conforme a pergunta feita. Onde a resposta abriu trabalho que
nenhuma task cobre, está marcado **🆕 ESCOPO NOVO**.

---

## Batalha — `commit-por-pet` (frente ativa)

**1. Quantos pets por lado.** Vários. O número não é fixo: o sistema tem de
aguentar N, e a CP1 mediu o teto de hoje.

**2. Quantas ações por turno.** **Dinâmico, não constante** — hoje três, e o
produto vai aumentar. Portanto o número é configuração, e nenhum teste pode
afirmar "três" como literal.

---

## Água e fundura — `fundura-no-tracado`

**3. Onde fica a cintura.** **Não é número fixo: é fração da ALTURA de quem
atravessa — 0 a 40% da altura é o que se faz a pé.**

> **Medição (02/09/2026):** o `ACharacter` do jogador usa a cápsula padrão,
> meia-altura **88** → altura cheia **176**. 40% de 176 = **70,4 unidades**.
> O limiar de hoje é `WadableDepthUnits() = 100`, que são **56,8% da altura** —
> altura do peito, não da cintura. **A regra nova é mais restritiva**, e
> reclassifica as travessias entre 70,4 e 100. Quantas são exatamente, medir
> na F1: hoje só se sabe que as 30 de vau estão todas abaixo de 94.

**4. Contínuo ou faixa nomeada.** A fundura é dado do mundo **e** propriedade
do personagem: o limiar sai de `0,40 × altura DELE`. Um pet baixo atravessa
menos fundo que o jogador, sem regra nova — só a mesma conta com outra altura.

**5. A carta pode mudar de números.** *(Pergunta reformulada — ver "Ainda em
aberto".)* O que ficou decidido: **o mundo é dinâmico, com âncoras fixas para
equilíbrio de onde as coisas nascem**, e **existem catástrofes**.

**6.** *(Ainda em aberto — ver abaixo.)*

**7.** *(Ainda em aberto — sugestão pedida.)*

---

## Segredos e mapa — `segredos-e-a-carta`

**8. Quantos segredos.** **Parametrizável**, número depois.

**9. 🆕 ESCOPO NOVO — o mapa do jogador.** O mapa **abre conforme se anda**, e
**partes do mapa podem ser COMPRADAS por região**. Isso é uma feature própria
(névoa de guerra + comércio de cartografia); a SC3 só previa revelar andando.

**10. Contar não deixa de ser segredo.** Sim: a contagem pode existir sem
entregar onde.

---

## Cavernas — `cavernas-nas-quedas`

**11. Quantas quedas ganham gruta.** **Por parâmetro, default 30%** das 13.

**12. Soma, não substitui** — a gruta de trás soma à de lado. **E precisa
decidir se ela tem ligação ou saída para outros lugares** (registrado como
pergunta derivada, não respondida).

**13. A lâmina de água é efeito visual.**

---

## Cidades — `cidades-do-interior`

**14. O que machuca o treinador.** Ataques de outros pets, ataques de outros
treinadores, desastres naturais e outros danos.

**15. 🆕 ESCOPO NOVO — especialização e liderança de centro.** As cidades
especializam, MAS todo bioma tem de ser jogável de nascença: **jogadores podem
nascer em qualquer bioma, então todos são independentes e cheios de skills.**
Ganhar o campeonato de uma cidade e de um centro de treinamento **torna o
jogador LÍDER do centro**, com renda por isso — e **ele não pode sair nem
aceitar desafios até alguém vencê-lo**, inclusive NPC. **NPCs têm histórias
próprias e evolutivas.**

**16. Curar é de graça.**

**17.** *(Ainda em aberto.)*

**18. Toda cidade tem nome**, e **o mundo precisa de PLACAS sinalizando
posições.** 🆕 (placas não existem hoje.)

**19.** *(Ainda em aberto — sugestão pedida.)*

**20. Comprar e capturar coexistem.** Pets podem ser capturados e vendidos —
**inclusive os roubados, mas roubado só se vende no MERCADO NEGRO.** 🆕

**21. 🆕 ESCOPO NOVO — massa monetária medida.** O dinheiro é **medido e
compartilhado entre os mundos**: é uma sociedade econômica. Precisa existir
medição da **quantidade de dinheiro do jogo** e da **distribuição dele entre
jogadores, NPCs e tarefas.** Isso é maior que "saldo inicial da carteira".

---

## Crime — `crime-e-recompensa`

**22. Como o roubo acontece.** Ataca-se o TREINADOR e toma-se o pet **se ele
estiver desmaiado, hipnotizado ou paralisado**. Repetir põe o jogador na lista
de procurados e o torna bandido — **e abre o clã oculto dos bandidos.** 🆕

**23. Duração da marca.** Dura **até o jogador mudar de aparência** (roupa,
corte de cabelo) a ponto de não ser identificado com o pet. Mas **quem viu o
cartaz reconhece**: jogador tem a OPÇÃO de denunciar; **NPC sempre denuncia,
conforme a moralidade dele.** 🆕 (exige aparência do personagem como dado, e
memória de quem viu o cartaz.)

**24. Redenção.** Devolver é possível, **com punição** — ou **caçar bandidos
procurados** até cobrir a própria recompensa.

**25. Organizações criminosas existem, e LUTAM ENTRE SI** — com ganho e perda
recorrente de domínio entre elas. 🆕

**26. A polícia erra**, quando um jogador coincide em características **e pet**
com o bandido procurado.

**27. A prisão confisca, tranca e multa.** **Parte do dinheiro do confisco e da
multa vai para o usuário roubado**, no valor do que ele perdeu.

**28. Sem saldo para a multa, DOBRA o tempo preso.** A recompensa sempre
existe: **quem paga é o governo.**

**29–31. O envelhecimento corre offline** — e **a prisão tem o que fazer
dentro.** 🆕

---

## Mundo vivo — `mundo-vivo`

**32. O pet morre de velho.**

**33. O tempo passa para o pet mesmo offline — a não ser que ele seja
CRIOPRESERVADO.** 🆕 (criopreservação não existe hoje.)

**34.** *(Ainda em aberto.)*

**35. A plantação é do ASSENTAMENTO** — e **o assentamento pode ser comprado,
inclusive em sociedade com outros.** 🆕

**36. A ferramenta importa** no que se colhe.

**37. O pet ajuda a coletar** (não é o jogador sozinho).

---

## Posse — `posse-no-servidor`

**38. A coleção local DESAPARECE** (não vira cache).

**39. Offline joga-se local, com NPCs** — a vida acontece normalmente, e **pode
até haver incremento de jogadores.** 🆕

**40. A conta entra por cadastro**, no jogo ou no portal.

**41. Troca entre contas** exige **autorização das DUAS**. Havendo relato de
troca indevida ou conta invadida, **a polícia segura o pet até resolver**, e
**verificação de sessão acontece aqui** — é onde dá para perceber que algo está
errado. 🆕 *(O `403` vs `404` continua em aberto — ver abaixo.)*

**42. Pet abandonado volta a ser selvagem.**

**43. A coleção NÃO tem teto.** E guarda-se **o conhecimento e o ganho de
experiência** de cada um. 🆕

---

## Montaria — `montaria-e-trilhas`

**44. Monta-se por tamanho, atributos e skills.**

**45. O cansaço se recupera descansando, dormindo, acampando e comendo** — e
**alguns pets ajudam na recuperação.** 🆕

**46. Barra própria** (não é a de HP). Confirma a recomendação das tasks.

**47. Voar ignora o custo do relevo**, mas **leva em conta temperatura,
altitude e gravidade.** 🆕

**48. Nadar no mundo usa os mesmos golpes da batalha** — todo golpe usado no
mundo pode ser usado em batalha.

---

## Biomas — `mundo-por-biomas`

**49. O nascimento é aleatório.**

**50. Há pets exclusivos de bioma E pets comuns a vários.**

**51.** *(Adiada pelo usuário: "vamos decidir ainda". Registrado: as ilhas podem
ligar-se por terra ou continuar ilhas; hoje todo servidor vê o mesmo mapa fixo.)*

**52.** *(Ainda em aberto.)*

---

## Deuses — `mae-natureza`

**53. A faixa-alvo é por ELEMENTOS DO MAPA** (não por ilha nem por região
administrativa).

**54. Dá para agradar os deuses: existem TEMPLOS, e orar AUMENTA os atributos
do personagem.** 🆕

**55. Os deuses discordam — e podem entrar em GUERRA.** 🆕

---

## Ainda em aberto — onze itens

| # | o que falta |
|---|---|
| 5 | reformulada: "a carta" é o GABARITO (`ChartConformance`), o teste que exige mundo e carta idênticos — não o mapa do jogo |
| 6 | reformulada em linguagem direta |
| 7 | sugestão pedida ao assistente |
| 8 | adiada pelo usuário ("pensamos nisso depois") |
| 9 | respondida sobre o MAPA; a pergunta era sobre o gabarito |
| 17 | reformulada |
| 19 | sugestão pedida ao assistente |
| 34 | reformulada |
| 41 | `403` vs `404` não respondido |
| 51 | adiada pelo usuário |
| 52 | reformulada |

# Roteiro de Verificação — Mobile

**Feature:** `.specs/features/mobile/`
**Status:** **BLOQUEADO** — nenhum item deste roteiro pode ser executado nesta
máquina hoje. Isto não é "não verificado ainda"; é diferente, e a diferença
importa: os outros roteiros do projeto esperam alguém com tempo, este espera
alguém com **acesso e permissão**.

---

## Antes de qualquer item: os bloqueios

Nenhum é resolvível por código. Todos precisam de uma ação do usuário.

| Blocker | O quê | Evidência exata | Remédio |
|---|---|---|---|
| **B-006** | Componente **IOS** da engine não instalado | `Build.sh BattleSquare IOS Development` → `Missing files required to build IOS targets. Enable IOS as an optional download component in the Epic Games Launcher.` | Epic Games Launcher → instalar o componente IOS do UE 5.8 |
| **B-006b** | Sem identidade de assinatura e sem perfil de provisionamento | `security find-identity -v -p codesigning` → `0 valid identities found`; `~/Library/MobileDevice/Provisioning Profiles/` vazio | Conta Apple Developer, certificado de desenvolvimento e perfil para o aparelho |
| **B-007** | SDK/NDK Android ausente | Validação de plataforma da engine → `Android INVALID r27c` | Rodar o `SetupAndroid` da engine, ou instalar Android Studio + NDK r27c |

**Basta resolver B-006 + B-006b (caminho iOS) OU B-007 (caminho Android)** para
destravar o roteiro inteiro. Não é preciso os dois.

---

## MOB-01 — O projeto compila para a plataforma móvel

- [ ] **BLOQUEADO** (B-006 ou B-007)

**Passo:** `Build.sh BattleSquare IOS Development -project=<caminho>` (ou
`Android`).

**Critério:** `Result: Succeeded`. Hoje o comando falha antes de compilar uma
linha, com a mensagem de B-006 — o toolchain não existe na máquina.

---

## MOB-02 — Empacota e roda no aparelho de referência

- [ ] **BLOQUEADO** (B-006 + B-006b, ou B-007)

**Passo:** empacotar `WorldStreamingTest` em Development e instalar no aparelho
de referência de `docs/performance/orcamento-mobile.md` (gama média de ~4 anos,
4 GB de RAM).

**Critério:** o app abre e o nível carrega.

---

## MOB-03 — O orçamento de performance, medido

- [ ] **BLOQUEADO** (depende de MOB-02)

**Passo:** possuir o `WorldExplorer`, percorrer a área de ponta a ponta e
preencher a coluna **Medido** da tabela de
`docs/performance/orcamento-mobile.md`, com `stat memory`, `stat unit` e
`stat streaming`.

**Critério:** este item é **medição, não aprovação**. O valor de rodá-lo é ter
o número. Onde medido e alvo discordarem, o documento exige escrever qual dos
dois muda e por quê — um alvo que se ajusta a cada medição não é orçamento, é
legenda.

**Expectativa registrada de antemão:** os 384 MB de streaming são o número que
mais provavelmente cai. Se cair, a resposta certa é reabrir DP-streaming-02a
(tamanho de célula e raio de World Partition) com o número medido na mão — não
subir o alvo em silêncio.

---

## MOB-04 — Andar com o dedo

- [ ] **BLOQUEADO** (depende de MOB-02)

**Passo:** arrastar o dedo na tela para mover o `WorldExplorer`.

**Critério:** arrastar para cima anda para frente; dedo parado não gera deriva;
arrastar mais longe que o raio máximo **não** deixa o personagem mais rápido.

**O que já está provado sem aparelho:** toda a matemática acima é testada
headless em `BattleSquare.World.TouchMovementInput`, incluindo que o eixo de
toque produz exatamente a mesma direção que o eixo de teclado. O que este item
acrescenta é só que o toque **chega** do sistema operacional — a única parte que
o aparelho decide.

**Pendência de fiação, honesta:** `FTouchMovementInput` existe e é testada, mas
**ainda não está ligada** a um `UInputAction` de toque no
`AWorldExplorerCharacter`. Ligar exige autorar os assets de Enhanced Input, que
é a mesma pendência que `traversal-camera-mundo.md` já registra na preparação.
Fazer essa fiação sem poder rodá-la uma vez sequer seria escrever código no
escuro; ela fica para quando MOB-02 destravar.

---

## MOB-05 — A escalabilidade declarada faz efeito

- [ ] **BLOQUEADO** (depende de MOB-02)

**Passo:** no aparelho, confirmar que `Config/DefaultDeviceProfiles.ini` e
`Config/DefaultScalability.ini` estão sendo aplicados (`DumpDeviceProfile` no
console) e comparar `stat unit` com e sem o perfil.

**Critério:** os CVars do perfil `Mobile` aparecem aplicados. Esta é a única
verificação possível daqueles dois arquivos — **nenhum teste headless prova que
uma escalabilidade de mobile faz o que promete**, e o design (DP-mobile-03) diz
isso em vez de fingir cobertura.

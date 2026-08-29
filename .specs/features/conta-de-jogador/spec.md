# Conta de Jogador — Especificação

**Status:** Concluída e verificada por teste (status corrigido em 29/08/2026 — a linha dizia "Draft" enquanto a feature já rodava)
**Depende de:** `apps/api-battle-pets` (backend de dados de pet, M4) — a conta nasce no mesmo serviço, que já tem Bun + Drizzle + Postgres, autenticação por token e validação Zod na fronteira. Primeira feature de M7.

---

## Problem Statement

Nada no jogo sobrevive ao fim de uma sessão de um jogador específico. A coleção de pets é local (`UPetCollectionSaveGame`, um `USaveGame` em disco). A progressão é local. A sala de M2 identifica um jogador por um `FGuid` de segredo que morre com a sala. **Não existe "quem é você" — existe "quem está nesta máquina agora".**

Isso já está cobrando juros em dois lugares concretos:

1. **`colecao-e-captura` teve de declarar** que a coleção é local "porque M7 não existe ainda". Trocar de máquina hoje é perder tudo.
2. **`backend-dados-pet` (AD-017) implementou a trilha de auditoria de adulteração de cache no Nível 1 — só sessão.** Banimento persistente (Nível 2) foi explicitamente adiado para este marco, porque banir exige saber **quem** voltar a reconhecer amanhã.

Esta feature entrega a identidade. Ela não entrega o que se faz com ela — banimento é a feature seguinte, e sincronizar coleção é escopo que ninguém abriu.

**O que ela NÃO pode fazer, e é a restrição de segurança que governa tudo:** guardar senha em texto, emitir token eterno, vazar e-mail em log, ou aceitar que a queda dela impeça uma partida local. As três primeiras vêm de `security.md` §1/§2/§4 e são bloqueantes em code review. A quarta é o princípio que `backend-dados-pet` já estabeleceu: o jogo não pode ficar refém do backend no caminho crítico.

## Goals

- [ ] Um jogador cria uma conta e volta a entrar nela de outra sessão, provando identidade persistente
- [ ] Senha nunca existe em texto no banco, no log, nem na resposta — só como hash resistente a GPU
- [ ] Token de acesso é de vida curta e o de renovação é rotativo, com o anterior invalidado ao ser usado
- [ ] E-mail duplicado é recusado com código estável, ancorável num campo de formulário
- [ ] Login tem limite de tentativas, e o limite é por conta — não só por IP
- [ ] A API de conta é testável sem banco em tudo que é decisão (token, política de senha, limite de tentativas)

## Out of Scope

| Item | Razão |
|---|---|
| Banimento e moderação | Segunda feature de M7. Consome esta, não é esta |
| Sincronizar coleção/progressão para a conta | Escopo grande e não aberto: exige decidir resolução de conflito entre save local e remoto. A identidade vem primeiro |
| Login social (Google, Apple, Steam) | Cada provedor é uma integração própria, com contrato e credencial de app. Sem conta própria funcionando, somar provedor é somar risco |
| Recuperação de senha por e-mail | Exige provedor de e-mail transacional configurado — infraestrutura que não existe no projeto |
| Cliente C++ na Unreal consumindo a API | Só faz sentido depois de decidir o que a conta sincroniza (fora de escopo acima). Esta feature entrega o serviço e o contrato |
| Verificação de e-mail | Mesmo bloqueio de infraestrutura da recuperação de senha |

---

## User Stories

### P1: Criar conta ⭐ MVP

**User Story:** Como jogador, quero criar uma conta com e-mail e senha, para que o que eu conquistar deixe de morrer com a máquina.

**Acceptance Criteria:**
1. WHEN um e-mail e uma senha válidos são enviados THEN o sistema SHALL criar a conta e responder `201`, **sem devolver o hash da senha nem qualquer campo interno**
2. WHEN o e-mail já existe THEN o sistema SHALL responder `409` com código estável `ACCOUNT_EMAIL_TAKEN`, ancorável no campo `email`
3. WHEN a senha não atende à política THEN o sistema SHALL responder `400` com **todos** os motivos de uma vez, nunca só o primeiro
4. WHEN a conta é criada THEN o sistema SHALL persistir a senha apenas como hash Argon2id, e nunca registrar o e-mail em log

**Independent Test:** a política de senha e a normalização de e-mail são funções puras, testadas sem banco. O `409` e o `201` são testados contra o caminho real.

---

### P1: Entrar e permanecer entrado ⭐ MVP

**User Story:** Como jogador, quero entrar na minha conta e continuar entrado sem redigitar a senha o tempo todo, sem que isso vire um token eterno na minha máquina.

**Acceptance Criteria:**
1. WHEN as credenciais estão corretas THEN o sistema SHALL emitir um **token de acesso de vida curta (≤15 min)** e um **token de renovação** rotativo
2. WHEN o token de renovação é usado THEN o sistema SHALL emitir um par novo e **invalidar o anterior** — reuso de um token já rotacionado é recusado
3. WHEN as credenciais estão erradas THEN o sistema SHALL responder `401` com a **mesma** mensagem para e-mail inexistente e senha errada, sem revelar qual dos dois falhou
4. WHEN um token de acesso expirado ou adulterado é apresentado THEN o sistema SHALL recusá-lo com `401`

**Independent Test:** emissão e verificação de token são puras (assinatura HMAC + expiração injetada como parâmetro, nunca `Date.now()` interno), testadas sem banco: token válido passa, expirado falha, assinatura adulterada falha, token de outro segredo falha.

---

### P1: Login não é um balcão aberto ⭐ MVP

**User Story:** Como dono do serviço, quero que tentativas repetidas de login sejam limitadas, para que a conta de um jogador não seja adivinhável por força bruta.

**Acceptance Criteria:**
1. WHEN um número de tentativas falhas é excedido para a **mesma conta** dentro de uma janela THEN o sistema SHALL responder `429` com `Retry-After`
2. WHEN uma tentativa é bem-sucedida THEN o sistema SHALL zerar a contagem daquela conta
3. WHEN a janela expira THEN o sistema SHALL voltar a aceitar tentativas

**Independent Test:** o limitador é puro, com o tempo injetado — testado sem banco e sem relógio real, no mesmo padrão que `UBattleTurnCoordinator` já usa para timeout (M2).

---

## Decisões Pendentes

**PROPOSTA** — decisões de produto, mudam se você discordar.

1. **E-mail + senha, não usuário + senha.** PROPOSTA: e-mail é o único identificador que o jogador não esquece e que permite recuperação futura. Custo aceito: e-mail é PII e nunca pode ir para log.
2. **A conta é opcional.** PROPOSTA: jogar sem conta continua funcionando exatamente como hoje. Obrigar conta para jogar sozinho seria cobrar um custo de fricção sem entregar nada em troca.
3. **A conta vive no `api-battle-pets`, não num serviço novo.** PROPOSTA: o serviço já tem Postgres, Drizzle, autenticação, validação e assinatura. Um segundo serviço custaria operação em dobro para um domínio que hoje cabe em duas tabelas. Se conta crescer (social, recuperação, perfil), separar depois é barato — o oposto não é.

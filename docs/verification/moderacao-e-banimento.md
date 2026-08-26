# Roteiro de Verificação — Moderação e Banimento

**Feature:** `.specs/features/moderacao-e-banimento/`
**Serviço:** `apps/api-battle-pets`
**Status:** ✅ **VERIFICADO em 2026-08-26** — os 8 itens rodados contra o
Postgres real (`battle-square-postgres`, porta 5445), com a API numa instância
dedicada na porta 3101.

**MOD-04 falhou na primeira execução, exatamente como este roteiro previa.**
Uma conta banida continuava renovando a sessão indefinidamente: `refreshSession`
não checava o banimento, e como cada renovação emite um refresh novo, o
banimento nunca chegaria a valer para quem já tinha entrado. Corrigido e
reverificado em cenário limpo (ver `STATE.md`, L-033).

Cobre só o que exige Postgres de pé (DP-mod-06). A regra de decisão —
"esta conta está banida neste instante?" — já está coberta por `bun test`
(10 testes puros, tempo injetado) e **não** se repete aqui.

---

## Preparação

```bash
cd apps/api-battle-pets
bunx drizzle-kit migrate      # aplica 0001 (conta) e 0002 (moderação)
bun run src/index.ts
```

Guardar dois valores do `.env`: `ADMIN_API_TOKEN` e `SYNC_API_TOKEN`.
Criar uma conta de teste com `CTA-01` de `conta-de-jogador.md` e guardar o
`data.id` — é o `<accountId>` daqui em diante.

---

## MOD-01 — Registrar evidência não bane

- [x] **Verificado em 2026-08-26** — 201 com `recordedBy: "sync"`, e o login da conta seguiu em 200 — registrar não pune.

```bash
curl -si -X POST http://localhost:3100/v1/accounts/<accountId>/moderation-events \
  -H "Authorization: Bearer $SYNC_API_TOKEN" \
  -H 'Content-Type: application/json' \
  -d '{"type":"tampered_pet_cache","detail":"assinatura inválida no espelho"}'
```

**Critério, em duas partes:**
1. `201`, com `data.recordedBy == "sync"`.
2. **O login daquela conta continua funcionando.** É este o ponto do teste:
   DP-mod-04 separa registrar de punir, e se o login começar a falhar aqui,
   alguém automatizou o banimento — que é exatamente o que o design proíbe.

---

## MOD-02 — Escopo é exigido

- [x] **Verificado em 2026-08-26** — 401 sem token e com token inválido; `sync` recebe 403 no histórico e ao banir; só `admin` recebe 200.

Repetir MOD-01 sem `Authorization`, e depois com um token inválido.

**Critério:** `401 UNAUTHENTICATED`. E o histórico
(`GET .../moderation-events`) com o `SYNC_API_TOKEN` devolve `403 FORBIDDEN` —
`sync` registra, mas **não** lê histórico (DP-mod-05).

---

## MOD-03 — Banir e ser recusado

- [x] **Verificado em 2026-08-26** — 201 com `expiresAt: null` e `createdBy: admin`; login com a senha CERTA devolveu 403 `ACCOUNT_BANNED`, com motivo e `permanente`.

```bash
curl -si -X POST http://localhost:3100/v1/accounts/<accountId>/bans \
  -H "Authorization: Bearer $ADMIN_API_TOKEN" \
  -H 'Content-Type: application/json' \
  -d '{"reason":"adulteração reincidente"}'
```

**Critério:** `201`, com `data.expiresAt == null` (ausência de prazo é
permanente). Em seguida, tentar o login **com a senha certa**:

**Critério:** `403` com `error.code == "ACCOUNT_BANNED"`, e `error.details`
trazendo o motivo e `"permanente"` como prazo.

---

## MOD-04 — O banimento não espera o token expirar ⭐ o mais importante

- [x] **Verificado em 2026-08-26** — **Falhou primeiro** (200, renovação livre). Depois da correção: 403 `ACCOUNT_BANNED` em cenário limpo, e o token fica inutilizável na segunda tentativa (401).

**Passo, na ordem:**
1. Fazer login **antes** de banir e guardar o `accessToken`.
2. Banir a conta (MOD-03).
3. Tentar **renovar** a sessão com o `refreshToken` obtido no passo 1.

**Critério:** a renovação é recusada. Se ela funcionar, o banimento é
**decorativo** durante toda a vida do token de acesso (até 15 minutos) —
justamente a janela em que mais importa, logo depois da decisão de banir.

**Este é o item que separa banimento real de anotação.** Se falhar, é
regressão de DP-mod-03, não comportamento aceitável.

---

## MOD-05 — Banimento com prazo expira sozinho

- [x] **Verificado em 2026-08-26** — 403 durante o prazo, 200 depois dele, sem nenhum processo de limpeza — a expiração é decidida na leitura.

Banir com `expiresAt` uns 60 segundos à frente:

```bash
curl -s -X POST http://localhost:3100/v1/accounts/<accountId>/bans \
  -H "Authorization: Bearer $ADMIN_API_TOKEN" \
  -H 'Content-Type: application/json' \
  -d "{\"reason\":\"teste de prazo\",\"expiresAt\":\"$(date -u -v+60S +%Y-%m-%dT%H:%M:%SZ)\"}"
```

**Critério:** login recusado agora; login **aceito** depois do prazo, **sem
ninguém ter feito nada**. Nenhum processo de limpeza roda — a expiração é
decidida na leitura (DP-mod-02).

---

## MOD-06 — Levantar preserva o histórico

- [x] **Verificado em 2026-08-26** — 204; login volta imediatamente; `DELETE` repetido devolve 404; a linha permanece com `liftedAt` e `liftedBy` preenchidos.

```bash
curl -si -X DELETE http://localhost:3100/v1/bans/<banId> \
  -H "Authorization: Bearer $ADMIN_API_TOKEN"
```

**Critério, em três partes:**
1. `204`, e o login volta a funcionar **imediatamente**.
2. `GET /v1/accounts/<accountId>/moderation-events` ainda lista o banimento,
   agora com `liftedAt` e `liftedBy` preenchidos — **a linha não sumiu**.
3. Repetir o `DELETE` no mesmo banimento devolve `404 BAN_NOT_FOUND` (já
   levantado), não um segundo `204`.

**Por que a parte 2 importa:** uma trilha de auditoria que pode ser apagada não
é trilha de auditoria (`security.md` §10, DP-mod-01).

---

## MOD-07 — Mais restritivo vence

- [x] **Verificado em 2026-08-26** — Com um temporário já expirado e um permanente ativos, a conta seguiu banida e o motivo devolvido foi o do **permanente**.

Criar **dois** banimentos ativos na mesma conta: um temporário de 60s e um
permanente.

**Critério:** depois dos 60s, a conta **continua** banida — o permanente
prevalece. A regra em si já é testada por `moderation.ban.test.ts`; o que este
item acrescenta é que a consulta ao banco alimenta a regra com **todos** os
banimentos daquela conta, e não só o mais recente.

---

## MOD-08 — Nenhum log com PII

- [x] **Verificado em 2026-08-26** — Log com 1 linha (a de boot). Zero ocorrências de e-mail, `argon2`, token ou uuid de conta.

**Passo:** com o serviço em terminal visível, executar MOD-01 a MOD-07 e ler a
saída.

**Critério:** nenhum e-mail aparece. As trilhas guardam **escopo do token**
(`admin`, `sync`) e `accountId` opaco — nunca identificador pessoal
(`security.md` §1, DP-mod-01).

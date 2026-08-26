# Moderação e Banimento — Design

**Spec:** `.specs/features/moderacao-e-banimento/spec.md`
**Status:** Draft — aguarda aprovação

---

## DP-mod-01: Duas tabelas, e nenhuma linha é apagada

**Decisão:**

```
moderation_events   id · account_id (fk) · type · detail · recorded_by · created_at
account_bans        id · account_id (fk) · reason · expires_at (null = permanente)
                       · created_by · created_at · lifted_at · lifted_by
```

- **Levantar um banimento marca `lifted_at`/`lifted_by`; nunca faz `DELETE`.** Uma trilha de auditoria que pode ser apagada não é trilha de auditoria (`security.md` §10).
- `expires_at` **nulo é permanente**. A alternativa (uma data absurda como "ano 9999") esconde a intenção atrás de uma sentinela e sempre acaba comparada errado em algum lugar.
- `type` é `varchar`, não ENUM nativo (§8 do `code-standart.md`).
- `recorded_by`/`created_by`/`lifted_by` guardam **o escopo do token** que agiu (`admin`, `sync`), não um e-mail — PII não entra em trilha (`security.md` §1).

## DP-mod-02: A decisão de "está banido?" é pura, com o tempo injetado

**Decisão:** `resolveBanState(bans, now)` — função pura sobre uma lista de banimentos, devolvendo o banimento ativo (se houver). Regras:

- banimento **levantado** nunca está ativo;
- banimento **expirado** (`expires_at <= now`) nunca está ativo;
- banimento **sem prazo** está sempre ativo;
- havendo mais de um ativo, **o mais restritivo vence** — permanente ganha de temporário, e entre temporários ganha o que termina mais tarde.

**Razão:** é a mesma disciplina de todo o projeto — decisão separada de efeito, tempo injetado, testável sem banco e sem esperar. A regra do "mais restritivo vence" existe porque o caso de dois banimentos ativos ao mesmo tempo é real (dois moderadores, dois episódios) e, sem regra escrita, quem decide é a ordem que o banco devolveu.

## DP-mod-03: O banimento não espera o token expirar

**Decisão:** a checagem de banimento acontece na **verificação do token de acesso**, consultando o banco, não só na emissão dele.

**Custo aceito, e é real:** isso põe uma consulta ao banco no caminho de cada requisição autenticada. É o preço de o banimento valer **agora** em vez de valer daqui a até 15 minutos (a vida do token de acesso).

**Por que aceitar esse custo:** um banimento que espera o token expirar é decorativo justamente na janela em que mais importa — logo depois da decisão de banir. E o critério 3 da terceira história existe exatamente para tornar isso verificável, em vez de virar detalhe esquecido.

**Se o custo doer depois:** a saída é cache curto (na ordem de segundos) da decisão, não abandonar a checagem. Fica registrado como caminho, não implementado — ninguém mediu que dói.

## DP-mod-04: Registrar evidência não bane

**Decisão:** `POST` de evento de moderação **nunca** cria banimento. São dois endpoints, dois escopos de decisão.

**Razão:** AD-017 é explícito que adulteração **não muda resultado** — não existe urgência que justifique punir sem um humano olhar. Automatizar aqui puniria falso positivo em silêncio, e a única pessoa que descobriria seria a injustiçada.

## DP-mod-05: Escopo administrativo, reusando o que já existe

**Decisão:** as rotas de moderação exigem o escopo `admin` do `auth.middleware.ts` que o módulo de pet já usa; registrar evento aceita também `sync` (é o worker quem observa adulteração).

**Razão:** o serviço já tem autenticação por token com escopo e comparação em tempo constante. Inventar um terceiro mecanismo de permissão para duas rotas seria custo sem ganho — e um mecanismo a mais para errar.

## DP-mod-06: O que é testável sem banco

| Verificação | Sem banco? | Como |
|---|---|---|
| `resolveBanState`: levantado, expirado, permanente, mais restritivo vence | ✅ Sim | `bun test`, puro, tempo injetado |
| Lista vazia não bane; instante exato de expiração | ✅ Sim | Puro |
| `403 ACCOUNT_BANNED` com prazo na resposta | ❌ Precisa de Postgres | Roteiro manual |
| Token válido de conta banida ser recusado | ❌ Precisa de Postgres | Roteiro manual — e é o item mais importante |
| Histórico preservado depois de levantar | ❌ Precisa de Postgres | Roteiro manual |

---

## O que muda

- `apps/api-battle-pets/src/moderation/` — `moderation.schema.ts`, `moderation.ban.ts` (puro), `moderation.validation.ts`, `moderation.use-case.ts`, `moderation.controller.ts`
- `src/account/account.use-case.ts` — login recusa conta banida
- `src/index.ts` — rotas de moderação
- Migration Drizzle aditiva

## O que NÃO muda

- **Unreal:** nenhuma linha. O cliente reportando adulteração está fora de escopo por decisão da spec.
- **O módulo de pet:** nenhuma rota, nenhum comportamento.
- **AD-017 continua valendo como está:** a consequência de sessão (W.O.) não é substituída — o banimento **soma** a ela.

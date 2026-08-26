# Objetivo — Fechar o Roadmap (M3 → M7)

**Criado:** 2026-08-26, ao concluir M1 e M2.
**O que este documento é:** o critério de "pronto" para o projeto inteiro, e a ordem em que os marcos restantes se encaixam — não um cronograma com datas, e não uma spec. Cada marco continua ganhando `spec.md`/`design.md`/`tasks.md` próprios quando chegar a vez dele, no mesmo fluxo spec-driven já usado em M1/M2.

---

## Onde estamos

M1 (Fatia Vertical do Combate) e M2 (Combate Online) estão concluídos e verificados — ver `ROADMAP.md`. O jogo hoje: dois jogadores reais resolvem um combate 1v1 por turnos, às cegas, num tabuleiro 3x3, com 1 pet de cada lado, via código de sala. Isso prova o núcleo (a mecânica é divertida) e a espinha de rede (o mesmo código serve local e online).

O que falta é **conteúdo, progressão, escala e alcance** — os marcos que decidem se isto vira um jogo que alguém joga por mais de uma sessão, e se alcança alguém fora do PC do desenvolvedor.

## O que "roadmap completo" significa

O jogo tem, ao final de M7:

1. **Combate com profundidade real** — N pets por lado (não 1), tipos com fraquezas/resistências, arenas com propriedades, balanceado por dados e testado em lote, não no olho.
2. **Razão para voltar** — captura, coleção, progressão de pet (nível/experiência/evolução).
3. **O salto de escopo pago com os pés no chão** — mundo aberto contínuo, só depois do combate provado divertido (gate explícito, ver B-001).
4. **Alcance multiplataforma** — mobile no mínimo; console se o custo de licença/devkit se justificar.
5. **Identidade persistente** — conta de jogador, e a trilha de auditoria que M2 já deixou pronta (AD-017) virando banimento de verdade.

**Critério de sucesso do roadmap inteiro:** um jogador novo consegue instalar o jogo, jogar sozinho ou com um amigo, evoluir um time de pets ao longo de várias sessões, e (quando M5/M6 chegarem) fazer isso num mundo persistente, em pelo menos uma plataforma além de PC — tudo sem que qualquer marco anterior tenha regredido no caminho.

## Ordem e por quê

A ordem do `ROADMAP.md` já não é arbitrária — cada marco existe onde está porque desbloqueia ou é bloqueado por outro:

```
M3 (Conteúdo e Balanceamento)
   │  prova que N pets, tipos e arenas variadas
   │  não exigem reescrever o núcleo — só dados novos
   ▼
M4 (Progressão e Meta)
   │  dá ao jogador um motivo para voltar — mas só faz
   │  sentido sobre um combate já variado (M3), senão
   │  progressão vira "grind do mesmo turno sempre"
   ▼
┌─── GATE: B-001 — go/no-go consciente ───┐
│  M1-M3 concluídos + combate validado    │
│  como divertido (decisão humana, não    │
│  técnica) antes de abrir M5             │
└──────────────────────────────────────────┘
   ▼
M5 (Mundo Aberto Contínuo)
   │  o salto de escopo — 10x a 100x mais caro que
   │  o battler sozinho. Só se paga se M1-M4 já
   │  provaram que existe um jogo por trás
   ▼
M6 (Plataformas)
   │  exige orçamento de performance definido a partir
   │  de M5 (mundo aberto muda o teto de hardware) —
   │  não faz sentido portar antes do maior consumidor
   │  de recurso existir
   ▼
M7 (Contas e Moderação)
      identidade persistente é pré-requisito de
      QUALQUER coisa que precise sobreviver ao fim de
      uma sessão (ranking, amizades, progressão em
      nuvem, banimento real) — mas não bloqueia nenhum
      marco anterior; é consumidor do que eles produzem
```

**M7 não é o último por ser menos importante — é o último porque nada antes dele depende dele.** Poderia, em tese, entrar mais cedo (logo depois de M2, já que a trilha de auditoria existe desde AD-017); fica em M7 porque contas sem conteúdo (M3/M4) e sem mundo (M5) não têm o que proteger ainda. Se prioridade de produto mudar, M7 é o marco com menor custo de reordenar.

## Gates que já existem e continuam valendo

- **B-001** (`STATE.md`): M5 bloqueado até M1–M3 concluídos e o combate validado como divertido — decisão consciente de go/no-go no fim de M3, com o combate na mão. Este documento não muda esse gate, só o reafirma no contexto do objetivo maior.
- **B-004** (`STATE.md`): verificação de `DedicatedServer` real bloqueada pela engine instalada (builds do Epic Games Launcher não compilam `TargetType.Server`). Não bloqueia M3/M4 — só precisa ser resolvido (trocar para engine compilada da fonte) antes de M2 ir para produção real, o que pode acontecer em paralelo a qualquer marco de conteúdo.

## O que este documento NÃO faz

- Não specifica M3–M7 antecipadamente — cada marco ganha spec própria na hora, com o contexto real de quando chegar lá (o que se aprendeu nos marcos anteriores muda decisões, sempre mudou nesta sessão).
- Não compromete prazo — não há estimativa de tempo aqui, só ordem e dependência.
- Não é permissão para pular os portões de aprovação spec-driven (Specify → Design → Tasks → Execute) de cada feature — o objetivo dá a direção; a disciplina de execução continua a mesma de M1/M2.

## Próximo passo concreto

M3 — Conteúdo e Balanceamento. Primeira feature: **Escala de Pets e Skills** (tipos/fraquezas/resistências por dados, ferramenta de balanceamento em lote). Segue o mesmo fluxo: `spec.md` primeiro, aprovação, depois design.

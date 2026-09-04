// Copyright 2026 Anderson. All Rights Reserved.

import { index, integer, pgTable, timestamp, unique, uuid, varchar } from 'drizzle-orm/pg-core';

/**
 * A ARVORE CORTADA e uma EXCECAO — so o que difere da semente mora aqui.
 *
 * O mundo e COMPARTILHADO: a marca do corte tem de estar no servidor, nao no
 * save de quem cortou, senao o segundo jogador que passa ve a arvore que o
 * primeiro derrubou. Mas a tabela cresce SO com o que foi cortado, nunca com a
 * mata inteira — arvore intacta jamais grava linha (o contrapeso da MV3). A
 * base continua sendo a que `ForestBackdrop` planta da semente.
 *
 * O corte e carimbado com a IDADE DO MUNDO em que aconteceu (nao relogio de
 * parede): a rebrota e uma subtracao pura de idades contra o prazo (MV4), e o
 * servidor nao precisa de relogio proprio.
 */
export const worldTreeCuts = pgTable(
  'world_tree_cuts',
  {
    id: uuid('id').defaultRandom().primaryKey(),
    // ONDE: o pedaco do mundo e a celula local dentro dele — chave estavel da
    // arvore, sem depender de indice de instancia (que a mata reordena).
    chunkKey: varchar('chunk_key', { length: 64 }).notNull(),
    cellKey: varchar('cell_key', { length: 64 }).notNull(),
    // QUANDO, em idade do mundo (dias). A rebrota se decide so com isto.
    cutAtWorldAgeDays: integer('cut_at_world_age_days').notNull(),
    createdAt: timestamp('created_at', { withTimezone: true }).notNull().defaultNow(),
  },
  (table) => [
    // Uma arvore cortada e uma marca so: recortar a mesma celula atualiza, nao
    // empilha. E a busca por pedaco e o caminho quente (ao entrar na regiao).
    unique('world_tree_cuts_place').on(table.chunkKey, table.cellKey),
    index('world_tree_cuts_by_chunk').on(table.chunkKey),
  ],
);

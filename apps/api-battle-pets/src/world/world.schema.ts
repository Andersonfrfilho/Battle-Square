// Copyright 2026 Anderson. All Rights Reserved.

import { pgTable, timestamp, uuid } from 'drizzle-orm/pg-core';

/**
 * A IDADE DO MUNDO mora numa data de nascimento, gravada UMA vez.
 *
 * A forma mais simples que atende "o tempo corre mesmo com ninguem jogando"
 * (decisao 33) nao e um contador que avanca — e uma data fixa, e a idade e
 * sempre "agora menos essa data". Subtracao pura, sem processo tickando, sem
 * estado para divergir entre dois clientes. O servidor e dono do relogio: o
 * "agora" e o dele, nunca o do cliente.
 *
 * Uma unica linha existe nesta tabela — o mundo nasce uma vez. A primeira
 * leitura semeia a data; as seguintes so a leem.
 */
export const worldState = pgTable('world_state', {
  id: uuid('id').defaultRandom().primaryKey(),
  bornAt: timestamp('born_at', { withTimezone: true }).notNull().defaultNow(),
});

// Copyright 2026 Anderson. All Rights Reserved.

import type { Element } from './model-mapping.pure';

/**
 * O que o NOME nao disse e alguem decidiu — por FAMILIA, com o motivo.
 *
 * O gerador continua sem chutar (invariante 1): cada linha aqui e uma decisao
 * registrada, do usuario ou provisoria do gerador a pedido dele, e o
 * classificador a aplica POR CIMA da pista do nome. Provisoria quer dizer que
 * o usuario disse "decide por enquanto": fica escrito quem decidiu, para a
 * troca depois ser barata.
 */
export type DecidedBy = 'usuario' | 'provisoria';

export type ElementDecision = {
  readonly familia: string;
  readonly elemento: Element;
  readonly decididoPor: DecidedBy;
  readonly motivo: string;
};

export const ELEMENT_DECISIONS: readonly ElementDecision[] = [
  // 04/09/2026 — decisoes do usuario
  { familia: 'Bee', elemento: 'Ar', decididoPor: 'usuario', motivo: 'inseto voa: e Ar. O nome sugeria Planta E Ar; o usuario desempatou' },
  { familia: 'Armabee', elemento: 'Ar', decididoPor: 'usuario', motivo: 'a mesma abelha, blindada — vale a decisao da Bee' },
  { familia: 'Cat', elemento: 'Comum', decididoPor: 'usuario', motivo: 'bicho sem elemento e do tipo Comum, criado a pedido do usuario' },
  { familia: 'Dog', elemento: 'Comum', decididoPor: 'usuario', motivo: 'bicho sem elemento e do tipo Comum' },
  { familia: 'Slime', elemento: 'Comum', decididoPor: 'usuario', motivo: 'gosma sem pista e Comum; a cor nao fala de elemento' },
  { familia: 'PinkBlob', elemento: 'Comum', decididoPor: 'usuario', motivo: 'blob sem pista e Comum' },
  { familia: 'GreenBlob', elemento: 'Comum', decididoPor: 'usuario', motivo: 'blob sem pista e Comum; verde aqui e cor, nao Planta' },
  { familia: 'GreenSpikyBlob', elemento: 'Comum', decididoPor: 'usuario', motivo: 'blob sem pista e Comum' },
  // 04/09/2026 — provisorias do gerador, a pedido ("Ninja, Alien, Tribal vc decide por enquanto")
  { familia: 'Alien', elemento: 'Raio', decididoPor: 'provisoria', motivo: 'energia de outro mundo; Raio so tinha o Hywirl, e o Alien traz cadeia de tres estagios' },
  { familia: 'Ninja', elemento: 'Fantasma', decididoPor: 'provisoria', motivo: 'sombra e furtividade sao a escola do Fantasma (camuflar, atravessar)' },
  { familia: 'Tribal', elemento: 'Terra', decididoPor: 'provisoria', motivo: 'mascara e tambor: povo da terra; Terra e o elemento com mais pets sem cadeia' },
];

export function elementDecisionFor(familia: string): ElementDecision | undefined {
  return ELEMENT_DECISIONS.find((d) => d.familia === familia);
}

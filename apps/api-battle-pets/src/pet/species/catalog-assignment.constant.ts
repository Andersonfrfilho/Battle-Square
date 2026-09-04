// Copyright 2026 Anderson. All Rights Reserved.

/**
 * AR4c — quem veste o que: cada pet de PET_CATALOG_SEED e uma FAMILIA do insumo.
 *
 * A escolha e CURADA, nao sorteada: o gerador so garante que o elemento bate e
 * que ninguem veste a mesma familia duas vezes (`catalog-matching.pure.ts`
 * reprova o resto). O motivo fica escrito porque a proxima pessoa vai
 * perguntar "por que o Cirro e um morcego?" — e a resposta nao deve estar
 * so na cabeca de quem escolheu.
 *
 * Pet SEM familia e resposta valida (invariante 1): registra-se o porque, e
 * nunca se chuta um modelo de outro elemento para preencher a lacuna.
 *
 * Toda familia vestida precisa de forma ADULTO: `EPetGrowthStage` nasce em
 * Adulto (PetMorphology.h), e uma cadeia so de Blob e Big deixaria o pet sem
 * corpo no estagio em que ele existe. Medido: Fish, Birb e Cactoro sao assim.
 */
export type PetModelAssignment = {
  readonly pet: string;
  /** Familia do insumo (`familyOf`). Ausente = pet sem modelo, por enquanto. */
  readonly familia?: string;
  /** Pele especifica para o Adulto quando a familia tem mais de uma (`Bat_CM`). */
  readonly adulto?: string;
  readonly motivo: string;
};

export const PET_MODEL_ASSIGNMENTS: readonly PetModelAssignment[] = [
  // Fogo (4 pets, 5 familias)
  { pet: 'Faísca', familia: 'Dragon', motivo: 'o dragao e a cadeia assinada pelo autor; o pet inicial de fogo merece a evolucao completa' },
  { pet: 'Cinza', familia: 'Demon', motivo: 'brasa que sobra: o demonio pequeno cresce para Demon_Big pelo porte' },
  { pet: 'Alfarrábio', familia: 'BlueDemon', motivo: 'fogo psiquico, chama azul — a cor decide' },
  { pet: 'Fornalha', familia: 'YellowDragon', motivo: 'fogo fisico e pesado; o dragao amarelo e o corpo mais parrudo do pack' },
  // Planta (4 pets, 3 familias + decisao pendente)
  { pet: 'Brisa', familia: 'Deer', motivo: 'planta natural: o cervo da mata. Cactoro seria a cadeia, mas so tem Blob e Big, sem Adulto' },
  { pet: 'Musgo', familia: 'Mushnub', motivo: 'fungo rasteiro, cadeia assinada pelo autor' },
  { pet: 'Névoa', familia: 'Mushroom', motivo: 'planta psiquica: o cogumelo que vira rei (MushroomKing)' },
  { pet: 'Zunido', motivo: 'e a abelha, mas Bee sugere Planta E Ar e Armabee nao sugere nada: aguarda a decisao do usuario' },
  // Agua (5 pets, 5 familias)
  { pet: 'Maré', familia: 'Glub', motivo: 'agua inicial, cadeia assinada pelo autor' },
  { pet: 'Corrente', familia: 'Penguin', motivo: 'nadador de correnteza fria' },
  { pet: 'Vigília', familia: 'Squidle', motivo: 'agua psiquica: a lula, olhos grandes' },
  { pet: 'Vagalhão', familia: 'Shark', adulto: 'Shark_CF', motivo: 'agua fisica: o tubarao. O Fish do Monsters so tem Blob e Big, sem Adulto — e o pet nasce Adulto' },
  { pet: 'Afogado', familia: 'Anglerfish', motivo: 'agua espiritual: o peixe do fundo com a luz na cabeca' },
  // Terra (3 pets, 3 familias)
  { pet: 'Estalagmite', familia: 'Goleling', motivo: 'terra fisica, o golem com cadeia assinada pelo autor' },
  { pet: 'Barro', familia: 'Pig', motivo: 'terra natural: o porco no barro' },
  { pet: 'Menir', familia: 'Cyclops', motivo: 'terra psiquica: a pedra em pe com um olho' },
  // Fantasma (2 pets, 2 familias)
  { pet: 'Véu', familia: 'Ghost', motivo: 'o fantasma classico; Ghost_Cute e Ghost_Skull ficam como variantes' },
  { pet: 'Bruma', familia: 'Skull', motivo: 'fantasma natural: a caveira solta na neblina' },
  // Luz (3 pets, 1 familia)
  { pet: 'Lume', familia: 'Wizard', motivo: 'luz psiquica: o unico modelo de Luz do insumo' },
  { pet: 'Candeia', motivo: 'Luz so tem Wizard no insumo, e ele ja veste o Lume: falta pack de Luz' },
  { pet: 'Farol', motivo: 'idem Candeia — falta pack de Luz' },
  // Ar (2 pets, 2 familias)
  { pet: 'Rajada', familia: 'Pigeon', motivo: 'ar natural: o pombo voando e o Adulto, o Blob e o filhote. Birb nao tem Adulto (so Blob e Big)' },
  { pet: 'Cirro', familia: 'Bat', adulto: 'Bat_CM', motivo: 'ar psiquico; a pele do Cute Monsters e a "veia Pokemon" da decisao 70' },
];

/** Opcoes que o GOAL ja sugeriu para as cadeias sem elemento; o resto pergunta aberto. */
export const SUGGESTED_ELEMENT_OPTIONS: Readonly<Record<string, string>> = {
  Alpaking: 'Terra ou Planta',
  Armabee: 'Ar ou Planta',
};

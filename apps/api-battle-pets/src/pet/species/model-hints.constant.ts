// Copyright 2026 Anderson. All Rights Reserved.

import type { Element, Stage } from './model-mapping.pure';

/**
 * As PISTAS do gerador, separadas do algoritmo para o algoritmo caber numa
 * leitura. Cada lista abaixo e dado medido em nome de arquivo real; nenhuma
 * fala de bioma (L-032: bioma->elemento mora no C++).
 */

/** Pistas de NOME por elemento. Minusculas; casam por token (igual ou prefixo). */
export const ELEMENT_HINTS: ReadonlyArray<readonly [Element, readonly string[]]> = [
  ['Fogo',     ['flame', 'fire', 'lava', 'magma', 'ember', 'burn', 'inferno', 'demon', 'imp',
                'dragon']],
  ['Agua',     ['water', 'aqua', 'fish', 'shark', 'squid', 'squidle', 'crab', 'wave', 'sea',
                'octo', 'frog', 'glub', 'penguin', 'turtle']],
  ['Planta',   ['plant', 'leaf', 'vine', 'flower', 'mush', 'fung', 'tree', 'forest', 'ent',
                'moss', 'cact', 'bee', 'deer']],
  ['Terra',    ['rock', 'stone', 'golem', 'gole', 'earth', 'sand', 'crystal', 'dino', 'raptor',
                'rex', 'orc', 'cyclops', 'yeti', 'panda', 'pig', 'bunny', 'monk']],
  ['Fantasma', ['ghost', 'spirit', 'skele', 'skull', 'undead', 'zombie', 'wraith', 'phantom',
                'bone', 'reaper', 'cthulhu']],
  ['Luz',      ['light', 'holy', 'angel', 'radiant', 'sun', 'star', 'divine', 'cleric',
                'wizard']],
  ['Ar',       ['air', 'wind', 'bird', 'birb', 'wing', 'sky', 'cloud', 'bat', 'harpy',
                'gryph', 'pigeon', 'chicken', 'bee']],
  ['Raio',     ['thunder', 'lightning', 'shock', 'spark', 'volt', 'storm', 'electric',
                'hywirl']],
];

/**
 * Pista com ate esta quantidade de letras casa so o TOKEN INTEIRO. Medido:
 * `pig` (Terra) por prefixo engolia `Pigeon` (Ar) e o pombo virava conflito.
 * Pista curta por prefixo e pista que casa qualquer coisa.
 */
export const SHORT_HINT_LENGTH = 3;

/** Palavras de PORTE que o autor pos no nome ("BabyDragon", "MushroomKing"). */
export const STAGE_HINTS: ReadonlyArray<readonly [Stage, readonly string[]]> = [
  ['Filhote',  ['baby', 'small', 'mini', 'little', 'young', 'tiny', 'cub']],
  ['Evoluido', ['king', 'giant', 'alpha', 'elder', 'greater', 'lord', 'ancient', 'boss', 'large']],
];

/**
 * O sufixo que o autor usa para a forma evoluida (Alpaking/Alpaking_Evolved).
 * Convencao explicita, nao heuristica — por isso ganha de tudo.
 */
export const EVOLVED_SUFFIX = /_evolved$/i;

/**
 * MARCADORES DE CORPO do Ultimate Monsters, medidos no import: a mesma familia
 * vem em varios corpos — `Alien`, `Alien_Blob`, `Alien_Big`, `Alien_Tall`.
 * Sao sufixos de FORMA, nao bichos novos; saem da familia. `_Blob` e o corpo
 * pequeno e `_Big` o grande, e so esses dois dizem porte.
 */
export const BODY_MARKERS = ['Big', 'Blob', 'Flying', 'Tall', 'Skull'] as const;
export const BODY_STAGE_BLOB = /_blob$/i;
export const BODY_STAGE_BIG = /_big$/i;

/**
 * MARCADORES DE PACOTE que a trilha A pos para desambiguar nome repetido entre
 * packs (`Bat_AM`/`Bat_CM`, `Shark_AF`/`Shark_CF`, `Dragon_Mon`, `Ghost_Cute`).
 * Sao o mesmo bicho de outro autor — mesma familia, mesmo estagio, outra pele.
 */
export const PACK_MARKERS = ['AM', 'CM', 'AF', 'CF', 'Cute', 'Mon'] as const;

/** Prefixo de malha esqueletal da Unreal — convencao de import, nao de nome. */
export const MODEL_PREFIX = /^SK_/;

/**
 * Tokens que denunciam que o arquivo NAO e criatura: rig, prop, cenario.
 *
 * Medido: o Cute Fish Pack traz `Lure_1..6`, `Dock_Long`, `Boat` e o `Worm`
 * (isca da vara, importado junto com os peixes); os packs de personagem trazem
 * `Rig_Medium_General`. Pedir ao humano que classificasse uma DOCA como bicho
 * mataria o relatorio.
 */
export const NON_CREATURE_TOKENS = [
  'rig', 'lure', 'dock', 'boat', 'prop', 'anim', 'skeleton_rig', 'socket',
  'rod', 'fishingrod', 'crate', 'barrel', 'chest', 'worm',
] as const;

/**
 * Classes de aventureiro humano (KayKit Adventurers). Pet e criatura com poder
 * (decisao 69); um cavaleiro e gente da vila, nao pet. Um esqueleto-mago
 * continua criatura: o morto-vivo ganha da classe.
 */
export const HUMAN_CLASS_TOKENS = ['barbarian', 'knight', 'mage', 'ranger', 'rogue'] as const;

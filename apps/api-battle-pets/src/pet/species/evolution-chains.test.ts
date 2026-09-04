// Copyright 2026 Anderson. All Rights Reserved.

import { describe, expect, test } from 'bun:test';

import { buildAssetChains, chainsByOrigin, groupAssetsByFamily, type AssetFamily } from './evolution-chains.pure';
import { IMPORTED_ASSETS_PATH, classifyImportedAssets, loadImportedAssets } from './imported-assets.pure';

const file = loadImportedAssets(await Bun.file(IMPORTED_ASSETS_PATH).text());
const assets = classifyImportedAssets(file.malhas);
const chains = buildAssetChains(assets);
const byFamily = new Map(chains.map((c) => [c.family, c]));

function chain(family: string): AssetFamily {
  const found = byFamily.get(family);
  if (!found) throw new Error(`${family} nao e cadeia`);
  return found;
}

describe('as cadeias que o AUTOR assinou', () => {
  test('as seis do Ultimate Monsters, pelo sufixo _Evolved', () => {
    for (const family of ['Dragon', 'Alpaking', 'Armabee', 'Glub', 'Goleling', 'Mushnub']) {
      expect(chain(family).origem).toBe('autor');
      expect(chain(family).estagios.Adulto).toBe(`/Game/Quaternius/Monsters/SK_${family}`);
      expect(chain(family).estagios.Evoluido).toBe(`/Game/Quaternius/Monsters/SK_${family}_Evolved`);
    }
  });

  test('o pack novo trouxe Mushroom -> MushroomKing', () => {
    expect(chain('Mushroom')).toMatchObject({
      origem: 'autor',
      element: 'Planta',
      estagios: {
        Adulto: '/Game/Quaternius/Monsters/SK_Mushroom',
        Evoluido: '/Game/Quaternius/Monsters/SK_MushroomKing',
      },
    });
  });

  test('sao exatamente sete', () => {
    expect(chainsByOrigin(chains, 'autor').map((c) => c.family).sort()).toEqual(
      ['Alpaking', 'Armabee', 'Dragon', 'Glub', 'Goleling', 'Mushnub', 'Mushroom'],
    );
  });

  test('SK_Dragon_Mon e variante do Dragon adulto, nao um terceiro estagio', () => {
    expect(chain('Dragon').variantes).toEqual(['/Game/Quaternius/Monsters/SK_Dragon_Mon']);
  });
});

describe('as cadeias que so o PORTE sugere — leitura nossa; o elemento veio de decisao', () => {
  test('Alien tem tres corpos em tres estagios e o Tall como variante; Raio e provisorio (04/09)', () => {
    expect(chain('Alien')).toMatchObject({
      origem: 'porte',
      element: 'Raio',
      estagios: {
        Filhote: '/Game/Quaternius/Monsters/SK_Alien_Blob',
        Adulto: '/Game/Quaternius/Monsters/SK_Alien',
        Evoluido: '/Game/Quaternius/Monsters/SK_Alien_Big',
      },
      variantes: ['/Game/Quaternius/Monsters/SK_Alien_Tall'],
    });
  });

  test('Cactoro e Planta com Blob -> Big', () => {
    expect(chain('Cactoro')).toMatchObject({ origem: 'porte', element: 'Planta' });
    expect(Object.keys(chain('Cactoro').estagios).sort()).toEqual(['Evoluido', 'Filhote']);
  });

  test('sao exatamente dez', () => {
    expect(chainsByOrigin(chains, 'porte').map((c) => c.family).sort()).toEqual(
      ['Alien', 'Birb', 'Cactoro', 'Demon', 'Fish', 'Ninja', 'Orc', 'Pigeon', 'Tribal', 'Yeti'],
    );
  });
});

describe('um modelo sozinho nao e evolucao', () => {
  test('SK_Bat_AM e SK_Bat_CM sao duas peles do mesmo adulto, sem cadeia', () => {
    expect(byFamily.has('Bat')).toBe(false);
    const bat = groupAssetsByFamily(assets).find((f) => f.family === 'Bat');
    expect(bat?.variantes).toEqual(['/Game/Quaternius/Monsters/SK_Bat_CM']);
  });

  test('SK_Chicken_Cute e SK_Ghost_Skull sao variantes, nao estagios', () => {
    expect(byFamily.has('Chicken')).toBe(false);
    expect(byFamily.has('Ghost')).toBe(false);
  });

  test('Squidle e os 42 peixes ficam fora, menos a familia Fish (Blob -> Big)', () => {
    expect(byFamily.has('Squidle')).toBe(false);
    expect(byFamily.has('Clownfish')).toBe(false);
    expect(chain('Fish').element).toBe('Agua');
  });

  test('humanos e props nunca formam familia', () => {
    const families = groupAssetsByFamily(assets).map((f) => f.family);
    expect(families).not.toContain('Knight');
    expect(families).not.toContain('Worm');
  });

  test('deterministico: mesma entrada, mesma saida', () => {
    expect(buildAssetChains(assets)).toEqual(chains);
  });
});

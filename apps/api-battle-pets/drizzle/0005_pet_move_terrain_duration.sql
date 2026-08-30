-- fundura-e-requisito-de-terreno, fatia 2: o gelo, e o terreno que PASSA.
--
-- ADITIVA com DEFAULT 0, e o zero e PARA SEMPRE: o alagamento nunca voltou
-- atras, e todo golpe ja cadastrado continua exatamente como era. Fosse zero
-- "some na hora", esta coluna apagaria em silencio o efeito de terreno de
-- todo golpe que existe.
ALTER TABLE "pet_moves"
  ADD COLUMN IF NOT EXISTS "terrain_duration" integer NOT NULL DEFAULT 0;

-- O teto e o mesmo do tradutor no jogo, e ele recorta de novo la: amarra de
-- jogo nao e acordo entre camadas. Gelo que atravessa varios turnos deixa de
-- ser jogada e vira mudanca de arena, e quem decide arena e a montagem.
ALTER TABLE "pet_moves"
  ADD CONSTRAINT "pet_moves_terrain_duration_range"
  CHECK ("terrain_duration" >= 0 AND "terrain_duration" <= 5);

-- A poca e o gelo entram no conjunto permitido. Sem isto o golpe de gelo e
-- RECUSADO na escrita, e o defeito aparece longe daqui — no cadastro, nao no
-- combate, que e onde ninguem procuraria.
ALTER TABLE "pet_moves"
  DROP CONSTRAINT IF EXISTS "pet_moves_terrain_effect_valid";

ALTER TABLE "pet_moves"
  ADD CONSTRAINT "pet_moves_terrain_effect_valid"
  CHECK ("terrain_effect" IN ('none', 'water', 'damage', 'shallow_water', 'ice'));

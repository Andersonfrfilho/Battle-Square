-- Lama como efeito de golpe: pet de TERRA e de AGUA pode criar.
--
-- Ate aqui a lama so nascia da agua secando, e nenhum jogador tinha como
-- provoca-la. Regra implementada que ninguem alcanca e o defeito que este
-- projeto ja registrou mais de uma vez.
ALTER TABLE "pet_moves"
  DROP CONSTRAINT IF EXISTS "pet_moves_terrain_effect_valid";

ALTER TABLE "pet_moves"
  ADD CONSTRAINT "pet_moves_terrain_effect_valid"
  CHECK ("terrain_effect" IN ('none', 'water', 'damage', 'shallow_water', 'ice', 'mud'));

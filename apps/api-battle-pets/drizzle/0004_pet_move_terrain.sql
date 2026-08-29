-- golpes-por-pet, fatia 4: o golpe DEIXA algo na casa que acertou.
--
-- ADITIVA com DEFAULT: golpe ja cadastrado passa a valer 'none' — nao muda a
-- casa, que e exatamente como ele se comportava antes de a coluna existir.
ALTER TABLE "pet_moves"
  ADD COLUMN IF NOT EXISTS "terrain_effect" varchar(16) NOT NULL DEFAULT 'none';

-- O conjunto de valores e REGRA, nao convencao: um efeito desconhecido faria o
-- nucleo escolher entre ignorar em silencio ou recusar a batalha, e as duas
-- saidas sao piores que barrar na escrita.
ALTER TABLE "pet_moves"
  ADD CONSTRAINT "pet_moves_terrain_effect_valid"
  CHECK ("terrain_effect" IN ('none', 'water', 'damage'));

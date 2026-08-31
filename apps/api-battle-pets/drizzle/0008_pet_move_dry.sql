-- SECAR entra no conjunto permitido de efeito de terreno.
--
-- Ela e EFEITO, e nao terreno: nunca fica numa casa, e apaga agua, poca, lama
-- e gelo de uma vez. Existe porque o campo so ficava MAIS molhado ou
-- congelava, e nada o secava de proposito — quem alagava tinha vantagem sem
-- resposta.
--
-- Sem esta linha o golpe de secar e RECUSADO na escrita, e o defeito aparece
-- no cadastro, longe de onde alguem procuraria.
ALTER TABLE "pet_moves"
  DROP CONSTRAINT IF EXISTS "pet_moves_terrain_effect_valid";

ALTER TABLE "pet_moves"
  ADD CONSTRAINT "pet_moves_terrain_effect_valid"
  CHECK ("terrain_effect" IN ('none', 'water', 'damage', 'shallow_water', 'ice', 'mud', 'dry'));

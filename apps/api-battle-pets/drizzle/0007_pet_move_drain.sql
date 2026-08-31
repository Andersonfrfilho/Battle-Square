-- Drenar: parte do dano volta como vida para quem bateu.
--
-- ADITIVA com DEFAULT 0 — o golpe de sempre nao devolve nada, e todo golpe ja
-- cadastrado continua exatamente como era.
ALTER TABLE "pet_moves"
  ADD COLUMN IF NOT EXISTS "drain_percent" integer NOT NULL DEFAULT 0;

-- Teto de 100, o mesmo do tradutor no jogo, e ele recorta de novo la: amarra
-- de jogo nao e acordo entre camadas. Devolver MAIS vida que o dano causado
-- faria um golpe fraco em alvo defendido render mais que um forte.
ALTER TABLE "pet_moves"
  ADD CONSTRAINT "pet_moves_drain_percent_range"
  CHECK ("drain_percent" >= 0 AND "drain_percent" <= 100);

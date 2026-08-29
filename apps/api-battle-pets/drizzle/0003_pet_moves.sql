-- golpes-por-pet (DP-golpe-01/02): quatro golpes por pet, vindos do backend.
--
-- ADITIVA: pet existente fica sem golpe e continua valendo. Exigir quatro de
-- imediato quebraria todo registro ja cadastrado, e o espelho pararia de
-- sincronizar por causa de dado que ninguem tinha como ter preenchido.
CREATE TABLE IF NOT EXISTS "pet_moves" (
  "id" uuid PRIMARY KEY DEFAULT gen_random_uuid() NOT NULL,
  "pet_id" uuid NOT NULL,
  "slot" integer NOT NULL,
  "name" varchar(60) NOT NULL,
  "power" integer NOT NULL,
  CONSTRAINT "pet_moves_pet_slot_unique" UNIQUE("pet_id","slot")
);

-- ON DELETE CASCADE: golpe sem pet nao e dado, e orfao no espelho viraria
-- payload assinado que nao corresponde a registro nenhum.
ALTER TABLE "pet_moves"
  ADD CONSTRAINT "pet_moves_pet_id_pets_id_fk"
  FOREIGN KEY ("pet_id") REFERENCES "pets"("id") ON DELETE CASCADE;

-- O slot e 0..3 e isso e REGRA, nao convencao: o indice viaja no commit
-- (DP-golpe-04), e um slot 7 produziria uma jogada que o jogo nao sabe montar.
ALTER TABLE "pet_moves"
  ADD CONSTRAINT "pet_moves_slot_range" CHECK ("slot" >= 0 AND "slot" <= 3);

-- Busca por pet e o unico acesso: o espelho le todos os golpes de um pet.
CREATE INDEX IF NOT EXISTS "pet_moves_pet_id_idx" ON "pet_moves" ("pet_id");

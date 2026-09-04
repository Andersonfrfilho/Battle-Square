ALTER TABLE "owned_pets" ADD COLUMN IF NOT EXISTS "genesis_world_age_days" integer DEFAULT 0 NOT NULL;

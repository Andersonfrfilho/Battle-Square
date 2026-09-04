CREATE TABLE IF NOT EXISTS "world_state" (
	"id" uuid PRIMARY KEY DEFAULT gen_random_uuid() NOT NULL,
	"born_at" timestamp with time zone DEFAULT now() NOT NULL
);

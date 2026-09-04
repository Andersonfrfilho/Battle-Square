CREATE TABLE IF NOT EXISTS "world_tree_cuts" (
	"id" uuid PRIMARY KEY DEFAULT gen_random_uuid() NOT NULL,
	"chunk_key" varchar(64) NOT NULL,
	"cell_key" varchar(64) NOT NULL,
	"cut_at_world_age_days" integer NOT NULL,
	"created_at" timestamp with time zone DEFAULT now() NOT NULL,
	CONSTRAINT "world_tree_cuts_place" UNIQUE("chunk_key","cell_key")
);
--> statement-breakpoint
CREATE INDEX IF NOT EXISTS "world_tree_cuts_by_chunk" ON "world_tree_cuts" ("chunk_key");

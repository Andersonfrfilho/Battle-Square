-- CR3 (crime-e-recompensa): a trilha de posse, ADITIVA — nada de 0000-0009 muda.
-- A mao, como as anteriores (o gerador do drizzle esta inutilizavel desde 0003).
-- So IDs opacos: nenhuma coluna de PII, por construcao (invariante 17).
CREATE TABLE "ownership_audit_log" (
	"id" uuid PRIMARY KEY DEFAULT gen_random_uuid() NOT NULL,
	"pet_id" uuid NOT NULL,
	"action" varchar(16) NOT NULL,
	"from_account_id" uuid NOT NULL,
	"to_account_id" uuid NOT NULL,
	"created_at" timestamp with time zone DEFAULT now() NOT NULL
);
--> statement-breakpoint
CREATE INDEX "ownership_audit_pet_idx" ON "ownership_audit_log" USING btree ("pet_id");

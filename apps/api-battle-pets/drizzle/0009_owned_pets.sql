-- PS1 (posse-no-servidor): a tabela da posse, ADITIVA — nada de 0000-0008 muda.
--
-- A MAO, como 0003-0008: o diario do drizzle parou em 0002 (as migrations de
-- golpes foram escritas por fora dele), entao o gerador acha que pet_moves nao
-- existe e tenta recria-la junto de qualquer coisa nova. Ate o diario ser
-- reconciliado, migration nova e SQL manual — e esta e exatamente o pedaco de
-- owned_pets que o gerador produziu, sem a bagagem.
CREATE TABLE "owned_pets" (
	"id" uuid PRIMARY KEY DEFAULT gen_random_uuid() NOT NULL,
	"owner_account_id" uuid NOT NULL,
	"catalog_id" varchar(64) NOT NULL,
	"experience" integer DEFAULT 0 NOT NULL,
	"musculature" integer DEFAULT 0 NOT NULL,
	"personality" integer DEFAULT 0 NOT NULL,
	"acquisition" varchar(16) DEFAULT 'captured' NOT NULL,
	"stolen_from_account_id" uuid,
	"created_at" timestamp with time zone DEFAULT now() NOT NULL,
	"updated_at" timestamp with time zone DEFAULT now() NOT NULL,
	-- Um dono por instancia e obvio; o que ESTA unicidade impede e o mesmo
	-- jogador ter o mesmo pet do catalogo duas vezes por escrita repetida.
	-- Sem ela, a idempotencia da captura dependeria do CLIENTE — que e o que
	-- uma posse de servidor existe para nao fazer.
	CONSTRAINT "owned_pets_owner_catalog_unique" UNIQUE("owner_account_id","catalog_id")
);
--> statement-breakpoint
ALTER TABLE "owned_pets" ADD CONSTRAINT "owned_pets_owner_account_id_player_accounts_id_fk" FOREIGN KEY ("owner_account_id") REFERENCES "public"."player_accounts"("id") ON DELETE cascade ON UPDATE no action;
--> statement-breakpoint
ALTER TABLE "owned_pets" ADD CONSTRAINT "owned_pets_stolen_from_account_id_player_accounts_id_fk" FOREIGN KEY ("stolen_from_account_id") REFERENCES "public"."player_accounts"("id") ON DELETE set null ON UPDATE no action;
--> statement-breakpoint
CREATE INDEX "owned_pets_owner_idx" ON "owned_pets" USING btree ("owner_account_id");

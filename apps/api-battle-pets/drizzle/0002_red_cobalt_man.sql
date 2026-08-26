CREATE TABLE "account_bans" (
	"id" uuid PRIMARY KEY DEFAULT gen_random_uuid() NOT NULL,
	"account_id" uuid NOT NULL,
	"reason" varchar(500) NOT NULL,
	"expires_at" timestamp with time zone,
	"created_by" varchar(20) NOT NULL,
	"created_at" timestamp with time zone DEFAULT now() NOT NULL,
	"lifted_at" timestamp with time zone,
	"lifted_by" varchar(20)
);
--> statement-breakpoint
CREATE TABLE "moderation_events" (
	"id" uuid PRIMARY KEY DEFAULT gen_random_uuid() NOT NULL,
	"account_id" uuid NOT NULL,
	"type" varchar(40) NOT NULL,
	"detail" text,
	"recorded_by" varchar(20) NOT NULL,
	"created_at" timestamp with time zone DEFAULT now() NOT NULL
);
--> statement-breakpoint
ALTER TABLE "account_bans" ADD CONSTRAINT "account_bans_account_id_player_accounts_id_fk" FOREIGN KEY ("account_id") REFERENCES "public"."player_accounts"("id") ON DELETE cascade ON UPDATE no action;--> statement-breakpoint
ALTER TABLE "moderation_events" ADD CONSTRAINT "moderation_events_account_id_player_accounts_id_fk" FOREIGN KEY ("account_id") REFERENCES "public"."player_accounts"("id") ON DELETE cascade ON UPDATE no action;--> statement-breakpoint
CREATE INDEX "account_bans_account_id_idx" ON "account_bans" USING btree ("account_id");--> statement-breakpoint
CREATE INDEX "moderation_events_account_id_idx" ON "moderation_events" USING btree ("account_id");
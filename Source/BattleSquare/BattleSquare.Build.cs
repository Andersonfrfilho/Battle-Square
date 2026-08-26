// Copyright 2026 Anderson. All Rights Reserved.

using UnrealBuildTool;

public class BattleSquare : ModuleRules
{
	public BattleSquare(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"GameplayTags",
			"BattleSim",
			// AD-019 revisada em 2026-08-26: o SQLite vem do plugin SQLiteCore
			// da engine, que expõe a API C de propósito
			// (SQLITE_API=SQLITECORE_API). Antes havia um módulo barreira
			// compilando a própria cópia de sqlite3.c — o que passava no
			// Editor (um dylib por módulo) e QUEBRAVA o build empacotado, que
			// é monolítico: 270 símbolos duplicados contra os do SQLiteCore.
			"SQLiteCore",
			// T11/T12 (apresentacao-combate): classes base C++ de widget —
			// só a classe, sem layout UMG (ver design.md, Limite de
			// Ferramenta).
			"UMG",
			"Slate",
			"SlateCore",
			// T2 (escala-pets-skills): FTypeEffectivenessTable::LoadFromJson
			// consome a tabela de efetividade de um arquivo JSON local.
			"Json",
			"JsonUtilities"
		});

		// AD-019: OpenSSL já vem empacotado na engine (1.1.1t) — cobre
		// AES-256-GCM (decifrar o espelho) e Ed25519 (verificar
		// assinatura). Padrão preferido da skill unreal-thirdparty:
		// consumir o que a engine já tem, não vendorizar de novo.
		AddEngineThirdPartyPrivateStaticDependencies(Target, "OpenSSL");
	}
}

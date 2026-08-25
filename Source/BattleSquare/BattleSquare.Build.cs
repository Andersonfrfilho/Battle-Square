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
			// AD-019: SQLite isolado em módulo barreira (regras de warning
			// próprias — ver SQLiteLibrary.Build.cs) para ler o espelho
			// local de pets.
			"SQLiteLibrary",
			// T11/T12 (apresentacao-combate): classes base C++ de widget —
			// só a classe, sem layout UMG (ver design.md, Limite de
			// Ferramenta).
			"UMG",
			"Slate",
			"SlateCore"
		});

		// AD-019: OpenSSL já vem empacotado na engine (1.1.1t) — cobre
		// AES-256-GCM (decifrar o espelho) e Ed25519 (verificar
		// assinatura). Padrão preferido da skill unreal-thirdparty:
		// consumir o que a engine já tem, não vendorizar de novo.
		AddEngineThirdPartyPrivateStaticDependencies(Target, "OpenSSL");
	}
}

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
			"BattleSim"
		});
	}
}

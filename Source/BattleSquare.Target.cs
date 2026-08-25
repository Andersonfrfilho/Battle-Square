// Copyright 2026 Anderson. All Rights Reserved.

using UnrealBuildTool;

public class BattleSquareTarget : TargetRules
{
	public BattleSquareTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.AddRange(new string[] { "BattleSim", "BattleSquare" });
	}
}

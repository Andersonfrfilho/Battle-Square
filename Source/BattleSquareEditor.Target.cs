// Copyright 2026 Anderson. All Rights Reserved.

using UnrealBuildTool;

public class BattleSquareEditorTarget : TargetRules
{
	public BattleSquareEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.AddRange(new string[] { "BattleSim", "BattleSquare" });
	}
}

// Copyright 2026 Anderson. All Rights Reserved.

using UnrealBuildTool;

// Servidor dedicado — AD-005: a simulação é autoritativa no servidor.
// Existe desde já para que nada no jogo compile só porque há cliente presente.
public class BattleSquareServerTarget : TargetRules
{
	public BattleSquareServerTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Server;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.AddRange(new string[] { "BattleSim", "BattleSquare" });
	}
}

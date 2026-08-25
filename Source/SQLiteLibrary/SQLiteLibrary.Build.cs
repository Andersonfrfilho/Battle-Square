// Copyright 2026 Anderson. All Rights Reserved.

using UnrealBuildTool;

// Módulo barreira (padrão da skill unreal-thirdparty): isola o SQLite
// vendorizado (amalgamation de domínio público, sqlite.org, hash SHA3-256
// verificado no download) com suas próprias regras de warning relaxadas —
// código C legado de 20+ anos não segue o rigor de -Wundef da Unreal, e
// afrouxar isso no módulo consumidor (BattleSquare) seria jogar fora o
// rigor do NOSSO código para acomodar o de terceiros.
//
// AD-019: SQLite aqui é PURO — sem extensão de criptografia. O espelho
// local já é cifrado por fora (AES-256-GCM via OpenSSL, ver AD-018).
public class SQLiteLibrary : ModuleRules
{
	public SQLiteLibrary(ReadOnlyTargetRules Target) : base(Target)
	{
		Type = ModuleType.CPlusPlus;
		PCHUsage = PCHUsageMode.NoPCHs;

		CppCompileWarningSettings.UndefinedIdentifierWarningLevel = WarningLevel.Off;
		CppCompileWarningSettings.UnreachableCodeWarningLevel = WarningLevel.Off;

		PublicDefinitions.Add("SQLITE_OMIT_LOAD_EXTENSION=1");
		PublicDefinitions.Add("SQLITE_THREADSAFE=1");
		PublicDefinitions.Add("SQLITE_DQS=0");

		// sqlite3.h expande SQLITE_API para NADA por padrão. A Unreal
		// compila dylibs no Mac com símbolos ocultos por padrão
		// (-fvisibility=hidden) — sem isto, sqlite3_open/step/etc. compilam
		// mas ficam invisíveis para o módulo consumidor (BattleSquare)
		// linkar contra. Confirmado via "nm -gU" no dylib gerado: zero
		// símbolos exportados até esta correção.
		if (Target.IsInPlatformGroup(UnrealPlatformGroup.Apple) || Target.IsInPlatformGroup(UnrealPlatformGroup.Unix))
		{
			PublicDefinitions.Add("SQLITE_API=__attribute__((visibility(\"default\")))");
		}
	}
}

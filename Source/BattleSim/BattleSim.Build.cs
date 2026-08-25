// Copyright 2026 Anderson. All Rights Reserved.

using UnrealBuildTool;

// Núcleo de simulação de combate.
//
// Depende SOMENTE de Core e CoreUObject, e isso é a regra — não esquecimento.
// PROJECT.md exige que a resolução de turno não dependa de UWorld, AActor nem
// de tick, para ser testável fora do editor e reutilizável no servidor dedicado.
//
// ATENÇÃO ao acrescentar qualquer dependência aqui: dependência pública é
// transitiva. "GameplayTags", por exemplo, declara "Engine" em
// PublicDependencyModuleNames — adicioná-lo devolve a engine inteira a este
// módulo e desfaz a fronteira sem nenhum aviso. Foi verificado por experimento
// em 2026-08-25; ver AD-012 em .specs/project/STATE.md.
//
// Antes de incluir um módulo novo, rode a sonda negativa do AD-012 e confirme
// que ela ainda falha.
public class BattleSim : ModuleRules
{
	public BattleSim(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject"
		});
	}
}

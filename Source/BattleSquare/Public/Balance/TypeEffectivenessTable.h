// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

// T1–T2 (tasks.md, Escala de Pets e Skills): tabela de efetividade de
// tipo — dado puro, sem I/O durante consulta. design.md, DP-escala-01:
// isto NUNCA entra em BattleSim; é consultada aqui, em BattleSquare,
// para pré-multiplicar o Attack de um pet antes de FPetState existir.
//
// Par ausente = neutro (100%) — mesma filosofia de "ausência não é
// erro" de GetDirectionDelta (BattleTypes.h).
class BATTLESQUARE_API FTypeEffectivenessTable
{
public:
	// T2: carrega de um arquivo JSON. Retorna false (sem alterar
	// OutTable) se o arquivo estiver ausente ou malformado — mesmo
	// padrão de FPetDataLoader::LoadVerifiedPets: falha explícita,
	// nunca tabela vazia silenciosa.
	static bool LoadFromJson(const FString& FilePath, FTypeEffectivenessTable& OutTable);

	// T1: percentual de dano do Attacker contra o Defender. 100 se o
	// par não estiver cadastrado (neutro).
	int32 GetPercent(const FString& AttackerType, const FString& DefenderType) const;

	// Testável sem arquivo — usado por T1 e por quem monta uma tabela
	// em memória diretamente (testes, ferramentas).
	void SetPercent(const FString& AttackerType, const FString& DefenderType, int32 Percent);

	/**
	 * Compõe os DOIS eixos num percentual só.
	 *
	 * `escola × elemento ÷ 100`. Duas tabelas de quatro linhas cobrem doze
	 * tipos; a matriz equivalente teria cento e quarenta e quatro células, e
	 * ninguém equilibra o que não consegue ler de uma vez.
	 *
	 * Eixo desconhecido vale NEUTRO, não zero: um tipo que esta versão não
	 * conhece precisa lutar normalmente, não deixar de causar dano.
	 */
	int32 GetComposedPercent(const FString& AttackerType, const FString& DefenderType) const;

private:
	TMap<TPair<FString, FString>, int32> Entries;
	TMap<TPair<FString, FString>, int32> SchoolEntries;
	TMap<TPair<FString, FString>, int32> ElementEntries;
};

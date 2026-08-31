// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Misc/ConfigCacheIni.h"

namespace IlhaDeTeste
{
	/**
	 * Troca o BIOMA da ilha por um instante, e devolve o de antes.
	 *
	 * Existe porque a ilha deixou de ter seis setores e passou a ter UM bioma:
	 * "o ponto na geleira" não é mais uma direção, é uma ILHA de geleira. Teste
	 * que quer geleira agora pede uma ilha de geleira.
	 *
	 * Devolve o valor no fim: teste que deixa configuração suja contamina
	 * todos os que rodarem depois dele.
	 */
	struct FBiomaTemporario
	{
		explicit FBiomaTemporario(const TCHAR* Novo)
		{
			GConfig->GetString(TEXT("/Script/BattleSquare.BattleSquareGameMode"),
				TEXT("WorldIslandBiome"), Anterior, GGameIni);
			GConfig->SetString(TEXT("/Script/BattleSquare.BattleSquareGameMode"),
				TEXT("WorldIslandBiome"), Novo, GGameIni);
		}

		~FBiomaTemporario()
		{
			GConfig->SetString(TEXT("/Script/BattleSquare.BattleSquareGameMode"),
				TEXT("WorldIslandBiome"), *Anterior, GGameIni);
		}

		FString Anterior = TEXT("Forest");
	};
}

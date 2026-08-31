// Copyright 2026 Anderson. All Rights Reserved.

#include "World/EncounterMatchAssembler.h"
#include "Battle/BattleArena.h"
#include "Environment/IslandGeography.h"
#include "Environment/ScenaryClimate.h"
#include "Misc/Paths.h"
#include "Balance/TypeEffectivenessTable.h"
#include "Net/BattleSquareGameMode.h"
#include "Misc/DateTime.h"

namespace
{
	const FLoadedPetRecord* FindPetByCatalogId(const TArray<FLoadedPetRecord>& Pets, const FString& CatalogId)
	{
		return Pets.FindByPredicate([&CatalogId](const FLoadedPetRecord& Pet) { return Pet.Id == CatalogId; });
	}
}

bool FEncounterMatchAssembler::AssembleFromEncounter(const FEncounterMatchParams& Params,
	FBattleState& OutInitialState,
	TArray<FPetPresentationInfo>& OutPresentations)
{
	const FLoadedPetRecord* PlayerRecord = FindPetByCatalogId(Params.AvailablePets, Params.PlayerCatalogId);
	const FLoadedPetRecord* EncounterRecord = FindPetByCatalogId(Params.AvailablePets, Params.EncounterCatalogId);

	// CatalogId que não existe no espelho é erro de configuração do nível,
	// nunca um pet substituído em silêncio — mesma postura de ARENA-02.
	if (!PlayerRecord || !EncounterRecord)
	{
		return false;
	}

	FPetState PlayerPet;
	FPetPresentationInfo PlayerPresentation;
	FPetState EncounterPet;
	FPetPresentationInfo EncounterPresentation;

	// TranslateMatchup, e não TranslatePet: só ele aplica a efetividade de
	// tipo. Com TranslatePet, Fogo contra Planta batia igual a Fogo contra
	// Água — a tabela existia, era testada, e nunca chegava ao jogo.
	FTypeEffectivenessTable Efetividade;
	if (!FTypeEffectivenessTable::LoadFromJson(
		FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("TypeEffectiveness.json")), Efetividade))
	{
		// Tabela ausente degrada para NEUTRO — GetPercent devolve 100 para
		// par desconhecido, e uma tabela vazia é exatamente isso. Nunca
		// impede a batalha de acontecer.
		UE_LOG(LogTemp, Warning,
			TEXT("EncounterMatchAssembler: tabela de efetividade nao carregou — combate neutro"));
	}

	FBattleDataTranslator::TranslateMatchup(*PlayerRecord, *EncounterRecord, Efetividade,
		/*LeftPetId=*/1, /*RightPetId=*/2,
		PlayerPet, PlayerPresentation, EncounterPet, EncounterPresentation);

	ABattleSquareGameMode::ApplyOwnedPetProgressionBonus(Params.PetCollectionSlotName, PlayerPet, PlayerPresentation);
	ABattleSquareGameMode::ApplyOwnedPetProgressionBonus(Params.PetCollectionSlotName, EncounterPet, EncounterPresentation);

	OutInitialState = FBattleState();
	// A semente é decisão de MONTAGEM, não do núcleo: o BattleSim recebe um
	// estado já semeado e nunca consulta relógio nenhum.
	OutInitialState.Random.State = Params.RandomSeed != 0
		? Params.RandomSeed
		: static_cast<uint64>(FDateTime::Now().GetTicks());
	OutInitialState.Pets.Add(PlayerPet);
	OutInitialState.Pets.Add(EncounterPet);
	OutInitialState.PlaceDuelistsAtStartingCells();

	// A UMIDADE do lugar entra no estado: é ela que decide se a poça vira lama
	// ou seca. Vem do mesmo clima que põe neve na serra — dois números
	// diferentes sobre o mesmo lugar, e não duas ideias de clima.
	//
	// E o LUGAR é onde o encontro aconteceu, não uma linha de `.ini`. Quem
	// tropeça no inimigo dentro do deserto luta num campo seco; quem tropeça
	// na mata luta na lama. A mesma coordenada que monta o terreno responde
	// pelo clima, então o chão e a umidade não podem discordar.
	//
	// E O TEMPO entra junto com o clima. O clima diz como o lugar é em geral;
	// o tempo diz como ele está agora. Só com o clima, chover não mudava nada
	// do combate — o comentário de `WorldWeather.h` já prometia lama depois da
	// chuva enquanto esta linha ignorava o céu inteiro.
	OutInitialState.Humidity = static_cast<uint8>(FMath::Clamp(
		WorldWeather::HumidityPercent(
			IslandGeography::ClimateAt(FVector2D(Params.EncounterLocation)),
			Params.Weather), 0, 100));

	// A ARENA É O LUGAR. Antes ela era sorteada de um catálogo, e o jogador
	// topava com um inimigo na beira do lago para cair num "Campo Aberto" —
	// o terreno era decoração aleatória em vez de consequência de para onde
	// ele tinha andado.
	//
	// Sem amostra de mundo, o layout sai VAZIO e a arena cai no catálogo:
	// batalha aberta direto pela tela não tem lugar nenhum de onde nascer.
	if (!Params.WorldFeatures.IsEmpty())
	{
		FArenaFromWorldParams Terreno;
		Terreno.EncounterLocation = Params.EncounterLocation;
		// O tamanho da casa vem do PADRÃO da arena, e não de um número escrito
		// aqui: `CellSize` já é a única fonte de verdade do espaçamento, e uma
		// segunda cópia produziria um tabuleiro montado numa escala e desenhado
		// noutra — as casas do mundo cairiam nas casas erradas da grade.
		Terreno.CellSize = GetDefault<ABattleArena>()->CellSize;
		Terreno.Columns = OutInitialState.GridColumns;
		Terreno.Rows = OutInitialState.GridRows;
		Terreno.HumidityPercent = OutInitialState.Humidity;
		Terreno.bFlooded = WorldWeather::IsFlooding(Params.Weather);
		Terreno.Features = Params.WorldFeatures;

		const TArray<uint8> DoMundo = FArenaFromWorld::Build(Terreno);
		if (DoMundo.Num() == OutInitialState.CellLayout.Num())
		{
			OutInitialState.CellLayout = DoMundo;
		}
	}



	OutPresentations.Reset();
	OutPresentations.Add(PlayerPresentation);
	OutPresentations.Add(EncounterPresentation);
	return true;
}

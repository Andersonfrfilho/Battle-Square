// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattleState.h"
#include "Algo/Sort.h"

namespace
{
	// FNV-1a de 64 bits. Determinístico por construção — sem dependência de
	// endereço de memória, ordem de alocação ou qualquer outra coisa que
	// varie entre execuções (ver AD-004).
	uint64 CombineBattleHash(uint64 Accumulator, uint64 Value)
	{
		constexpr uint64 Prime = 1099511628211ULL;
		Accumulator ^= Value;
		Accumulator *= Prime;
		return Accumulator;
	}
}

uint64 FBattleState::ComputeHash() const
{
	constexpr uint64 Offset = 14695981039346656037ULL;
	uint64 Hash = Offset;

	// Ordena por PetId antes de combinar — a ordem de TArray::Pets não é
	// garantia de determinismo (ver BTL-17); o hash não pode depender dela.
	TArray<const FPetState*> SortedPets;
	SortedPets.Reserve(Pets.Num());
	for (const FPetState& Pet : Pets)
	{
		SortedPets.Add(&Pet);
	}
	Algo::Sort(SortedPets, [](const FPetState* A, const FPetState* B) { return A->PetId < B->PetId; });

	for (const FPetState* Pet : SortedPets)
	{
		Hash = CombineBattleHash(Hash, Pet->PetId);
		Hash = CombineBattleHash(Hash, Pet->Side);
		Hash = CombineBattleHash(Hash, Pet->Column);
		Hash = CombineBattleHash(Hash, Pet->Row);
		Hash = CombineBattleHash(Hash, static_cast<uint64>(static_cast<int64>(Pet->Health)));
		Hash = CombineBattleHash(Hash, static_cast<uint64>(static_cast<int64>(Pet->MaxHealth)));
		Hash = CombineBattleHash(Hash, static_cast<uint64>(static_cast<int64>(Pet->Attack)));
		Hash = CombineBattleHash(Hash, static_cast<uint64>(static_cast<int64>(Pet->Defense)));
		Hash = CombineBattleHash(Hash, static_cast<uint64>(static_cast<int64>(Pet->Speed)));
		Hash = CombineBattleHash(Hash, static_cast<uint64>(static_cast<int64>(Pet->ReflexDodgePercent)));
		Hash = CombineBattleHash(Hash, static_cast<uint64>(static_cast<int64>(Pet->DamageVariancePercent)));
		// Os poderes entram no hash: eles decidem dano, e estado que decide
		// resultado tem de estar no hash, senão duas partidas com golpes
		// diferentes teriam a mesma assinatura.
		for (int32 Indice = 0; Indice < 4; ++Indice)
		{
			Hash = CombineBattleHash(Hash, static_cast<uint64>(static_cast<int64>(Pet->MovePowers[Indice])));
			Hash = CombineBattleHash(Hash, Pet->MoveTerrainEffects[Indice]);
			Hash = CombineBattleHash(Hash, Pet->MoveTerrainDurations[Indice]);
			Hash = CombineBattleHash(Hash, Pet->MoveEffectStats[Indice]);
			Hash = CombineBattleHash(Hash, static_cast<uint64>(static_cast<int64>(Pet->MoveEffectPercents[Indice])));
		}

		Hash = CombineBattleHash(Hash, Pet->ActiveEffectStat);
		Hash = CombineBattleHash(Hash, static_cast<uint64>(static_cast<int64>(Pet->ActiveEffectPercent)));
		Hash = CombineBattleHash(Hash, Pet->ActiveEffectSlotsRemaining);

		Hash = CombineBattleHash(Hash, Pet->PostureFlags);
		Hash = CombineBattleHash(Hash, Pet->Traits);
	}

	// Arenas Variadas (design.md): layout entra no hash na ordem fixa do
	// array — não precisa de sort, CellLayout não é um contêiner cuja
	// ordem de iteração varia (é indexado por posição de casa, estável).
	for (int32 Indice = 0; Indice < 16; ++Indice)
	{
		Hash = CombineBattleHash(Hash, SkillTerrainRequirement[Indice]);
		Hash = CombineBattleHash(Hash, SkillTerrainLevel[Indice]);
		Hash = CombineBattleHash(Hash, TerrainSlipPercent[Indice]);
		Hash = CombineBattleHash(Hash, TerrainSlowPercent[Indice]);
		Hash = CombineBattleHash(Hash, TerrainDriesTo[Indice]);
		Hash = CombineBattleHash(Hash, TerrainDryDelay[Indice]);
	}

	Hash = CombineBattleHash(Hash, Humidity);
	Hash = CombineBattleHash(Hash, GridColumns);
	Hash = CombineBattleHash(Hash, GridRows);

	for (uint8 CellProperty : CellLayout)
	{
		Hash = CombineBattleHash(Hash, CellProperty);
	}

	// O PRAZO entra no hash junto com o terreno. Duas partidas com o mesmo
	// tabuleiro, uma com gelo que derrete no próximo slot e outra com gelo que
	// dura três, são duas partidas diferentes — e sem isto teriam a mesma
	// assinatura, que é exatamente a divergência que o hash existe para pegar.
	for (uint8 Prazo : CellCountdown)
	{
		Hash = CombineBattleHash(Hash, Prazo);
	}
	for (uint8 Volta : CellRevertsTo)
	{
		Hash = CombineBattleHash(Hash, Volta);
	}

	Hash = CombineBattleHash(Hash, static_cast<uint64>(static_cast<int64>(TurnNumber)));
	Hash = CombineBattleHash(Hash, bBattleEnded ? 1ULL : 0ULL);
	Hash = CombineBattleHash(Hash, WinningSide);
	Hash = CombineBattleHash(Hash, Random.State);
	Hash = CombineBattleHash(Hash, Random.Increment);

	return Hash;
}

void FBattleState::ResizeGrid(int32 Columns, int32 Rows)
{
	GridColumns = static_cast<uint8>(
		FMath::Clamp(Columns, BattleGridMinSide, BattleGridMaxSide));
	GridRows = static_cast<uint8>(
		FMath::Clamp(Rows, BattleGridMinSide, BattleGridMaxSide));

	const int32 Casas = static_cast<int32>(GridColumns) * static_cast<int32>(GridRows);
	CellLayout.Init(static_cast<uint8>(ECellProperty::None), Casas);
	CellCountdown.Init(0, Casas);
	CellRevertsTo.Init(static_cast<uint8>(ECellProperty::None), Casas);
}

void FBattleState::PlaceDuelistsAtStartingCells()
{
	// Linha do meio, arredondando para baixo em grade de altura par: numa
	// 4x6 os dois ficam na mesma linha, que é o que faz o duelo ser um
	// duelo e não uma perseguição na diagonal.
	const uint8 LinhaDoMeio = static_cast<uint8>((GridRows - 1) / 2);

	for (FPetState& Pet : Pets)
	{
		Pet.Column = Pet.Side == 0
			? 0
			: static_cast<uint8>(GridColumns - 1);
		Pet.Row = LinhaDoMeio;
	}
}

FPetState* FBattleState::FindAlivePetOnSide(uint8 Side)
{
	for (FPetState& Pet : Pets)
	{
		if (Pet.Side == Side && Pet.IsAlive())
		{
			return &Pet;
		}
	}
	return nullptr;
}

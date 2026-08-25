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
		Hash = CombineBattleHash(Hash, Pet->PostureFlags);
	}

	Hash = CombineBattleHash(Hash, static_cast<uint64>(static_cast<int64>(TurnNumber)));
	Hash = CombineBattleHash(Hash, bBattleEnded ? 1ULL : 0ULL);
	Hash = CombineBattleHash(Hash, WinningSide);
	Hash = CombineBattleHash(Hash, Random.State);
	Hash = CombineBattleHash(Hash, Random.Increment);

	return Hash;
}

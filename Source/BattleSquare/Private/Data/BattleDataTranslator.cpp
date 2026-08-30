// Copyright 2026 Anderson. All Rights Reserved.

#include "Data/BattleDataTranslator.h"

#include "Battle/BattleArenaConstants.h"

namespace
{
	/**
	 * Nome do atributo como o dado assinado o escreve.
	 *
	 * Desconhecido vira "nenhum", e é a escolha segura: um golpe que não mexe
	 * em atributo nenhum ainda é um golpe. O erro de cadastro é barrado na
	 * ESCRITA, pelo enum do Zod — aqui, recusar a batalha seria pior.
	 */
	uint8 StatFromName(const FString& Nome)
	{
		if (Nome.Equals(TEXT("attack"), ESearchCase::IgnoreCase))
		{
			return static_cast<uint8>(EBattleStat::Ataque);
		}
		if (Nome.Equals(TEXT("defense"), ESearchCase::IgnoreCase))
		{
			return static_cast<uint8>(EBattleStat::Defesa);
		}
		if (Nome.Equals(TEXT("speed"), ESearchCase::IgnoreCase))
		{
			return static_cast<uint8>(EBattleStat::Velocidade);
		}
		return static_cast<uint8>(EBattleStat::Nenhum);
	}

	uint8 TerrainEffectFromName(const FString& Nome)
	{
		if (Nome.Equals(TEXT("water"), ESearchCase::IgnoreCase))
		{
			return static_cast<uint8>(ECellProperty::Water);
		}
		if (Nome.Equals(TEXT("damage"), ESearchCase::IgnoreCase))
		{
			return static_cast<uint8>(ECellProperty::Damage);
		}
		if (Nome.Equals(TEXT("shallow_water"), ESearchCase::IgnoreCase))
		{
			return static_cast<uint8>(ECellProperty::ShallowWater);
		}
		if (Nome.Equals(TEXT("ice"), ESearchCase::IgnoreCase))
		{
			return static_cast<uint8>(ECellProperty::Ice);
		}
		if (Nome.Equals(TEXT("mud"), ESearchCase::IgnoreCase))
		{
			return static_cast<uint8>(ECellProperty::Mud);
		}

		// "none" e qualquer coisa desconhecida caem aqui.
		return static_cast<uint8>(ECellProperty::None);
	}
}

void FBattleDataTranslator::TranslatePet(
	const FLoadedPetRecord& Source,
	uint8 PetId,
	uint8 Side,
	uint8 Column,
	uint8 Row,
	FPetState& OutBattleState,
	FPetPresentationInfo& OutPresentation)
{
	// attack/defense/speed/maxHealth: cópia direta, sem conversão — já
	// chegam inteiros do backend (T18, tasks.md).
	OutBattleState = FPetState();
	OutBattleState.PetId = PetId;
	OutBattleState.Side = Side;
	// O ±20% "por padrão" da spec mora AQUI, e não no núcleo: um FPetState
	// montado à mão — que é todo estado de teste — precisa continuar
	// resolvendo dano exato, senão a fórmula perde os testes que a protegem.
	// Todo pet que passa por esta tradução ganha a faixa cheia; quem tem
	// agressividade a estreita depois, em ApplyToBattleState.
	OutBattleState.DamageVariancePercent = BattleArenaConstants::DamageVarianceBasePercent;

	OutBattleState.Column = Column;
	OutBattleState.Row = Row;
	OutBattleState.Attack = Source.Attack;
	OutBattleState.Defense = Source.Defense;
	OutBattleState.Speed = Source.Speed;
	OutBattleState.MaxHealth = Source.MaxHealth;
	// Health = MaxHealth no início da batalha (T18).
	OutBattleState.Health = Source.MaxHealth;

	OutPresentation = FPetPresentationInfo();
	OutPresentation.PetId = PetId;
	OutPresentation.Name = Source.Name;
	OutPresentation.Type = Source.Type;
	OutPresentation.CatalogId = Source.Id;

	// "type" NUNCA entra em FPetState — vira FGameplayTag só aqui, na
	// camada de apresentação (BattleSquare). ErrorIfNotFound=false: tags
	// de tipo de pet ainda não foram registradas como conteúdo (fora do
	// escopo deste backend) — tag não encontrada vira tag vazia, não
	// crash. Registro de conteúdo é trabalho futuro (M3).
	const FName TagName(*FString::Printf(TEXT("Pet.Type.%s"), *Source.Type));
	OutPresentation.TypeTag = FGameplayTag::RequestGameplayTag(TagName, /*ErrorIfNotFound=*/false);
	// Golpes: nome, na ordem em que vieram assinados. A ordem É o índice que
	// viaja no commit (DP-golpe-04) — reordenar aqui faria o jogador escolher
	// um golpe e o resolvedor usar outro.
	OutPresentation.MoveNames.Reset();
	OutPresentation.MoveRequiresAttribute.Reset();
	OutPresentation.MoveRequiresValue.Reset();
	OutPresentation.MoveUnlocked.Reset();
	for (const FLoadedPetMove& Move : Source.Moves)
	{
		OutPresentation.MoveNames.Add(Move.Name);
		OutPresentation.MoveRequiresAttribute.Add(Move.RequiresAttribute);
		OutPresentation.MoveRequiresValue.Add(Move.RequiresValue);
		OutPresentation.MoveUnlocked.Add(true);
	}

	// O PODER vai para o núcleo, o NOME fica na apresentação.
	//
	// Separados de propósito: o núcleo precisa do poder para resolver dano e
	// não pode conhecer texto (AD-012); a tela precisa do nome e não pode
	// recalcular dano (audit_no_recalculation.sh). Cada lado recebe o que usa,
	// e nada além.
	for (int32 Indice = 0; Indice < 4; ++Indice)
	{
		OutBattleState.MovePowers[Indice] = Source.Moves.IsValidIndex(Indice)
			? Source.Moves[Indice].Power
			: 0;

		// O TEXTO do backend vira valor do núcleo AQUI, na montagem.
		//
		// O núcleo não conhece string (AD-012), e o backend não conhece
		// ECellProperty — a tradução tem de acontecer em algum lugar, e é este
		// o lugar que já traduz tipo, atributo e efetividade.
		//
		// Efeito desconhecido vira "não muda nada", nunca um valor qualquer:
		// um erro de digitação no cadastro não pode alagar o tabuleiro.
		OutBattleState.MoveEffectStats[Indice] = Source.Moves.IsValidIndex(Indice)
			? StatFromName(Source.Moves[Indice].EffectStat)
			: 0;
		OutBattleState.MoveEffectPercents[Indice] = Source.Moves.IsValidIndex(Indice)
			? Source.Moves[Indice].EffectPercent
			: 0;

		OutBattleState.MoveTerrainEffects[Indice] = Source.Moves.IsValidIndex(Indice)
			? TerrainEffectFromName(Source.Moves[Indice].TerrainEffect)
			: static_cast<uint8>(ECellProperty::None);

		// Duração negativa ou absurda não vira prazo maluco: o teto é o turno
		// inteiro mais dois, porque gelo que atravessa vários turnos deixa de
		// ser jogada e vira mudança de arena — e quem decide arena é a
		// montagem, não um golpe.
		OutBattleState.MoveTerrainDurations[Indice] = Source.Moves.IsValidIndex(Indice)
			? static_cast<uint8>(FMath::Clamp(Source.Moves[Indice].TerrainDuration, 0, 5))
			: 0;
	}

}

void FBattleDataTranslator::TranslateMatchup(
	const FLoadedPetRecord& LeftSource,
	const FLoadedPetRecord& RightSource,
	const FTypeEffectivenessTable& EffectivenessTable,
	uint8 LeftPetId,
	uint8 RightPetId,
	FPetState& OutLeftState,
	FPetPresentationInfo& OutLeftPresentation,
	FPetState& OutRightState,
	FPetPresentationInfo& OutRightPresentation)
{
	// Casa de saída PROVISÓRIA. Quem decide de verdade é
	// FBattleState::PlaceDuelistsAtStartingCells, depois de os pets
	// entrarem no estado: só lá se sabe o tamanho da grade, e a casa
	// inicial de um campo 4x6 não é a de um 3x3.
	TranslatePet(LeftSource, LeftPetId, /*Side=*/0, /*Column=*/0, /*Row=*/0, OutLeftState, OutLeftPresentation);
	TranslatePet(RightSource, RightPetId, /*Side=*/1, /*Column=*/0, /*Row=*/0, OutRightState, OutRightPresentation);

	// Efetividade é sobre O ATAQUE do lado — o Attack do Left muda pela
	// efetividade DO TIPO DO LEFT CONTRA O TIPO DO RIGHT, nunca o
	// inverso (T3 é 🧠 justamente por essa inversão ser fácil de errar).
	const int32 LeftEffectivenessPercent = EffectivenessTable.GetPercent(LeftSource.Type, RightSource.Type);
	const int32 RightEffectivenessPercent = EffectivenessTable.GetPercent(RightSource.Type, LeftSource.Type);

	OutLeftState.Attack = (OutLeftState.Attack * LeftEffectivenessPercent) / 100;
	OutRightState.Attack = (OutRightState.Attack * RightEffectivenessPercent) / 100;

	// O mesmo número que multiplicou o ataque vai para a apresentação. Assim a
	// tela DIZ a efetividade sem recalculá-la — um valor só, computado uma vez.
	OutLeftPresentation.EffectivenessPercent = LeftEffectivenessPercent;
	OutRightPresentation.EffectivenessPercent = RightEffectivenessPercent;
}

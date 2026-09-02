// Copyright 2026 Anderson. All Rights Reserved.

#include "Data/BattleDataTranslator.h"

#include "Balance/PetTypeCatalog.h"
#include "Battle/FluidRegistry.h"
#include "Balance/PetTypeIdentity.h"

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
		if (Nome.Equals(TEXT("dry"), ESearchCase::IgnoreCase))
		{
			return static_cast<uint8>(ECellProperty::Dries);
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

	// O TRAÇO vem do ELEMENTO, e é aqui que ele atravessa a fronteira: o
	// núcleo guarda o efeito (`Incorporeo`), a camada de fora guarda o
	// significado (o elemento se chama "Fantasma"). Mesma divisão da umidade
	// e do efeito de terreno.
	OutBattleState.Traits = 0;
	if (FPetTypeIdentity::Parse(Source.Type).Element.Equals(
		TEXT("Fantasma"), ESearchCase::IgnoreCase))
	{
		OutBattleState.Traits |= static_cast<uint8>(EPetTrait::Incorporeo);
	}

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
	// A RESISTÊNCIA A FLUIDO: o traço dá a base, o item soma.
	//
	// A base vem do ELEMENTO, pelo mesmo caminho do Incorpóreo — o núcleo
	// guarda o número, a camada de fora guarda o significado. O item ainda não
	// existe como sistema, então soma ZERO aqui; o ponto onde ele entra é
	// `ComposeFluidResist`, que tem prova própria justamente para que criar o
	// sistema seja uma linha e não uma descoberta.
	{
		const FPetTypeIdentity ParaResistir = FPetTypeIdentity::Parse(Source.Type);
		if (const FPetElementDefinition* Dele =
			FPetTypeCatalog::Get().FindElement(ParaResistir.Element))
		{
			// A FIRMEZA contra a corrente, do mesmo lugar e pela mesma razão.
			OutBattleState.FootingPerMille = Dele->FootingPerMille;

			for (int32 Qual = 1; Qual < static_cast<int32>(EFluidKind::Count); ++Qual)
			{
				const EFluidKind Fluido = static_cast<EFluidKind>(Qual);
				const int32* DoTraco =
					Dele->FluidResists.Find(FluidRegistry::TraitsOf(Fluido).DebugName);

				// A chave é o nome do fluido NO REGISTRO — uma segunda grafia
				// seria uma resistência que nunca casa e nunca acusa. (A
				// diferença de caixa não separa: `FString` no Unreal compara
				// sem diferenciar maiúscula, e o teste confirma que
				// `"Lava"` no JSON acha `"lava"` do registro.)
				OutBattleState.SetResistPercentFor(Fluido,
					ComposeFluidResist(DoTraco ? *DoTraco : 0, /*FromItem=*/0));
			}
		}
	}

	// O ELEMENTO decide se os golpes conduzem, e a decisão acontece AQUI —
	// onde o tipo ainda é conhecido. O núcleo nunca aprende o que é "Raio":
	// ele recebe uma bandeira por golpe, do mesmo jeito que recebe o Attack já
	// pré-multiplicado pela efetividade.
	//
	// Golpe não carrega elemento próprio no cadastro, então quem responde é o
	// elemento do PET: os golpes de uma criatura de raio são de raio.
	const FPetTypeIdentity Identidade = FPetTypeIdentity::Parse(Source.Type);
	const FPetElementDefinition* Elemento =
		FPetTypeCatalog::Get().FindElement(Identidade.Element);
	const bool bConduz = Elemento != nullptr && Elemento->bConducts;

	for (int32 Indice = 0; Indice < 4; ++Indice)
	{
		OutBattleState.MoveConducts[Indice] =
			(bConduz && Source.Moves.IsValidIndex(Indice)) ? 1 : 0;

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

		// Concentração: o núcleo recebe o golpe já EFETIVO, e "exige foco" é
		// característica do golpe, não do pet.
		OutBattleState.MoveNeedsFocus[Indice] =
			(Source.Moves.IsValidIndex(Indice) && Source.Moves[Indice].bNeedsFocus)
				? 1 : 0;

		// Teto de 100: devolver MAIS vida do que o dano causado faria um golpe
		// fraco em alvo defendido render mais que um forte, e o jogador não
		// teria como ler isso.
		OutBattleState.MoveDrainPercents[Indice] = Source.Moves.IsValidIndex(Indice)
			? static_cast<uint8>(FMath::Clamp(Source.Moves[Indice].DrainPercent, 0, 100))
			: 0;
	}

}

int32 FBattleDataTranslator::ComposeFluidResist(int32 DoTraco, int32 DoItem)
{
	// SOMA, e não o maior dos dois: o item ACRESCENTA ao que a criatura já é.
	// Tomar o maior faria uma bota de lava não valer nada num pet de Fogo, que
	// é justamente o pet que mais a usaria.
	//
	// O TRAÇO PODE SER NEGATIVO — fraqueza é parte do que a criatura é, e
	// prendê-lo em zero apagaria metade do que distingue uma das outras. Isto
	// aqui já apagou: a versão anterior fazia `Max(DoTraco, 0)`, e ela teria
	// engolido em silêncio toda fraqueza que alguém cadastrasse.
	//
	// O ITEM, não: ele só acrescenta. Item que ENFRAQUECE é maldição, e
	// maldição é outra mecânica — com outra narração e outra forma de sair
	// dela. Um item mal cadastrado não pode virar uma por acidente.
	return FMath::Clamp(DoTraco + FMath::Max(DoItem, 0), -100, 100);
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

// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattlePhases.h"
#include "Battle/BattleState.h"
#include "Battle/BattleEvent.h"
#include "Battle/BattleArenaConstants.h"

namespace
{
	struct FMoveIntent
	{
		FPetState* Pet = nullptr;
		int32 DestColumn = 0;
		int32 DestRow = 0;
		bool bInsideGrid = false;
	};

	void CollectIntent(FBattleState& State, uint8 Side, const FBattleAction& Action, TArray<FMoveIntent>& OutIntents)
	{
		if (Action.Type != EActionType::Mover)
		{
			return;
		}

		for (FPetState& Pet : State.Pets)
		{
			if (Pet.Side != Side || !Pet.IsAlive())
			{
				continue;
			}

			// DP-ia-04: quem está emergindo do subsolo não anda neste slot.
			// A intenção nem é coletada — sair daqui como intenção anulada
			// emitiria MovimentoBloqueado, que o feed narra como "esbarrou no
			// limite da arena", e a causa é outra.
			if ((Pet.PostureFlags & static_cast<uint8>(EBattlePostureFlags::Emerging)) != 0)
			{
				continue;
			}

			int8 DeltaColumn = 0;
			int8 DeltaRow = 0;
			GetDirectionDelta(Action.Direction, DeltaColumn, DeltaRow);

			const int32 DestColumn = static_cast<int32>(Pet.Column) + DeltaColumn;
			const int32 DestRow = static_cast<int32>(Pet.Row) + DeltaRow;

			FMoveIntent Intent;
			Intent.Pet = &Pet;
			Intent.DestColumn = DestColumn;
			Intent.DestRow = DestRow;
			// Arenas Variadas (design.md): casa bloqueada é o MESMO
			// caminho de "fora da grade" — bInsideGrid é, na prática,
			// "destino válido", nome mantido para não mexer no resto do
			// algoritmo (colisão entre aliados, EmitBlocked já existentes).
			const bool bWithinBounds = IsInsideGrid(DestColumn, DestRow);
			const bool bDestinationBlocked = bWithinBounds
				&& State.CellLayout[CellLayoutIndex(DestColumn, DestRow)] == static_cast<uint8>(ECellProperty::Blocked);
			Intent.bInsideGrid = bWithinBounds && !bDestinationBlocked;
			OutIntents.Add(Intent);

			// v1 é 1v1 — um único pet vivo por lado, então este laço
			// encontra no máximo uma intenção por lado. Mantido como
			// busca (em vez de índice fixo) para generalizar sem mudar
			// assinatura quando M3 trouxer múltiplos pets por lado.
			break;
		}
	}

	void EmitMoved(TArray<FBattleEvent>& OutTrace, uint8 SlotIndex, const FPetState& Pet, uint8 FromCell, uint8 ToCell)
	{
		FBattleEvent Event;
		Event.Type = EBattleEventType::Moveu;
		Event.SlotIndex = SlotIndex;
		Event.Phase = 3; // F3
		Event.ActorId = Pet.PetId;
		Event.TargetId = BattleEventNoActor;
		Event.FromCell = FromCell;
		Event.ToCell = ToCell;
		OutTrace.Add(Event);
	}

	void EmitEncounter(TArray<FBattleEvent>& OutTrace, uint8 SlotIndex,
		const FPetState& First, const FPetState& Second)
	{
		FBattleEvent Event;
		Event.Type = EBattleEventType::EncontroNoMesmoPonto;
		Event.SlotIndex = SlotIndex;
		Event.Phase = 3; // F3
		Event.ActorId = First.PetId;
		Event.TargetId = Second.PetId;
		Event.FromCell = PackCell(First.Column, First.Row);
		Event.ToCell = PackCell(Second.Column, Second.Row);
		OutTrace.Add(Event);
	}

	void EmitBlocked(TArray<FBattleEvent>& OutTrace, uint8 SlotIndex, const FPetState& Pet)
	{
		FBattleEvent Event;
		Event.Type = EBattleEventType::MovimentoBloqueado;
		Event.SlotIndex = SlotIndex;
		Event.Phase = 3; // F3
		Event.ActorId = Pet.PetId;
		Event.TargetId = BattleEventNoActor;
		Event.FromCell = PackCell(Pet.Column, Pet.Row);
		Event.ToCell = Event.FromCell; // Bloqueado: fica onde estava.
		OutTrace.Add(Event);
	}
}

void BattlePhases::ApplyMovement(
	FBattleState& State,
	const FBattleAction& LeftAction,
	const FBattleAction& RightAction,
	uint8 SlotIndex,
	TArray<FBattleEvent>& OutTrace)
{
	// Passo 1: coletar TODAS as intenções antes de aplicar qualquer uma —
	// é o que torna o movimento simultâneo de verdade (design.md).
	TArray<FMoveIntent> Intents;
	CollectIntent(State, /*Side=*/0, LeftAction, Intents);
	CollectIntent(State, /*Side=*/1, RightAction, Intents);

	// Passos 2–3 (validade de destino, colisão entre aliados) só têm
	// sentido se alguém tentou se mover — mas o Passo 4 (dano de casa)
	// precisa rodar sempre, mesmo com Intents vazio (ex.: os dois lados
	// dão Aguardar, parados numa casa de dano). Por isso nenhum
	// early-return: os passos de movimento ficam dentro deste bloco.
	if (!Intents.IsEmpty())
	{
		// Passo 2: fora da grade é bloqueio individual, resolvido já aqui —
		// não compete por destino com mais ninguém.
		TArray<FMoveIntent> ValidIntents;
		for (const FMoveIntent& Intent : Intents)
		{
			if (Intent.bInsideGrid)
			{
				ValidIntents.Add(Intent);
			}
			else
			{
				EmitBlocked(OutTrace, SlotIndex, *Intent.Pet);
			}
		}

		// Passo 3: disputa de destino, decidida por POSIÇÃO FINAL.
		//
		// DP-02 foi INVERTIDO em 2026-08-27: dois pets não ocupam a mesma
		// casa. Decidir por posição final, e não conferindo a casa de destino
		// contra o estado vivo, é o que mantém a TROCA de casas permitida —
		// na troca ninguém termina no mesmo ponto, mas durante a aplicação
		// cada um ainda está na casa que o outro quer. Checar contra o estado
		// vivo bloquearia a troca, e o resultado dependeria da ordem em que os
		// pets fossem processados.
		TMap<uint8, int32> IntentByPetId;
		for (int32 Index = 0; Index < ValidIntents.Num(); ++Index)
		{
			IntentByPetId.Add(ValidIntents[Index].Pet->PetId, Index);
		}

		struct FFinalPlace
		{
			FPetState* Pet = nullptr;
			int32 Column = 0;
			int32 Row = 0;
			int32 IntentIndex = INDEX_NONE;
		};

		TArray<FFinalPlace> FinalPlaces;
		for (FPetState& Pet : State.Pets)
		{
			if (!Pet.IsAlive())
			{
				continue;
			}

			FFinalPlace Place;
			Place.Pet = &Pet;
			Place.Column = static_cast<int32>(Pet.Column);
			Place.Row = static_cast<int32>(Pet.Row);

			if (const int32* Index = IntentByPetId.Find(Pet.PetId))
			{
				Place.IntentIndex = *Index;
				Place.Column = ValidIntents[*Index].DestColumn;
				Place.Row = ValidIntents[*Index].DestRow;
			}
			FinalPlaces.Add(Place);
		}

		TMap<uint32, TArray<int32>> ClaimsByCell;
		for (int32 Index = 0; Index < FinalPlaces.Num(); ++Index)
		{
			const uint32 Key = (static_cast<uint32>(FinalPlaces[Index].Column) << 8)
				| static_cast<uint32>(FinalPlaces[Index].Row);
			ClaimsByCell.FindOrAdd(Key).Add(Index);
		}

		TSet<uint8> PetsBarrados;
		for (const auto& ClaimPair : ClaimsByCell)
		{
			const TArray<int32>& Claimants = ClaimPair.Value;
			if (Claimants.Num() < 2)
			{
				continue;
			}

			for (int32 First = 0; First < Claimants.Num(); ++First)
			{
				for (int32 Second = First + 1; Second < Claimants.Num(); ++Second)
				{
					const FFinalPlace& Um = FinalPlaces[Claimants[First]];
					const FFinalPlace& Outro = FinalPlaces[Claimants[Second]];

					// Encontro só existe entre lados OPOSTOS. Dois aliados na
					// mesma casa é BTL-05: bloqueio simples, ninguém se fere.
					if (Um.Pet->Side != Outro.Pet->Side)
					{
						EmitEncounter(OutTrace, SlotIndex, *Um.Pet, *Outro.Pet);
					}
				}
			}

			// Quem tentou andar e esbarrou fica onde estava. Quem já estava
			// parado ali não recebe bloqueio: ele não tentou nada.
			for (int32 Index : Claimants)
			{
				if (FinalPlaces[Index].IntentIndex != INDEX_NONE)
				{
					PetsBarrados.Add(FinalPlaces[Index].Pet->PetId);
					EmitBlocked(OutTrace, SlotIndex, *FinalPlaces[Index].Pet);
				}
			}
		}

		for (const FMoveIntent& Intent : ValidIntents)
		{
			if (PetsBarrados.Contains(Intent.Pet->PetId))
			{
				continue;
			}

			const uint8 FromCell = PackCell(Intent.Pet->Column, Intent.Pet->Row);
			Intent.Pet->Column = static_cast<uint8>(Intent.DestColumn);
			Intent.Pet->Row = static_cast<uint8>(Intent.DestRow);
			const uint8 ToCell = PackCell(Intent.Pet->Column, Intent.Pet->Row);
			EmitMoved(OutTrace, SlotIndex, *Intent.Pet, FromCell, ToCell);
		}
	}

	// Passo 4 (Arenas Variadas, design.md — DP-arena-02): dano de casa,
	// avaliado ao fim DESTE slot, pela posição atual de cada pet vivo —
	// já depois do movimento acima ter sido aplicado (ou a mesma posição
	// de antes, se ele não se moveu ou foi bloqueado). Soma em
	// PendingDamage, mesmo acumulador do combate — NUNCA aplica aqui
	// (BTL-07). Sem evento próprio: F5 (ApplyResolution) já emite
	// DanoAplicado para qualquer PendingDamage > 0, não importa a origem.
	for (FPetState& Pet : State.Pets)
	{
		if (!Pet.IsAlive())
		{
			continue;
		}
		// DP-ia-04: voar e submergir tiram o pet do CHÃO, e a casa só alcança
		// quem está pisando nela. Camuflar não conta — quem se esconde
		// continua em pé no mesmo lugar.
		const bool bFolgaDoChao =
			(Pet.PostureFlags & static_cast<uint8>(EBattlePostureFlags::Flying)) != 0
			|| (Pet.PostureFlags & static_cast<uint8>(EBattlePostureFlags::Underground)) != 0;

		if (!bFolgaDoChao
			&& State.CellLayout[CellLayoutIndex(Pet.Column, Pet.Row)] == static_cast<uint8>(ECellProperty::Damage))
		{
			Pet.PendingDamage += BattleArenaConstants::CellDamageAmount;
		}
	}
}

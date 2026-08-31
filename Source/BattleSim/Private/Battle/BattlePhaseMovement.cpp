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
		// Destino dentro da grade, mas ocupado por tronco ou pedra. Guardado
		// em separado de bInsideGrid porque agora há TRÊS saídas para ele —
		// derrubar, subir, esbarrar — e dobrar as três dentro de um booleano
		// de validade escondia justamente a que interessa.
		bool bDestinationIsObstacle = false;
	};

	bool IsCellOccupied(const FBattleState& State, int32 Column, int32 Row, const FPetState& Ignoring)
	{
		for (const FPetState& Pet : State.Pets)
		{
			if (&Pet == &Ignoring || !Pet.IsAlive())
			{
				continue;
			}
			if (static_cast<int32>(Pet.Column) == Column && static_cast<int32>(Pet.Row) == Row)
			{
				return true;
			}
		}
		return false;
	}

	void CollectIntent(FBattleState& State, uint8 Side, const FBattleAction& Action,
		uint8 SlotIndex, TArray<FBattleEvent>& OutTrace, TArray<FMoveIntent>& OutIntents)
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
			if ((Pet.PostureFlags & static_cast<uint16>(EBattlePostureFlags::Emerging)) != 0)
			{
				continue;
			}

			// O CHÃO DE BAIXO cobra de quem tenta sair dele.
			//
			// O sorteio só acontece onde há chance, e isso NÃO é otimização:
			// tirar um número do gerador em chão seco deslocaria o fluxo
			// aleatório de toda partida já gravada, e todo snapshot de
			// determinismo passaria a divergir por uma regra que aquelas
			// batalhas nem exercem.
			// O CHÃO não cobra de quem não pisa nele. Vale para o fantasma
			// sempre, e para quem voa ou submerge enquanto durar — a mesma
			// pergunta, num lugar só.
			const uint8 ChaoDaCasa = State.CellLayout[State.CellIndex(Pet.Column, Pet.Row)];
			if (!Pet.IsOffTheGround() && State.TerrainAffectsDeparture(ChaoDaCasa))
			{
				const int32 Sorte = State.Random.NextRange(1, 100);
				const int32 ChanceDeEscorregar = State.TerrainSlipPercent[ChaoDaCasa];
				const int32 ChanceDeAtrasar = State.TerrainSlowPercent[ChaoDaCasa];

				if (Sorte <= ChanceDeEscorregar)
				{
					// Com evento, ao contrário do caso de emergir logo acima:
					// ali o jogador já sabe que submergiu e por que não anda,
					// enquanto aqui a ação simplesmente não aconteceria. Ação
					// que some sem frase parece defeito, e este projeto já
					// gastou rodadas com regra funcionando e parecendo
					// quebrada (DP-atr-08).
					FBattleEvent Escorregou;
					Escorregou.Type = EBattleEventType::Escorregou;
					Escorregou.SlotIndex = SlotIndex;
					Escorregou.Phase = 3; // F3
					Escorregou.ActorId = Pet.PetId;
					Escorregou.TargetId = BattleEventNoActor;
					Escorregou.FromCell = PackCell(Pet.Column, Pet.Row);
					Escorregou.ToCell = Escorregou.FromCell;
					Escorregou.Detail = ChaoDaCasa;
					OutTrace.Add(Escorregou);
					continue;
				}

				if (Sorte <= ChanceDeEscorregar + ChanceDeAtrasar)
				{
					// ANDA, mas devagar. A marca vive em PostureFlags e some
					// no fim do slot com o resto das posturas — atraso que
					// sobrevivesse ao slot seria um segundo efeito de
					// atributo, com regra própria de expiração para manter.
					Pet.PostureFlags |= static_cast<uint16>(EBattlePostureFlags::Slowed);

					FBattleEvent Atrasou;
					Atrasou.Type = EBattleEventType::AtravessouDevagar;
					Atrasou.SlotIndex = SlotIndex;
					Atrasou.Phase = 3; // F3
					Atrasou.ActorId = Pet.PetId;
					Atrasou.TargetId = BattleEventNoActor;
					Atrasou.FromCell = PackCell(Pet.Column, Pet.Row);
					Atrasou.Detail = ChaoDaCasa;
					OutTrace.Add(Atrasou);
				}

				// O terceiro caso — atravessar firme — não emite nada, e é
				// deliberado: linha em todo passo por chão comum afogaria as
				// duas que importam. Quem atravessou firme vê o pet andar.
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
			const bool bWithinBounds = State.IsInside(DestColumn, DestRow);
			// ATRAVESSANDO, o obstáculo não é obstáculo: o tronco continua em
			// pé e ele passa por dentro. Não é derrubar — quem derruba abre a
			// passagem para todo mundo, e isso é outra jogada, com outro
			// custo e outro evento.
			const bool bAtravessando =
				(Pet.PostureFlags & static_cast<uint16>(EBattlePostureFlags::Phasing)) != 0;

			Intent.bDestinationIsObstacle = bWithinBounds && !bAtravessando
				&& State.CellLayout[State.CellIndex(DestColumn, DestRow)] == static_cast<uint8>(ECellProperty::Blocked);
			Intent.bInsideGrid = bWithinBounds && !Intent.bDestinationIsObstacle;
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

	void EmitObstacleFelled(TArray<FBattleEvent>& OutTrace, uint8 SlotIndex,
		const FPetState& Pet, uint8 ObstacleCell)
	{
		FBattleEvent Event;
		Event.Type = EBattleEventType::ObstaculoDerrubado;
		Event.SlotIndex = SlotIndex;
		Event.Phase = 3; // F3
		Event.ActorId = Pet.PetId;
		Event.TargetId = BattleEventNoActor;
		Event.FromCell = PackCell(Pet.Column, Pet.Row);
		Event.ToCell = ObstacleCell; // Derrubou dali; ele próprio fica onde está.
		OutTrace.Add(Event);
	}

	void EmitClimbed(TArray<FBattleEvent>& OutTrace, uint8 SlotIndex,
		const FPetState& Pet, uint8 FromCell, uint8 ToCell)
	{
		FBattleEvent Event;
		Event.Type = EBattleEventType::SubiuNoObstaculo;
		Event.SlotIndex = SlotIndex;
		Event.Phase = 3; // F3
		Event.ActorId = Pet.PetId;
		Event.TargetId = BattleEventNoActor;
		Event.FromCell = FromCell;
		Event.ToCell = ToCell;
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
	CollectIntent(State, /*Side=*/0, LeftAction, SlotIndex, OutTrace, Intents);
	CollectIntent(State, /*Side=*/1, RightAction, SlotIndex, OutTrace, Intents);

	// Passos 2–4 (obstáculo, validade de destino, colisão entre aliados) só
	// têm sentido se alguém tentou se mover — mas o Passo 5 (dano de casa)
	// precisa rodar sempre, mesmo com Intents vazio (ex.: os dois lados
	// dão Aguardar, parados numa casa de dano). Por isso nenhum
	// early-return: os passos de movimento ficam dentro deste bloco.
	if (!Intents.IsEmpty())
	{
		// Passo 2: a casa BLOQUEADA tem CORPO — tronco ou pedra, não parede
		// lisa. Quem anda contra ela derruba (força), sobe (agilidade) ou
		// esbarra, nesta ordem.
		//
		// Antes do Passo 4 de propósito: derrubar abre a casa para a disputa
		// de destino do MESMO slot, e é isso que faz o forte trabalhar para o
		// ágil quando os dois miram o mesmo obstáculo.
		//
		// Em ordem de PetId, e não de container: os dois lados podem mirar o
		// mesmo obstáculo, e quem chega primeiro decide o que acontece com
		// ele. Ordem de container não é determinismo (o mesmo motivo de
		// ResolveTarget desempatar por PetId).
		TArray<int32> OrdemDeResolucao;
		for (int32 Index = 0; Index < Intents.Num(); ++Index)
		{
			OrdemDeResolucao.Add(Index);
		}
		OrdemDeResolucao.Sort([&Intents](int32 Esquerda, int32 Direita)
		{
			return Intents[Esquerda].Pet->PetId < Intents[Direita].Pet->PetId;
		});

		TArray<int32> IntencoesConsumidas;
		for (int32 Index : OrdemDeResolucao)
		{
			FMoveIntent& Intent = Intents[Index];
			if (!Intent.bDestinationIsObstacle)
			{
				continue;
			}

			// Relê o tabuleiro em vez de confiar na coleta: um aliado com
			// força pode ter derrubado este mesmo obstáculo agora há pouco, e
			// o caminho está aberto.
			const int32 IndiceDaCasa = State.CellIndex(Intent.DestColumn, Intent.DestRow);
			if (State.CellLayout[IndiceDaCasa] != static_cast<uint8>(ECellProperty::Blocked))
			{
				// Sem obstáculo já não há escalada: dali em diante é andar
				// para uma casa vazia, e é `Moveu` que o feed precisa contar.
				Intent.bDestinationIsObstacle = false;
				Intent.bInsideGrid = true;
				continue;
			}

			// Obstáculo com alguém em cima não cai nem recebe segundo
			// morador: derrubá-lo tiraria o chão de quem já subiu, e a queda
			// seria consequência de uma jogada de OUTRO pet. Esbarra, e o
			// Passo 4 nem chega a ver a disputa.
			if (IsCellOccupied(State, Intent.DestColumn, Intent.DestRow, *Intent.Pet))
			{
				continue;
			}

			// Força DERRUBA — e gasta o slot nisso: quem abre a passagem não
			// atravessa no mesmo movimento. Sem esse custo, o obstáculo seria
			// um pedágio de zero para o pet forte.
			if (Intent.Pet->GetEffectiveAttack() >= BattleArenaConstants::ObstacleBreakAttack)
			{
				State.CellLayout[IndiceDaCasa] = static_cast<uint8>(ECellProperty::None);
				EmitObstacleFelled(OutTrace, SlotIndex, *Intent.Pet,
					PackCell(static_cast<uint8>(Intent.DestColumn), static_cast<uint8>(Intent.DestRow)));
				IntencoesConsumidas.Add(Index);

				// Quem mirava esta casa passa a andar para ela, tenha já sido
				// visitado ou não. Sem isto, o ágil de PetId menor que decidiu
				// escalar UM PASSO antes entraria numa casa cujo tronco acabou
				// de ir ao chão — e o feed diria que ele escalou o nada.
				const int32 IndiceQueCaiu = IndiceDaCasa;
				for (int32 Outro = 0; Outro < Intents.Num(); ++Outro)
				{
					if (Outro == Index)
					{
						continue;
					}
					FMoveIntent& Vizinho = Intents[Outro];
					if (Vizinho.bDestinationIsObstacle
						&& State.CellIndex(Vizinho.DestColumn, Vizinho.DestRow) == IndiceQueCaiu)
					{
						Vizinho.bDestinationIsObstacle = false;
						Vizinho.bInsideGrid = true;
					}
				}
				continue;
			}

			// Agilidade SOBE. Daqui em diante a intenção é um movimento
			// comum: entra na disputa de destino como qualquer outra, e o que
			// a distingue na hora de aplicar é a casa ter continuado
			// bloqueada.
			if (Intent.Pet->GetEffectiveSpeed() >= BattleArenaConstants::ObstacleClimbSpeed)
			{
				Intent.bInsideGrid = true;
			}
		}

		// Descendente: remover de trás para a frente mantém válidos os
		// índices ainda não visitados.
		IntencoesConsumidas.Sort([](int32 Esquerda, int32 Direita) { return Esquerda > Direita; });
		for (int32 Index : IntencoesConsumidas)
		{
			Intents.RemoveAt(Index);
		}

		// Passo 3: fora da grade é bloqueio individual, resolvido já aqui —
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

		// Passo 4: disputa de destino, decidida por POSIÇÃO FINAL.
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

			// Destino que continuou bloqueado só chega aqui pela escalada do
			// Passo 2 — e subir num tronco não é andar: quem lê o feed
			// precisa saber que aquele pet passou a lutar de cima.
			if (Intent.bDestinationIsObstacle)
			{
				EmitClimbed(OutTrace, SlotIndex, *Intent.Pet, FromCell, ToCell);
			}
			else
			{
				EmitMoved(OutTrace, SlotIndex, *Intent.Pet, FromCell, ToCell);
			}
		}
	}

	// Passo 5 (Arenas Variadas, design.md — DP-arena-02): dano de casa,
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
		// continua em pé no mesmo lugar. O INCORPÓREO nunca esteve nele.
		if (!Pet.IsOffTheGround()
			&& State.CellLayout[State.CellIndex(Pet.Column, Pet.Row)] == static_cast<uint8>(ECellProperty::Damage))
		{
			Pet.PendingDamage += BattleArenaConstants::CellDamageAmount;
		}
	}
}

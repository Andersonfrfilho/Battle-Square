// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattlePhases.h"
#include "Battle/BattleState.h"
#include "Battle/BattleEvent.h"

namespace
{
	// v1 é 1v1 (spec: mais de um pet por lado é M3) — o primeiro pet vivo
	// do lado é o único pet do lado. A busca por Side, em vez de índice
	// fixo, é o que deixa a fase pronta para N pets sem mudar assinatura.

	void ApplyPostureForSide(
		FBattleState& State,
		uint8 Side,
		const FBattleAction& Action,
		uint8 SlotIndex,
		TArray<FBattleEvent>& OutTrace)
	{
		FPetState* Pet = State.FindAlivePetOnSide(Side);
		if (!Pet)
		{
			return;
		}

		uint16 AssumedFlag = 0;
		if (Action.Type == EActionType::Defender)
		{
			AssumedFlag = static_cast<uint16>(EBattlePostureFlags::Defending);
		}
		else if (Action.Type == EActionType::Esquivar)
		{
			AssumedFlag = static_cast<uint16>(EBattlePostureFlags::Dodging);
		}
		else if (Action.Type == EActionType::Camuflar)
		{
			AssumedFlag = static_cast<uint16>(EBattlePostureFlags::Camouflaged);
		}
		else if (Action.Type == EActionType::Voar)
		{
			AssumedFlag = static_cast<uint16>(EBattlePostureFlags::Flying);
		}
		else if (Action.Type == EActionType::Submergir)
		{
			AssumedFlag = static_cast<uint16>(EBattlePostureFlags::Underground);
		}
		else if (Action.Type == EActionType::Escavar)
		{
			// ERGUE TERRA na casa à frente. Não é postura de quem cava — é
			// mudança do TABULEIRO, e por isso não veste bandeira nenhuma.
			int8 DeltaColuna = 0;
			int8 DeltaLinha = 0;
			GetDirectionDelta(Action.Direction, DeltaColuna, DeltaLinha);

			const int32 Coluna = static_cast<int32>(Pet->Column) + DeltaColuna;
			const int32 Linha = static_cast<int32>(Pet->Row) + DeltaLinha;

			// Fora da grade, sem direção, ou casa OCUPADA: não ergue. Levantar
			// terra em cima de alguém seria enterrá-lo, e enterrar é outra
			// jogada — com outro nome, outro custo e outro evento.
			const bool bTemAlguem = State.Pets.ContainsByPredicate(
				[Coluna, Linha](const FPetState& Outro)
				{
					return Outro.IsAlive()
						&& static_cast<int32>(Outro.Column) == Coluna
						&& static_cast<int32>(Outro.Row) == Linha;
				});

			const int32 Casa = State.IsInside(Coluna, Linha)
				? State.CellIndex(Coluna, Linha) : INDEX_NONE;

			const bool bPodeErguer = Casa != INDEX_NONE && !bTemAlguem
				&& State.CellLayout[Casa] == static_cast<uint8>(ECellProperty::None);

			if (!bPodeErguer)
			{
				FBattleEvent Falhou;
				Falhou.Type = EBattleEventType::PosturaFalhou;
				Falhou.SlotIndex = SlotIndex;
				Falhou.Phase = 2;
				Falhou.ActorId = Pet->PetId;
				Falhou.TargetId = BattleEventNoActor;
				Falhou.Detail = static_cast<uint8>(Action.Type);
				OutTrace.Add(Falhou);
				return;
			}

			State.SetTemporaryTerrain(Coluna, Linha,
				static_cast<uint8>(ECellProperty::Blocked), /*Slots=*/0);

			FBattleEvent Ergueu;
			Ergueu.Type = EBattleEventType::TerrenoMudou;
			Ergueu.SlotIndex = SlotIndex;
			Ergueu.Phase = 2;
			Ergueu.ActorId = Pet->PetId;
			Ergueu.TargetId = BattleEventNoActor;
			Ergueu.ToCell = PackCell(static_cast<uint8>(Coluna), static_cast<uint8>(Linha));
			Ergueu.Value = static_cast<int32>(ECellProperty::Blocked);
			OutTrace.Add(Ergueu);
			return;
		}
		else if (Action.Type == EActionType::Incendiar)
		{
			// QUEIMA a casa à frente. Par de escavar: a terra constrói, o fogo
			// cria perigo. Os dois mudam a casa da frente e nenhum causa dano
			// direto — são negação de espaço, e é isso que os separa de atacar.
			int8 DeltaColuna = 0;
			int8 DeltaLinha = 0;
			GetDirectionDelta(Action.Direction, DeltaColuna, DeltaLinha);

			const int32 Coluna = static_cast<int32>(Pet->Column) + DeltaColuna;
			const int32 Linha = static_cast<int32>(Pet->Row) + DeltaLinha;
			const int32 Casa = State.IsInside(Coluna, Linha)
				? State.CellIndex(Coluna, Linha) : INDEX_NONE;

			// Casa BLOQUEADA não pega fogo: quem explica aquela casa é a pedra
			// que está nela, e brasa sobre pedra seria a tela dizendo duas
			// coisas ao mesmo tempo.
			const bool bPodeQueimar = Casa != INDEX_NONE
				&& State.CellLayout[Casa] != static_cast<uint8>(ECellProperty::Blocked);

			if (!bPodeQueimar)
			{
				FBattleEvent Falhou;
				Falhou.Type = EBattleEventType::PosturaFalhou;
				Falhou.SlotIndex = SlotIndex;
				Falhou.Phase = 2;
				Falhou.ActorId = Pet->PetId;
				Falhou.TargetId = BattleEventNoActor;
				Falhou.Detail = static_cast<uint8>(Action.Type);
				OutTrace.Add(Falhou);
				return;
			}

			State.SetTemporaryTerrain(Coluna, Linha,
				static_cast<uint8>(ECellProperty::Damage), /*Slots=*/0);

			FBattleEvent Queimou;
			Queimou.Type = EBattleEventType::TerrenoMudou;
			Queimou.SlotIndex = SlotIndex;
			Queimou.Phase = 2;
			Queimou.ActorId = Pet->PetId;
			Queimou.TargetId = BattleEventNoActor;
			Queimou.ToCell = PackCell(static_cast<uint8>(Coluna), static_cast<uint8>(Linha));
			Queimou.Value = static_cast<int32>(ECellProperty::Damage);
			OutTrace.Add(Queimou);
			return;
		}
		else if (Action.Type == EActionType::Iluminar)
		{
			// ILUMINAR não é postura de quem a usa: ela marca o OUTRO. Por
			// isso não passa pelo caminho comum abaixo, que veste a bandeira
			// em quem agiu — iluminar a si mesmo não revelaria ninguém.
			FPetState* Alvo = State.FindAlivePetOnSide(Side == 0 ? 1 : 0);
			if (!Alvo)
			{
				return;
			}

			Alvo->PostureFlags |= static_cast<uint16>(EBattlePostureFlags::Revealed);

			FBattleEvent Luz;
			Luz.Type = EBattleEventType::Revelado;
			Luz.SlotIndex = SlotIndex;
			Luz.Phase = 2;
			Luz.ActorId = Pet->PetId;
			Luz.TargetId = Alvo->PetId;
			OutTrace.Add(Luz);
			return;
		}
		else if (Action.Type == EActionType::Atravessar)
		{
			// ATRAVESSAR é do INCORPÓREO, e a recusa é dado como todas as
			// outras: quem tem corpo não passa por dentro do tronco, e dizer
			// isso aqui como `if` reabriria a porta que a fundura fechou.
			if (!Pet->HasTrait(EPetTrait::Incorporeo))
			{
				FBattleEvent Falhou;
				Falhou.Type = EBattleEventType::PosturaFalhou;
				Falhou.SlotIndex = SlotIndex;
				Falhou.Phase = 2;
				Falhou.ActorId = Pet->PetId;
				Falhou.TargetId = BattleEventNoActor;
				Falhou.Detail = static_cast<uint8>(Action.Type);
				OutTrace.Add(Falhou);
				return;
			}

			AssumedFlag = static_cast<uint16>(EBattlePostureFlags::Phasing);
		}
		else
		{
			return; // Ação não é de postura — nada a fazer nesta fase.
		}

		// O requisito de terreno é DADO, e não mais um `if` por skill.
		//
		// Era "submergir exige água", escrito aqui — e por isso `escavar`, que
		// quer pedra, nunca pôde existir sem editar este arquivo. Agora a
		// montagem declara o que cada ação exige, e este ponto só confere.
		if (!State.TerrainAllowsSkill(Action.Type,
			State.CellLayout[State.CellIndex(Pet->Column, Pet->Row)]))
		{
			// Falha ALTA, com evento próprio. Silenciosamente virar Aguardar
			// deixaria o jogador achando que a skill não funciona, quando o
			// que faltava era ele estar na casa certa.
			FBattleEvent Falha;
			Falha.Type = EBattleEventType::PosturaFalhou;
			Falha.SlotIndex = SlotIndex;
			Falha.Phase = 2;
			Falha.ActorId = Pet->PetId;
			Falha.TargetId = BattleEventNoActor;
			Falha.Value = static_cast<int32>(AssumedFlag);
			OutTrace.Add(Falha);
			return;
		}

		Pet->PostureFlags |= AssumedFlag;

		FBattleEvent Event;
		Event.Type = EBattleEventType::PosturaAssumida;
		Event.SlotIndex = SlotIndex;
		Event.Phase = 2; // F2
		Event.ActorId = Pet->PetId;
		Event.TargetId = BattleEventNoActor;

		// QUAL postura, e não só "assumiu alguma". Sem isto a tela diria
		// "assumiu postura" para camuflar, voar e submergir igualmente — e o
		// jogador não teria como aprender que magia fura camuflagem.
		Event.Value = static_cast<int32>(AssumedFlag);
		OutTrace.Add(Event);
	}
}

void BattlePhases::ApplyPostures(
	FBattleState& State,
	const FBattleAction& LeftAction,
	const FBattleAction& RightAction,
	uint8 SlotIndex,
	TArray<FBattleEvent>& OutTrace)
{
	ApplyPostureForSide(State, /*Side=*/0, LeftAction, SlotIndex, OutTrace);
	ApplyPostureForSide(State, /*Side=*/1, RightAction, SlotIndex, OutTrace);
}

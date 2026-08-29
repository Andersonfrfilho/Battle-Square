// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattlePhases.h"
#include "Battle/BattleState.h"
#include "Battle/BattleEvent.h"
#include "Battle/BattleArenaConstants.h"

namespace
{
	// Balanceamento é parâmetro, não estrutura (design.md) — mas os valores
	// vivem em algum lugar até virarem DataAsset de skill (M3/DP-04).
	// Percentuais inteiros: 100 = 1.0x. Nenhum float em lugar nenhum (AD-004).
	constexpr int32 MinDamage = 1;
	constexpr int32 DefendingDefenseFactorPercent = 150; // +50% de defesa efetiva ao defender

	// DP-ia-04: alvo no céu não tem para onde desviar da magia.
	constexpr int32 ExposedInTheAirDamagePercent = 150;
	constexpr int32 AttackDamageMultiplierPercent = 100;
	constexpr int32 MagicDamageMultiplierPercent = 150;

	bool HasPosture(const FPetState& Pet, EBattlePostureFlags Flag)
	{
		return (Pet.PostureFlags & static_cast<uint8>(Flag)) != 0;
	}


	// Encontra um oponente vivo na célula informada.
	FPetState* FindLivingOpponentAtCell(FBattleState& State, uint8 AttackerSide, int32 Column, int32 Row)
	{
		for (FPetState& Pet : State.Pets)
		{
			if (Pet.Side != AttackerSide && Pet.IsAlive() && Pet.Column == Column && Pet.Row == Row)
			{
				return &Pet;
			}
		}
		return nullptr;
	}

	// DP-04 (spec.md, decidida): alcance 1 na direção escolhida, mais a
	// própria casa se houver oponente coabitando (zero-alcance/melee).
	// A própria casa é checada primeiro: se o oponente já está em cima de
	// você, não faz sentido a direção decidir se ele é alvo ou não.
	/**
	 * Alvo do ataque: o adversário ADJACENTE, escolhido sozinho.
	 *
	 * A direção deixou de decidir o alvo em 2026-08-29 (DP-golpe-05). Ela
	 * existia porque o commit é às cegas — mirar era apostar onde o inimigo
	 * VAI estar — mas respondia a pergunta errada: num 3x3 com um oponente,
	 * "onde" quase sempre é "nele", e a decisão virava cerimônia com chance de
	 * acertar o vazio. O que se escolhe agora é QUAL GOLPE.
	 *
	 * A busca por oponente COABITANDO a própria casa saiu daqui com a inversão
	 * do DP-02: F3 impede que dois pets terminem no mesmo ponto.
	 *
	 * Empate é resolvido pelo MENOR PetId, e não pela ordem do array: ordem de
	 * contêiner não é determinismo, e com mais de um pet por lado (M3+) isso
	 * decidiria a batalha por acaso de inserção.
	 */
	FPetState* ResolveTarget(FBattleState& State, const FPetState& Attacker)
	{
		FPetState* Escolhido = nullptr;

		for (FPetState& Candidato : State.Pets)
		{
			if (Candidato.Side == Attacker.Side || !Candidato.IsAlive())
			{
				continue;
			}

			const int32 DistanciaColuna = FMath::Abs(
				static_cast<int32>(Candidato.Column) - static_cast<int32>(Attacker.Column));
			const int32 DistanciaLinha = FMath::Abs(
				static_cast<int32>(Candidato.Row) - static_cast<int32>(Attacker.Row));

			if (DistanciaColuna > 1 || DistanciaLinha > 1)
			{
				continue;
			}

			if (!Escolhido || Candidato.PetId < Escolhido->PetId)
			{
				Escolhido = &Candidato;
			}
		}

		return Escolhido;
	}

	bool IsOnBuffCell(const FPetState& Pet, const TArray<uint8>& CellLayout)
	{
		return CellLayout[CellLayoutIndex(Pet.Column, Pet.Row)] == static_cast<uint8>(ECellProperty::Buff);
	}

	// Fórmula de dano — só inteiros, multiplicador em percentual (design.md).
	// EfetivoAtaque = Ataque * (BuffAtacante ? CellBuffPercent : 100) / 100
	// DefesaEfetiva = Defesa * FatorDefesa / 100, onde FatorDefesa combina
	//   Defendendo e casa de buff (Arenas Variadas, design.md — buff é
	//   contextual: fortalece quem ataca a partir dela E quem defende
	//   nela, nunca persiste em FPetState).
	// Dano          = Max(DanoMinimo, EfetivoAtaque - DefesaEfetiva)
	int32 ComputeDamage(const FPetState& Attacker, const FPetState& Target, int32 ActionMultiplierPercent, const TArray<uint8>& CellLayout)
	{
		const bool bTargetDefending = HasPosture(Target, EBattlePostureFlags::Defending);
		const bool bAttackerBuffed = IsOnBuffCell(Attacker, CellLayout);
		const bool bTargetBuffed = IsOnBuffCell(Target, CellLayout);

		const int32 EffectiveAttack = bAttackerBuffed
			? (Attacker.Attack * BattleArenaConstants::CellBuffPercent) / 100
			: Attacker.Attack;

		int32 DefenseFactorPercent = bTargetDefending ? DefendingDefenseFactorPercent : 100;
		if (bTargetBuffed)
		{
			DefenseFactorPercent = (DefenseFactorPercent * BattleArenaConstants::CellBuffPercent) / 100;
		}
		const int32 EffectiveDefense = (Target.Defense * DefenseFactorPercent) / 100;

		const int32 RawDamage = (EffectiveAttack * ActionMultiplierPercent) / 100 - EffectiveDefense;
		return FMath::Max(MinDamage, RawDamage);
	}

	void EmitMiss(TArray<FBattleEvent>& OutTrace, uint8 SlotIndex, const FPetState& Attacker)
	{
		FBattleEvent Event;
		Event.Type = EBattleEventType::AtaqueErrou;
		Event.SlotIndex = SlotIndex;
		Event.Phase = 4; // F4
		Event.ActorId = Attacker.PetId;
		Event.TargetId = BattleEventNoActor;
		OutTrace.Add(Event);
	}

	void EmitDodged(TArray<FBattleEvent>& OutTrace, uint8 SlotIndex, const FPetState& Attacker, const FPetState& Target)
	{
		FBattleEvent Event;
		Event.Type = EBattleEventType::Esquivou;
		Event.SlotIndex = SlotIndex;
		Event.Phase = 4; // F4
		Event.ActorId = Target.PetId; // quem executou a esquiva é o sujeito do evento
		Event.TargetId = Attacker.PetId;
		OutTrace.Add(Event);
	}

	void EmitHit(TArray<FBattleEvent>& OutTrace, uint8 SlotIndex, const FPetState& Attacker, const FPetState& Target, int32 Damage)
	{
		FBattleEvent Event;
		Event.Type = EBattleEventType::AtaqueAcertou;
		Event.SlotIndex = SlotIndex;
		Event.Phase = 4; // F4
		Event.ActorId = Attacker.PetId;
		Event.TargetId = Target.PetId;
		Event.Value = Damage;
		OutTrace.Add(Event);
	}

	// Declarada antes porque ResolveAttackForSide a chama e ela vem depois.
	void ApplyHitAgainst(FBattleState& State, FPetState& Attacker, FPetState& Target,
		bool bIsMagic, int32 MovePower, uint8 TerrainEffect, uint8 SlotIndex, TArray<FBattleEvent>& OutTrace);

	FPetState* FindPetById(FBattleState& State, uint8 PetId)
	{
		for (FPetState& Pet : State.Pets)
		{
			if (Pet.PetId == PetId && Pet.IsAlive())
			{
				return &Pet;
			}
		}
		return nullptr;
	}

	void ResolveAttackForSide(
		FBattleState& State,
		uint8 AttackerSide,
		const FBattleAction& Action,
		uint8 SlotIndex,
		TArray<FBattleEvent>& OutTrace)
	{
		if (Action.Type != EActionType::Atacar && Action.Type != EActionType::Magia)
		{
			return;
		}

		FPetState* Attacker = State.FindAlivePetOnSide(AttackerSide);
		if (!Attacker)
		{
			return;
		}

		// DP-ia-04: o preço de ter ficado intocável no slot anterior. Sai como
		// ataque que ERRA, e não como silêncio, para o jogador ver o custo da
		// escolha dele em vez de achar que a ação sumiu.
		if (HasPosture(*Attacker, EBattlePostureFlags::Revealing)
			|| HasPosture(*Attacker, EBattlePostureFlags::Emerging))
		{
			EmitMiss(OutTrace, SlotIndex, *Attacker);
			return;
		}

		FPetState* Target = ResolveTarget(State, *Attacker);
		if (!Target)
		{
			EmitMiss(OutTrace, SlotIndex, *Attacker);
			return;
		}

		const bool bIsMagic = (Action.Type == EActionType::Magia);

		// O poder do GOLPE substitui o multiplicador fixo. Poder 0 = pet sem
		// golpe cadastrado, e aí vale o multiplicador de antes: tratar zero
		// como poder faria esse pet bater sem dano nenhum.
		const uint8 MoveIndex = GetMoveIndexFromAction(Action);
		const int32 MovePower = Attacker->GetMovePower(MoveIndex);
		const uint8 TerrainEffect = Attacker->GetMoveTerrainEffect(MoveIndex);

		ApplyHitAgainst(State, *Attacker, *Target, bIsMagic, MovePower, TerrainEffect, SlotIndex, OutTrace);
	}

	/**
	 * O golpe em si, a partir de atacante e alvo JÁ resolvidos.
	 *
	 * Existe separado porque o encontro no mesmo ponto (DP-02) precisa passar
	 * exatamente por aqui: é o que faz "defendeu, sofre menos; esquivou, não
	 * sofre" valer na trombada sem ninguém reescrever as posturas. Duas
	 * escadas de postura em lugares diferentes concordariam até a primeira
	 * edição.
	 */
	void ApplyHitAgainst(
		FBattleState& State,
		FPetState& Attacker,
		FPetState& Target,
		bool bIsMagic,
		int32 MovePower,
		uint8 TerrainEffect,
		uint8 SlotIndex,
		TArray<FBattleEvent>& OutTrace)
	{
		FPetState* TargetPtr = &Target;
		FPetState* AttackerPtr = &Attacker;

		// DP-ia-04. Camuflado e submerso não são ALCANÇÁVEIS — nem por magia.
		// É isso que os separa de Esquivar, que barra só o físico: se
		// barrassem o mesmo, seriam três nomes para a mesma ação.
		if (HasPosture(*TargetPtr, EBattlePostureFlags::Camouflaged)
			|| HasPosture(*TargetPtr, EBattlePostureFlags::Underground))
		{
			EmitMiss(OutTrace, SlotIndex, *AttackerPtr);
			return;
		}

		// Voar tira o pet do alcance do golpe FÍSICO, mas o expõe no céu — a
		// magia acerta, e acerta mais forte. É troca, não escudo.
		if (!bIsMagic && HasPosture(*TargetPtr, EBattlePostureFlags::Flying))
		{
			EmitMiss(OutTrace, SlotIndex, *AttackerPtr);
			return;
		}

		// Esquiva anula ataque FÍSICO. Magia ignora esquiva (BTL-10) —
		// é o segundo lado do triângulo ataque/defesa/esquiva.
		if (!bIsMagic && HasPosture(*TargetPtr, EBattlePostureFlags::Dodging))
		{
			EmitDodged(OutTrace, SlotIndex, *AttackerPtr, *TargetPtr);
			return;
		}

		// Poder do golpe manda; sem golpe, o multiplicador padrão do tipo de
		// ação. Assim um pet legado continua lutando como antes.
		int32 Multiplier = MovePower > 0
			? MovePower
			: (bIsMagic ? MagicDamageMultiplierPercent : AttackDamageMultiplierPercent);
		if (bIsMagic && HasPosture(*TargetPtr, EBattlePostureFlags::Flying))
		{
			Multiplier = (Multiplier * ExposedInTheAirDamagePercent) / 100;
		}

		const int32 Damage = ComputeDamage(*AttackerPtr, *TargetPtr, Multiplier, State.CellLayout);

		// Acumula — NÃO aplica. F5 (T8) aplica tudo de uma vez (BTL-07).
		TargetPtr->PendingDamage += Damage;

		EmitHit(OutTrace, SlotIndex, *AttackerPtr, *TargetPtr, Damage);

		// O golpe DEIXA algo na casa que acertou.
		//
		// Só no ACERTO: terreno mudando num golpe que errou tiraria do jogador
		// a relação entre causa e efeito, que é o que torna a cadeia
		// jogável — bater na grama e ver a casa mudar ensina; ver mudar sem
		// acertar é ruído.
		if (TerrainEffect != static_cast<uint8>(ECellProperty::None))
		{
			const int32 CellIndex = CellLayoutIndex(TargetPtr->Column, TargetPtr->Row);
			if (State.CellLayout.IsValidIndex(CellIndex)
				&& State.CellLayout[CellIndex] != static_cast<uint8>(ECellProperty::Blocked))
			{
				// Casa BLOQUEADA nunca muda: ela é estrutura da arena, não
				// superfície. Um golpe que abrisse passagem mudaria o
				// tabuleiro no meio do turno, e o movimento já foi resolvido.
				State.CellLayout[CellIndex] = TerrainEffect;

				FBattleEvent Terreno;
				Terreno.Type = EBattleEventType::TerrenoMudou;
				Terreno.SlotIndex = SlotIndex;
				Terreno.Phase = 4;
				Terreno.ActorId = AttackerPtr->PetId;
				Terreno.TargetId = TargetPtr->PetId;
				Terreno.ToCell = PackCell(TargetPtr->Column, TargetPtr->Row);
				Terreno.Value = static_cast<int32>(TerrainEffect);
				OutTrace.Add(Terreno);
			}
		}
	}
}

void BattlePhases::ApplyCombat(
	FBattleState& State,
	const FBattleAction& LeftAction,
	const FBattleAction& RightAction,
	uint8 SlotIndex,
	TArray<FBattleEvent>& OutTrace)
{
	ResolveAttackForSide(State, /*AttackerSide=*/0, LeftAction, SlotIndex, OutTrace);
	ResolveAttackForSide(State, /*AttackerSide=*/1, RightAction, SlotIndex, OutTrace);

	// DP-02: o encontro no mesmo ponto vira golpe MÚTUO, resolvido pelo mesmo
	// caminho do ataque. F3 registrou o encontro no traço; ler dali evita um
	// campo novo em FBattleState, que entraria no hash e invalidaria os
	// snapshots de determinismo de cenários que nem se trombam.
	//
	// O laço percorre uma cópia dos índices porque ApplyHitAgainst ACRESCENTA
	// eventos ao traço enquanto ele é lido.
	TArray<int32> Encontros;
	for (int32 Index = 0; Index < OutTrace.Num(); ++Index)
	{
		if (OutTrace[Index].Type == EBattleEventType::EncontroNoMesmoPonto
			&& OutTrace[Index].SlotIndex == SlotIndex)
		{
			Encontros.Add(Index);
		}
	}

	for (int32 Index : Encontros)
	{
		FPetState* Um = FindPetById(State, OutTrace[Index].ActorId);
		FPetState* Outro = FindPetById(State, OutTrace[Index].TargetId);
		if (!Um || !Outro)
		{
			continue;
		}

		// Trombada é FÍSICA nos dois sentidos: quem esquivou escapa, quem
		// defendeu amortece — exatamente as ações que já estavam registradas.
		// Trombada não é golpe: passa poder 0 para valer o multiplicador
		// físico padrão. Usar o golpe de alguém aqui faria a colisão ferir
		// conforme uma escolha que ninguém fez.
		ApplyHitAgainst(State, *Um, *Outro, /*bIsMagic=*/false, /*MovePower=*/0,
			/*TerrainEffect=*/0, SlotIndex, OutTrace);
		ApplyHitAgainst(State, *Outro, *Um, /*bIsMagic=*/false, /*MovePower=*/0,
			/*TerrainEffect=*/0, SlotIndex, OutTrace);
	}
}

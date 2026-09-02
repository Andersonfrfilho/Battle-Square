// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattlePhases.h"
#include "Battle/BattleState.h"
#include "Battle/FluidRegistry.h"
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

	// Encaminha para FPetState::HasPosture — uma definição só. A versão que
	// vivia aqui convertia para uint8 e apagava todo flag acima do sétimo.
	bool HasPosture(const FPetState& Pet, EBattlePostureFlags Flag)
	{
		return Pet.HasPosture(Flag);
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

	bool IsOnBuffCell(const FPetState& Pet, const FBattleState& State)
	{
		return State.CellLayout[State.CellIndex(Pet.Column, Pet.Row)] == static_cast<uint8>(ECellProperty::Buff);
	}

	// Fórmula de dano — só inteiros, multiplicador em percentual (design.md).
	// EfetivoAtaque = Ataque * (BuffAtacante ? CellBuffPercent : 100) / 100
	// DefesaEfetiva = Defesa * FatorDefesa / 100, onde FatorDefesa combina
	//   Defendendo e casa de buff (Arenas Variadas, design.md — buff é
	//   contextual: fortalece quem ataca a partir dela E quem defende
	//   nela, nunca persiste em FPetState).
	// Dano          = Max(DanoMinimo, EfetivoAtaque - DefesaEfetiva)
	int32 ComputeDamage(const FPetState& Attacker, const FPetState& Target, int32 ActionMultiplierPercent, const FBattleState& State)
	{
		const bool bTargetDefending = HasPosture(Target, EBattlePostureFlags::Defending);
		const bool bAttackerBuffed = IsOnBuffCell(Attacker, State);
		const bool bTargetBuffed = IsOnBuffCell(Target, State);

		// Atributo JÁ com a magia de efeito somada: é aqui que "subir o ataque"
		// deixa de ser um número guardado e vira dano na tela.
		const int32 AtaqueDoPet = Attacker.GetEffectiveAttack();
		const int32 EffectiveAttack = bAttackerBuffed
			? (AtaqueDoPet * BattleArenaConstants::CellBuffPercent) / 100
			: AtaqueDoPet;

		int32 DefenseFactorPercent = bTargetDefending ? DefendingDefenseFactorPercent : 100;
		if (bTargetBuffed)
		{
			DefenseFactorPercent = (DefenseFactorPercent * BattleArenaConstants::CellBuffPercent) / 100;
		}
		const int32 EffectiveDefense = (Target.GetEffectiveDefense() * DefenseFactorPercent) / 100;

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

	void EmitStatEffect(TArray<FBattleEvent>& OutTrace, uint8 SlotIndex,
		const FPetState& Caster, const FPetState& Affected, EBattleStat Stat, int32 Percent)
	{
		FBattleEvent Event;
		Event.Type = EBattleEventType::AtributoAlterado;
		Event.SlotIndex = SlotIndex;
		Event.Phase = 4; // F4
		Event.ActorId = Caster.PetId;
		Event.TargetId = Affected.PetId;
		Event.Detail = static_cast<uint8>(Stat);
		Event.Value = Percent;
		OutTrace.Add(Event);
	}

	/**
	 * Aplica o efeito do golpe, SUBSTITUINDO o que estivesse ativo.
	 *
	 * O sinal decide o alvo: positivo em si, negativo no oponente. Recortar no
	 * teto acontece AQUI e não na montagem, pelo mesmo motivo dos tetos de
	 * esquiva: amarra de jogo não é acordo entre camadas, e um estado vindo da
	 * rede não pode passar por cima dela.
	 */
	void ApplyStatEffect(FPetState& Caster, FPetState& Opponent, uint8 SlotIndex,
		uint8 StatRaw, int32 Percent, TArray<FBattleEvent>& OutTrace)
	{
		const EBattleStat Stat = static_cast<EBattleStat>(StatRaw);
		if (Stat == EBattleStat::Nenhum || Percent == 0)
		{
			return;
		}

		const int32 Recortado = FMath::Clamp(Percent,
			-BattleStatEffectMaxPercent, BattleStatEffectMaxPercent);

		FPetState& Afetado = Recortado > 0 ? Caster : Opponent;

		Afetado.ActiveEffectStat = StatRaw;
		Afetado.ActiveEffectPercent = Recortado;
		Afetado.ActiveEffectSlotsRemaining = BattleStatEffectSlots;

		EmitStatEffect(OutTrace, SlotIndex, Caster, Afetado, Stat, Recortado);
	}

	void EmitReflexDodge(TArray<FBattleEvent>& OutTrace, uint8 SlotIndex, const FPetState& Attacker, const FPetState& Target)
	{
		FBattleEvent Event;
		Event.Type = EBattleEventType::EsquivouPorReflexo;
		Event.SlotIndex = SlotIndex;
		Event.Phase = 4; // F4
		Event.ActorId = Target.PetId; // quem desviou é o sujeito do evento
		Event.TargetId = Attacker.PetId;
		OutTrace.Add(Event);
	}

	/**
	 * Os tetos, aplicados AQUI mesmo que a montagem já os tenha respeitado.
	 *
	 * DP-atr-07 é uma amarra do jogo, não uma convenção entre camadas: se ela
	 * morasse só na montagem, bastaria um erro lá — ou um estado vindo da
	 * rede — para atributo alto virar imunidade. Recortar duas vezes custa uma
	 * comparação; confiar custa a partida.
	 */
	int32 ClampedReflexDodgePercent(const FPetState& Target)
	{
		return FMath::Clamp(Target.ReflexDodgePercent, 0,
			BattleArenaConstants::ReflexDodgeMaxPercent);
	}

	int32 ClampedDamageVariancePercent(const FPetState& Attacker)
	{
		return FMath::Clamp(Attacker.DamageVariancePercent, 0,
			BattleArenaConstants::DamageVarianceBasePercent);
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
		bool bIsMagic, int32 MovePower, FTerrainDeposit Deposito, int32 DrainPercent,
		bool bConducts, uint8 SlotIndex, TArray<FBattleEvent>& OutTrace);

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
		const FTerrainDeposit Deposito{
			Attacker->GetMoveTerrainEffect(MoveIndex),
			Attacker->GetMoveTerrainDuration(MoveIndex) };

		// CONCENTRAÇÃO QUEBRADA: quem está apanhando não conclui um golpe que
		// exige foco.
		//
		// A checagem vem DEPOIS de resolver alvo e golpe, e antes de qualquer
		// efeito: quebrar o foco não pode aplicar meio golpe — nem o efeito de
		// atributo, nem o terreno, nem a drenagem. Golpe pela metade seria a
		// regra punindo e recompensando no mesmo slot.
		//
		// E é aqui, e não na escolha da ação: o jogador escolhe antes de saber
		// se vai apanhar, e proibir a escolha diria "você não pode" a quem
		// ainda podia. A ação se escolhe; o dano é que a desfaz.
		if (Attacker->MoveNeedsFocusAt(MoveIndex) && Attacker->IsUnderFire())
		{
			FBattleEvent Quebrou;
			Quebrou.Type = EBattleEventType::ConcentracaoQuebrada;
			Quebrou.SlotIndex = SlotIndex;
			Quebrou.Phase = 4;
			Quebrou.ActorId = Attacker->PetId;
			Quebrou.TargetId = Target->PetId;
			Quebrou.Value = Attacker->PendingDamage;
			OutTrace.Add(Quebrou);
			return;
		}

		// A MAGIA DE ATRIBUTO acontece ANTES do dano, e por dois motivos: um
		// golpe que sobe o próprio ataque precisa valer já neste acerto, senão
		// o jogador gasta um slot para nada; e um que derruba a defesa do
		// outro precisa valer contra o dano que vem em seguida — que é
		// exatamente o que torna a jogada interessante.
		//
		// Só a MAGIA carrega efeito. Ataque físico que mexesse em atributo
		// apagaria a diferença entre as duas ações, e é ela que dá sentido à
		// escola psíquica.
		if (bIsMagic)
		{
			ApplyStatEffect(*Attacker, *Target, SlotIndex,
				Attacker->GetMoveEffectStat(MoveIndex),
				Attacker->GetMoveEffectPercent(MoveIndex),
				OutTrace);
		}

		ApplyHitAgainst(State, *Attacker, *Target, bIsMagic, MovePower, Deposito,
			Attacker->GetMoveDrainPercent(MoveIndex),
			Attacker->MoveConducts[FMath::Clamp(static_cast<int32>(MoveIndex), 0, 3)] != 0,
			SlotIndex, OutTrace);
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
	/**
	 * Está EM CIMA de um obstáculo — a casa dele continuou bloqueada, e só se
	 * chega a uma dessas escalando (BattlePhaseMovement, Passo 2).
	 *
	 * A altura não é campo de FPetState de propósito: ela já está escrita na
	 * posição do pet somada ao tabuleiro, e um campo separado seria uma
	 * segunda cópia da mesma verdade — foi assim que L-032 e L-033
	 * aconteceram. Também é o que dispensa zerá-la: o pet desce quando anda
	 * para fora, e cai junto se alguém derrubar o tronco.
	 */
	bool IsStandingOnObstacle(const FBattleState& State, const FPetState& Pet)
	{
		return State.CellLayout[State.CellIndex(Pet.Column, Pet.Row)]
			== static_cast<uint8>(ECellProperty::Blocked);
	}

	void ApplyHitAgainst(
		FBattleState& State,
		FPetState& Attacker,
		FPetState& Target,
		bool bIsMagic,
		int32 MovePower,
		FTerrainDeposit Deposito,
		int32 DrainPercent,
		bool bConducts,
		uint8 SlotIndex,
		TArray<FBattleEvent>& OutTrace)
	{
		FPetState* TargetPtr = &Target;
		FPetState* AttackerPtr = &Attacker;

		// A LUZ desfaz o esconderijo. Enquanto revelado, o alvo é alcançável
		// como qualquer outro — e é isto que dá contra-jogo ao fantasma em vez
		// de deixá-lo intocável para quem não trouxe magia.
		const bool bRevelado = HasPosture(*TargetPtr, EBattlePostureFlags::Revealed);

		// DP-ia-04. Camuflado e submerso não são ALCANÇÁVEIS — nem por magia.
		// É isso que os separa de Esquivar, que barra só o físico: se
		// barrassem o mesmo, seriam três nomes para a mesma ação.
		if (!bRevelado
			&& (HasPosture(*TargetPtr, EBattlePostureFlags::Camouflaged)
				|| HasPosture(*TargetPtr, EBattlePostureFlags::Underground)))
		{
			EmitMiss(OutTrace, SlotIndex, *AttackerPtr);
			return;
		}

		// O GOLPE FÍSICO NÃO ALCANÇA O INCORPÓREO. É o que ele É: o punho
		// atravessa, e não há o que acertar.
		//
		// Isso NÃO o torna invencível, e o motivo é estrutural: `Magia` é ação
		// UNIVERSAL (DP-skill-03), então todo pet do jogo consegue alcançá-lo.
		// O que ele compra é obrigar o outro a lutar pela metade do repertório
		// — e quem trouxe luz desfaz até isso.
		if (!bIsMagic && !bRevelado && TargetPtr->HasTrait(EPetTrait::Incorporeo))
		{
			EmitMiss(OutTrace, SlotIndex, *AttackerPtr);
			return;
		}

		// Voar tira o pet do alcance do golpe FÍSICO, mas o expõe no céu — a
		// magia acerta, e acerta mais forte. É troca, não escudo.
		//
		// Do alto de um obstáculo, porém, o golpe físico ALCANÇA quem voou:
		// é a habilidade que subir destrava, e ela cai por si quando o pet
		// desce ou o tronco vai ao chão. Sem isto, escalar seria só dano
		// extra, e voar continuaria imune a tudo que não fosse magia.
		const bool bAttackerIsElevated = IsStandingOnObstacle(State, *AttackerPtr);
		if (!bIsMagic && HasPosture(*TargetPtr, EBattlePostureFlags::Flying) && !bAttackerIsElevated)
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

		// Esquiva por REFLEXO: sem ter gastado a ação em Esquivar.
		//
		// Só contra FÍSICO, pela mesma razão da declarada: magia já ignora
		// esquiva (BTL-10), e ignorar as duas tornaria magia obrigatória.
		//
		// O sorteio sai do gerador DO ESTADO (DP-atr-06). Qualquer outra
		// fonte faria o replay divergir, e numa partida em rede os dois lados
		// resolveriam diferente — o defeito mais caro que este projeto pode
		// ter. É também por isso que ele só é consultado DEPOIS de todas as
		// recusas determinísticas: sortear antes gastaria um número da
		// sequência num caso já decidido, e o hash divergiria por nada.
		if (!bIsMagic)
		{
			const int32 ChanceDeReflexo = ClampedReflexDodgePercent(*TargetPtr);
			if (ChanceDeReflexo > 0 && State.Random.NextRange(1, 100) <= ChanceDeReflexo)
			{
				EmitReflexDodge(OutTrace, SlotIndex, *AttackerPtr, *TargetPtr);
				return;
			}
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

		// De CIMA para BAIXO bate mais forte. Exige que o alvo esteja mais
		// embaixo: dois pets escalados brigam de igual para igual, senão
		// subir premiaria até quem ataca alguém já no alto.
		if (bAttackerIsElevated && !IsStandingOnObstacle(State, *TargetPtr))
		{
			Multiplier = (Multiplier * BattleArenaConstants::ElevatedAttackPercent) / 100;
		}

		const int32 DanoBase = ComputeDamage(*AttackerPtr, *TargetPtr, Multiplier, State);

		// A variação é aplicada ao dano JÁ calculado, e não ao multiplicador:
		// no multiplicador ela se misturaria ao bônus de casa e à efetividade
		// de tipo, e o mesmo sorteio pesaria diferente conforme o terreno.
		// Faixa ZERO não sorteia. Sortear em [0,0] devolveria sempre zero e
		// pareceria inofensivo, mas gastaria um número da sequência — e o
		// hash de um pet sem acaso passaria a depender de quantos ataques
		// aconteceram antes dele.
		const int32 Faixa = ClampedDamageVariancePercent(*AttackerPtr);
		const int32 Damage = Faixa > 0
			// Mínimo de 1 depois da variação, como na fórmula base: um golpe
			// que acerta nunca vira zero de dano — seria indistinguível de
			// erro para quem está olhando.
			? FMath::Max(1, (DanoBase * (100 + State.Random.NextRange(-Faixa, Faixa))) / 100)
			: DanoBase;

		// Acumula — NÃO aplica. F5 (T8) aplica tudo de uma vez (BTL-07).
		TargetPtr->PendingDamage += Damage;

		EmitHit(OutTrace, SlotIndex, *AttackerPtr, *TargetPtr, Damage);

		// DRENAR: parte do dano volta como vida. Acumula, não aplica — F5
		// resolve as duas coisas no mesmo instante, e é isso que faz quem
		// drena o golpe que o mataria sobreviver.
		//
		// Sai do dano REAL, e não do poder do golpe: drenar de um alvo que
		// defendeu tem de render menos, senão a defesa dele viraria vantagem
		// para quem bate.
		if (DrainPercent > 0 && Damage > 0)
		{
			const int32 Recuperado = FMath::Max(1, (Damage * DrainPercent) / 100);
			AttackerPtr->PendingHeal += Recuperado;

			FBattleEvent Drenou;
			Drenou.Type = EBattleEventType::VidaDrenada;
			Drenou.SlotIndex = SlotIndex;
			Drenou.Phase = 4;
			Drenou.ActorId = AttackerPtr->PetId;
			Drenou.TargetId = TargetPtr->PetId;
			Drenou.Value = Recuperado;
			OutTrace.Add(Drenou);
		}

		// A ÁGUA CONDUZ.
		//
		// A corrente parte de ONDE O RAIO CAIU, e não de onde o lançador está:
		// assim atirar de fora da poça é uma escolha que se paga com posição,
		// e acertar alguém molhado é o que a recompensa. Quem estiver na mesma
		// poça é alcançado — inclusive o lançador, se ele estiver nela.
		//
		// O alvo direto fica de FORA: ele já levou o raio. Somar a corrente por
		// cima cobraria duas vezes do mesmo pet pelo mesmo golpe.
		if (bConducts)
		{
			TArray<int32> Ligadas;
			State.ConductiveCellsFrom(TargetPtr->Column, TargetPtr->Row, Ligadas);

			const EFluidKind NaPoca = State.FluidAt(TargetPtr->Column, TargetPtr->Row);
			const int32 Corrente = FluidRegistry::ConductedShare(MovePower, NaPoca);

			for (FPetState& Alcancado : State.Pets)
			{
				if (!Alcancado.IsAlive() || Alcancado.PetId == TargetPtr->PetId)
				{
					continue;
				}

				// Fora do chão, fora da corrente: quem voa não encosta na água.
				if (Alcancado.IsOffTheGround()
					|| !Ligadas.Contains(State.CellIndex(Alcancado.Column, Alcancado.Row)))
				{
					continue;
				}

				// QUEM GERA, AGUENTA. Absorve até a própria capacidade e só o
				// excedente passa — e é isto que responde, sem regra à parte,
				// se o lançador se machuca: a corrente que ele mesmo produziu
				// nunca supera o que ele sabe produzir.
				const int32 Passou = Corrente - Alcancado.ConductionCapacity();
				if (Passou <= 0)
				{
					continue;
				}

				// Mesmo acumulador de sempre. F5 aplica e narra.
				Alcancado.PendingDamage += Passou;

				FBattleEvent Conduziu;
				Conduziu.Type = EBattleEventType::AtaqueAcertou;
				Conduziu.SlotIndex = SlotIndex;
				Conduziu.Phase = 4;
				Conduziu.ActorId = AttackerPtr->PetId;
				Conduziu.TargetId = Alcancado.PetId;
				Conduziu.Value = Passou;
				OutTrace.Add(Conduziu);
			}
		}

		// O golpe DEIXA algo na casa que acertou.
		//
		// Só no ACERTO: terreno mudando num golpe que errou tiraria do jogador
		// a relação entre causa e efeito, que é o que torna a cadeia
		// jogável — bater na grama e ver a casa mudar ensina; ver mudar sem
		// acertar é ruído.
		if (Deposito.Terrain != static_cast<uint8>(ECellProperty::None))
		{
			const int32 CellIndex = State.CellIndex(TargetPtr->Column, TargetPtr->Row);
			if (State.CellLayout.IsValidIndex(CellIndex)
				&& State.CellLayout[CellIndex] != static_cast<uint8>(ECellProperty::Blocked))
			{
				// Casa BLOQUEADA não muda AQUI. Ela tem corpo, e derrubá-la
				// é decisão do movimento (BattlePhaseMovement, Passo 2),
				// onde o pet gasta o slot para isso e todo mundo ainda pode
				// reagir ao caminho aberto. Um golpe de F4 que abrisse
				// passagem mudaria o tabuleiro depois de o movimento já ter
				// sido resolvido — e passariam a existir duas regras para
				// remover o mesmo obstáculo, que concordariam até a primeira
				// edição.
				// SECAR apaga em vez de pintar: água, lama e gelo somem e a
				// casa volta a ser chão. É o único depósito que REMOVE, e por
				// isso ele passa por aqui em vez de virar mais um terreno que
				// se acumula sobre o anterior.
				// A ÁGUA APAGA O FOGO.
				//
				// Antes disto, o fogo apenas NÃO CONDUZIA — a ausência de uma
				// regra, não uma regra. Deixar brasa acesa dentro d'água é a
				// casa contando duas coisas incompatíveis ao mesmo tempo, e o
				// jogador que aprendeu "molhado conduz" não tem como adivinhar
				// que molhado também deveria apagar.
				//
				// LAVA NÃO APAGA, e é por isso que a pergunta é ao registro de
				// fluidos e não à fundura da casa: a lava é funda como a água
				// e faz o oposto dela. `IsWater` é onde essa diferença já mora.
				const EFluidKind FluidoDaCasa =
					State.FluidAt(TargetPtr->Column, TargetPtr->Row);
				const bool bMolhado = FluidRegistry::IsWater(FluidoDaCasa);
				const bool bEhFogo =
					Deposito.Terrain == static_cast<uint8>(ECellProperty::Damage);

				if (bMolhado && bEhFogo)
				{
					FBattleEvent Apagou;
					Apagou.Type = EBattleEventType::FogoApagou;
					Apagou.SlotIndex = SlotIndex;
					Apagou.Phase = 4;
					Apagou.ActorId = AttackerPtr->PetId;
					Apagou.TargetId = TargetPtr->PetId;
					Apagou.ToCell = PackCell(TargetPtr->Column, TargetPtr->Row);
					Apagou.Detail = static_cast<int32>(FluidoDaCasa);
					OutTrace.Add(Apagou);
					return;
				}

				// O GOLPE FORTE O BASTANTE TOMA O CAMPO INTEIRO.
				//
				// O limiar compara o PODER com o TAMANHO do tabuleiro, e é
				// isso que impede a regra de virar "golpe de área": o mesmo
				// golpe que alaga uma arena pequena inteira alaga uma casa só
				// numa grande. Uma bandeira no cadastro não teria essa
				// propriedade — bastaria marcá-la num golpe fraco.
				//
				// A regra passa por AQUI, e não por um caminho próprio, para
				// herdar tudo o que este bloco já decide: casa bloqueada não
				// muda, secar apaga em vez de pintar, e a água apaga o fogo
				// casa a casa. Um segundo caminho teria de repetir as três, e
				// as cópias concordariam até a primeira edição.
				if (MovePower >= State.PowerToTakeTheField())
				{
					int32 Mudadas = 0;
					for (int32 Linha = 0; Linha < static_cast<int32>(State.GridRows); ++Linha)
					{
						for (int32 Coluna = 0; Coluna < static_cast<int32>(State.GridColumns); ++Coluna)
						{
							const int32 Indice = State.CellIndex(Coluna, Linha);
							if (!State.CellLayout.IsValidIndex(Indice)
								|| State.CellLayout[Indice] == static_cast<uint8>(ECellProperty::Blocked))
							{
								continue;
							}

							// A ÁGUA APAGA O FOGO TAMBÉM AQUI. Um golpe de
							// campo inteiro que acendesse brasa dentro d'água
							// desfaria a regra de cima só por ser grande.
							if (bEhFogo && FluidRegistry::IsWater(
								State.FluidAt(static_cast<uint8>(Coluna), static_cast<uint8>(Linha))))
							{
								continue;
							}

							State.SetTemporaryTerrain(
								static_cast<uint8>(Coluna), static_cast<uint8>(Linha),
								Deposito.Terrain, Deposito.Slots);
							++Mudadas;
						}
					}

					FBattleEvent Campo;
					Campo.Type = EBattleEventType::CampoInteiroMudou;
					Campo.SlotIndex = SlotIndex;
					Campo.Phase = 4;
					Campo.ActorId = AttackerPtr->PetId;
					Campo.TargetId = TargetPtr->PetId;
					Campo.Value = static_cast<int32>(Deposito.Terrain);
					Campo.Detail = Mudadas;
					OutTrace.Add(Campo);
					return;
				}

				const bool bSeca =
					Deposito.Terrain == static_cast<uint8>(ECellProperty::Dries);

				State.SetTemporaryTerrain(TargetPtr->Column, TargetPtr->Row,
					bSeca ? static_cast<uint8>(ECellProperty::None) : Deposito.Terrain,
					bSeca ? 0 : Deposito.Slots);

				FBattleEvent Terreno;
				Terreno.Type = EBattleEventType::TerrenoMudou;
				Terreno.SlotIndex = SlotIndex;
				Terreno.Phase = 4;
				Terreno.ActorId = AttackerPtr->PetId;
				Terreno.TargetId = TargetPtr->PetId;
				Terreno.ToCell = PackCell(TargetPtr->Column, TargetPtr->Row);
				Terreno.Value = static_cast<int32>(Deposito.Terrain);
				Terreno.Detail = Deposito.Slots;
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
			FTerrainDeposit{}, /*DrainPercent=*/0, /*bConducts=*/false,
			SlotIndex, OutTrace);
		ApplyHitAgainst(State, *Outro, *Um, /*bIsMagic=*/false, /*MovePower=*/0,
			FTerrainDeposit{}, /*DrainPercent=*/0, /*bConducts=*/false,
			SlotIndex, OutTrace);
	}
}

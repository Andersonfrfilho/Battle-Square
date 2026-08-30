// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattleArenaConstants.h"
#include "Battle/BattleEvent.h"
#include "Battle/BattlePhases.h"
#include "Battle/BattleState.h"
#include "Misc/AutomationTest.h"

// A casa BLOQUEADA tem CORPO: é tronco caído ou pedra em pé, e não parede
// lisa. Quem anda contra ela DERRUBA (força), SOBE (agilidade) ou ESBARRA,
// nesta ordem — e quem subiu passa a lutar de cima.
//
// Estes testes existem porque a regra antiga ("bloqueada rejeita movimento")
// era um caso só, e agora são três com uma disputa entre eles.

namespace CasaComObstaculo
{
	FPetState PetNoObstaculo(uint8 PetId, uint8 Side, uint8 Column, uint8 Row, int32 Attack, int32 Speed)
	{
		FPetState Estado;
		Estado.PetId = PetId;
		Estado.Side = Side;
		Estado.Column = Column;
		Estado.Row = Row;
		Estado.Health = 100;
		Estado.MaxHealth = 100;
		Estado.Attack = Attack;
		Estado.Speed = Speed;
		return Estado;
	}

	FBattleAction PassoContraObstaculo(EBattleDirection Direction)
	{
		FBattleAction Acao;
		Acao.Type = EActionType::Mover;
		Acao.Direction = Direction;
		return Acao;
	}

	FBattleAction GolpeNoObstaculo(EBattleDirection Direction)
	{
		FBattleAction Acao;
		Acao.Type = EActionType::Atacar;
		Acao.Direction = Direction;
		return Acao;
	}

	FBattleAction EsperaNoObstaculo()
	{
		FBattleAction Acao;
		Acao.Type = EActionType::Aguardar;
		return Acao;
	}

	bool TracoDoObstaculoTem(const TArray<FBattleEvent>& Trace, EBattleEventType Tipo)
	{
		for (const FBattleEvent& Evento : Trace)
		{
			if (Evento.Type == Tipo)
			{
				return true;
			}
		}
		return false;
	}

	/** Fraco e lento: o pet que só esbarra, para servir de contraste. */
	constexpr int32 AtaqueQueNaoDerruba = BattleArenaConstants::ObstacleBreakAttack - 1;
	constexpr int32 VelocidadeQueNaoSobe = BattleArenaConstants::ObstacleClimbSpeed - 1;
}

// FORÇA derruba — e gasta o slot nisso.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FObstacleStrongPetFellsItTest,
	"BattleSim.Obstacle.StrongPetFellsIt",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FObstacleStrongPetFellsItTest::RunTest(const FString& Parameters)
{
	using namespace CasaComObstaculo;

	FBattleState Estado;
	Estado.Pets.Add(PetNoObstaculo(1, 0, 1, 1, BattleArenaConstants::ObstacleBreakAttack, VelocidadeQueNaoSobe));
	Estado.Pets.Add(PetNoObstaculo(2, 1, 0, 0, 0, 0));
	const int32 IndiceDaCasa = Estado.CellIndex(2, 1);
	Estado.CellLayout[IndiceDaCasa] = static_cast<uint8>(ECellProperty::Blocked);

	TArray<FBattleEvent> Traco;
	BattlePhases::ApplyMovement(Estado, PassoContraObstaculo(EBattleDirection::Direita), EsperaNoObstaculo(), 0, Traco);

	TestEqual(TEXT("a casa abriu"), static_cast<int32>(Estado.CellLayout[IndiceDaCasa]),
		static_cast<int32>(ECellProperty::None));
	TestTrue(TEXT("ObstaculoDerrubado no traco"),
		TracoDoObstaculoTem(Traco, EBattleEventType::ObstaculoDerrubado));

	// Derrubar CUSTA o slot: sem esse preço o obstáculo seria pedágio de zero
	// para o pet forte, e a escolha entre derrubar e escalar não existiria.
	TestEqual(TEXT("quem derrubou nao atravessa no mesmo movimento"),
		Estado.Pets[0].Column, static_cast<uint8>(1));
	TestFalse(TEXT("e nao emite Moveu"), TracoDoObstaculoTem(Traco, EBattleEventType::Moveu));

	return true;
}

// AGILIDADE sobe — e o feed diz que foi escalada, não caminhada.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FObstacleAgilePetClimbsItTest,
	"BattleSim.Obstacle.AgilePetClimbsIt",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FObstacleAgilePetClimbsItTest::RunTest(const FString& Parameters)
{
	using namespace CasaComObstaculo;

	FBattleState Estado;
	Estado.Pets.Add(PetNoObstaculo(1, 0, 1, 1, AtaqueQueNaoDerruba, BattleArenaConstants::ObstacleClimbSpeed));
	Estado.Pets.Add(PetNoObstaculo(2, 1, 0, 0, 0, 0));
	const int32 IndiceDaCasa = Estado.CellIndex(2, 1);
	Estado.CellLayout[IndiceDaCasa] = static_cast<uint8>(ECellProperty::Blocked);

	TArray<FBattleEvent> Traco;
	BattlePhases::ApplyMovement(Estado, PassoContraObstaculo(EBattleDirection::Direita), EsperaNoObstaculo(), 0, Traco);

	TestEqual(TEXT("o pet esta na casa do obstaculo"), Estado.Pets[0].Column, static_cast<uint8>(2));
	TestEqual(TEXT("na mesma linha"), Estado.Pets[0].Row, static_cast<uint8>(1));

	// O obstáculo CONTINUA lá: é ele que sustenta o pet, e é a casa
	// continuar bloqueada que faz o combate saber que ele está no alto.
	TestEqual(TEXT("o obstaculo continua de pe"), static_cast<int32>(Estado.CellLayout[IndiceDaCasa]),
		static_cast<int32>(ECellProperty::Blocked));

	TestTrue(TEXT("SubiuNoObstaculo no traco"),
		TracoDoObstaculoTem(Traco, EBattleEventType::SubiuNoObstaculo));
	TestFalse(TEXT("e nao Moveu — subir num tronco nao e' andar"),
		TracoDoObstaculoTem(Traco, EBattleEventType::Moveu));

	return true;
}

// Nem força nem agilidade: esbarra, como sempre foi.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FObstacleWeakAndSlowPetBumpsTest,
	"BattleSim.Obstacle.WeakAndSlowPetBumps",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FObstacleWeakAndSlowPetBumpsTest::RunTest(const FString& Parameters)
{
	using namespace CasaComObstaculo;

	FBattleState Estado;
	Estado.Pets.Add(PetNoObstaculo(1, 0, 1, 1, AtaqueQueNaoDerruba, VelocidadeQueNaoSobe));
	Estado.Pets.Add(PetNoObstaculo(2, 1, 0, 0, 0, 0));
	const int32 IndiceDaCasa = Estado.CellIndex(2, 1);
	Estado.CellLayout[IndiceDaCasa] = static_cast<uint8>(ECellProperty::Blocked);

	TArray<FBattleEvent> Traco;
	BattlePhases::ApplyMovement(Estado, PassoContraObstaculo(EBattleDirection::Direita), EsperaNoObstaculo(), 0, Traco);

	TestEqual(TEXT("nao saiu do lugar"), Estado.Pets[0].Column, static_cast<uint8>(1));
	TestEqual(TEXT("o obstaculo continua de pe"), static_cast<int32>(Estado.CellLayout[IndiceDaCasa]),
		static_cast<int32>(ECellProperty::Blocked));
	TestTrue(TEXT("MovimentoBloqueado, o vocabulario de sempre"),
		TracoDoObstaculoTem(Traco, EBattleEventType::MovimentoBloqueado));

	return true;
}

// Obstáculo COM ALGUÉM EM CIMA não cai nem recebe segundo morador.
//
// Derrubá-lo tiraria o chão de quem já subiu, e a queda seria consequência de
// uma jogada de OUTRO pet — regra que ninguém consegue prever ao escolher.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FObstacleOccupiedIsNeitherFelledNorClimbedTest,
	"BattleSim.Obstacle.OccupiedIsNeitherFelledNorClimbed",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FObstacleOccupiedIsNeitherFelledNorClimbedTest::RunTest(const FString& Parameters)
{
	using namespace CasaComObstaculo;

	FBattleState Estado;
	// O de PetId 1 JÁ está em cima; o forte vem de baixo tentar derrubar.
	Estado.Pets.Add(PetNoObstaculo(1, 0, 2, 1, 0, 0));
	Estado.Pets.Add(PetNoObstaculo(2, 1, 2, 2, BattleArenaConstants::ObstacleBreakAttack, 0));
	const int32 IndiceDaCasa = Estado.CellIndex(2, 1);
	Estado.CellLayout[IndiceDaCasa] = static_cast<uint8>(ECellProperty::Blocked);

	TArray<FBattleEvent> Traco;
	BattlePhases::ApplyMovement(Estado, EsperaNoObstaculo(), PassoContraObstaculo(EBattleDirection::Cima), 0, Traco);

	TestEqual(TEXT("o obstaculo continua de pe"), static_cast<int32>(Estado.CellLayout[IndiceDaCasa]),
		static_cast<int32>(ECellProperty::Blocked));
	TestEqual(TEXT("quem estava em cima continua em cima"), Estado.Pets[0].Row, static_cast<uint8>(1));
	TestEqual(TEXT("e o forte esbarrou"), Estado.Pets[1].Row, static_cast<uint8>(2));
	TestFalse(TEXT("nada foi derrubado"), TracoDoObstaculoTem(Traco, EBattleEventType::ObstaculoDerrubado));

	return true;
}

// Derrubar ABRE a passagem para o MESMO slot: o forte trabalha para o outro.
//
// É o que torna derrubar uma escolha e não um ganho puro — limpa o tabuleiro
// e entrega o terreno alto que o adversário talvez fosse usar.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FObstacleFellingOpensThePathInTheSameSlotTest,
	"BattleSim.Obstacle.FellingOpensThePathInTheSameSlot",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FObstacleFellingOpensThePathInTheSameSlotTest::RunTest(const FString& Parameters)
{
	using namespace CasaComObstaculo;

	FBattleState Estado;
	// PetId 1 derruba (chega primeiro na ordem); PetId 2, lento e fraco,
	// entraria esbarrando — e entra andando porque a casa abriu.
	Estado.Pets.Add(PetNoObstaculo(1, 0, 1, 1, BattleArenaConstants::ObstacleBreakAttack, 0));
	Estado.Pets.Add(PetNoObstaculo(2, 1, 2, 2, AtaqueQueNaoDerruba, VelocidadeQueNaoSobe));
	const int32 IndiceDaCasa = Estado.CellIndex(2, 1);
	Estado.CellLayout[IndiceDaCasa] = static_cast<uint8>(ECellProperty::Blocked);

	TArray<FBattleEvent> Traco;
	BattlePhases::ApplyMovement(Estado, PassoContraObstaculo(EBattleDirection::Direita), PassoContraObstaculo(EBattleDirection::Cima), 0, Traco);

	TestEqual(TEXT("a casa abriu"), static_cast<int32>(Estado.CellLayout[IndiceDaCasa]),
		static_cast<int32>(ECellProperty::None));
	TestEqual(TEXT("o fraco entrou pela passagem aberta"), Estado.Pets[1].Row, static_cast<uint8>(1));
	TestTrue(TEXT("e o feed conta que ele ANDOU"), TracoDoObstaculoTem(Traco, EBattleEventType::Moveu));
	TestFalse(TEXT("ninguem escalou um tronco que ja caiu"),
		TracoDoObstaculoTem(Traco, EBattleEventType::SubiuNoObstaculo));

	return true;
}

// A ordem é por PetId, e não a do container. Ordem de container não é
// determinismo — o mesmo motivo de ResolveTarget desempatar por PetId.
//
// O caso que dói: o ÁGIL tem o id menor, decide escalar, e o FORTE derruba o
// tronco logo em seguida no mesmo passo. Sem varrer as outras intenções
// depois de derrubar, o feed contaria que alguém escalou um tronco que já
// estava no chão.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FObstacleResolutionOrderIsByPetIdTest,
	"BattleSim.Obstacle.ResolutionOrderIsByPetId",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FObstacleResolutionOrderIsByPetIdTest::RunTest(const FString& Parameters)
{
	using namespace CasaComObstaculo;

	// O mesmo par, montado nas DUAS ordens possíveis de container.
	const FPetState Agil = PetNoObstaculo(1, 0, 1, 1, AtaqueQueNaoDerruba, BattleArenaConstants::ObstacleClimbSpeed);
	const FPetState Forte = PetNoObstaculo(2, 1, 2, 2, BattleArenaConstants::ObstacleBreakAttack, 0);
	const FBattleAction PassoDoAgil = PassoContraObstaculo(EBattleDirection::Direita);
	const FBattleAction PassoDoForte = PassoContraObstaculo(EBattleDirection::Cima);

	for (int32 Montagem = 0; Montagem < 2; ++Montagem)
	{
		FBattleState Estado;
		if (Montagem == 0)
		{
			Estado.Pets.Add(Agil);
			Estado.Pets.Add(Forte);
		}
		else
		{
			Estado.Pets.Add(Forte);
			Estado.Pets.Add(Agil);
		}
		const int32 IndiceDaCasa = Estado.CellIndex(2, 1);
		Estado.CellLayout[IndiceDaCasa] = static_cast<uint8>(ECellProperty::Blocked);

		// O ágil é do lado 0 e o forte do lado 1, montagem nenhuma muda isso.
		TArray<FBattleEvent> Traco;
		BattlePhases::ApplyMovement(Estado, PassoDoAgil, PassoDoForte, 0, Traco);

		TestEqual(TEXT("o tronco caiu, em qualquer montagem"),
			static_cast<int32>(Estado.CellLayout[IndiceDaCasa]),
			static_cast<int32>(ECellProperty::None));
		TestFalse(TEXT("e ninguem escalou um tronco que ja estava no chao"),
			TracoDoObstaculoTem(Traco, EBattleEventType::SubiuNoObstaculo));
		TestTrue(TEXT("o agil entrou ANDANDO na casa aberta"),
			TracoDoObstaculoTem(Traco, EBattleEventType::Moveu));
	}

	return true;
}

// DE CIMA o golpe físico ALCANÇA quem voou — é a habilidade que subir
// destrava, e ela cai por si quando o pet desce ou o tronco vai ao chão.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FObstacleElevatedAttackReachesFlyerTest,
	"BattleSim.Obstacle.ElevatedAttackReachesFlyer",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FObstacleElevatedAttackReachesFlyerTest::RunTest(const FString& Parameters)
{
	using namespace CasaComObstaculo;

	FBattleState Estado;
	Estado.Pets.Add(PetNoObstaculo(1, 0, 1, 1, 30, 0));
	Estado.Pets.Add(PetNoObstaculo(2, 1, 2, 1, 0, 0));
	Estado.Pets[1].PostureFlags = static_cast<uint8>(EBattlePostureFlags::Flying);

	// Do CHÃO, o golpe físico erra quem voou — a regra que já existia.
	{
		TArray<FBattleEvent> Traco;
		BattlePhases::ApplyCombat(Estado, GolpeNoObstaculo(EBattleDirection::Direita), EsperaNoObstaculo(), 0, Traco);
		TestEqual(TEXT("do chao nao encosta em quem voa"), Estado.Pets[1].PendingDamage, 0);
		TestTrue(TEXT("e o feed diz que errou"), TracoDoObstaculoTem(Traco, EBattleEventType::AtaqueErrou));
	}

	// Do ALTO do obstáculo, alcança.
	Estado.CellLayout[Estado.CellIndex(1, 1)] = static_cast<uint8>(ECellProperty::Blocked);
	{
		TArray<FBattleEvent> Traco;
		BattlePhases::ApplyCombat(Estado, GolpeNoObstaculo(EBattleDirection::Direita), EsperaNoObstaculo(), 0, Traco);
		TestTrue(TEXT("de cima do obstaculo o golpe encosta"), Estado.Pets[1].PendingDamage > 0);
		TestTrue(TEXT("e o feed diz que acertou"), TracoDoObstaculoTem(Traco, EBattleEventType::AtaqueAcertou));
	}

	return true;
}

// De CIMA para BAIXO bate mais forte — e só de cima para baixo.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FObstacleElevatedAttackHitsHarderTest,
	"BattleSim.Obstacle.ElevatedAttackHitsHarder",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FObstacleElevatedAttackHitsHarderTest::RunTest(const FString& Parameters)
{
	using namespace CasaComObstaculo;

	auto DanoDeUmGolpe = [](bool bAtacanteNoAlto, bool bAlvoNoAlto) -> int32
	{
		FBattleState Estado;
		Estado.Pets.Add(PetNoObstaculo(1, 0, 1, 1, 40, 0));
		Estado.Pets.Add(PetNoObstaculo(2, 1, 2, 1, 0, 0));
		Estado.Pets[1].Defense = 10;
		if (bAtacanteNoAlto)
		{
			Estado.CellLayout[Estado.CellIndex(1, 1)] = static_cast<uint8>(ECellProperty::Blocked);
		}
		if (bAlvoNoAlto)
		{
			Estado.CellLayout[Estado.CellIndex(2, 1)] = static_cast<uint8>(ECellProperty::Blocked);
		}

		TArray<FBattleEvent> Traco;
		BattlePhases::ApplyCombat(Estado, GolpeNoObstaculo(EBattleDirection::Direita), EsperaNoObstaculo(), 0, Traco);
		return Estado.Pets[1].PendingDamage;
	};

	const int32 DoChao = DanoDeUmGolpe(false, false);
	const int32 DoAlto = DanoDeUmGolpe(true, false);
	const int32 EntreDoisNoAlto = DanoDeUmGolpe(true, true);

	TestTrue(TEXT("do chao ja machuca"), DoChao > 0);
	TestEqual(TEXT("de cima para baixo o golpe vale ElevatedAttackPercent"),
		DoAlto, (40 * BattleArenaConstants::ElevatedAttackPercent / 100) - 10);

	// Dois pets em obstáculos diferentes brigam de igual para igual: um bônus
	// que valesse sempre que o atacante estivesse no alto premiaria subir
	// mesmo contra quem já está lá.
	TestEqual(TEXT("dois no alto brigam de igual para igual"), EntreDoisNoAlto, DoChao);

	return true;
}

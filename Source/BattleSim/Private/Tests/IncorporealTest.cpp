// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattlePhases.h"
#include "Battle/BattleState.h"
#include "Battle/BattleEvent.h"
#include "Battle/BattleArenaConstants.h"
#include "Misc/AutomationTest.h"

namespace IncorporeoTeste
{
	FBattleState CampoCom(uint8 TerrenoDoPet, bool bFantasma)
	{
		FBattleState Estado;
		Estado.Random.State = 99;

		FPetState Ele;
		Ele.PetId = 1; Ele.Side = 0; Ele.Column = 1; Ele.Row = 1;
		Ele.Health = 90; Ele.MaxHealth = 90; Ele.Speed = 40;
		if (bFantasma)
		{
			Ele.Traits |= static_cast<uint8>(EPetTrait::Incorporeo);
		}

		FPetState Outro;
		Outro.PetId = 2; Outro.Side = 1; Outro.Column = 2; Outro.Row = 2;
		Outro.Health = 90; Outro.MaxHealth = 90;

		Estado.Pets.Add(Ele);
		Estado.Pets.Add(Outro);
		Estado.CellLayout[Estado.CellIndex(1, 1)] = TerrenoDoPet;
		return Estado;
	}

	FBattleAction Andar(EBattleDirection Direcao)
	{
		FBattleAction Acao;
		Acao.Type = EActionType::Mover;
		Acao.Direction = Direcao;
		return Acao;
	}

	int32 Conta(const TArray<FBattleEvent>& Traco, EBattleEventType Tipo)
	{
		int32 Total = 0;
		for (const FBattleEvent& Evento : Traco)
		{
			if (Evento.Type == Tipo) { ++Total; }
		}
		return Total;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOChaoNaoAlcancaOFantasmaTest,
	"BattleSim.Traits.Incorporeo.OChaoNaoAlcancaOFantasma",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FOChaoNaoAlcancaOFantasmaTest::RunTest(const FString&)
{
	// Não é imunidade CONCEDIDA — é consequência de não encostar. E por isso
	// vale para os três terrenos de uma vez, sem uma exceção para cada.
	const uint8 Gelo = static_cast<uint8>(ECellProperty::Ice);
	const uint8 Lama = static_cast<uint8>(ECellProperty::Mud);
	const uint8 Brasa = static_cast<uint8>(ECellProperty::Damage);

	FBattleAction Nada;

	// GELO: quem tem corpo escorrega SEMPRE ali; o fantasma passa.
	{
		FBattleState ComCorpo = IncorporeoTeste::CampoCom(Gelo, /*bFantasma=*/false);
		TArray<FBattleEvent> Traco;
		BattlePhases::ApplyMovement(ComCorpo,
			IncorporeoTeste::Andar(EBattleDirection::Cima), Nada, 0, Traco);
		TestEqual(TEXT("Com corpo, o gelo prende"),
			IncorporeoTeste::Conta(Traco, EBattleEventType::Escorregou), 1);

		FBattleState Fantasma = IncorporeoTeste::CampoCom(Gelo, /*bFantasma=*/true);
		TArray<FBattleEvent> Outro;
		BattlePhases::ApplyMovement(Fantasma,
			IncorporeoTeste::Andar(EBattleDirection::Cima), Nada, 0, Outro);
		TestEqual(TEXT("O fantasma não escorrega"),
			IncorporeoTeste::Conta(Outro, EBattleEventType::Escorregou), 0);
		TestEqual(TEXT("E anda"), Fantasma.Pets[0].Row, static_cast<uint8>(0));
	}

	// LAMA: nem escorrega nem atola.
	{
		FBattleState Fantasma = IncorporeoTeste::CampoCom(Lama, /*bFantasma=*/true);
		TArray<FBattleEvent> Traco;
		BattlePhases::ApplyMovement(Fantasma,
			IncorporeoTeste::Andar(EBattleDirection::Cima), Nada, 0, Traco);
		TestEqual(TEXT("A lama não o atola"),
			IncorporeoTeste::Conta(Traco, EBattleEventType::AtravessouDevagar), 0);
		TestEqual(TEXT("Nem o faz escorregar"),
			IncorporeoTeste::Conta(Traco, EBattleEventType::Escorregou), 0);
	}

	// BRASA: a casa que fere não fere quem não pisa.
	{
		FBattleState Fantasma = IncorporeoTeste::CampoCom(Brasa, /*bFantasma=*/true);
		TArray<FBattleEvent> Traco;
		BattlePhases::ApplyMovement(Fantasma, Nada, Nada, 0, Traco);
		TestEqual(TEXT("A brasa não o queima"), Fantasma.Pets[0].PendingDamage, 0);

		FBattleState ComCorpo = IncorporeoTeste::CampoCom(Brasa, /*bFantasma=*/false);
		TArray<FBattleEvent> Outro;
		BattlePhases::ApplyMovement(ComCorpo, Nada, Nada, 0, Outro);
		TestEqual(TEXT("Mas queima quem pisa"), ComCorpo.Pets[0].PendingDamage,
			BattleArenaConstants::CellDamageAmount);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAtravessarSoQuemNaoTemCorpoTest,
	"BattleSim.Traits.Incorporeo.AtravessarSoQuemNaoTemCorpo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAtravessarSoQuemNaoTemCorpoTest::RunTest(const FString&)
{
	// A recusa é pela mesma porta que a fundura abriu: quem tem corpo não
	// passa por dentro do tronco, e isso falha ALTO, com evento — ação que
	// some calada parece defeito.
	FBattleState ComCorpo = IncorporeoTeste::CampoCom(
		static_cast<uint8>(ECellProperty::None), /*bFantasma=*/false);

	FTurnCommit Passar;
	Passar.Actions[0].Type = EActionType::Atravessar;

	FBattleAction Nada;
	TArray<FBattleEvent> Traco;
	BattlePhases::ApplyPostures(ComCorpo, Passar.Actions[0], Nada, 0, Traco);

	TestEqual(TEXT("Quem tem corpo não atravessa"),
		IncorporeoTeste::Conta(Traco, EBattleEventType::PosturaFalhou), 1);
	TestEqual(TEXT("E não fica marcado como atravessando"),
		ComCorpo.Pets[0].PostureFlags & static_cast<uint16>(EBattlePostureFlags::Phasing),
		static_cast<uint16>(0));

	FBattleState Fantasma = IncorporeoTeste::CampoCom(
		static_cast<uint8>(ECellProperty::None), /*bFantasma=*/true);
	TArray<FBattleEvent> Outro;
	BattlePhases::ApplyPostures(Fantasma, Passar.Actions[0], Nada, 0, Outro);

	TestEqual(TEXT("O fantasma atravessa"),
		IncorporeoTeste::Conta(Outro, EBattleEventType::PosturaFalhou), 0);
	TestTrue(TEXT("E fica marcado"),
		(Fantasma.Pets[0].PostureFlags
			& static_cast<uint16>(EBattlePostureFlags::Phasing)) != 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAtravessandoOTroncoNaoBarraTest,
	"BattleSim.Traits.Incorporeo.AtravessandoOTroncoNaoBarra",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAtravessandoOTroncoNaoBarraTest::RunTest(const FString&)
{
	FBattleState Estado = IncorporeoTeste::CampoCom(
		static_cast<uint8>(ECellProperty::None), /*bFantasma=*/true);

	// Um tronco na casa acima dele.
	const int32 Bloqueada = Estado.CellIndex(1, 0);
	Estado.CellLayout[Bloqueada] = static_cast<uint8>(ECellProperty::Blocked);

	FBattleAction Nada;
	TArray<FBattleEvent> Traco;

	// Sem atravessar, o tronco barra — é o comportamento de sempre, e ele
	// precisa continuar valendo para o fantasma que NÃO gastou a ação.
	BattlePhases::ApplyMovement(Estado,
		IncorporeoTeste::Andar(EBattleDirection::Cima), Nada, 0, Traco);
	TestEqual(TEXT("Sem atravessar, o fantasma esbarra igual"),
		Estado.Pets[0].Row, static_cast<uint8>(1));

	// Com a postura assumida, passa POR DENTRO — e o tronco continua em pé.
	Estado.Pets[0].PostureFlags |= static_cast<uint16>(EBattlePostureFlags::Phasing);
	TArray<FBattleEvent> Depois;
	BattlePhases::ApplyMovement(Estado,
		IncorporeoTeste::Andar(EBattleDirection::Cima), Nada, 1, Depois);

	TestEqual(TEXT("Atravessando, ele passa"), Estado.Pets[0].Row, static_cast<uint8>(0));
	TestEqual(TEXT("E o tronco CONTINUA em pé"), Estado.CellLayout[Bloqueada],
		static_cast<uint8>(ECellProperty::Blocked));

	// Não é derrubar: quem derruba abre a passagem para todo mundo, e isso é
	// outra jogada, com outro custo e outro evento.
	TestEqual(TEXT("E ninguém derrubou nada"),
		IncorporeoTeste::Conta(Depois, EBattleEventType::ObstaculoDerrubado), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTracoEDoBichoNaoDoTurnoTest,
	"BattleSim.Traits.Incorporeo.TracoEDoBichoNaoDoTurno",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTracoEDoBichoNaoDoTurnoTest::RunTest(const FString&)
{
	// Postura some no fim do slot; TRAÇO não. Guardar "incorpóreo" em
	// PostureFlags faria o fantasma virar corpo toda vez que o slot acabasse
	// — e ele seria fantasma só no primeiro terço de cada turno.
	FBattleState Estado = IncorporeoTeste::CampoCom(
		static_cast<uint8>(ECellProperty::Ice), /*bFantasma=*/true);

	TArray<FBattleEvent> Traco;
	for (uint8 Slot = 0; Slot < 3; ++Slot)
	{
		BattlePhases::ApplyResolution(Estado, Slot, Traco);
	}

	TestTrue(TEXT("Depois de três slots ele ainda é incorpóreo"),
		Estado.Pets[0].HasTrait(EPetTrait::Incorporeo));
	TestTrue(TEXT("E continua fora do chão"), Estado.Pets[0].IsOffTheGround());

	// E o traço entra no hash: ele decide resultado.
	FBattleState ComCorpo = IncorporeoTeste::CampoCom(
		static_cast<uint8>(ECellProperty::Ice), /*bFantasma=*/false);
	FBattleState Fantasma = IncorporeoTeste::CampoCom(
		static_cast<uint8>(ECellProperty::Ice), /*bFantasma=*/true);
	TestNotEqual(TEXT("Fantasma e corpo têm hashes diferentes"),
		ComCorpo.ComputeHash(), Fantasma.ComputeHash());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOFisicoNaoAlcancaOIncorporeoTest,
	"BattleSim.Traits.Incorporeo.OFisicoNaoAlcancaOIncorporeo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FOFisicoNaoAlcancaOIncorporeoTest::RunTest(const FString&)
{
	// O punho atravessa: não há o que acertar. E o pet fica ao lado, para o
	// alcance do golpe não ser a razão do erro.
	FBattleState Estado = IncorporeoTeste::CampoCom(
		static_cast<uint8>(ECellProperty::None), /*bFantasma=*/false);
	Estado.Pets[0].Column = 0; Estado.Pets[0].Row = 1; Estado.Pets[0].Attack = 40;
	Estado.Pets[1].Column = 1; Estado.Pets[1].Row = 1; Estado.Pets[1].Defense = 10;
	Estado.Pets[1].Traits |= static_cast<uint8>(EPetTrait::Incorporeo);

	FBattleAction Soco;
	Soco.Type = EActionType::Atacar;
	Soco.Direction = EBattleDirection::Direita;

	FBattleAction Nada;
	TArray<FBattleEvent> Traco;
	BattlePhases::ApplyCombat(Estado, Soco, Nada, 0, Traco);

	TestEqual(TEXT("O golpe físico erra o fantasma"), Estado.Pets[1].PendingDamage, 0);
	TestEqual(TEXT("E o feed conta que errou"),
		IncorporeoTeste::Conta(Traco, EBattleEventType::AtaqueErrou), 1);

	// A MAGIA alcança — e é o que impede o fantasma de ser invencível, porque
	// `Magia` é ação universal: todo pet do jogo consegue usá-la.
	FBattleAction Magia;
	Magia.Type = EActionType::Magia;
	Magia.Direction = EBattleDirection::Direita;

	TArray<FBattleEvent> ComMagia;
	BattlePhases::ApplyCombat(Estado, Magia, Nada, 1, ComMagia);
	TestTrue(TEXT("A magia acerta"), Estado.Pets[1].PendingDamage > 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FALuzDesfazOEsconderijoTest,
	"BattleSim.Traits.Incorporeo.ALuzDesfazOEsconderijo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FALuzDesfazOEsconderijoTest::RunTest(const FString&)
{
	FBattleState Estado = IncorporeoTeste::CampoCom(
		static_cast<uint8>(ECellProperty::None), /*bFantasma=*/false);
	Estado.Pets[0].Column = 0; Estado.Pets[0].Row = 1; Estado.Pets[0].Attack = 40;
	Estado.Pets[1].Column = 1; Estado.Pets[1].Row = 1; Estado.Pets[1].Defense = 10;
	Estado.Pets[1].Traits |= static_cast<uint8>(EPetTrait::Incorporeo);

	FBattleAction Iluminar;
	Iluminar.Type = EActionType::Iluminar;
	FBattleAction Nada;

	TArray<FBattleEvent> Traco;
	BattlePhases::ApplyPostures(Estado, Iluminar, Nada, 0, Traco);

	// A bandeira vai para o ALVO, não para quem iluminou: quem deixa de estar
	// escondido é ele, e é isso que faz a luz de um pet servir ao golpe que
	// vem depois.
	TestTrue(TEXT("O fantasma ficou revelado"),
		(Estado.Pets[1].PostureFlags
			& static_cast<uint16>(EBattlePostureFlags::Revealed)) != 0);
	TestEqual(TEXT("E foi narrado"),
		IncorporeoTeste::Conta(Traco, EBattleEventType::Revelado), 1);

	FBattleAction Soco;
	Soco.Type = EActionType::Atacar;
	Soco.Direction = EBattleDirection::Direita;

	TArray<FBattleEvent> Depois;
	BattlePhases::ApplyCombat(Estado, Soco, Nada, 0, Depois);
	TestTrue(TEXT("Revelado, o físico ACERTA o fantasma"),
		Estado.Pets[1].PendingDamage > 0);

	// E dura um SLOT só: a luz é jogada, não estado permanente. Se ficasse, o
	// fantasma perderia o que ele é depois de um único erro.
	TArray<FBattleEvent> Fim;
	BattlePhases::ApplyResolution(Estado, 0, Fim);
	TestEqual(TEXT("A revelação some no fim do slot"),
		Estado.Pets[1].PostureFlags & static_cast<uint16>(EBattlePostureFlags::Revealed),
		static_cast<uint16>(0));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FALuzTambemDesfazACamuflagemTest,
	"BattleSim.Traits.Incorporeo.ALuzTambemDesfazACamuflagem",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FALuzTambemDesfazACamuflagemTest::RunTest(const FString&)
{
	// A luz não é só anti-fantasma: ela desfaz QUALQUER esconderijo. Fosse só
	// contra o incorpóreo, o elemento Luz seria inútil em toda partida onde o
	// outro lado não trouxesse fantasma — e ninguém o escolheria.
	FBattleState Estado = IncorporeoTeste::CampoCom(
		static_cast<uint8>(ECellProperty::None), /*bFantasma=*/false);
	Estado.Pets[0].Column = 0; Estado.Pets[0].Row = 1; Estado.Pets[0].Attack = 40;
	Estado.Pets[1].Column = 1; Estado.Pets[1].Row = 1; Estado.Pets[1].Defense = 10;
	Estado.Pets[1].PostureFlags |= static_cast<uint16>(EBattlePostureFlags::Camouflaged);

	FBattleAction Soco;
	Soco.Type = EActionType::Atacar;
	Soco.Direction = EBattleDirection::Direita;
	FBattleAction Nada;

	TArray<FBattleEvent> SemLuz;
	BattlePhases::ApplyCombat(Estado, Soco, Nada, 0, SemLuz);
	TestEqual(TEXT("Camuflado, não é alcançado"), Estado.Pets[1].PendingDamage, 0);

	Estado.Pets[1].PostureFlags |= static_cast<uint16>(EBattlePostureFlags::Revealed);
	TArray<FBattleEvent> ComLuz;
	BattlePhases::ApplyCombat(Estado, Soco, Nada, 1, ComLuz);
	TestTrue(TEXT("Iluminado, o camuflado é alcançado"),
		Estado.Pets[1].PendingDamage > 0);

	return true;
}

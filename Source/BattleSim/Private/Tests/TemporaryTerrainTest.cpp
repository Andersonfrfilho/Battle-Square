// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattlePhases.h"
#include "Battle/BattleState.h"
#include "Battle/BattleEvent.h"
#include "Misc/AutomationTest.h"

namespace TerrenoQuePassaTeste
{
	FBattleState CampoVazio()
	{
		FBattleState Estado;
		Estado.Random.State = 4242;
		return Estado;
	}

	int32 ContaEventos(const TArray<FBattleEvent>& Traco, EBattleEventType Tipo)
	{
		int32 Total = 0;
		for (const FBattleEvent& Evento : Traco)
		{
			if (Evento.Type == Tipo)
			{
				++Total;
			}
		}
		return Total;
	}

	/** Passa um slot inteiro pela fase que faz o tempo correr. */
	void PassaUmSlot(FBattleState& Estado, uint8 SlotIndex, TArray<FBattleEvent>& Traco)
	{
		BattlePhases::ApplyResolution(Estado, SlotIndex, Traco);
	}
}

// SEM `using namespace` no escopo do arquivo: em unity build os dois
// arquivos viram uma TU só, os dois `using` ficam visíveis, e a chamada
// volta a ser ambígua — o namespace nomeado deixa de isolar justamente
// onde ele precisava isolar. Qualificar no ponto de chamada é o que
// realmente separa (L-042).


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeloDerreteNoPrazoTest,
	"BattleSim.Terrain.Temporary.GeloDerreteNoPrazo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGeloDerreteNoPrazoTest::RunTest(const FString&)
{
	FBattleState Estado = TerrenoQuePassaTeste::CampoVazio();
	Estado.SetTemporaryTerrain(1, 1, static_cast<uint8>(ECellProperty::Ice), 2);

	const int32 Casa = Estado.CellIndex(1, 1);
	TestEqual(TEXT("A casa congelou"), Estado.CellLayout[Casa],
		static_cast<uint8>(ECellProperty::Ice));

	TArray<FBattleEvent> Traco;
	TerrenoQuePassaTeste::PassaUmSlot(Estado, 0, Traco);

	// Prazo 2 tem que sobreviver ao PRIMEIRO slot. Fosse "derrete no fim do
	// slot em que foi posto", congelar por 1 seria congelar por nada, e o
	// menor congelamento do jogo não existiria.
	TestEqual(TEXT("Depois de um slot ainda é gelo"), Estado.CellLayout[Casa],
		static_cast<uint8>(ECellProperty::Ice));
	TestEqual(TEXT("E ninguém anunciou derretimento"),
		TerrenoQuePassaTeste::ContaEventos(Traco, EBattleEventType::TerrenoDerreteu), 0);

	TerrenoQuePassaTeste::PassaUmSlot(Estado, 1, Traco);

	// Derrete em POÇA, não em chão seco: gelo é água congelada, e derreter
	// sem molhar nada não é derreter. A casa continua a secar depois — quem
	// verifica a cadeia inteira é FCadeiaDeSecagemTest.
	TestEqual(TEXT("No segundo slot derreteu, e virou poça"),
		Estado.CellLayout[Casa], static_cast<uint8>(ECellProperty::ShallowWater));
	TestEqual(TEXT("E foi anunciado"),
		TerrenoQuePassaTeste::ContaEventos(Traco, EBattleEventType::TerrenoDerreteu), 1);

	// O GELO não volta. O que vem depois é a poça seguindo o caminho dela, e
	// não o gelo derretendo de novo — a distinção importa porque um gelo que
	// reaparecesse negaria terreno que o jogador já dava por liberado.
	TerrenoQuePassaTeste::PassaUmSlot(Estado, 2, Traco);
	TestNotEqual(TEXT("Não volta a congelar"), Estado.CellLayout[Casa],
		static_cast<uint8>(ECellProperty::Ice));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeloDevolveOQueHaviaEmbaixoTest,
	"BattleSim.Terrain.Temporary.GeloDevolveOQueHaviaEmbaixo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGeloDevolveOQueHaviaEmbaixoTest::RunTest(const FString&)
{
	// ESTE é o teste que justifica a terceira lista. Congelar uma POÇA e
	// esperar derreter não pode devolver água FUNDA: o jogador ganharia
	// fundura de graça, e o gelo viraria a maneira mais barata de alagar.
	FBattleState Estado = TerrenoQuePassaTeste::CampoVazio();
	const int32 Casa = Estado.CellIndex(1, 1);
	Estado.CellLayout[Casa] = static_cast<uint8>(ECellProperty::ShallowWater);

	Estado.SetTemporaryTerrain(1, 1, static_cast<uint8>(ECellProperty::Ice), 1);

	TArray<FBattleEvent> Traco;
	TerrenoQuePassaTeste::PassaUmSlot(Estado, 0, Traco);

	TestEqual(TEXT("A poça volta a ser POÇA, não água funda"),
		Estado.CellLayout[Casa], static_cast<uint8>(ECellProperty::ShallowWater));

	TestFalse(TEXT("E continua rasa demais para submergir"),
		Estado.TerrainAllowsSkill(EActionType::Submergir, Estado.CellLayout[Casa]));

	// O evento CARREGA o terreno de volta: quem desenha a tela não precisa
	// adivinhar no que a casa virou.
	bool bAchouComValor = false;
	for (const FBattleEvent& Evento : Traco)
	{
		if (Evento.Type == EBattleEventType::TerrenoDerreteu)
		{
			bAchouComValor =
				Evento.Value == static_cast<int32>(ECellProperty::ShallowWater);
		}
	}
	TestTrue(TEXT("O anúncio diz no que a casa voltou a ser"), bAchouComValor);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCongelarDeNovoNaoPerdeOFundoTest,
	"BattleSim.Terrain.Temporary.CongelarDeNovoNaoPerdeOFundo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCongelarDeNovoNaoPerdeOFundoTest::RunTest(const FString&)
{
	// Congelar sobre gelo renova o prazo, mas o que está EMBAIXO continua
	// sendo a água. Fosse o de cima, a segunda camada faria a casa derreter
	// para GELO — e ela nunca mais seria água.
	FBattleState Estado = TerrenoQuePassaTeste::CampoVazio();
	const int32 Casa = Estado.CellIndex(1, 1);
	Estado.CellLayout[Casa] = static_cast<uint8>(ECellProperty::Water);

	Estado.SetTemporaryTerrain(1, 1, static_cast<uint8>(ECellProperty::Ice), 1);
	Estado.SetTemporaryTerrain(1, 1, static_cast<uint8>(ECellProperty::Ice), 1);

	TArray<FBattleEvent> Traco;
	TerrenoQuePassaTeste::PassaUmSlot(Estado, 0, Traco);

	TestEqual(TEXT("Derrete para ÁGUA, não para gelo"),
		Estado.CellLayout[Casa], static_cast<uint8>(ECellProperty::Water));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAlagamentoContinuaPermanenteTest,
	"BattleSim.Terrain.Temporary.AlagamentoContinuaPermanente",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAlagamentoContinuaPermanenteTest::RunTest(const FString&)
{
	// ZERO É PARA SEMPRE. É a propriedade de migração desta fatia inteira:
	// todo golpe já assinado tem zero aqui, e se zero fosse "some na hora"
	// esta feature apagaria em silêncio o efeito de terreno de todos eles.
	FBattleState Estado = TerrenoQuePassaTeste::CampoVazio();
	const int32 Casa = Estado.CellIndex(1, 1);
	Estado.SetTemporaryTerrain(1, 1, static_cast<uint8>(ECellProperty::Water), 0);

	TArray<FBattleEvent> Traco;
	for (uint8 Slot = 0; Slot < 3; ++Slot)
	{
		TerrenoQuePassaTeste::PassaUmSlot(Estado, Slot, Traco);
	}

	TestEqual(TEXT("A água alagada não volta atrás"),
		Estado.CellLayout[Casa], static_cast<uint8>(ECellProperty::Water));
	TestEqual(TEXT("E nada derreteu"),
		TerrenoQuePassaTeste::ContaEventos(Traco, EBattleEventType::TerrenoDerreteu), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAlagarDeVezApagaOPrazoTest,
	"BattleSim.Terrain.Temporary.AlagarDeVezApagaOPrazo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAlagarDeVezApagaOPrazoTest::RunTest(const FString&)
{
	// Alagar de vez uma casa congelada não pode deixar para trás o
	// cronômetro do gelo — ele devolveria a casa ao que ela era ANTES, e o
	// alagamento permanente duraria dois slots sem ninguém ter pedido isso.
	FBattleState Estado = TerrenoQuePassaTeste::CampoVazio();
	const int32 Casa = Estado.CellIndex(1, 1);

	Estado.SetTemporaryTerrain(1, 1, static_cast<uint8>(ECellProperty::Ice), 2);
	Estado.SetTemporaryTerrain(1, 1, static_cast<uint8>(ECellProperty::Water), 0);

	TArray<FBattleEvent> Traco;
	for (uint8 Slot = 0; Slot < 3; ++Slot)
	{
		TerrenoQuePassaTeste::PassaUmSlot(Estado, Slot, Traco);
	}

	TestEqual(TEXT("Continua alagada"), Estado.CellLayout[Casa],
		static_cast<uint8>(ECellProperty::Water));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPrazoEntraNoHashTest,
	"BattleSim.Terrain.Temporary.PrazoEntraNoHash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPrazoEntraNoHashTest::RunTest(const FString&)
{
	// Mesmo tabuleiro, gelos com prazos diferentes: são duas partidas
	// diferentes, e sem isto teriam a mesma assinatura — que é exatamente a
	// divergência que o hash existe para pegar.
	FBattleState Curto = TerrenoQuePassaTeste::CampoVazio();
	Curto.SetTemporaryTerrain(1, 1, static_cast<uint8>(ECellProperty::Ice), 1);

	FBattleState Longo = TerrenoQuePassaTeste::CampoVazio();
	Longo.SetTemporaryTerrain(1, 1, static_cast<uint8>(ECellProperty::Ice), 3);

	TestNotEqual(TEXT("Prazos diferentes, hashes diferentes"),
		Curto.ComputeHash(), Longo.ComputeHash());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGolpeDeGeloNegaOTerrenoEDepoisDevolveTest,
	"BattleSim.Terrain.Temporary.GolpeDeGeloNegaOTerrenoEDepoisDevolve",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGolpeDeGeloNegaOTerrenoEDepoisDevolveTest::RunTest(const FString&)
{
	// A CADEIA INTEIRA, e é ela que importa: golpe congela → o outro não
	// submerge → o gelo derrete → ele submerge. Sem este teste, cada peça
	// estaria certa sozinha e o recurso não existiria para ninguém — que é
	// como este projeto já produziu regra testada que nenhum jogador alcança.
	FBattleState Estado = TerrenoQuePassaTeste::CampoVazio();

	FPetState Congelador;
	Congelador.PetId = 1; Congelador.Side = 0;
	Congelador.Column = 0; Congelador.Row = 1;
	Congelador.Health = 90; Congelador.MaxHealth = 90;
	Congelador.Attack = 20; Congelador.Defense = 10;
	Congelador.MovePowers[0] = 45;
	Congelador.MoveTerrainEffects[0] = static_cast<uint8>(ECellProperty::Ice);
	Congelador.MoveTerrainDurations[0] = 2;

	FPetState Mergulhador;
	Mergulhador.PetId = 2; Mergulhador.Side = 1;
	Mergulhador.Column = 1; Mergulhador.Row = 1;
	Mergulhador.Health = 90; Mergulhador.MaxHealth = 90;
	Mergulhador.Attack = 20; Mergulhador.Defense = 10;

	Estado.Pets.Add(Congelador);
	Estado.Pets.Add(Mergulhador);

	// Ele está em água FUNDA — submergir é jogada legítima dele hoje.
	const int32 Casa = Estado.CellIndex(1, 1);
	Estado.CellLayout[Casa] = static_cast<uint8>(ECellProperty::Water);
	TestTrue(TEXT("Antes do gelo, ele podia submergir"),
		Estado.TerrainAllowsSkill(EActionType::Submergir, Estado.CellLayout[Casa]));

	FBattleAction Ataque = MakeMoveAction(EActionType::Atacar, 0);
	Ataque.Direction = EBattleDirection::Direita;

	FBattleAction Nada;
	TArray<FBattleEvent> Traco;
	BattlePhases::ApplyCombat(Estado, Ataque, Nada, 0, Traco);

	TestEqual(TEXT("O golpe congelou a casa dele"), Estado.CellLayout[Casa],
		static_cast<uint8>(ECellProperty::Ice));
	TestFalse(TEXT("E agora ele NÃO submerge"),
		Estado.TerrainAllowsSkill(EActionType::Submergir, Estado.CellLayout[Casa]));

	TerrenoQuePassaTeste::PassaUmSlot(Estado, 0, Traco);
	TerrenoQuePassaTeste::PassaUmSlot(Estado, 1, Traco);

	TestEqual(TEXT("Derreteu de volta para água"), Estado.CellLayout[Casa],
		static_cast<uint8>(ECellProperty::Water));
	TestTrue(TEXT("E ele voltou a poder submergir"),
		Estado.TerrainAllowsSkill(EActionType::Submergir, Estado.CellLayout[Casa]));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNoGeloEscorregaEPerdeOMovimentoTest,
	"BattleSim.Terrain.Temporary.NoGeloEscorregaEPerdeOMovimento",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNoGeloEscorregaEPerdeOMovimentoTest::RunTest(const FString&)
{
	FBattleState Estado = TerrenoQuePassaTeste::CampoVazio();

	FPetState Preso;
	Preso.PetId = 1; Preso.Side = 0; Preso.Column = 1; Preso.Row = 1;
	Preso.Health = 90; Preso.MaxHealth = 90;

	FPetState Outro;
	Outro.PetId = 2; Outro.Side = 1; Outro.Column = 2; Outro.Row = 2;
	Outro.Health = 90; Outro.MaxHealth = 90;

	Estado.Pets.Add(Preso);
	Estado.Pets.Add(Outro);
	Estado.SetTemporaryTerrain(1, 1, static_cast<uint8>(ECellProperty::Ice), 1);

	FBattleAction Andar;
	Andar.Type = EActionType::Mover;
	Andar.Direction = EBattleDirection::Cima;

	FBattleAction Nada;
	TArray<FBattleEvent> Traco;
	BattlePhases::ApplyMovement(Estado, Andar, Nada, 0, Traco);

	TestEqual(TEXT("Ficou onde estava"), Estado.Pets[0].Row, static_cast<uint8>(1));
	TestEqual(TEXT("E foi narrado como escorregão"),
		TerrenoQuePassaTeste::ContaEventos(Traco, EBattleEventType::Escorregou), 1);

	// NÃO é "esbarrou": o feed narra aquele como limite da arena, e a causa
	// aqui é outra. Confundir os dois ensinaria a lição errada.
	TestEqual(TEXT("Não sai como movimento bloqueado"),
		TerrenoQuePassaTeste::ContaEventos(Traco, EBattleEventType::MovimentoBloqueado), 0);

	// Depois do degelo ele anda de novo — o gelo prende, não aleija.
	TArray<FBattleEvent> Depois;
	BattlePhases::ApplyResolution(Estado, 0, Depois);
	BattlePhases::ApplyMovement(Estado, Andar, Nada, 1, Depois);

	TestEqual(TEXT("Derretido, o movimento volta"),
		Estado.Pets[0].Row, static_cast<uint8>(0));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEscorregarEDadoNaoCodigoTest,
	"BattleSim.Terrain.Temporary.EscorregarEDadoNaoCodigo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEscorregarEDadoNaoCodigoTest::RunTest(const FString&)
{
	FBattleState Estado = TerrenoQuePassaTeste::CampoVazio();

	TestEqual(TEXT("No gelo escorrega SEMPRE — quem congelou sabe o que comprou"),
		static_cast<int32>(Estado.TerrainSlipPercent[static_cast<int32>(ECellProperty::Ice)]), 100);
	TestFalse(TEXT("E a água funda não faz nada com quem sai dela"),
		Estado.TerrainAffectsDeparture(static_cast<uint8>(ECellProperty::Water)));

	// A LAMA é aposta, e os três desfechos precisam caber: escorregar,
	// atrasar, e o que sobra. Se as duas chances somassem 100, atravessar
	// firme deixaria de existir e a lama viraria um gelo com duas caras.
	const int32 Escorrega = Estado.TerrainSlipPercent[static_cast<int32>(ECellProperty::Mud)];
	const int32 Atrasa = Estado.TerrainSlowPercent[static_cast<int32>(ECellProperty::Mud)];
	TestTrue(TEXT("Na lama dá para escorregar"), Escorrega > 0);
	TestTrue(TEXT("Na lama dá para atrasar"), Atrasa > 0);
	TestTrue(TEXT("E sobra chance de atravessar firme"), Escorrega + Atrasa < 100);

	// "Areia que atola" tem de ser uma LINHA, não uma exceção nova no núcleo.
	// É a mesma promessa da fatia 1, e ela só vale se der para exercê-la.
	Estado.TerrainSlipPercent[static_cast<int32>(ECellProperty::ShallowWater)] = 100;
	TestTrue(TEXT("Um terreno novo passa a prender sem tocar no núcleo"),
		Estado.TerrainAffectsDeparture(static_cast<uint8>(ECellProperty::ShallowWater)));

	// E entra no hash: prender ou não prender muda o resultado.
	FBattleState Normal = TerrenoQuePassaTeste::CampoVazio();
	TestNotEqual(TEXT("Regra de escorregão diferente, hash diferente"),
		Estado.ComputeHash(), Normal.ComputeHash());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCadeiaDeSecagemTest,
	"BattleSim.Terrain.Temporary.CadeiaDeSecagem",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCadeiaDeSecagemTest::RunTest(const FString&)
{
	// GELO → POÇA → LAMA → CHÃO SECO, sozinho. É o que faz o tabuleiro
	// continuar vivo depois da jogada: uma ação de gelo deixa rastro por
	// vários slots, em vez de piscar e sumir.
	FBattleState Estado = TerrenoQuePassaTeste::CampoVazio();
	const int32 Casa = Estado.CellIndex(1, 1);
	Estado.SetTemporaryTerrain(1, 1, static_cast<uint8>(ECellProperty::Ice), 1);

	TArray<FBattleEvent> Traco;
	uint8 Slot = 0;

	BattlePhases::ApplyResolution(Estado, Slot++, Traco);
	// Gelo sobre chão SECO derrete em POÇA, e não em chão seco: gelo é água
	// congelada, e derreter sem molhar nada não é derreter.
	TestEqual(TEXT("O gelo virou poça"), Estado.CellLayout[Casa],
		static_cast<uint8>(ECellProperty::ShallowWater));

	while (Estado.CellLayout[Casa] == static_cast<uint8>(ECellProperty::ShallowWater)
		&& Slot < 12)
	{
		BattlePhases::ApplyResolution(Estado, Slot++, Traco);
	}
	TestEqual(TEXT("A poça virou lama"), Estado.CellLayout[Casa],
		static_cast<uint8>(ECellProperty::Mud));

	while (Estado.CellLayout[Casa] == static_cast<uint8>(ECellProperty::Mud)
		&& Slot < 24)
	{
		BattlePhases::ApplyResolution(Estado, Slot++, Traco);
	}
	TestEqual(TEXT("E a lama secou"), Estado.CellLayout[Casa],
		static_cast<uint8>(ECellProperty::None));

	// A cadeia PARA. Chão seco não vira nada, e um passo a mais aqui seria
	// um tabuleiro que nunca descansa.
	const int32 AntesDoExtra = TerrenoQuePassaTeste::ContaEventos(Traco, EBattleEventType::TerrenoDerreteu);
	BattlePhases::ApplyResolution(Estado, Slot, Traco);
	TestEqual(TEXT("Seco fica seco"),
		TerrenoQuePassaTeste::ContaEventos(Traco, EBattleEventType::TerrenoDerreteu), AntesDoExtra);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRioCongeladoVoltaASerRioTest,
	"BattleSim.Terrain.Temporary.RioCongeladoVoltaASerRio",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRioCongeladoVoltaASerRioTest::RunTest(const FString&)
{
	// A cadeia diz "gelo vira poça". O que havia embaixo diz "era um rio".
	// Vence o mais MOLHADO — senão congelar seria a maneira mais barata de
	// secar um rio, que é o oposto do que o gelo é.
	FBattleState Estado = TerrenoQuePassaTeste::CampoVazio();
	const int32 Casa = Estado.CellIndex(1, 1);
	Estado.CellLayout[Casa] = static_cast<uint8>(ECellProperty::Water);
	Estado.SetTemporaryTerrain(1, 1, static_cast<uint8>(ECellProperty::Ice), 1);

	TArray<FBattleEvent> Traco;
	BattlePhases::ApplyResolution(Estado, 0, Traco);

	TestEqual(TEXT("O rio voltou a ser rio"), Estado.CellLayout[Casa],
		static_cast<uint8>(ECellProperty::Water));

	// E CONTINUA rio: água funda posta pela arena não seca sozinha, senão o
	// campo inteiro viraria lama numa partida longa.
	for (uint8 Slot = 1; Slot < 8; ++Slot)
	{
		BattlePhases::ApplyResolution(Estado, Slot, Traco);
	}
	TestEqual(TEXT("E não seca com o tempo"), Estado.CellLayout[Casa],
		static_cast<uint8>(ECellProperty::Water));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLamaDaNosTresDesfechosTest,
	"BattleSim.Terrain.Temporary.LamaDaNosTresDesfechos",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLamaDaNosTresDesfechosTest::RunTest(const FString&)
{
	// Os TRÊS têm de acontecer. Um sorteio que na prática só dá dois
	// resultados é um sorteio que mente sobre o que a lama é — e ninguém
	// notaria, porque cada caso isolado continua correto.
	int32 Escorregou = 0;
	int32 Atrasou = 0;
	int32 Firme = 0;

	for (uint32 Semente = 1; Semente <= 60; ++Semente)
	{
		FBattleState Estado = TerrenoQuePassaTeste::CampoVazio();
		Estado.Random.State = Semente;

		FPetState Andarilho;
		Andarilho.PetId = 1; Andarilho.Side = 0;
		Andarilho.Column = 1; Andarilho.Row = 1;
		Andarilho.Health = 90; Andarilho.MaxHealth = 90; Andarilho.Speed = 40;

		FPetState Outro;
		Outro.PetId = 2; Outro.Side = 1; Outro.Column = 2; Outro.Row = 2;
		Outro.Health = 90; Outro.MaxHealth = 90;

		Estado.Pets.Add(Andarilho);
		Estado.Pets.Add(Outro);
		Estado.CellLayout[Estado.CellIndex(1, 1)] = static_cast<uint8>(ECellProperty::Mud);

		FBattleAction Andar;
		Andar.Type = EActionType::Mover;
		Andar.Direction = EBattleDirection::Cima;

		FBattleAction Nada;
		TArray<FBattleEvent> Traco;
		BattlePhases::ApplyMovement(Estado, Andar, Nada, 0, Traco);

		if (TerrenoQuePassaTeste::ContaEventos(Traco, EBattleEventType::Escorregou) > 0)
		{
			++Escorregou;
			TestEqual(TEXT("Quem escorregou não saiu do lugar"),
				Estado.Pets[0].Row, static_cast<uint8>(1));
		}
		else if (TerrenoQuePassaTeste::ContaEventos(Traco, EBattleEventType::AtravessouDevagar) > 0)
		{
			++Atrasou;
			// Atrasado ANDOU: é o que o separa de quem escorregou, e trocar
			// os dois faria a lama ter duas formas de não sair do lugar.
			TestEqual(TEXT("Quem atrasou ANDOU"),
				Estado.Pets[0].Row, static_cast<uint8>(0));
			TestTrue(TEXT("E está mais lento que o normal"),
				Estado.Pets[0].GetEffectiveSpeed() < Estado.Pets[0].Speed);
		}
		else
		{
			++Firme;
			TestEqual(TEXT("Quem passou firme andou"),
				Estado.Pets[0].Row, static_cast<uint8>(0));
			TestEqual(TEXT("E na velocidade dele"),
				Estado.Pets[0].GetEffectiveSpeed(), Estado.Pets[0].Speed);
		}
	}

	TestTrue(TEXT("Escorregar acontece"), Escorregou > 0);
	TestTrue(TEXT("Atrasar acontece"), Atrasou > 0);
	TestTrue(TEXT("Atravessar firme acontece"), Firme > 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FChaoComumNaoConsomeSorteioTest,
	"BattleSim.Terrain.Temporary.ChaoComumNaoConsomeSorteio",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChaoComumNaoConsomeSorteioTest::RunTest(const FString&)
{
	// Andar em chão seco NÃO pode tirar número do gerador. Se tirasse, o
	// fluxo aleatório de toda partida já gravada se deslocaria, e todo
	// snapshot de determinismo divergiria por causa de uma regra que aquelas
	// batalhas nem exercem — falha que apareceria longe daqui e sem sintoma
	// que apontasse para cá.
	FBattleState Estado = TerrenoQuePassaTeste::CampoVazio();

	FPetState Andarilho;
	Andarilho.PetId = 1; Andarilho.Side = 0; Andarilho.Column = 1; Andarilho.Row = 1;
	Andarilho.Health = 90; Andarilho.MaxHealth = 90;

	FPetState Outro;
	Outro.PetId = 2; Outro.Side = 1; Outro.Column = 2; Outro.Row = 2;
	Outro.Health = 90; Outro.MaxHealth = 90;

	Estado.Pets.Add(Andarilho);
	Estado.Pets.Add(Outro);

	const uint64 AntesDoPasso = Estado.Random.State;

	FBattleAction Andar;
	Andar.Type = EActionType::Mover;
	Andar.Direction = EBattleDirection::Cima;

	FBattleAction Nada;
	TArray<FBattleEvent> Traco;
	BattlePhases::ApplyMovement(Estado, Andar, Nada, 0, Traco);

	TestEqual(TEXT("O gerador não andou"), Estado.Random.State, AntesDoPasso);

	return true;
}

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

using namespace TerrenoQuePassaTeste;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGeloDerreteNoPrazoTest,
	"BattleSim.Terrain.Temporary.GeloDerreteNoPrazo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGeloDerreteNoPrazoTest::RunTest(const FString&)
{
	FBattleState Estado = CampoVazio();
	Estado.SetTemporaryTerrain(1, 1, static_cast<uint8>(ECellProperty::Ice), 2);

	const int32 Casa = Estado.CellIndex(1, 1);
	TestEqual(TEXT("A casa congelou"), Estado.CellLayout[Casa],
		static_cast<uint8>(ECellProperty::Ice));

	TArray<FBattleEvent> Traco;
	PassaUmSlot(Estado, 0, Traco);

	// Prazo 2 tem que sobreviver ao PRIMEIRO slot. Fosse "derrete no fim do
	// slot em que foi posto", congelar por 1 seria congelar por nada, e o
	// menor congelamento do jogo não existiria.
	TestEqual(TEXT("Depois de um slot ainda é gelo"), Estado.CellLayout[Casa],
		static_cast<uint8>(ECellProperty::Ice));
	TestEqual(TEXT("E ninguém anunciou derretimento"),
		ContaEventos(Traco, EBattleEventType::TerrenoDerreteu), 0);

	PassaUmSlot(Estado, 1, Traco);

	TestEqual(TEXT("No segundo slot derreteu"), Estado.CellLayout[Casa],
		static_cast<uint8>(ECellProperty::None));
	TestEqual(TEXT("E foi anunciado"),
		ContaEventos(Traco, EBattleEventType::TerrenoDerreteu), 1);

	// Derretido é derretido: nada de anunciar de novo a cada slot seguinte.
	PassaUmSlot(Estado, 2, Traco);
	TestEqual(TEXT("Não derrete duas vezes"),
		ContaEventos(Traco, EBattleEventType::TerrenoDerreteu), 1);

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
	FBattleState Estado = CampoVazio();
	const int32 Casa = Estado.CellIndex(1, 1);
	Estado.CellLayout[Casa] = static_cast<uint8>(ECellProperty::ShallowWater);

	Estado.SetTemporaryTerrain(1, 1, static_cast<uint8>(ECellProperty::Ice), 1);

	TArray<FBattleEvent> Traco;
	PassaUmSlot(Estado, 0, Traco);

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
	FBattleState Estado = CampoVazio();
	const int32 Casa = Estado.CellIndex(1, 1);
	Estado.CellLayout[Casa] = static_cast<uint8>(ECellProperty::Water);

	Estado.SetTemporaryTerrain(1, 1, static_cast<uint8>(ECellProperty::Ice), 1);
	Estado.SetTemporaryTerrain(1, 1, static_cast<uint8>(ECellProperty::Ice), 1);

	TArray<FBattleEvent> Traco;
	PassaUmSlot(Estado, 0, Traco);

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
	FBattleState Estado = CampoVazio();
	const int32 Casa = Estado.CellIndex(1, 1);
	Estado.SetTemporaryTerrain(1, 1, static_cast<uint8>(ECellProperty::Water), 0);

	TArray<FBattleEvent> Traco;
	for (uint8 Slot = 0; Slot < 3; ++Slot)
	{
		PassaUmSlot(Estado, Slot, Traco);
	}

	TestEqual(TEXT("A água alagada não volta atrás"),
		Estado.CellLayout[Casa], static_cast<uint8>(ECellProperty::Water));
	TestEqual(TEXT("E nada derreteu"),
		ContaEventos(Traco, EBattleEventType::TerrenoDerreteu), 0);

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
	FBattleState Estado = CampoVazio();
	const int32 Casa = Estado.CellIndex(1, 1);

	Estado.SetTemporaryTerrain(1, 1, static_cast<uint8>(ECellProperty::Ice), 2);
	Estado.SetTemporaryTerrain(1, 1, static_cast<uint8>(ECellProperty::Water), 0);

	TArray<FBattleEvent> Traco;
	for (uint8 Slot = 0; Slot < 3; ++Slot)
	{
		PassaUmSlot(Estado, Slot, Traco);
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
	FBattleState Curto = CampoVazio();
	Curto.SetTemporaryTerrain(1, 1, static_cast<uint8>(ECellProperty::Ice), 1);

	FBattleState Longo = CampoVazio();
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
	FBattleState Estado = CampoVazio();

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

	PassaUmSlot(Estado, 0, Traco);
	PassaUmSlot(Estado, 1, Traco);

	TestEqual(TEXT("Derreteu de volta para água"), Estado.CellLayout[Casa],
		static_cast<uint8>(ECellProperty::Water));
	TestTrue(TEXT("E ele voltou a poder submergir"),
		Estado.TerrainAllowsSkill(EActionType::Submergir, Estado.CellLayout[Casa]));

	return true;
}

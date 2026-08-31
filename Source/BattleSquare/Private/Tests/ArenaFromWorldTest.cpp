// Copyright 2026 Anderson. All Rights Reserved.

#include "World/ArenaFromWorld.h"
#include "Battle/BattleState.h"
#include "Misc/AutomationTest.h"

namespace ArenaDoMundoTeste
{
	FArenaFromWorldParams Campo3x3()
	{
		FArenaFromWorldParams Params;
		Params.EncounterLocation = FVector(0.0f, 0.0f, 0.0f);
		Params.CellSize = 200.0f;
		Params.Columns = 3;
		Params.Rows = 3;
		return Params;
	}

	/** Centro da casa (Coluna, Linha) numa grade centrada na origem. */
	FVector CentroDaCasa(const FArenaFromWorldParams& Params, int32 Coluna, int32 Linha)
	{
		const float CantoX = Params.EncounterLocation.X - Params.Columns * Params.CellSize * 0.5f;
		const float CantoY = Params.EncounterLocation.Y - Params.Rows * Params.CellSize * 0.5f;
		return FVector(
			CantoX + (Coluna + 0.5f) * Params.CellSize,
			CantoY + (Linha + 0.5f) * Params.CellSize,
			0.0f);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOMundoVirouTabuleiroTest,
	"BattleSquare.Arena.FromWorld.OMundoVirouTabuleiro",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FOMundoVirouTabuleiroTest::RunTest(const FString&)
{
	FArenaFromWorldParams Params = ArenaDoMundoTeste::Campo3x3();
	Params.Features.Add({ ArenaDoMundoTeste::CentroDaCasa(Params, 1, 0),
		EWorldFeatureKind::Solid });
	Params.Features.Add({ ArenaDoMundoTeste::CentroDaCasa(Params, 2, 0),
		EWorldFeatureKind::DeepWater });
	Params.Features.Add({ ArenaDoMundoTeste::CentroDaCasa(Params, 1, 2),
		EWorldFeatureKind::TrainingGround });

	const TArray<uint8> Layout = FArenaFromWorld::Build(Params);

	TestEqual(TEXT("O tabuleiro tem o tamanho da grade"), Layout.Num(), 9);
	TestEqual(TEXT("A árvore virou casa bloqueada"),
		Layout[CellLayoutIndex(1, 0, 3)], static_cast<uint8>(ECellProperty::Blocked));
	TestEqual(TEXT("O rio virou água funda"),
		Layout[CellLayoutIndex(2, 0, 3)], static_cast<uint8>(ECellProperty::Water));
	TestEqual(TEXT("A clareira de treino virou bônus"),
		Layout[CellLayoutIndex(1, 2, 3)], static_cast<uint8>(ECellProperty::Buff));
	TestEqual(TEXT("E o que não tinha nada continua neutro"),
		Layout[CellLayoutIndex(1, 1, 3)], static_cast<uint8>(ECellProperty::None));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSemMundoNaoInventaArenaTest,
	"BattleSquare.Arena.FromWorld.SemMundoNaoInventaArena",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSemMundoNaoInventaArenaTest::RunTest(const FString&)
{
	// Vazio, e NÃO um campo todo neutro. Um tabuleiro neutro pareceria uma
	// escolha — "aqui não tem nada" — quando na verdade é ausência de
	// informação, e quem recebe precisa poder cair no catálogo de sempre.
	FArenaFromWorldParams Params = ArenaDoMundoTeste::Campo3x3();
	TestEqual(TEXT("Sem amostra, sem arena"), FArenaFromWorld::Build(Params).Num(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMesmoLugarMesmaArenaTest,
	"BattleSquare.Arena.FromWorld.MesmoLugarMesmaArena",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMesmoLugarMesmaArenaTest::RunTest(const FString&)
{
	// A ordem em que os atores do mundo são varridos NÃO é estável entre
	// execuções. Se ela decidisse o terreno de uma casa disputada, o mesmo
	// encontro no mesmo lugar abriria arenas diferentes — e o determinismo do
	// núcleo não valeria de nada acima dele.
	FArenaFromWorldParams Params = ArenaDoMundoTeste::Campo3x3();
	const FVector MesmaCasa = ArenaDoMundoTeste::CentroDaCasa(Params, 2, 0);

	Params.Features.Add({ MesmaCasa, EWorldFeatureKind::DeepWater });
	Params.Features.Add({ MesmaCasa, EWorldFeatureKind::Solid });
	const TArray<uint8> UmaOrdem = FArenaFromWorld::Build(Params);

	FArenaFromWorldParams Invertido = ArenaDoMundoTeste::Campo3x3();
	Invertido.Features.Add({ MesmaCasa, EWorldFeatureKind::Solid });
	Invertido.Features.Add({ MesmaCasa, EWorldFeatureKind::DeepWater });
	const TArray<uint8> OutraOrdem = FArenaFromWorld::Build(Invertido);

	TestTrue(TEXT("A ordem da varredura não muda o tabuleiro"), UmaOrdem == OutraOrdem);

	// E o CORPO vence: uma casa desenhada como rio com uma pedra em pé nela
	// seria a tela mentindo sobre a regra.
	TestEqual(TEXT("A pedra vence o rio"),
		UmaOrdem[CellLayoutIndex(2, 0, 3)], static_cast<uint8>(ECellProperty::Blocked));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBeiraDeAguaSegueOClimaTest,
	"BattleSquare.Arena.FromWorld.BeiraDeAguaSegueOClima",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBeiraDeAguaSegueOClimaTest::RunTest(const FString&)
{
	// A mesma regra que decide se uma poça vira lama ao secar, aplicada ao
	// LUGAR em vez de ao tempo. Sem isto a umidade valeria para a água que o
	// jogador cria e não para a que já estava lá — duas físicas no campo.
	FArenaFromWorldParams Umido = ArenaDoMundoTeste::Campo3x3();
	Umido.HumidityPercent = 70;
	Umido.Features.Add({ ArenaDoMundoTeste::CentroDaCasa(Umido, 0, 0),
		EWorldFeatureKind::Shore });

	TestEqual(TEXT("Margem em mata úmida é lamaçal"),
		FArenaFromWorld::Build(Umido)[CellLayoutIndex(0, 0, 3)],
		static_cast<uint8>(ECellProperty::Mud));

	FArenaFromWorldParams Seco = ArenaDoMundoTeste::Campo3x3();
	Seco.HumidityPercent = 10;
	Seco.Features.Add({ ArenaDoMundoTeste::CentroDaCasa(Seco, 0, 0),
		EWorldFeatureKind::Shore });

	TestEqual(TEXT("E no deserto é só areia molhada"),
		FArenaFromWorld::Build(Seco)[CellLayoutIndex(0, 0, 3)],
		static_cast<uint8>(ECellProperty::ShallowWater));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPedraNaCasaInicialSaiDaFrenteTest,
	"BattleSquare.Arena.FromWorld.PedraNaCasaInicialSaiDaFrente",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPedraNaCasaInicialSaiDaFrenteTest::RunTest(const FString&)
{
	// Com catálogo dava para tentar o próximo layout. Com o mundo não existe
	// próximo: o lugar é aquele. Se a pedra ficasse, a montagem seria
	// rejeitada e a batalha simplesmente não abriria — o jogador topa com o
	// inimigo e nada acontece, que é a pior falha possível aqui.
	for (const int32 Colunas : { 3, 4, 5 })
	{
		for (const int32 Linhas : { 2, 3, 6 })
		{
			FArenaFromWorldParams Params = ArenaDoMundoTeste::Campo3x3();
			Params.Columns = Colunas;
			Params.Rows = Linhas;

			for (int32 Coluna = 0; Coluna < Colunas; ++Coluna)
			{
				for (int32 Linha = 0; Linha < Linhas; ++Linha)
				{
					Params.Features.Add({ ArenaDoMundoTeste::CentroDaCasa(Params, Coluna, Linha),
						EWorldFeatureKind::Solid });
				}
			}

			const TArray<uint8> Layout = FArenaFromWorld::Build(Params);

			// O ONDE vem do núcleo, e não de uma conta escrita no teste: é a
			// duplicação que este par de arquivos assume, e é aqui que ela é
			// cobrada. Se PlaceDuelistsAtStartingCells mudar sozinha, isto cai.
			FBattleState Estado;
			Estado.ResizeGrid(Colunas, Linhas);
			FPetState Esquerda; Esquerda.PetId = 1; Esquerda.Side = 0;
			FPetState Direita; Direita.PetId = 2; Direita.Side = 1;
			Estado.Pets.Add(Esquerda);
			Estado.Pets.Add(Direita);
			Estado.PlaceDuelistsAtStartingCells();

			for (const FPetState& Pet : Estado.Pets)
			{
				const int32 Indice = CellLayoutIndex(Pet.Column, Pet.Row, Colunas);
				TestNotEqual(
					FString::Printf(TEXT("Casa inicial livre em %dx%d"), Colunas, Linhas),
					Layout[Indice], static_cast<uint8>(ECellProperty::Blocked));
			}
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAEnchenteSobeUmaCasaTest,
	"BattleSquare.Arena.FromWorld.AEnchenteSobeUmaCasa",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAEnchenteSobeUmaCasaTest::RunTest(const FString&)
{
	FArenaFromWorldParams Params = ArenaDoMundoTeste::Campo3x3();
	Params.Features.Add({ ArenaDoMundoTeste::CentroDaCasa(Params, 2, 0),
		EWorldFeatureKind::DeepWater });

	// Sem tempestade, o mesmo campo continua seco. É a metade do teste que
	// impede o alagamento de virar o normal: sem ela, um `bFlooded` ignorado
	// (ou sempre ligado) passaria despercebido.
	const TArray<uint8> Seco = FArenaFromWorld::Build(Params);
	TestEqual(TEXT("Sem tempestade a casa ao lado do rio continua seca"),
		Seco[CellLayoutIndex(1, 0, 3)], static_cast<uint8>(ECellProperty::None));

	Params.bFlooded = true;
	const TArray<uint8> Alagado = FArenaFromWorld::Build(Params);

	TestEqual(TEXT("O rio continua fundo"),
		Alagado[CellLayoutIndex(2, 0, 3)], static_cast<uint8>(ECellProperty::Water));
	TestEqual(TEXT("A casa encostada no rio virou poça"),
		Alagado[CellLayoutIndex(1, 0, 3)], static_cast<uint8>(ECellProperty::ShallowWater));

	// UMA casa, e não o tabuleiro inteiro: a poça recém-nascida não pode molhar
	// a vizinha dela na mesma passada, senão a água atravessaria o campo de uma
	// vez e o resultado dependeria da ordem de varredura.
	TestEqual(TEXT("A casa duas de distância continua seca"),
		Alagado[CellLayoutIndex(1, 1, 3)], static_cast<uint8>(ECellProperty::None));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAEnchenteNaoDerrubaPedraTest,
	"BattleSquare.Arena.FromWorld.AEnchenteNaoDerrubaPedra",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAEnchenteNaoDerrubaPedraTest::RunTest(const FString&)
{
	FArenaFromWorldParams Params = ArenaDoMundoTeste::Campo3x3();
	Params.bFlooded = true;
	Params.Features.Add({ ArenaDoMundoTeste::CentroDaCasa(Params, 1, 0),
		EWorldFeatureKind::DeepWater });
	Params.Features.Add({ ArenaDoMundoTeste::CentroDaCasa(Params, 0, 0),
		EWorldFeatureKind::Solid });
	Params.Features.Add({ ArenaDoMundoTeste::CentroDaCasa(Params, 1, 1),
		EWorldFeatureKind::TrainingGround });

	const TArray<uint8> Layout = FArenaFromWorld::Build(Params);

	// Enchente COBRE o chão; ela não demole nem apaga o que estava ali. Uma
	// pedra continua de pé dentro d'água, e comer a casa de bônus faria a
	// tempestade destruir terreno em vez de alagá-lo.
	TestEqual(TEXT("A pedra continua bloqueando"),
		Layout[CellLayoutIndex(0, 0, 3)], static_cast<uint8>(ECellProperty::Blocked));
	TestEqual(TEXT("A clareira de treino continua dando bônus"),
		Layout[CellLayoutIndex(1, 1, 3)], static_cast<uint8>(ECellProperty::Buff));

	return true;
}

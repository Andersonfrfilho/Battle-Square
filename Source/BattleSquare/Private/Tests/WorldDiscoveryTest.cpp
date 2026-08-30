// Copyright 2026 Anderson. All Rights Reserved.

#include "World/WorldDiscovery.h"
#include "UI/WorldMapProjection.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMapaComecaEmBrancoTest,
	"BattleSquare.World.Discovery.MapaComecaEmBranco",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMapaComecaEmBrancoTest::RunTest(const FString&)
{
	// É a razão de existir da feature. Um mapa que já nasce completo responde
	// "onde fica o campo de Voo?" antes de a pergunta existir, e o mundo aberto
	// perde a única coisa que o faz valer a pena atravessar.
	const FWorldDiscovery Novo;

	TestEqual(TEXT("Ninguém conhece nada ao começar"), Novo.DiscoveredCount(), 0);
	TestFalse(TEXT("Nem o próprio ponto de partida"),
		Novo.IsDiscovered(FVector2D::ZeroVector));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAndarRevelaOEntornoTest,
	"BattleSquare.World.Discovery.AndarRevelaOEntorno",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAndarRevelaOEntornoTest::RunTest(const FString&)
{
	FWorldDiscovery Conhecido;
	const int32 Novas = Conhecido.MarkSeenFrom(FVector2D::ZeroVector);

	TestTrue(TEXT("Onde pisei está descoberto"),
		Conhecido.IsDiscovered(FVector2D::ZeroVector));

	// O ENTORNO também. Revelar só a casa sob o pé faria o mapa contar por onde
	// se pisou, não o que se viu — e ninguém enxerga apenas para baixo. Na tela
	// a diferença é entre uma mancha e um rastro de migalhas.
	const float UmaRegiao = FWorldDiscovery::RegionSizeUnits;
	TestTrue(TEXT("O vizinho ao norte também"),
		Conhecido.IsDiscovered(FVector2D(UmaRegiao, 0.0f)));
	TestTrue(TEXT("E o da diagonal"),
		Conhecido.IsDiscovered(FVector2D(UmaRegiao, UmaRegiao)));

	// Mas o mundo NÃO inteiro: descoberta que vaza longe demais é mapa
	// completo com passos extras.
	TestFalse(TEXT("O que está a três regiões continua escuro"),
		Conhecido.IsDiscovered(FVector2D(UmaRegiao * 3.0f, 0.0f)));

	TestEqual(TEXT("Uma passada revela um quadrado 3x3 de regiões"), Novas, 9);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRedescobrirNaoContaDeNovoTest,
	"BattleSquare.World.Discovery.RedescobrirNaoContaDeNovo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRedescobrirNaoContaDeNovoTest::RunTest(const FString&)
{
	// Andar dentro do que já se conhece é o caso COMUM, e precisa devolver
	// zero: quem chama grava o save quando algo muda, e um retorno que sempre
	// diz "novidade" faria o jogo gravar a cada passo.
	FWorldDiscovery Conhecido;
	Conhecido.MarkSeenFrom(FVector2D::ZeroVector);

	const int32 Repetido = Conhecido.MarkSeenFrom(FVector2D(10.0f, 10.0f));
	TestEqual(TEXT("Voltar ao mesmo lugar não descobre nada"), Repetido, 0);
	TestEqual(TEXT("E a contagem não muda"), Conhecido.DiscoveredCount(), 9);

	// Andar ADIANTE descobre a faixa nova, e só ela.
	const float UmaRegiao = FWorldDiscovery::RegionSizeUnits;
	const int32 Avancou = Conhecido.MarkSeenFrom(FVector2D(UmaRegiao, 0.0f));
	TestEqual(TEXT("Um passo à frente revela só a coluna nova"), Avancou, 3);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDescobertaFuncionaEmCoordenadaNegativaTest,
	"BattleSquare.World.Discovery.FuncionaEmCoordenadaNegativa",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDescobertaFuncionaEmCoordenadaNegativaTest::RunTest(const FString&)
{
	// Metade do mundo tem coordenada negativa, e truncar em vez de arredondar
	// para baixo faria as regiões -0 e +0 serem a MESMA: quem anda para o sul
	// revelaria o norte junto, e a mancha do mapa sairia espelhada. É o mesmo
	// tipo de erro de eixo que já custou uma investigação inteira a este
	// projeto ("Baixo" andava para a direita).
	FWorldDiscovery Conhecido;
	const float UmaRegiao = FWorldDiscovery::RegionSizeUnits;

	Conhecido.MarkSeenFrom(FVector2D(-UmaRegiao * 5.0f, -UmaRegiao * 5.0f));

	TestTrue(TEXT("O sudoeste distante ficou descoberto"),
		Conhecido.IsDiscovered(FVector2D(-UmaRegiao * 5.0f, -UmaRegiao * 5.0f)));
	TestFalse(TEXT("E a origem continua escura"),
		Conhecido.IsDiscovered(FVector2D::ZeroVector));

	TestNotEqual(TEXT("Meia região ao sul não é a mesma que meia ao norte"),
		FWorldDiscovery::RegionRowOf(-1.0f), FWorldDiscovery::RegionRowOf(1.0f));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMapaEscondeOQueNaoFoiDescobertoTest,
	"BattleSquare.World.Discovery.MapaEscondeOQueNaoFoiDescoberto",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMapaEscondeOQueNaoFoiDescobertoTest::RunTest(const FString&)
{
	const float Longe = FWorldDiscovery::RegionSizeUnits * 10.0f;

	FWorldMapSnapshot Retrato;
	Retrato.Discovery.MarkSeenFrom(FVector2D::ZeroVector);

	FWorldMapMarkerInfo Perto;
	Perto.WorldXY = FVector2D::ZeroVector;
	Perto.Kind = EWorldMapMarker::CampoDeTreino;

	FWorldMapMarkerInfo NoEscuro;
	NoEscuro.WorldXY = FVector2D(Longe, Longe);
	NoEscuro.Kind = EWorldMapMarker::CampoDeTreino;

	TestTrue(TEXT("O que está em região conhecida aparece"),
		FWorldMapProjection::IsMarkerVisible(Perto, Retrato));
	TestFalse(TEXT("O que está no escuro NÃO aparece"),
		FWorldMapProjection::IsMarkerVisible(NoEscuro, Retrato));

	// O JOGADOR é sempre visível. Escondê-lo por causa de uma região não
	// marcada seria o mapa negar a única coisa que ele sabe com certeza — e
	// o jogador some do próprio mapa no primeiro passo depois de carregar.
	FWorldMapMarkerInfo Eu;
	Eu.WorldXY = FVector2D(Longe, Longe);
	Eu.Kind = EWorldMapMarker::Jogador;
	TestTrue(TEXT("O jogador aparece mesmo em região não marcada"),
		FWorldMapProjection::IsMarkerVisible(Eu, Retrato));

	// E o mapa COMPLETO continua possível: falso explícito, e não deduzido de
	// "a descoberta está vazia" — vazia é o estado de quem nunca andou, e
	// confundir os dois faria o mapa nascer completo para quem começou agora.
	Retrato.bHidesUndiscovered = false;
	TestTrue(TEXT("Sem a regra, tudo aparece"),
		FWorldMapProjection::IsMarkerVisible(NoEscuro, Retrato));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLegendaDescreveOMapaQueExisteTest,
	"BattleSquare.World.Map.LegendaDescreveOMapaQueExiste",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLegendaDescreveOMapaQueExisteTest::RunTest(const FString&)
{
	const EWorldMapTerrain Todos[] = {
		EWorldMapTerrain::Clareira, EWorldMapTerrain::Mata,
		EWorldMapTerrain::Margem, EWorldMapTerrain::Agua,
		EWorldMapTerrain::Relevo };

	// Todo terreno tem NOME. Um sem rótulo apareceria no mapa como mancha que
	// a legenda não explica — e a legenda existe justamente para isso.
	TSet<FString> Nomes;
	for (const EWorldMapTerrain Terreno : Todos)
	{
		const FString Nome = FWorldMapProjection::LabelForTerrain(Terreno).ToString();
		TestFalse(TEXT("Nenhum terreno fica sem nome"), Nome.IsEmpty());
		TestFalse(TEXT("E nenhum cai no rótulo de desconhecido"),
			Nome.Equals(TEXT("desconhecido")));
		Nomes.Add(Nome);
	}
	TestEqual(TEXT("Nomes não se repetem"), Nomes.Num(), static_cast<int32>(UE_ARRAY_COUNT(Todos)));

	// E toda COR é distinta. Dois terrenos da mesma cor são um terreno só na
	// tela, e a legenda passaria a listar uma diferença que ninguém vê.
	TSet<FString> Cores;
	for (const EWorldMapTerrain Terreno : Todos)
	{
		Cores.Add(FWorldMapProjection::ColorForTerrain(Terreno).ToString());
	}
	TestEqual(TEXT("Cores não se repetem"), Cores.Num(), static_cast<int32>(UE_ARRAY_COUNT(Todos)));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTerrenoEFundoNaoInformacaoTest,
	"BattleSquare.World.Map.TerrenoEFundoNaoInformacao",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTerrenoEFundoNaoInformacaoTest::RunTest(const FString&)
{
	// Terreno é FUNDO: os marcadores precisam saltar por cima dele. É a mesma
	// regra da paleta do mundo — criatura é viva, terreno é terra — e é o que
	// impede o adversário laranja de sumir dentro de uma mancha de mata.
	const FLinearColor Adversario(0.90f, 0.45f, 0.20f);
	const float ForcaDoAdversario = Adversario.R + Adversario.G + Adversario.B;

	const EWorldMapTerrain Todos[] = {
		EWorldMapTerrain::Clareira, EWorldMapTerrain::Mata,
		EWorldMapTerrain::Margem, EWorldMapTerrain::Agua,
		EWorldMapTerrain::Relevo };

	for (const EWorldMapTerrain Terreno : Todos)
	{
		const FLinearColor Cor = FWorldMapProjection::ColorForTerrain(Terreno);
		TestTrue(TEXT("Todo terreno é mais surdo que o marcador de adversário"),
			(Cor.R + Cor.G + Cor.B) < ForcaDoAdversario);
	}

	return true;
}

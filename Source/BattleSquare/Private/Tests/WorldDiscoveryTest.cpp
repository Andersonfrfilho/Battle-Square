// Copyright 2026 Anderson. All Rights Reserved.

#include "World/WorldDiscovery.h"
#include "UI/WorldMapProjection.h"
#include "Environment/WorldBoundaryWater.h"
#include "Environment/IslandGeography.h"
#include "Misc/AutomationTest.h"
#include "Misc/ConfigCacheIni.h"

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
	const float UmaRegiao = FWorldDiscovery::RegionSizeUnits();
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
	const float UmaRegiao = FWorldDiscovery::RegionSizeUnits();
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
	const float UmaRegiao = FWorldDiscovery::RegionSizeUnits();

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
	const float Longe = FWorldDiscovery::RegionSizeUnits() * 10.0f;

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
	const int32 QuantosTerrenos = static_cast<int32>(EWorldMapTerrain::Count);

	// Todo terreno tem NOME. Um sem rótulo apareceria no mapa como mancha que
	// a legenda não explica — e a legenda existe justamente para isso.
	TSet<FString> Nomes;
	for (int32 Qual = 0; Qual < QuantosTerrenos; ++Qual)
	{
		const EWorldMapTerrain Terreno = static_cast<EWorldMapTerrain>(Qual);
		const FString Nome = FWorldMapProjection::LabelForTerrain(Terreno).ToString();
		TestFalse(TEXT("Nenhum terreno fica sem nome"), Nome.IsEmpty());
		TestFalse(TEXT("E nenhum cai no rótulo de desconhecido"),
			Nome.Equals(TEXT("desconhecido")));
		Nomes.Add(Nome);
	}
	TestEqual(TEXT("Nomes não se repetem"), Nomes.Num(), QuantosTerrenos);

	// E toda COR é distinta. Dois terrenos da mesma cor são um terreno só na
	// tela, e a legenda passaria a listar uma diferença que ninguém vê.
	TSet<FString> Cores;
	for (int32 Qual = 0; Qual < QuantosTerrenos; ++Qual)
	{
		Cores.Add(FWorldMapProjection::ColorForTerrain(
			static_cast<EWorldMapTerrain>(Qual)).ToString());
	}
	TestEqual(TEXT("Cores não se repetem"), Cores.Num(), QuantosTerrenos);

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

	for (int32 Qual = 0; Qual < static_cast<int32>(EWorldMapTerrain::Count); ++Qual)
	{
		const FLinearColor Cor = FWorldMapProjection::ColorForTerrain(
			static_cast<EWorldMapTerrain>(Qual));
		TestTrue(TEXT("Todo terreno é mais surdo que o marcador de adversário"),
			(Cor.R + Cor.G + Cor.B) < ForcaDoAdversario);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMapaDistingueOsBiomasTest,
	"BattleSquare.World.Map.MapaDistingueOsBiomas",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMapaDistingueOsBiomasTest::RunTest(const FString&)
{
	// Deserto, glaciar e vulcão precisam ser MANCHAS DIFERENTES no mapa. Se
	// os três caíssem em "clareira", o mapa continuaria dizendo que a ilha
	// inteira é a mesma coisa — e foi para desfazer isso que ela ganhou
	// setores de bioma.
	const EIslandBiome Biomas[] = {
		EIslandBiome::Forest, EIslandBiome::Desert,
		EIslandBiome::Glacier, EIslandBiome::Volcano };

	TSet<int32> Terrenos;
	for (const EIslandBiome Bioma : Biomas)
	{
		const EWorldMapTerrain Terreno = FWorldMapProjection::TerrainForBiome(Bioma);
		TestNotEqual(TEXT("Nenhum bioma vira o sentinela de contagem"),
			static_cast<int32>(Terreno), static_cast<int32>(EWorldMapTerrain::Count));
		Terrenos.Add(static_cast<int32>(Terreno));
	}
	TestEqual(TEXT("Cada bioma é uma mancha própria no mapa"),
		Terrenos.Num(), static_cast<int32>(UE_ARRAY_COUNT(Biomas)));

	// A praia é MARGEM: o mapa já tinha esse nome para a terra molhada, e um
	// segundo nome para a mesma faixa seria duas cores para o mesmo lugar.
	TestEqual(TEXT("A praia é a margem que o mapa já conhecia"),
		static_cast<int32>(FWorldMapProjection::TerrainForBiome(EIslandBiome::Beach)),
		static_cast<int32>(EWorldMapTerrain::Margem));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPedacoDoMapaCabeNaRegiaoDescobertaTest,
	"BattleSquare.World.Map.PedacoCabeNaRegiaoDescoberta",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPedacoDoMapaCabeNaRegiaoDescobertaTest::RunTest(const FString&)
{
	// UMA RELAÇÃO, e não um valor — que é por isso que nenhum teste anterior a
	// pegou: o tamanho do pedaço estava certo, o tamanho da região estava
	// certo, e o par estava errado.
	//
	// O pedaço é pintado conforme a descoberta do CENTRO dele. Maior que a
	// região, ele cobre mais de uma e revela terreno nunca visto — ou esconde
	// o já visto. Com a ilha em 6000 dividia POR ACASO; a ilha virou 20000 e
	// a fronteira passou a mentir nos dois sentidos.
	// A ilha é trocada NA CONFIGURAÇÃO, e não passada por parâmetro. A região
	// lê o raio do ini; alimentar o pedaço por argumento e a região pelo ini
	// mede uma combinação que nunca acontece de verdade — e foi assim que a
	// primeira versão deste par de testes se enganou.
	const FString Secao = TEXT("/Script/BattleSquare.BattleSquareGameMode");
	const FString Chave = TEXT("WorldIslandRadiusUnits");

	float RaioOriginal = 20000.0f;
	GConfig->GetFloat(*Secao, *Chave, RaioOriginal, GGameIni);

	for (const float RaioDaIlha : { 1000.0f, 6000.0f, 20000.0f, 50000.0f, 100000.0f })
	{
		GConfig->SetFloat(*Secao, *Chave, RaioDaIlha, GGameIni);

		const float Regiao = FWorldDiscovery::RegionSizeUnits();
		const float Lado = FWorldMapProjection::TerrainTileSideUnits(RaioDaIlha);

		TestTrue(*FString::Printf(
			TEXT("Ilha %.0f: o pedaço nunca passa da região"), RaioDaIlha),
			Lado <= Regiao + KINDA_SMALL_NUMBER);

		const float Quantos = Regiao / Lado;
		TestTrue(*FString::Printf(
			TEXT("Ilha %.0f: cabe um número inteiro de pedaços na região"), RaioDaIlha),
			FMath::Abs(Quantos - FMath::RoundToFloat(Quantos)) < KINDA_SMALL_NUMBER);
	}

	GConfig->SetFloat(*Secao, *Chave, RaioOriginal, GGameIni);
	const float Regiao = FWorldDiscovery::RegionSizeUnits();

	// E o pedaço não vira poeira: mancha de duas unidades seria custo de
	// desenho que ninguém vê, que é o motivo de o tamanho seguir a ilha.
	TestTrue(TEXT("O pedaço continua grande o bastante para ser mancha"),
		FWorldMapProjection::TerrainTileSideUnits(20000.0f) >= Regiao * 0.25f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUmaFonteSoParaORaioDaTerraTest,
	"BattleSquare.World.Map.UmaFonteSoParaORaioDaTerra",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUmaFonteSoParaORaioDaTerraTest::RunTest(const FString&)
{
	// O retrato do mapa e a água do mundo precisam concordar sobre onde a
	// terra acaba. Duas fontes concordam até a primeira edição, e o sintoma
	// seria o mapa desenhando praia onde o jogador já está nadando.
	const float DaGeografia = IslandGeography::LandRadiusUnits();

	const FWorldMapSnapshot RetratoNovo;
	TestEqual(TEXT("O retrato nasce com o raio da geografia"),
		RetratoNovo.ShoreRadiusUnits, DaGeografia);

	const AWorldBoundaryWater* AguaPadrao = GetDefault<AWorldBoundaryWater>();
	TestEqual(TEXT("E a água do mundo com o mesmo"),
		AguaPadrao->ShoreRadiusUnits, DaGeografia);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FQuandoAIlhaCrescerDemaisSepararPorAreasTest,
	"BattleSquare.World.Map.QuandoAIlhaCrescerDemaisSepararPorAreas",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FQuandoAIlhaCrescerDemaisSepararPorAreasTest::RunTest(const FString&)
{
	// ESTE TESTE É UM GATILHO, não uma verificação de defeito.
	//
	// O mapa desenha um pedaço por casa da ilha inteira, e a contagem cresce
	// com o QUADRADO do raio: 6.400 pedaços a 20.000, 25.600 a 40.000, 57.600
	// a 60.000. O pedaço não pode crescer junto — ele já está preso ao tamanho
	// da região de descoberta, e passar disso faz a fronteira do que se andou
	// mentir (ver PedacoCabeNaRegiaoDescoberta).
	//
	// A saída decidida é MAPA POR ÁREA: o mapa passa a ter o tamanho do setor
	// onde o jogador está, e não do planeta. A costura já existe —
	// `IslandGeography::BiomeOfSector` já divide a ilha — então o dia da
	// separação não pede arquitetura nova, só o mapa parar de olhar o mundo
	// inteiro de uma vez.
	//
	// Isto NÃO foi construído: com a ilha em 20.000 não é preciso, e feature
	// especulativa envelhece mal. O que foi construído é o AVISO — porque um
	// plano que depende de alguém lembrar é um plano que ninguém executa, e
	// este projeto já registrou mais de uma vez o custo disso.
	//
	// Quando este teste falhar, ele não achou um defeito: ele avisou que
	// chegou a hora. Separe o mapa por área, e suba o teto junto.
	constexpr int32 TetoDePedacos = 10000;

	const float RaioDaIlha = IslandGeography::LandRadiusUnits();
	const float Lado = FWorldMapProjection::TerrainTileSideUnits(RaioDaIlha);
	const float Travessia =
		(RaioDaIlha * FWorldMapProjection::TerrainMarginFactor * 2.0f) / Lado;
	const int32 Pedacos = FMath::CeilToInt(Travessia) * FMath::CeilToInt(Travessia);

	TestTrue(FString::Printf(
		TEXT("A ilha (%.0f) pede %d pedaços de mapa, acima do teto de %d. ")
		TEXT("Não é defeito: é a hora de separar o mapa por ÁREA, mostrando o ")
		TEXT("setor onde o jogador está em vez do planeta inteiro. ")
		TEXT("IslandGeography::BiomeOfSector já divide a ilha."),
		RaioDaIlha, Pedacos, TetoDePedacos),
		Pedacos <= TetoDePedacos);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBussolaGiraComOMapaTest,
	"BattleSquare.World.Map.BussolaGiraComOMapa",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBussolaGiraComOMapaTest::RunTest(const FString&)
{
	// Um "N" fixo no topo de um mapa que GIRA é a tela mentindo sobre a
	// direção. É o mesmo tipo de erro de eixo que já custou uma investigação
	// inteira aqui, e é caro justamente por parecer certo de relance.
	FWorldMapSnapshot Retrato;

	// NORTE ACIMA: a promessa está no nome, e ela vale para qualquer olhar.
	for (const float Yaw : { 0.0f, 90.0f, 180.0f, 270.0f })
	{
		Retrato.PlayerYawDegrees = Yaw;
		TestEqual(TEXT("Com norte acima, o norte fica no topo"),
			FWorldMapProjection::NorthAngleDegrees(
				Retrato, FWorldMapProjection::EMode::NorteAcima), 0.0f);
	}

	// SEGUINDO O OLHAR: olhando para o norte, o norte está em cima; virando
	// para o leste, o norte vai para a ESQUERDA da tela — porque o mapa girou
	// no sentido contrário para pôr a frente em cima.
	Retrato.PlayerYawDegrees = 0.0f;
	TestEqual(TEXT("Olhando para o norte, o N fica no topo"),
		FWorldMapProjection::NorthAngleDegrees(
			Retrato, FWorldMapProjection::EMode::SeguindoOOlhar), 0.0f);

	Retrato.PlayerYawDegrees = 90.0f;
	TestEqual(TEXT("Virado para o leste, o N vai para a esquerda"),
		FWorldMapProjection::NorthAngleDegrees(
			Retrato, FWorldMapProjection::EMode::SeguindoOOlhar), 270.0f);

	Retrato.PlayerYawDegrees = 180.0f;
	TestEqual(TEXT("De costas para o norte, o N fica embaixo"),
		FWorldMapProjection::NorthAngleDegrees(
			Retrato, FWorldMapProjection::EMode::SeguindoOOlhar), 180.0f);

	// E a bússola concorda com o MAPA: o norte que a letra marca é a mesma
	// direção em que uma coisa ao norte é desenhada. Duas contas separadas
	// para a mesma direção se desencontram na primeira edição.
	Retrato.PlayerYawDegrees = 90.0f;
	Retrato.PlayerXY = FVector2D::ZeroVector;
	const FVector2D AoNorte(1000.0f, 0.0f);
	const FVector2D NoMapa = FWorldMapProjection::ToMapSpace(
		AoNorte, Retrato, FWorldMapProjection::EMode::SeguindoOOlhar, 3000.0f);

	const float LetraEmGraus = FWorldMapProjection::NorthAngleDegrees(
		Retrato, FWorldMapProjection::EMode::SeguindoOOlhar);
	const float Radianos = FMath::DegreesToRadians(LetraEmGraus);
	const FVector2D DirecaoDaLetra(FMath::Sin(Radianos), -FMath::Cos(Radianos));

	const FVector2D DirecaoNoMapa = NoMapa.GetSafeNormal();
	TestTrue(TEXT("A letra N aponta para onde o mapa desenha o norte"),
		FVector2D::DotProduct(DirecaoDaLetra, DirecaoNoMapa) > 0.95f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARegiaoAcompanhaAIlhaTest,
	"BattleSquare.World.Discovery.ARegiaoAcompanhaAIlha",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARegiaoAcompanhaAIlhaTest::RunTest(const FString&)
{
	// 800 unidades foi escolhido quando a ilha tinha 200 metros. Numa ilha de
	// 1 km o mapa desenharia 160 mil pedaços — e a causa é a mesma dos anéis
	// das peças: número absoluto de quando só existia um tamanho.
	//
	// O que precisa ficar constante é a CONTAGEM: é ela que decide se o mapa
	// desenha ou engasga.
	const FString Secao = TEXT("/Script/BattleSquare.BattleSquareGameMode");
	const FString Chave = TEXT("WorldIslandRadiusUnits");

	float RaioDeHoje = 20000.0f;
	GConfig->GetFloat(*Secao, *Chave, RaioDeHoje, GGameIni);

	// A ilha é trocada NA CONFIGURAÇÃO, e não passada por parâmetro: o
	// tamanho do pedaço recebe o raio como argumento mas a região o lê do ini,
	// e alimentar os dois de fontes diferentes mede uma combinação que nunca
	// acontece de verdade. Foi assim que este teste falhou na primeira versão.
	auto PedacosComIlhaDe = [&Secao, &Chave](float RaioDaIlha)
	{
		GConfig->SetFloat(*Secao, *Chave, RaioDaIlha, GGameIni);

		const float Lado = FWorldMapProjection::TerrainTileSideUnits(RaioDaIlha);
		const float Travessia =
			(RaioDaIlha * FWorldMapProjection::TerrainMarginFactor * 2.0f) / Lado;
		const int32 PorLado = FMath::CeilToInt(Travessia);
		return PorLado * PorLado;
	};

	const int32 Agora = PedacosComIlhaDe(RaioDeHoje);
	const int32 CincoVezes = PedacosComIlhaDe(RaioDeHoje * 5.0f);

	// Volta ao raio de verdade antes de qualquer asserção: teste que deixa
	// configuração suja contamina todos os que rodarem depois dele.
	GConfig->SetFloat(*Secao, *Chave, RaioDeHoje, GGameIni);

	TestTrue(*FString::Printf(TEXT("O mapa de hoje cabe no teto (%d)"), Agora),
		Agora <= 10000);
	TestTrue(*FString::Printf(
		TEXT("Ilha cinco vezes maior não estoura o mapa (%d)"), CincoVezes),
		CincoVezes <= 10000);
	TestTrue(*FString::Printf(TEXT("A contagem fica igual (%d contra %d)"),
		Agora, CincoVezes), FMath::Abs(CincoVezes - Agora) <= Agora / 10);

	// E o pedaço continua cabendo na região — a relação que faz a borda da
	// descoberta ser a borda que se vê.
	const float Regiao = FWorldDiscovery::RegionSizeUnits();
	TestTrue(TEXT("A região é fração do raio, não número"),
		Regiao > 1.0f && Regiao < RaioDeHoje);
	TestTrue(TEXT("O pedaço cabe na região"),
		FWorldMapProjection::TerrainTileSideUnits(RaioDeHoje) <= Regiao + KINDA_SMALL_NUMBER);

	return true;
}

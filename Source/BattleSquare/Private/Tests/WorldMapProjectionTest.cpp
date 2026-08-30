// Copyright 2026 Anderson. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "UI/WorldMapProjection.h"

namespace
{
	FWorldMapSnapshot MapaComJogadorEm(const FVector2D& Onde, float Yaw)
	{
		FWorldMapSnapshot Retrato;
		Retrato.PlayerXY = Onde;
		Retrato.PlayerYawDegrees = Yaw;
		return Retrato;
	}

	constexpr float Alcance = 1000.0f;
}

// OS QUATRO PONTOS CARDEAIS, com o norte acima.
//
// É o teste que este projeto aprendeu a escrever doendo: "Baixo" andava para a
// direita porque coluna virou X e linha virou Y, e a descoberta custou o
// usuário jogar e descrever o que viu. Erro de eixo não se vê lendo código —
// as duas versões parecem igualmente razoáveis.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMapNorthUpPutsNorthUpTest,
	"BattleSquare.UI.WorldMap.NorthUpPutsNorthUp",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMapNorthUpPutsNorthUpTest::RunTest(const FString& Parameters)
{
	const FWorldMapSnapshot Retrato = MapaComJogadorEm(FVector2D::ZeroVector, /*Yaw=*/0.0f);
	const auto Modo = FWorldMapProjection::EMode::NorteAcima;

	// +X é o NORTE. Na tela ele sobe, e o Y da tela cresce para baixo.
	const FVector2D Norte = FWorldMapProjection::ToMapSpace(
		FVector2D(500.0f, 0.0f), Retrato, Modo, Alcance);
	TestTrue(TEXT("Norte fica ACIMA do centro"), Norte.Y < 0.0f);
	TestTrue(TEXT("E não desloca para os lados"), FMath::IsNearlyZero(Norte.X, 0.001f));

	// +Y é o LESTE, e vai à direita.
	const FVector2D Leste = FWorldMapProjection::ToMapSpace(
		FVector2D(0.0f, 500.0f), Retrato, Modo, Alcance);
	TestTrue(TEXT("Leste fica à DIREITA"), Leste.X > 0.0f);
	TestTrue(TEXT("E não sobe nem desce"), FMath::IsNearlyZero(Leste.Y, 0.001f));

	const FVector2D Sul = FWorldMapProjection::ToMapSpace(
		FVector2D(-500.0f, 0.0f), Retrato, Modo, Alcance);
	TestTrue(TEXT("Sul fica ABAIXO"), Sul.Y > 0.0f);

	const FVector2D Oeste = FWorldMapProjection::ToMapSpace(
		FVector2D(0.0f, -500.0f), Retrato, Modo, Alcance);
	TestTrue(TEXT("Oeste fica à ESQUERDA"), Oeste.X < 0.0f);

	// E a escala: metade do alcance é meio caminho até a borda.
	TestTrue(TEXT("500 de 1000 dá meio raio"),
		FMath::IsNearlyEqual(FMath::Abs(Norte.Y), 0.5f, 0.001f));

	return true;
}

// O NORTE ACIMA não gira quando o jogador gira — é essa estabilidade que
// permite memorizar o mapa.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNorthUpIgnoresWhereYouLookTest,
	"BattleSquare.UI.WorldMap.NorthUpIgnoresWhereYouLook",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FNorthUpIgnoresWhereYouLookTest::RunTest(const FString& Parameters)
{
	const auto Modo = FWorldMapProjection::EMode::NorteAcima;
	const FVector2D Alvo(500.0f, 0.0f);

	const FVector2D Olhando0 = FWorldMapProjection::ToMapSpace(
		Alvo, MapaComJogadorEm(FVector2D::ZeroVector, 0.0f), Modo, Alcance);
	const FVector2D Olhando90 = FWorldMapProjection::ToMapSpace(
		Alvo, MapaComJogadorEm(FVector2D::ZeroVector, 90.0f), Modo, Alcance);

	TestTrue(TEXT("O ponto não se move quando o jogador vira"),
		Olhando0.Equals(Olhando90, 0.001f));

	// E a SETA gira, porque é ela que mostra para onde ele olha.
	TestEqual(TEXT("A seta acompanha o olhar"),
		FWorldMapProjection::PlayerArrowAngleDegrees(
			MapaComJogadorEm(FVector2D::ZeroVector, 90.0f), Modo), 90.0f);

	return true;
}

// SEGUINDO O OLHAR: o que está à FRENTE fica em cima, sempre.
//
// É o que faz o minimapa casar com a tela enquanto se anda. Girar o mundo no
// mesmo sentido do olhar — o erro fácil — poria o que está ATRÁS em cima: o
// mapa ficaria consistente consigo mesmo e errado contra o jogo.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FHeadingUpPutsWhatIsAheadOnTopTest,
	"BattleSquare.UI.WorldMap.HeadingUpPutsWhatIsAheadOnTop",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FHeadingUpPutsWhatIsAheadOnTopTest::RunTest(const FString& Parameters)
{
	const auto Modo = FWorldMapProjection::EMode::SeguindoOOlhar;

	// Olhando para o norte (yaw 0), o que está ao norte fica em cima.
	const FVector2D AoNorte = FWorldMapProjection::ToMapSpace(
		FVector2D(500.0f, 0.0f), MapaComJogadorEm(FVector2D::ZeroVector, 0.0f), Modo, Alcance);
	TestTrue(TEXT("Olhando ao norte, o norte fica em cima"), AoNorte.Y < 0.0f);

	// Olhando para o LESTE (yaw 90), o que está a LESTE passa a ficar em cima.
	const FVector2D AoLeste = FWorldMapProjection::ToMapSpace(
		FVector2D(0.0f, 500.0f), MapaComJogadorEm(FVector2D::ZeroVector, 90.0f), Modo, Alcance);
	TestTrue(TEXT("Olhando ao leste, o leste fica em cima"), AoLeste.Y < 0.0f);
	TestTrue(TEXT("E centrado, não de lado"), FMath::IsNearlyZero(AoLeste.X, 0.001f));

	// E o que está ATRÁS fica embaixo — a metade que prova que o giro tem o
	// sinal certo. Sem ela, girar ao contrário passaria neste teste.
	const FVector2D AtrasDele = FWorldMapProjection::ToMapSpace(
		FVector2D(0.0f, -500.0f), MapaComJogadorEm(FVector2D::ZeroVector, 90.0f), Modo, Alcance);
	TestTrue(TEXT("Olhando ao leste, o oeste fica embaixo"), AtrasDele.Y > 0.0f);

	// A SETA não gira neste modo: quem gira é o mundo em volta dela. Somar os
	// dois giros faria a seta mentir.
	TestEqual(TEXT("A seta fica parada quando o mundo gira"),
		FWorldMapProjection::PlayerArrowAngleDegrees(
			MapaComJogadorEm(FVector2D::ZeroVector, 137.0f), Modo), 0.0f);

	return true;
}

// A posição do JOGADOR é sempre o centro, em qualquer modo e em qualquer canto
// do mundo — o mapa é relativo a ele, não ao zero do mundo.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPlayerIsAlwaysAtTheCentreTest,
	"BattleSquare.UI.WorldMap.PlayerIsAlwaysAtTheCentre",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPlayerIsAlwaysAtTheCentreTest::RunTest(const FString& Parameters)
{
	const FVector2D LongeDoZero(-4200.0f, 3100.0f);
	const FWorldMapSnapshot Retrato = MapaComJogadorEm(LongeDoZero, /*Yaw=*/47.0f);

	for (const auto Modo : { FWorldMapProjection::EMode::NorteAcima,
		FWorldMapProjection::EMode::SeguindoOOlhar })
	{
		const FVector2D NoCentro =
			FWorldMapProjection::ToMapSpace(LongeDoZero, Retrato, Modo, Alcance);
		TestTrue(TEXT("O jogador fica no centro do mapa"),
			NoCentro.IsNearlyZero(0.001f));
	}

	return true;
}

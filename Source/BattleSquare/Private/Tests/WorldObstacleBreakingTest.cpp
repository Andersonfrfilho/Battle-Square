// Copyright 2026 Anderson. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "World/WorldObstacleBreaking.h"

namespace
{
	FWorldObstacleCandidate Obstaculo(const FVector& Onde, EScenaryRole Papel, int32 Vida = 30)
	{
		FWorldObstacleCandidate Candidato;
		Candidato.Location = Onde;
		Candidato.Role = Papel;
		Candidato.RemainingHealth = Vida;
		return Candidato;
	}

	const FVector ParaONorte(1.0f, 0.0f, 0.0f);
}

// Só o que É obstáculo cai. Capim e flor não são: derrubá-los seria esforço
// sem consequência, e faria o jogador achar o golpe fraco quando o que houve
// foi ele acertar o alvo errado.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FOnlyRealObstaclesCanFallTest,
	"BattleSquare.World.Obstacle.OnlyRealObstaclesCanFall",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FOnlyRealObstaclesCanFallTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Pedra tem vida"),
		FWorldObstacleBreaking::StartingHealthFor(EScenaryRole::Rock) > 0);
	TestTrue(TEXT("Árvore tem vida"),
		FWorldObstacleBreaking::StartingHealthFor(EScenaryRole::ForestTree) > 0);
	TestTrue(TEXT("Tronco caído tem vida"),
		FWorldObstacleBreaking::StartingHealthFor(EScenaryRole::DeadWood) > 0);

	TestEqual(TEXT("Capim NÃO é obstáculo"),
		FWorldObstacleBreaking::StartingHealthFor(EScenaryRole::GroundCover), 0);
	TestEqual(TEXT("Flor NÃO é obstáculo"),
		FWorldObstacleBreaking::StartingHealthFor(EScenaryRole::Accent), 0);
	TestEqual(TEXT("Arbusto NÃO é obstáculo"),
		FWorldObstacleBreaking::StartingHealthFor(EScenaryRole::Undergrowth), 0);

	// Pedra aguenta mais que tronco caído: a dureza é a informação, e ela
	// precisa se sentir sem ninguém ler tabela nenhuma.
	TestTrue(TEXT("Pedra é mais dura que tronco"),
		FWorldObstacleBreaking::StartingHealthFor(EScenaryRole::Rock)
			> FWorldObstacleBreaking::StartingHealthFor(EScenaryRole::DeadWood));

	return true;
}

// O pet FRACO derruba, devagar. Progresso lento ensina; progresso ZERO parece
// defeito — e o jogador desiste achando que o golpe não funciona.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEvenAWeakPetMakesProgressTest,
	"BattleSquare.World.Obstacle.EvenAWeakPetMakesProgress",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FEvenAWeakPetMakesProgressTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Musculatura zero ainda tira dano"),
		FWorldObstacleBreaking::DamageFromMusculature(0) > 0);
	TestTrue(TEXT("Musculatura negativa também"),
		FWorldObstacleBreaking::DamageFromMusculature(-50) > 0);

	// E treinar SE SENTE: mais musculatura tira mais.
	TestTrue(TEXT("O treinado bate mais forte que o cru"),
		FWorldObstacleBreaking::DamageFromMusculature(40)
			> FWorldObstacleBreaking::DamageFromMusculature(0));

	// Um tronco cai em poucos golpes mesmo sem treino: se levasse trinta, o
	// jogador nunca descobriria que dá para derrubar.
	const int32 Golpes =
		FWorldObstacleBreaking::StartingHealthFor(EScenaryRole::DeadWood)
			/ FWorldObstacleBreaking::DamageFromMusculature(0);
	TestTrue(*FString::Printf(TEXT("Tronco cai em %d golpes, sem treino"), Golpes),
		Golpes <= 6);

	return true;
}

// O alvo é o que está À FRENTE e PERTO. Atrás não conta, longe não conta.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTargetIsAheadAndWithinReachTest,
	"BattleSquare.World.Obstacle.TargetIsAheadAndWithinReach",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTargetIsAheadAndWithinReachTest::RunTest(const FString& Parameters)
{
	const float Alcance = FWorldObstacleBreaking::ReachUnits;

	// À frente e perto: acerta.
	TestEqual(TEXT("Árvore à frente é o alvo"),
		FWorldObstacleBreaking::FindTarget(FVector::ZeroVector, ParaONorte,
			{ Obstaculo(FVector(Alcance * 0.5f, 0.0f, 0.0f), EScenaryRole::ForestTree) }), 0);

	// Atrás: não.
	TestEqual(TEXT("Árvore ATRÁS não é alvo"),
		FWorldObstacleBreaking::FindTarget(FVector::ZeroVector, ParaONorte,
			{ Obstaculo(FVector(-Alcance * 0.5f, 0.0f, 0.0f), EScenaryRole::ForestTree) }),
		INDEX_NONE);

	// Fora do alcance: não.
	TestEqual(TEXT("Árvore longe não é alvo"),
		FWorldObstacleBreaking::FindTarget(FVector::ZeroVector, ParaONorte,
			{ Obstaculo(FVector(Alcance * 2.0f, 0.0f, 0.0f), EScenaryRole::ForestTree) }),
		INDEX_NONE);

	// JÁ CAÍDA não é alvo: continuar batendo no toco daria resposta de acerto
	// e nenhuma mudança, e o jogador conclui que o golpe parou de funcionar.
	TestEqual(TEXT("Árvore já derrubada não é alvo"),
		FWorldObstacleBreaking::FindTarget(FVector::ZeroVector, ParaONorte,
			{ Obstaculo(FVector(50.0f, 0.0f, 0.0f), EScenaryRole::ForestTree, /*Vida=*/0) }),
		INDEX_NONE);

	// A ALTURA não conta: a copa fica muito acima, e medir em três dimensões
	// tiraria do alcance justamente a árvore mais alta.
	TestEqual(TEXT("Altura não tira a árvore do alcance"),
		FWorldObstacleBreaking::FindTarget(FVector::ZeroVector, ParaONorte,
			{ Obstaculo(FVector(80.0f, 0.0f, 900.0f), EScenaryRole::ForestTree) }), 0);

	return true;
}

// Entre dois no arco, ganha o MAIS PERTO — e não o mais alinhado.
//
// O jogador mira com o corpo, e a árvore encostada nele é a que ele espera
// acertar. Escolher a mais alinhada faria o golpe atravessar a que está na
// cara dele para acertar outra lá atrás.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNearestInTheArcWinsTest,
	"BattleSquare.World.Obstacle.NearestInTheArcWins",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FNearestInTheArcWinsTest::RunTest(const FString& Parameters)
{
	const TArray<FWorldObstacleCandidate> Dois = {
		// Perfeitamente alinhada, porém LONGE.
		Obstaculo(FVector(250.0f, 0.0f, 0.0f), EScenaryRole::ForestTree),
		// De lado, dentro do arco, e PERTO.
		Obstaculo(FVector(60.0f, 45.0f, 0.0f), EScenaryRole::Rock),
	};

	TestEqual(TEXT("Ganha a mais perto, não a mais alinhada"),
		FWorldObstacleBreaking::FindTarget(FVector::ZeroVector, ParaONorte, Dois), 1);

	return true;
}

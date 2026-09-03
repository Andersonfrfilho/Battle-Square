// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/AutoBattleResolver.h"
#include "Misc/AutomationTest.h"

namespace DefesaAutomaticaTeste
{
	// Nome próprio (L-042): helper homônimo em outro arquivo de teste vira
	// sobrecarga ambígua quando o unity build junta os dois.
	FBattleState DueloParaADefesaAutomatica(int32 VidaEsquerda, int32 VidaDireita)
	{
		FBattleState Estado;
		Estado.Random.State = 77;

		FPetState Esquerda;
		Esquerda.PetId = 1;
		Esquerda.Side = 0;
		Esquerda.Health = VidaEsquerda;
		Esquerda.MaxHealth = VidaEsquerda;
		Esquerda.Attack = 60;
		Esquerda.Defense = 10;
		Esquerda.Speed = 10;

		// GOLPES COM PODER, senão ninguém machuca ninguém e todo duelo morre
		// no teto — foi exatamente assim que a primeira versão deste teste
		// falhou: empate eterno lido como "a semente não decide nada".
		Esquerda.MovePowers[0] = 55;
		Esquerda.MovePowers[1] = 40;
		Esquerda.MovePowers[2] = 40;
		Esquerda.MovePowers[3] = 30;
		Estado.Pets.Add(Esquerda);

		FPetState Direita = Esquerda;
		Direita.PetId = 2;
		Direita.Side = 1;
		Direita.Health = VidaDireita;
		Direita.MaxHealth = VidaDireita;
		Estado.Pets.Add(Direita);

		Estado.PlaceDuelistsAtStartingCells();
		return Estado;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAutoBattleIsDeterministicAndEndsTest,
	"BattleSquare.Battle.DefesaAutomatica.DeterministicaEComFim",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAutoBattleIsDeterministicAndEndsTest::RunTest(const FString&)
{
	// A defesa que aconteceu longe dos olhos tem de ser a MESMA que
	// aconteceria de novo: mesmo estado e mesma semente, mesmo vencedor —
	// senão "automático" vira "sorte de quem reclama".
	const uint8 Primeira = AutoBattleResolver::ResolveBotVsBot(
		DefesaAutomaticaTeste::DueloParaADefesaAutomatica(200, 200), 42);
	const uint8 Segunda = AutoBattleResolver::ResolveBotVsBot(
		DefesaAutomaticaTeste::DueloParaADefesaAutomatica(200, 200), 42);

	TestEqual(TEXT("mesma semente, mesmo desfecho"), Primeira, Segunda);

	// E o desfecho é um dos três que existem — nunca um valor inventado.
	TestTrue(TEXT("o desfecho e lado 0, lado 1 ou empate"),
		Primeira == 0 || Primeira == 1 || Primeira == 0xFF);

	// Sementes diferentes PODEM divergir — é o que separa semente de
	// constante. Afirmado na forma existencial: entre várias, alguma difere.
	bool bAlgumaDiverge = false;
	for (uint64 Semente = 1; Semente <= 16 && !bAlgumaDiverge; ++Semente)
	{
		bAlgumaDiverge = AutoBattleResolver::ResolveBotVsBot(
			DefesaAutomaticaTeste::DueloParaADefesaAutomatica(200, 200), Semente)
			!= Primeira;
	}
	TestTrue(TEXT("a semente decide algo"), bAlgumaDiverge);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAutoBattleFavorsTheStrongerTest,
	"BattleSquare.Battle.DefesaAutomatica.OMaisForteVenceMais",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAutoBattleFavorsTheStrongerTest::RunTest(const FString&)
{
	// O automático é o SIM jogando, não um dado: um lado com dez vezes a vida
	// do outro tem de vencer a maioria das defesas. Sementes fixas — o teste
	// mede a simulação, não a sorte.
	int32 VitoriasDoForte = 0;
	constexpr int32 Rodadas = 12;

	for (uint64 Semente = 1; Semente <= Rodadas; ++Semente)
	{
		if (AutoBattleResolver::ResolveBotVsBot(
			DefesaAutomaticaTeste::DueloParaADefesaAutomatica(2000, 200), Semente) == 0)
		{
			++VitoriasDoForte;
		}
	}

	TestTrue(TEXT("o lado dez vezes mais vivo vence a maioria"),
		VitoriasDoForte > Rodadas / 2);

	return true;
}

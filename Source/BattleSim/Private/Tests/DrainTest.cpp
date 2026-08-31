// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattlePhases.h"
#include "Battle/BattleState.h"
#include "Battle/BattleEvent.h"
#include "Misc/AutomationTest.h"

namespace DrenoTeste
{
	FBattleState Duelo(int32 DrenoDoAtacante, int32 VidaDoAtacante)
	{
		FBattleState Estado;
		Estado.Random.State = 7;

		FPetState Atacante;
		Atacante.PetId = 1; Atacante.Side = 0; Atacante.Column = 0; Atacante.Row = 1;
		Atacante.Health = VidaDoAtacante; Atacante.MaxHealth = 100; Atacante.Attack = 40;
		Atacante.MovePowers[0] = 80;
		Atacante.MoveDrainPercents[0] = static_cast<uint8>(DrenoDoAtacante);

		FPetState Alvo;
		Alvo.PetId = 2; Alvo.Side = 1; Alvo.Column = 1; Alvo.Row = 1;
		Alvo.Health = 100; Alvo.MaxHealth = 100; Alvo.Defense = 10;

		Estado.Pets.Add(Atacante);
		Estado.Pets.Add(Alvo);
		return Estado;
	}

	FBattleAction Golpe()
	{
		FBattleAction Acao = MakeMoveAction(EActionType::Atacar, 0);
		Acao.Direction = EBattleDirection::Direita;
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDrenarDevolveVidaTest,
	"BattleSim.Moves.Drenar.DevolveVida",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDrenarDevolveVidaTest::RunTest(const FString&)
{
	FBattleState Estado = DrenoTeste::Duelo(/*Dreno=*/50, /*Vida=*/40);
	FBattleAction Nada;
	TArray<FBattleEvent> Traco;

	BattlePhases::ApplyCombat(Estado, DrenoTeste::Golpe(), Nada, 0, Traco);

	const int32 Dano = Estado.Pets[1].PendingDamage;
	TestTrue(TEXT("O golpe causou dano"), Dano > 0);

	// Sai do dano REAL, e não do poder do golpe: drenar de um alvo que
	// defendeu tem de render menos, senão a defesa dele viraria vantagem para
	// quem bate.
	TestEqual(TEXT("Metade do dano vira vida a recuperar"),
		Estado.Pets[0].PendingHeal, Dano / 2);
	TestEqual(TEXT("E foi narrado"),
		DrenoTeste::Conta(Traco, EBattleEventType::VidaDrenada), 1);

	// ACUMULA, não aplica — F5 resolve.
	TestEqual(TEXT("A vida só muda no encerramento"), Estado.Pets[0].Health, 40);

	TArray<FBattleEvent> Fim;
	BattlePhases::ApplyResolution(Estado, 0, Fim);
	TestEqual(TEXT("Aí sim ele curou"), Estado.Pets[0].Health, 40 + Dano / 2);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGolpeSemDrenoNaoCuraTest,
	"BattleSim.Moves.Drenar.GolpeSemDrenoNaoCura",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGolpeSemDrenoNaoCuraTest::RunTest(const FString&)
{
	// ZERO é o golpe de sempre, e é o valor de todo golpe já assinado. Fosse
	// zero "drena tudo", esta feature curaria o jogo inteiro de uma vez.
	FBattleState Estado = DrenoTeste::Duelo(/*Dreno=*/0, /*Vida=*/40);
	FBattleAction Nada;
	TArray<FBattleEvent> Traco;

	BattlePhases::ApplyCombat(Estado, DrenoTeste::Golpe(), Nada, 0, Traco);
	BattlePhases::ApplyResolution(Estado, 0, Traco);

	TestEqual(TEXT("Sem dreno, a vida não sobe"), Estado.Pets[0].Health, 40);
	TestEqual(TEXT("E nada foi narrado"),
		DrenoTeste::Conta(Traco, EBattleEventType::VidaDrenada), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDrenoNaoPassaDaVidaCheiaTest,
	"BattleSim.Moves.Drenar.NaoPassaDaVidaCheia",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDrenoNaoPassaDaVidaCheiaTest::RunTest(const FString&)
{
	// Sem teto, drenar viraria uma barra que cresce sem fim, e o pet mais
	// forte do jogo seria simplesmente o que bate mais vezes.
	FBattleState Estado = DrenoTeste::Duelo(/*Dreno=*/100, /*Vida=*/98);
	FBattleAction Nada;
	TArray<FBattleEvent> Traco;

	BattlePhases::ApplyCombat(Estado, DrenoTeste::Golpe(), Nada, 0, Traco);
	BattlePhases::ApplyResolution(Estado, 0, Traco);

	TestEqual(TEXT("A vida para no máximo"), Estado.Pets[0].Health, 100);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDrenoESimultaneoAoDanoTest,
	"BattleSim.Moves.Drenar.ESimultaneoAoDano",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDrenoESimultaneoAoDanoTest::RunTest(const FString&)
{
	// A pergunta que decide o desenho: quem drena o golpe que o mataria
	// sobrevive? Sim — as duas coisas acontecem no MESMO instante, como o
	// dano mútuo (BTL-07). Aplicar em passos separados faria a ordem da
	// resolução decidir quem vive, que é exatamente o que F5 existe para
	// impedir.
	FBattleState Estado = DrenoTeste::Duelo(/*Dreno=*/100, /*Vida=*/10);
	FBattleAction Nada;
	TArray<FBattleEvent> Traco;

	BattlePhases::ApplyCombat(Estado, DrenoTeste::Golpe(), Nada, 0, Traco);

	const int32 Curado = Estado.Pets[0].PendingHeal;
	TestTrue(TEXT("Ele vai curar alguma coisa"), Curado > 0);

	// Um golpe do outro lado que levaria exatamente a vida dele.
	Estado.Pets[0].PendingDamage = 10;

	TArray<FBattleEvent> Fim;
	BattlePhases::ApplyResolution(Estado, 0, Fim);

	TestTrue(TEXT("Drenar salvou quem morreria"), Estado.Pets[0].IsAlive());
	TestEqual(TEXT("E ninguém morreu neste slot"),
		DrenoTeste::Conta(Fim, EBattleEventType::PetMorreu), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDrenoEntraNoHashTest,
	"BattleSim.Moves.Drenar.EntraNoHash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDrenoEntraNoHashTest::RunTest(const FString&)
{
	// Dreno decide resultado, então tem de entrar na assinatura: dois pets com
	// o mesmo golpe e drenos diferentes são duas partidas diferentes.
	TestNotEqual(TEXT("Drenos diferentes, hashes diferentes"),
		DrenoTeste::Duelo(/*Dreno=*/0, /*Vida=*/40).ComputeHash(),
		DrenoTeste::Duelo(/*Dreno=*/50, /*Vida=*/40).ComputeHash());

	return true;
}

// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/AutoBattleResolver.h"
#include "Battle/BattleRandom.h"
#include "Battle/DumbOpponentAI.h"
#include "Meta/PlayStyleRules.h"
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStyledBotSimulatesThePlayerTest,
	"BattleSquare.Battle.DefesaAutomatica.AIASimulaOJogador",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStyledBotSimulatesThePlayerTest::RunTest(const FString&)
{
	// "A I.A. batalha por você SIMULANDO VOCÊ, baseando nos seus movimentos"
	// (decisão 15-d): um perfil carregado de Atacar tem de gerar commits
	// carregados de Atacar. Semente fixa — mede-se a roleta, não a sorte.
	const FBattleState Estado =
		DefesaAutomaticaTeste::DueloParaADefesaAutomatica(200, 200);

	TArray<int32> EstiloAgressivo;
	EstiloAgressivo.SetNumZeroed(static_cast<int32>(EActionType::Atacar) + 1);
	EstiloAgressivo[static_cast<int32>(EActionType::Atacar)] = 200;

	FBattleRandom Random;
	Random.State = 9;

	int32 Ataques = 0;
	int32 Total = 0;
	for (int32 Rodada = 0; Rodada < 40; ++Rodada)
	{
		const FTurnCommit Commit = FDumbOpponentAI::GenerateStyledCommit(
			Estado, 0, Random, EstiloAgressivo, TArray<FStyleTransition>());
		for (const FBattleAction& Acao : Commit.Actions)
		{
			++Total;
			Ataques += Acao.Type == EActionType::Atacar ? 1 : 0;
		}
	}

	// Com peso 200 contra 1 dos outros cinco, atacar tem de DOMINAR — e não
	// pode ser TUDO: a suavização (+1) garante que estilo é viés, não
	// mordaça, e o pet encurralado ainda degrada para Aguardar.
	TestTrue(TEXT("o agressivo ataca na maioria dos slots"), Ataques > Total / 2);

	// E o histórico VAZIO joga: estilo nenhum é um estilo, não um erro.
	const FTurnCommit SemHistorico = FDumbOpponentAI::GenerateStyledCommit(
		Estado, 0, Random, TArray<int32>(), TArray<FStyleTransition>());
	TestTrue(TEXT("sem historico, o commit sai valido"),
		SemHistorico.Actions[0].Type == EActionType::Aguardar
			|| SemHistorico.Actions[0].Type == EActionType::Mover
			|| SemHistorico.Actions[0].Type == EActionType::Atacar
			|| SemHistorico.Actions[0].Type == EActionType::Magia
			|| SemHistorico.Actions[0].Type == EActionType::Defender
			|| SemHistorico.Actions[0].Type == EActionType::Esquivar);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStyledBotFollowsSequencesTest,
	"BattleSquare.Battle.DefesaAutomatica.AIASegueASequencia",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStyledBotFollowsSequencesTest::RunTest(const FString&)
{
	// OS PADRÕES DE SEQUÊNCIA (decisão 15-d): "quem abre defendendo e fecha
	// atacando" tem de ser simulado ASSIM — a abertura pesada em Defender, e
	// depois de Defender, Atacar. Tendências uniformes de propósito: o que se
	// mede aqui é SÓ a sequência.
	const FBattleState Estado =
		DefesaAutomaticaTeste::DueloParaADefesaAutomatica(200, 200);

	// Peso 31 contra 1, e não 301: com 301 a chance de TODAS as 60 aberturas
	// defenderem era ~37%, e a asserção "alguma não defende" media a sorte —
	// a primeira rodada deste teste reprovou exatamente assim. Com 31, a
	// maioria continua clara (86%) e o resto aparece quase sempre.
	TArray<FStyleTransition> Sequencias;
	for (int32 Volta = 0; Volta < 30; ++Volta)
	{
		PlayStyleRules::AddTransition(Sequencias,
			PlayStyleRules::StartOfTurn, static_cast<uint8>(EActionType::Defender));
		PlayStyleRules::AddTransition(Sequencias,
			static_cast<uint8>(EActionType::Defender),
			static_cast<uint8>(EActionType::Atacar));
	}

	FBattleRandom Random;
	Random.State = 21;

	int32 AbreDefendendo = 0;
	int32 DepoisDeDefenderAtaca = 0;
	int32 VezesQueDefendeuAbrindo = 0;
	constexpr int32 Rodadas = 60;

	for (int32 Rodada = 0; Rodada < Rodadas; ++Rodada)
	{
		const FTurnCommit Commit = FDumbOpponentAI::GenerateStyledCommit(
			Estado, 0, Random, TArray<int32>(), Sequencias);

		if (Commit.Actions[0].Type == EActionType::Defender)
		{
			++AbreDefendendo;
			++VezesQueDefendeuAbrindo;
			if (Commit.Actions[1].Type == EActionType::Atacar)
			{
				++DepoisDeDefenderAtaca;
			}
		}
	}

	// A abertura domina, e o elo também — maiorias, nunca 100%: a suavização
	// mantém o resto vivo, e estilo é viés, não roteiro.
	TestTrue(TEXT("abre defendendo na maioria"), AbreDefendendo > Rodadas / 2);
	TestTrue(TEXT("e ha aberturas que NAO defendem — vies, nao roteiro"),
		AbreDefendendo < Rodadas);
	TestTrue(TEXT("depois de defender, ataca na maioria"),
		VezesQueDefendeuAbrindo > 0
			&& DepoisDeDefenderAtaca > VezesQueDefendeuAbrindo / 2);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayStyleTransitionsCountTest,
	"BattleSquare.Battle.DefesaAutomatica.ASequenciaSeConta",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPlayStyleTransitionsCountTest::RunTest(const FString&)
{
	// A conta pura da sequência: soma acumula no par certo, e o par nunca
	// visto responde zero — nunca inventa.
	TArray<FStyleTransition> Sequencias;

	PlayStyleRules::AddTransition(Sequencias, 1, 2);
	PlayStyleRules::AddTransition(Sequencias, 1, 2);
	PlayStyleRules::AddTransition(Sequencias, 2, 1);

	TestEqual(TEXT("o par somado acumula"),
		PlayStyleRules::TransitionCount(Sequencias, 1, 2), 2);
	TestEqual(TEXT("o par inverso e OUTRO par"),
		PlayStyleRules::TransitionCount(Sequencias, 2, 1), 1);
	TestEqual(TEXT("o par nunca visto e zero"),
		PlayStyleRules::TransitionCount(Sequencias, 3, 4), 0);
	TestEqual(TEXT("e a lista e esparsa: tres somas, dois pares"),
		Sequencias.Num(), 2);

	return true;
}

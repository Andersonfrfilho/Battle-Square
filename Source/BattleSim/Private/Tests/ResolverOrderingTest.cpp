// Copyright 2026 Anderson. All Rights Reserved.

#include "Algo/Reverse.h"
#include "Battle/BattleResolver.h"
#include "Battle/BattleState.h"
#include "Battle/BattleTypes.h"
#include "Misc/AutomationTest.h"

// Namespace NOMEADO (L-042); variáveis de arquivo teriam nome próprio (L-045).
namespace OrdemDoResolvedorTeste
{
	FBattleState MontarComAliados(uint8 Colunas, uint8 Linhas)
	{
		FBattleState State;
		State.GridColumns = Colunas;
		State.GridRows = Linhas;
		State.CellLayout.SetNumZeroed(Colunas * Linhas);

		uint8 ProximoId = 1;
		for (uint8 Lado = 0; Lado < 2; ++Lado)
		{
			for (int32 Qual = 0; Qual < 2; ++Qual)
			{
				FPetState Pet;
				Pet.PetId = ProximoId++;
				Pet.Side = Lado;
				Pet.Health = 100; Pet.MaxHealth = 100;
				Pet.Attack = 40; Pet.Defense = 20; Pet.Speed = 30;
				Pet.MovePowers[0] = 60;
				State.Pets.Add(Pet);
			}
		}

		State.PlaceDuelistsAtStartingCells();
		return State;
	}

	FTurnCommit CommitPara(uint8 PetId, EActionType Tipo, EBattleDirection Rumo)
	{
		FTurnCommit Commit;
		Commit.PetId = PetId;
		for (int32 Slot = 0; Slot < FTurnCommit::ActionsPerTurn; ++Slot)
		{
			Commit.Actions[Slot] = { Tipo, Rumo };
		}
		return Commit;
	}
}

// ---------------------------------------------------------------------------
// CP6 — A ORDEM NÃO PODE DECIDIR NADA.
//
// `BattleResolver.cpp` carregava a nota de que, em v1 (1 pet por lado), não
// existia ponto de decisão onde a ordem mudasse o resultado — e que o desempate
// por velocidade ficaria para quando múltiplos pets do MESMO lado competissem
// por precedência. Este teste mede se esse dia chegou.
//
// O defeito que ele impede PARECERIA: a mesma partida com a mesma semente dá
// dois resultados, e o culpado aparente é o sorteio.
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FResolverOrderingPetArrayDoesNotDecideTest,
	"BattleSim.Resolver.Ordering.PetArrayDoesNotDecide",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FResolverOrderingPetArrayDoesNotDecideTest::RunTest(const FString& Parameters)
{
	const TArray<FTurnCommit> Commits = {
		OrdemDoResolvedorTeste::CommitPara(1, EActionType::Mover, EBattleDirection::Direita),
		OrdemDoResolvedorTeste::CommitPara(2, EActionType::Mover, EBattleDirection::Direita),
		OrdemDoResolvedorTeste::CommitPara(3, EActionType::Atacar, EBattleDirection::Esquerda),
		OrdemDoResolvedorTeste::CommitPara(4, EActionType::Atacar, EBattleDirection::Esquerda),
	};

	FBattleState Direta = OrdemDoResolvedorTeste::MontarComAliados(4, 3);

	FBattleState Invertida = Direta;
	Algo::Reverse(Invertida.Pets);

	const uint32 HashDireto =
		FBattleResolver::ResolveTurn(Direta, Commits).NextState.ComputeHash();
	const uint32 HashInvertido =
		FBattleResolver::ResolveTurn(Invertida, Commits).NextState.ComputeHash();

	AddInfo(FString::Printf(
		TEXT("2 aliados por lado — hash com Pets direto %u, invertido %u"),
		HashDireto, HashInvertido));

	TestEqual(TEXT("a ORDEM de State.Pets nao decide o resultado"),
		HashDireto, HashInvertido);

	return true;
}

// E A ORDEM DOS COMMITS TAMBÉM NÃO.
//
// Este é o caso que a rede traz: os commits chegam pelo fio na ordem em que os
// jogadores confirmaram, e essa ordem não é a mesma nos dois clientes. Se ela
// decidisse, a partida divergiria sem nada acusar — e o culpado aparente seria
// a latência.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FResolverOrderingCommitOrderDoesNotDecideTest,
	"BattleSim.Resolver.Ordering.CommitOrderDoesNotDecide",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FResolverOrderingCommitOrderDoesNotDecideTest::RunTest(const FString& Parameters)
{
	TArray<FTurnCommit> Commits = {
		OrdemDoResolvedorTeste::CommitPara(1, EActionType::Mover, EBattleDirection::Direita),
		OrdemDoResolvedorTeste::CommitPara(2, EActionType::Mover, EBattleDirection::Direita),
		OrdemDoResolvedorTeste::CommitPara(3, EActionType::Atacar, EBattleDirection::Esquerda),
		OrdemDoResolvedorTeste::CommitPara(4, EActionType::Atacar, EBattleDirection::Esquerda),
	};

	const FBattleState Estado = OrdemDoResolvedorTeste::MontarComAliados(4, 3);

	const uint32 HashDireto =
		FBattleResolver::ResolveTurn(Estado, Commits).NextState.ComputeHash();

	Algo::Reverse(Commits);
	const uint32 HashInvertido =
		FBattleResolver::ResolveTurn(Estado, Commits).NextState.ComputeHash();

	AddInfo(FString::Printf(
		TEXT("hash com commits em ordem %u, invertidos %u"),
		HashDireto, HashInvertido));

	TestEqual(TEXT("a ORDEM dos commits nao decide o resultado"),
		HashDireto, HashInvertido);

	return true;
}

// COM LAMA, que é onde o SORTEIO entra.
//
// A lama é a única casa cujo efeito é sorteado, e o sorteio vem de
// `State.Random` — um gerador com estado. Quem pede um número primeiro recebe
// um número diferente de quem pede depois, então a ORDEM DE CONSUMO é um ponto
// de decisão de verdade, e não uma preocupação teórica.
//
// Este é o teste que tem chance real de reprovar, e é por isso que ele existe
// separado: se reprovar, a resposta da CP6 é "há ponto de decisão, e o
// desempate por `PetId` tem de alcançar a coleta de intenções".
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FResolverOrderingMudDoesNotDependOnOrderTest,
	"BattleSim.Resolver.Ordering.MudDoesNotDependOnOrder",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FResolverOrderingMudDoesNotDependOnOrderTest::RunTest(const FString& Parameters)
{
	const TArray<FTurnCommit> Commits = {
		OrdemDoResolvedorTeste::CommitPara(1, EActionType::Mover, EBattleDirection::Direita),
		OrdemDoResolvedorTeste::CommitPara(2, EActionType::Mover, EBattleDirection::Direita),
	};

	FBattleState Direta = OrdemDoResolvedorTeste::MontarComAliados(4, 3);
	Direta.Pets.RemoveAll([](const FPetState& Pet) { return Pet.Side == 1; });

	// LAMA sob os dois aliados: os dois vão pedir um número ao sorteio.
	for (const FPetState& Pet : Direta.Pets)
	{
		Direta.CellLayout[Direta.CellIndex(Pet.Column, Pet.Row)] =
			static_cast<uint8>(ECellProperty::Mud);
	}

	FBattleState Invertida = Direta;
	Algo::Reverse(Invertida.Pets);

	const uint32 HashDireto =
		FBattleResolver::ResolveTurn(Direta, Commits).NextState.ComputeHash();
	const uint32 HashInvertido =
		FBattleResolver::ResolveTurn(Invertida, Commits).NextState.ComputeHash();

	AddInfo(FString::Printf(
		TEXT("na LAMA — hash direto %u, invertido %u"),
		HashDireto, HashInvertido));

	TestEqual(TEXT("nem na lama a ordem decide"), HashDireto, HashInvertido);

	return true;
}


// ---------------------------------------------------------------------------
// A COMBINAÇÃO QUE FALTAVA — e ela é a única que exercita o sorteio.
//
// Os três testes acima passaram, e por pouco eu os teria dado como resposta da
// CP6. Mas nenhum deles exercita o que importa: no da lama eu inverti
// `State.Pets`, e `CollectIntent` não itera `State.Pets` — itera as AÇÕES DO
// SLOT, achando o pet por id. Inverter os pets não muda nada ali. E o teste de
// ordem de commits, que inverte a coisa certa, não tem lama, então ninguém pede
// número ao sorteio.
//
// É o caso do teste que passa sem exercitar o caminho que diz testar — a mesma
// armadilha que a CP5 nomeia. Três verdes sobre a pergunta errada não respondem
// a pergunta certa.
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FResolverOrderingMudWithReversedCommitsTest,
	"BattleSim.Resolver.Ordering.MudWithReversedCommits",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FResolverOrderingMudWithReversedCommitsTest::RunTest(const FString& Parameters)
{
	TArray<FTurnCommit> Commits = {
		OrdemDoResolvedorTeste::CommitPara(1, EActionType::Mover, EBattleDirection::Direita),
		OrdemDoResolvedorTeste::CommitPara(2, EActionType::Mover, EBattleDirection::Direita),
	};

	FBattleState Estado = OrdemDoResolvedorTeste::MontarComAliados(4, 3);
	Estado.Pets.RemoveAll([](const FPetState& Pet) { return Pet.Side == 1; });

	// LAMA sob os dois: os dois pedem um número ao gerador, que TEM ESTADO.
	// Quem pede primeiro recebe um número diferente de quem pede depois.
	for (const FPetState& Pet : Estado.Pets)
	{
		Estado.CellLayout[Estado.CellIndex(Pet.Column, Pet.Row)] =
			static_cast<uint8>(ECellProperty::Mud);
	}

	const uint32 HashDireto =
		FBattleResolver::ResolveTurn(Estado, Commits).NextState.ComputeHash();

	Algo::Reverse(Commits);
	const uint32 HashInvertido =
		FBattleResolver::ResolveTurn(Estado, Commits).NextState.ComputeHash();

	AddInfo(FString::Printf(
		TEXT("LAMA com commits invertidos — direto %u, invertido %u"),
		HashDireto, HashInvertido));

	TestEqual(TEXT("a ordem dos commits nao decide QUEM escorrega"),
		HashDireto, HashInvertido);

	// E O SORTEIO FOI MESMO CONSUMIDO. Sem esta afirmação, uma lama que não
	// sorteasse nada faria os dois hashes baterem por não haver acaso nenhum —
	// e o teste passaria provando o contrário do que diz provar.
	const FBattleState Seco = OrdemDoResolvedorTeste::MontarComAliados(4, 3);
	FBattleState SemLama = Seco;
	SemLama.Pets.RemoveAll([](const FPetState& Pet) { return Pet.Side == 1; });

	const uint32 HashSemLama =
		FBattleResolver::ResolveTurn(SemLama, Commits).NextState.ComputeHash();

	TestNotEqual(TEXT("a lama MUDA o resultado — logo o sorteio foi consumido"),
		HashDireto, HashSemLama);

	return true;
}

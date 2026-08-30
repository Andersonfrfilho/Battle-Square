// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattleResolver.h"
#include "Misc/AutomationTest.h"

// Namespace NOMEADO: o anônimo não isola em unity build, onde os arquivos
// de teste viram uma unidade de tradução só e dois helpers homônimos
// colidem (L-042).
namespace RequisitoDeTerrenoTeste
{
	FBattleState EstadoComTerreno(ECellProperty TerrenoDoPet)
	{
		FBattleState Estado;

		FPetState Meu;
		Meu.PetId = 1; Meu.Side = 0; Meu.Column = 0; Meu.Row = 1;
		Meu.Health = 80; Meu.MaxHealth = 80; Meu.Attack = 20; Meu.Defense = 10;

		FPetState Dele;
		Dele.PetId = 2; Dele.Side = 1; Dele.Column = 2; Dele.Row = 1;
		Dele.Health = 80; Dele.MaxHealth = 80; Dele.Attack = 20; Dele.Defense = 10;

		Estado.Pets.Add(Meu);
		Estado.Pets.Add(Dele);
		Estado.Random.State = 777;
		Estado.CellLayout[Estado.CellIndex(0, 1)] = static_cast<uint8>(TerrenoDoPet);
		return Estado;
	}

	FTurnCommit Submergir()
	{
		FTurnCommit Commit;
		Commit.Actions[0].Type = EActionType::Submergir;
		Commit.Actions[1].Type = EActionType::Aguardar;
		Commit.Actions[2].Type = EActionType::Aguardar;
		return Commit;
	}

	FTurnCommit Aguardar()
	{
		FTurnCommit Commit;
		for (int32 Slot = 0; Slot < FTurnCommit::ActionsPerTurn; ++Slot)
		{
			Commit.Actions[Slot].Type = EActionType::Aguardar;
		}
		return Commit;
	}

	bool TemEvento(const TArray<FBattleEvent>& Trace, EBattleEventType Tipo)
	{
		return Trace.ContainsByPredicate(
			[Tipo](const FBattleEvent& E) { return E.Type == Tipo; });
	}
}

// SEM `using namespace` no escopo do arquivo: em unity build os dois
// arquivos viram uma TU só, os dois `using` ficam visíveis, e a chamada
// volta a ser ambígua — o namespace nomeado deixa de isolar justamente
// onde ele precisava isolar. Qualificar no ponto de chamada é o que
// realmente separa (L-042).


// A POÇA não serve para submergir, e a FUNDA serve.
//
// É o que dá sentido à fundura: sem isso, água é água e a profundidade é
// enfeite. Com isso, a poça vira decisão — vale gastar movimento para chegar
// ao fundo?
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPuddleIsNotDeepEnoughToSubmergeTest,
	"BattleSim.Terrain.PuddleIsNotDeepEnoughToSubmerge",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPuddleIsNotDeepEnoughToSubmergeTest::RunTest(const FString& Parameters)
{
	FBattleState NaFunda = RequisitoDeTerrenoTeste::EstadoComTerreno(ECellProperty::Water);
	const FBattleResolveResult Funda =
		FBattleResolver::ResolveTurn(NaFunda, RequisitoDeTerrenoTeste::Submergir(), RequisitoDeTerrenoTeste::Aguardar());
	TestFalse(TEXT("Na água funda submergir NÃO falha"),
		RequisitoDeTerrenoTeste::TemEvento(Funda.Trace, EBattleEventType::PosturaFalhou));

	FBattleState NaPoca = RequisitoDeTerrenoTeste::EstadoComTerreno(ECellProperty::ShallowWater);
	const FBattleResolveResult Poca =
		FBattleResolver::ResolveTurn(NaPoca, RequisitoDeTerrenoTeste::Submergir(), RequisitoDeTerrenoTeste::Aguardar());
	TestTrue(TEXT("Na poça submergir FALHA"),
		RequisitoDeTerrenoTeste::TemEvento(Poca.Trace, EBattleEventType::PosturaFalhou));

	// E a recusa é ALTA: silenciosamente virar Aguardar deixaria o jogador
	// achando que a skill não funciona, quando o que falta é fundura.
	FBattleState NoSeco = RequisitoDeTerrenoTeste::EstadoComTerreno(ECellProperty::None);
	TestTrue(TEXT("Em terra seca também falha"),
		RequisitoDeTerrenoTeste::TemEvento(FBattleResolver::ResolveTurn(NoSeco, RequisitoDeTerrenoTeste::Submergir(), RequisitoDeTerrenoTeste::Aguardar()).Trace,
			EBattleEventType::PosturaFalhou));

	return true;
}

// O requisito é DADO: sem ele declarado, nada exige terreno.
//
// É o que torna `escavar` possível sem editar o núcleo — e o que faz um teste
// montar um caso sem reconstruir a tabela inteira.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTerrainRequirementIsDataNotCodeTest,
	"BattleSim.Terrain.RequirementIsDataNotCode",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTerrainRequirementIsDataNotCodeTest::RunTest(const FString& Parameters)
{
	// O estado NASCE com as regras do jogo — ninguém precisa lembrar de
	// aplicá-las, e foi por isso que a primeira versão desta feature quebrou
	// dois testes antigos.
	FBattleState Padrao;
	TestFalse(TEXT("Por padrão, submergir exige terreno"),
		Padrao.TerrainAllowsSkill(EActionType::Submergir,
			static_cast<uint8>(ECellProperty::None)));

	// E dá para SOBRESCREVER: declarar "nenhum" solta a ação de novo. É o que
	// permite a um teste montar um caso sem reconstruir a tabela inteira.
	FBattleState SemRegra;
	SemRegra.RequireTerrainForSkill(EActionType::Submergir, ECellProperty::None);
	TestTrue(TEXT("Sem requisito, terra seca serve"),
		SemRegra.TerrainAllowsSkill(EActionType::Submergir,
			static_cast<uint8>(ECellProperty::None)));

	// Uma skill NOVA exigindo PEDRA — o `escavar` que o tipo Terra queria e
	// não podia ter — é uma linha, e não uma edição do núcleo.
	FBattleState ComEscavar;
	ComEscavar.RequireTerrainForSkill(EActionType::Camuflar, ECellProperty::Blocked);
	TestTrue(TEXT("Na pedra, a skill declarada passa"),
		ComEscavar.TerrainAllowsSkill(EActionType::Camuflar,
			static_cast<uint8>(ECellProperty::Blocked)));
	TestFalse(TEXT("Na água, não"),
		ComEscavar.TerrainAllowsSkill(EActionType::Camuflar,
			static_cast<uint8>(ECellProperty::Water)));

	// Ação SEM requisito continua livre, mesmo com outra declarada.
	TestTrue(TEXT("Voar não foi condicionado, e continua livre"),
		ComEscavar.TerrainAllowsSkill(EActionType::Voar,
			static_cast<uint8>(ECellProperty::None)));

	return true;
}

// O nível é COMPARAÇÃO, e não igualdade.
//
// Exigir "água ao menos funda" deixa a poça de fora sem listar terreno por
// terreno — e é o que permite um poder exigir mais fundura sem que cada
// requisito novo vire um caso no código.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWaterRequirementComparesDepthTest,
	"BattleSim.Terrain.WaterRequirementComparesDepth",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FWaterRequirementComparesDepthTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Terra seca não tem fundura"),
		WaterDepthOf(static_cast<uint8>(ECellProperty::None)), 0);
	TestEqual(TEXT("Poça é fundura 1"),
		WaterDepthOf(static_cast<uint8>(ECellProperty::ShallowWater)), 1);
	TestEqual(TEXT("Funda é fundura 2"),
		WaterDepthOf(static_cast<uint8>(ECellProperty::Water)), 2);

	FBattleState Exige;

	// Exigindo POÇA, a funda também serve — mais fundura nunca é menos.
	Exige.RequireTerrainForSkill(EActionType::Submergir, ECellProperty::ShallowWater);
	TestTrue(TEXT("Quem exige poça aceita a funda"),
		Exige.TerrainAllowsSkill(EActionType::Submergir,
			static_cast<uint8>(ECellProperty::Water)));

	// Exigindo FUNDA, a poça não serve.
	Exige.RequireTerrainForSkill(EActionType::Submergir, ECellProperty::Water);
	TestFalse(TEXT("Quem exige funda recusa a poça"),
		Exige.TerrainAllowsSkill(EActionType::Submergir,
			static_cast<uint8>(ECellProperty::ShallowWater)));

	return true;
}

// O requisito entra no HASH: dois estados que resolvem diferente não podem
// ter a mesma assinatura.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTerrainRequirementEntersHashTest,
	"BattleSim.Terrain.RequirementEntersHash",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTerrainRequirementEntersHashTest::RunTest(const FString& Parameters)
{
	FBattleState Livre = RequisitoDeTerrenoTeste::EstadoComTerreno(ECellProperty::ShallowWater);
	FBattleState Exigente = Livre;
	Exigente.RequireTerrainForSkill(EActionType::Voar, ECellProperty::Water);

	TestNotEqual(TEXT("Requisito diferente, assinatura diferente"),
		Livre.ComputeHash(), Exigente.ComputeHash());

	return true;
}

// Copyright 2026 Anderson. All Rights Reserved.

#include "Balance/TypeEffectivenessTable.h"
#include "Data/BattleDataTranslator.h"
#include "Data/PetDataLoader.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "World/EncounterMatchAssembler.h"

namespace
{
	FLoadedPetRecord MakeEffectivenessRecord(const FString& Id, const FString& Tipo)
	{
		FLoadedPetRecord Record;
		Record.Id = Id;
		Record.Name = Id;
		Record.Type = Tipo;
		Record.Attack = 100;
		Record.Defense = 50;
		Record.Speed = 10;
		Record.MaxHealth = 200;
		return Record;
	}
}

// A tabela de efetividade que o JOGO usa precisa existir e ter conteúdo.
//
// Ela era carregada só em teste: em produção ninguém chamava LoadFromJson, e a
// montagem usava TranslatePet (que não recebe tabela) em vez de
// TranslateMatchup (que recebe). Resultado: Fogo contra Planta batia igual a
// Fogo contra Água — o coração de um jogo de coleção estava desligado, e nada
// acusava porque cada peça passava isolada.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShippedTypeEffectivenessFileIsUsableTest,
	"BattleSquare.Balance.TypeEffectiveness.ShippedFileIsUsable",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FShippedTypeEffectivenessFileIsUsableTest::RunTest(const FString& Parameters)
{
	const FString Caminho = FPaths::Combine(
		FPaths::ProjectConfigDir(), TEXT("TypeEffectiveness.json"));

	FTypeEffectivenessTable Tabela;
	TestTrue(TEXT("O arquivo que acompanha o jogo carrega"),
		FTypeEffectivenessTable::LoadFromJson(Caminho, Tabela));

	// Super efetivo e pouco efetivo precisam DIFERIR de neutro, senão a tabela
	// existe e não muda nada — que é indistinguível de não existir.
	const int32 SuperEfetivo = Tabela.GetPercent(TEXT("Fogo"), TEXT("Planta"));
	const int32 PoucoEfetivo = Tabela.GetPercent(TEXT("Fogo"), TEXT("Agua"));

	TestTrue(TEXT("Fogo contra Planta é mais forte que neutro"), SuperEfetivo > 100);
	TestTrue(TEXT("Fogo contra Água é mais fraco que neutro"), PoucoEfetivo < 100);

	return true;
}

// E o efeito precisa CHEGAR ao ataque com que se luta.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTypeEffectivenessChangesTheAttackTest,
	"BattleSquare.Balance.TypeEffectiveness.ChangesTheAttack",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTypeEffectivenessChangesTheAttackTest::RunTest(const FString& Parameters)
{
	FTypeEffectivenessTable Tabela;
	FTypeEffectivenessTable::LoadFromJson(
		FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("TypeEffectiveness.json")), Tabela);

	const FLoadedPetRecord Fogo = MakeEffectivenessRecord(TEXT("fogo"), TEXT("Fogo"));
	const FLoadedPetRecord Planta = MakeEffectivenessRecord(TEXT("planta"), TEXT("Planta"));

	FPetState EsquerdoState, DireitoState;
	FPetPresentationInfo EsquerdoInfo, DireitoInfo;

	FBattleDataTranslator::TranslateMatchup(Fogo, Planta, Tabela, 1, 2,
		EsquerdoState, EsquerdoInfo, DireitoState, DireitoInfo);

	TestTrue(TEXT("Fogo bate MAIS forte em Planta que o ataque base"),
		EsquerdoState.Attack > Fogo.Attack);
	TestTrue(TEXT("E Planta bate mais fraco em Fogo"),
		DireitoState.Attack < Planta.Attack);

	return true;
}

// A efetividade precisa CHEGAR À APRESENTAÇÃO, não só ao ataque.
//
// Ela passou a se aplicar e continuava invisível: o jogador tomava mais dano
// sem ter como saber por quê. Guardar o número na montagem é o que permite a
// tela DIZER "é super efetivo" sem reescrever a fórmula de dano — que é o que
// audit_no_recalculation.sh proíbe.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTypeEffectivenessReachesPresentationTest,
	"BattleSquare.Balance.TypeEffectiveness.ReachesPresentation",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTypeEffectivenessReachesPresentationTest::RunTest(const FString& Parameters)
{
	FTypeEffectivenessTable Tabela;
	FTypeEffectivenessTable::LoadFromJson(
		FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("TypeEffectiveness.json")), Tabela);

	const FLoadedPetRecord Fogo = MakeEffectivenessRecord(TEXT("fogo"), TEXT("Fogo"));
	const FLoadedPetRecord Planta = MakeEffectivenessRecord(TEXT("planta"), TEXT("Planta"));

	FPetState EsquerdoState, DireitoState;
	FPetPresentationInfo EsquerdoInfo, DireitoInfo;

	FBattleDataTranslator::TranslateMatchup(Fogo, Planta, Tabela, 1, 2,
		EsquerdoState, EsquerdoInfo, DireitoState, DireitoInfo);

	TestTrue(TEXT("Quem é forte carrega isso na apresentação"),
		EsquerdoInfo.EffectivenessPercent > 100);
	TestTrue(TEXT("E quem é fraco também"),
		DireitoInfo.EffectivenessPercent < 100);

	// O número da apresentação tem de ser O MESMO que multiplicou o ataque.
	// Dois valores para a mesma coisa divergiriam na primeira edição.
	TestEqual(TEXT("Apresentação e ataque concordam"),
		EsquerdoState.Attack, (Fogo.Attack * EsquerdoInfo.EffectivenessPercent) / 100);

	// Montagem sem tabela é NEUTRA, não zero: 100 é o padrão, e zero faria a
	// tela anunciar "pouco efetivo" para todo mundo.
	FPetPresentationInfo SemTabela;
	TestEqual(TEXT("Padrão é neutro"), SemTabela.EffectivenessPercent, 100);

	return true;
}

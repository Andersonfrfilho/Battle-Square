// Copyright 2026 Anderson. All Rights Reserved.

#include "Data/BattleDataTranslator.h"
#include "Data/PetDataLoader.h"
#include "Battle/BattleResolver.h"
#include "Misc/AutomationTest.h"

namespace
{
	FLoadedPetRecord MakeSourcePet(const FString& Id, const FString& Name, const FString& Type, int32 Attack, int32 Defense, int32 Speed, int32 MaxHealth)
	{
		FLoadedPetRecord Record;
		Record.Id = Id;
		Record.Name = Name;
		Record.Type = Type;
		Record.Attack = Attack;
		Record.Defense = Defense;
		Record.Speed = Speed;
		Record.MaxHealth = MaxHealth;
		Record.UpdatedAt = TEXT("2026-08-25T00:00:00.000Z");
		return Record;
	}
}

// T18: campos numéricos copiados sem conversão; Health = MaxHealth no
// início; type NUNCA chega como string a FPetState.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleDataTranslatorBasicFieldsTest,
	"BattleSquare.BattleDataTranslator.CopiesNumericFieldsAndInitializesHealth",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleDataTranslatorBasicFieldsTest::RunTest(const FString& Parameters)
{
	const FLoadedPetRecord Source = MakeSourcePet(TEXT("uuid-abc"), TEXT("Fluffy"), TEXT("Cat"), 10, 5, 8, 50);

	FPetState BattleState;
	FPetPresentationInfo Presentation;
	FBattleDataTranslator::TranslatePet(Source, /*PetId=*/1, /*Side=*/0, /*Column=*/1, /*Row=*/1, BattleState, Presentation);

	TestEqual(TEXT("Attack copiado sem conversão"), BattleState.Attack, 10);
	TestEqual(TEXT("Defense copiado sem conversão"), BattleState.Defense, 5);
	TestEqual(TEXT("Speed copiado sem conversão"), BattleState.Speed, 8);
	TestEqual(TEXT("MaxHealth copiado sem conversão"), BattleState.MaxHealth, 50);
	TestEqual(TEXT("Health == MaxHealth no início"), BattleState.Health, BattleState.MaxHealth);
	TestEqual(TEXT("PetId atribuído pelo chamador"), BattleState.PetId, static_cast<uint8>(1));
	TestEqual(TEXT("Side atribuído pelo chamador"), BattleState.Side, static_cast<uint8>(0));

	TestEqual(TEXT("Nome vai para a apresentação, não para o núcleo"), Presentation.Name, Source.Name);
	TestEqual(TEXT("PetId espelhado na apresentação"), Presentation.PetId, BattleState.PetId);

	return true;
}

// T18/DP-06: dois pets na mesma partida recebem PetId distintos e
// estáveis — verificado pelo padrão de uso correto (chamador atribui
// ids distintos), não por geração interna do tradutor.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleDataTranslatorDistinctPetIdsTest,
	"BattleSquare.BattleDataTranslator.TwoPetsGetDistinctStableIds",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleDataTranslatorDistinctPetIdsTest::RunTest(const FString& Parameters)
{
	const FLoadedPetRecord SourceLeft = MakeSourcePet(TEXT("uuid-left"), TEXT("Fluffy"), TEXT("Cat"), 10, 5, 8, 50);
	const FLoadedPetRecord SourceRight = MakeSourcePet(TEXT("uuid-right"), TEXT("Spike"), TEXT("Dog"), 12, 4, 10, 45);

	FPetState LeftState, RightState;
	FPetPresentationInfo LeftPresentation, RightPresentation;
	FBattleDataTranslator::TranslatePet(SourceLeft, /*PetId=*/1, /*Side=*/0, 0, 1, LeftState, LeftPresentation);
	FBattleDataTranslator::TranslatePet(SourceRight, /*PetId=*/2, /*Side=*/1, 2, 1, RightState, RightPresentation);

	TestNotEqual(TEXT("PetIds são distintos"), LeftState.PetId, RightState.PetId);
	TestEqual(TEXT("Left é PetId 1"), LeftState.PetId, static_cast<uint8>(1));
	TestEqual(TEXT("Right é PetId 2"), RightState.PetId, static_cast<uint8>(2));

	return true;
}

// T18, verificação de integração final: o FBattleState montado a partir
// da tradução é aceito e resolvido normalmente pelo núcleo — prova de
// que a fronteira dados->núcleo funciona de ponta a ponta.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleDataTranslatorFeedsResolverTest,
	"BattleSquare.BattleDataTranslator.TranslatedStateFeedsResolverCorrectly",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleDataTranslatorFeedsResolverTest::RunTest(const FString& Parameters)
{
	const FLoadedPetRecord SourceLeft = MakeSourcePet(TEXT("uuid-left"), TEXT("Fluffy"), TEXT("Cat"), 20, 5, 8, 50);
	const FLoadedPetRecord SourceRight = MakeSourcePet(TEXT("uuid-right"), TEXT("Spike"), TEXT("Dog"), 10, 5, 10, 45);

	FBattleState State;
	FPetPresentationInfo LeftPresentation, RightPresentation;

	FPetState LeftBattleState, RightBattleState;
	FBattleDataTranslator::TranslatePet(SourceLeft, 1, /*Side=*/0, 1, 1, LeftBattleState, LeftPresentation);
	FBattleDataTranslator::TranslatePet(SourceRight, 2, /*Side=*/1, 2, 1, RightBattleState, RightPresentation);
	State.Pets.Add(LeftBattleState);
	State.Pets.Add(RightBattleState);

	FTurnCommit LeftCommit;
	LeftCommit.Actions[0] = { EActionType::Atacar, EBattleDirection::Direita };
	FTurnCommit RightCommit; // Aguardar nos 3 slots

	FBattleResolveResult Result = FBattleResolver::ResolveTurn(State, FBattleResolver::DuelCommits(State, LeftCommit, RightCommit));

	TestTrue(TEXT("Resolver aceitou o estado traduzido sem crash"), Result.Trace.Num() > 0);
	TestTrue(TEXT("Right (Spike) recebeu dano do ataque de Fluffy"), Result.NextState.Pets[1].Health < RightBattleState.Health);

	return true;
}

// T4 (colecao-e-captura): CatalogId e Type em texto puro são
// preservados da tradução — é o que permite ABattleArena saber, quando
// a batalha termina, qual registro de catálogo capturar.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleDataTranslatorPreservesCatalogIdTest,
	"BattleSquare.Data.TranslatePetPreservesCatalogId",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleDataTranslatorPreservesCatalogIdTest::RunTest(const FString& Parameters)
{
	const FLoadedPetRecord Source = MakeSourcePet(TEXT("uuid-catalogo-123"), TEXT("Fluffy"), TEXT("Cat"), 20, 5, 8, 50);

	FPetState BattleState;
	FPetPresentationInfo Presentation;
	FBattleDataTranslator::TranslatePet(Source, /*PetId=*/1, /*Side=*/0, /*Column=*/1, /*Row=*/1, BattleState, Presentation);

	TestEqual(TEXT("CatalogId preservado do FLoadedPetRecord::Id"), Presentation.CatalogId, Source.Id);
	TestEqual(TEXT("Type em texto puro preservado"), Presentation.Type, Source.Type);

	return true;
}

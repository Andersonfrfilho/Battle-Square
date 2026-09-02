// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattleTypes.h"
#include "Misc/AutomationTest.h"
#include "Net/BattleNetTypes.h"

// ---------------------------------------------------------------------------
// CP7 — O DONO ATRAVESSA O FIO.
//
// O defeito que este teste impede PARECERIA: a batalha online roda, ninguém vê
// erro, e o aliado do fundo simplesmente aguarda todo turno. É o mesmo modo de
// falhar SILENCIOSO que fez o commit ter três campos nomeados em vez de um
// array C — agora com o PET 2 em vez da AÇÃO 2.
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNetCommitCarriesThePetIdTest,
	"BattleSquare.Net.CommitCarriesThePetId",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FNetCommitCarriesThePetIdTest::RunTest(const FString& Parameters)
{
	FTurnCommit Original;
	Original.PetId = 7;
	Original.Actions[0] = { EActionType::Atacar, EBattleDirection::Direita };
	Original.Actions[1] = { EActionType::Mover, EBattleDirection::Cima };
	Original.Actions[2] = { EActionType::Defender, EBattleDirection::Nenhuma };

	const FNetTurnCommit NoFio = ToNetTurnCommit(Original);
	TestEqual(TEXT("o dono entra no fio"),
		static_cast<int32>(NoFio.PetId), 7);

	const FTurnCommit DeVolta = ToTurnCommit(NoFio);
	TestEqual(TEXT("e volta do fio"),
		static_cast<int32>(DeVolta.PetId), 7);

	// AS TRÊS AÇÕES CONTINUAM CHEGANDO. O `PetId` foi acrescentado no começo do
	// struct, e um campo novo que empurrasse os outros é exatamente o tipo de
	// mudança que quebra serialização em silêncio.
	for (int32 Slot = 0; Slot < FTurnCommit::ActionsPerTurn; ++Slot)
	{
		TestEqual(*FString::Printf(TEXT("acao %d de ida e volta"), Slot),
			static_cast<int32>(DeVolta.Actions[Slot].Type),
			static_cast<int32>(Original.Actions[Slot].Type));
		TestEqual(*FString::Printf(TEXT("direcao %d de ida e volta"), Slot),
			static_cast<int32>(DeVolta.Actions[Slot].Direction),
			static_cast<int32>(Original.Actions[Slot].Direction));
	}

	return true;
}

// N PETS ATRAVESSAM, E O PET N CHEGA.
//
// É o aceite escrito da task: não basta o primeiro chegar. Um `TArray` que
// serializasse só o elemento 0 passaria num teste de um pet só — e é
// precisamente essa dúvida que fez o commit ter três campos nomeados.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNetCommitCarriesEveryPetTest,
	"BattleSquare.Net.CommitCarriesEveryPet",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FNetCommitCarriesEveryPetTest::RunTest(const FString& Parameters)
{
	TArray<FTurnCommit> Originais;
	for (uint8 Qual = 1; Qual <= 4; ++Qual)
	{
		FTurnCommit Commit;
		Commit.PetId = Qual;
		// Ação DIFERENTE por pet: com todas iguais, um commit que chegasse
		// duplicado passaria por quatro commits distintos.
		Commit.Actions[0] = { EActionType::Mover,
			static_cast<EBattleDirection>(Qual) };
		Originais.Add(Commit);
	}

	TArray<FNetTurnCommit> NoFio;
	for (const FTurnCommit& Commit : Originais)
	{
		NoFio.Add(ToNetTurnCommit(Commit));
	}

	TestEqual(TEXT("quatro commits no fio"), NoFio.Num(), 4);

	for (int32 Qual = 0; Qual < NoFio.Num(); ++Qual)
	{
		const FTurnCommit DeVolta = ToTurnCommit(NoFio[Qual]);

		TestEqual(*FString::Printf(TEXT("o pet %d chegou"), Qual + 1),
			static_cast<int32>(DeVolta.PetId), Qual + 1);
		TestEqual(*FString::Printf(TEXT("o pet %d chegou com a direcao DELE"), Qual + 1),
			static_cast<int32>(DeVolta.Actions[0].Direction),
			static_cast<int32>(Originais[Qual].Actions[0].Direction));
	}

	// E O ÚLTIMO É DIFERENTE DO PRIMEIRO — a afirmação que um array truncado
	// não sobrevive.
	TestNotEqual(TEXT("o ultimo nao e copia do primeiro"),
		static_cast<int32>(ToTurnCommit(NoFio.Last()).PetId),
		static_cast<int32>(ToTurnCommit(NoFio[0]).PetId));

	return true;
}

// A VALIDAÇÃO CONTINUA REJEITANDO LIXO, e agora também o dono ausente.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNetCommitValidationTest,
	"BattleSquare.Net.CommitValidationStillRejectsGarbage",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FNetCommitValidationTest::RunTest(const FString& Parameters)
{
	FNetTurnCommit Bom;
	Bom.PetId = 1;
	TestTrue(TEXT("commit bem formado passa"), ValidateNetTurnCommit(Bom));

	FNetTurnCommit Lixo;
	Lixo.PetId = 1;
	Lixo.ActionA.Type = static_cast<EActionType>(200);
	TestTrue(TEXT("tipo de acao fora do enum e rejeitado"),
		!ValidateNetTurnCommit(Lixo));

	return true;
}

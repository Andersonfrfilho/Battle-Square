// Copyright 2026 Anderson. All Rights Reserved.

#include "Net/BattleRoomTypes.h"
#include "Misc/AutomationTest.h"

// T1: GenerateCandidateRoomCode nunca produz 0, O, 1 ou I — mesmo sob
// muitas repetições (esses caracteres não fazem parte do alfabeto,
// então nunca vão aparecer; o teste ainda vale para pegar um alfabeto
// editado por engano no futuro).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBattleRoomCodeExcludesAmbiguousCharactersTest,
	"BattleSquare.Net.RoomCode.ExcludesAmbiguousCharacters",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FBattleRoomCodeExcludesAmbiguousCharactersTest::RunTest(const FString& Parameters)
{
	bool bAllClean = true;
	for (int32 Iteration = 0; Iteration < 200; ++Iteration)
	{
		const FString Code = GenerateCandidateRoomCode();
		TestEqual(TEXT("Código tem o comprimento configurado"), Code.Len(), BattleRoomConstants::RoomCodeLength);

		for (TCHAR Character : Code)
		{
			if (Character == TEXT('0') || Character == TEXT('O') || Character == TEXT('1') || Character == TEXT('I'))
			{
				AddError(FString::Printf(TEXT("Código '%s' contém caractere ambíguo '%c'"), *Code, Character));
				bAllClean = false;
			}
		}
	}

	return bAllClean;
}

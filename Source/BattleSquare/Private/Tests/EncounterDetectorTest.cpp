// Copyright 2026 Anderson. All Rights Reserved.

#include "World/EncounterDetector.h"
#include "Misc/AutomationTest.h"

namespace
{
	FEncounterCandidate MakeCandidate(const FVector& Location, float RadiusUnits, FName CatalogId, bool bIsResolved = false)
	{
		FEncounterCandidate Candidate;
		Candidate.WorldLocation = Location;
		Candidate.EncounterRadiusUnits = RadiusUnits;
		Candidate.CatalogId = CatalogId;
		Candidate.bIsResolved = bIsResolved;
		return Candidate;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEncounterDetectorEmptyCandidateListTest,
	"BattleSquare.World.EncounterDetector.EmptyCandidateListTriggersNothing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEncounterDetectorEmptyCandidateListTest::RunTest(const FString& Parameters)
{
	FEncounterDetectionParams Params;
	Params.PawnWorldLocation = FVector::ZeroVector;

	TestEqual(TEXT("lista vazia não dispara encontro"), FEncounterDetector::FindTriggeredEncounter(Params), INDEX_NONE);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEncounterDetectorInsideRadiusTest,
	"BattleSquare.World.EncounterDetector.InsideRadiusTriggers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEncounterDetectorInsideRadiusTest::RunTest(const FString& Parameters)
{
	FEncounterDetectionParams Params;
	Params.PawnWorldLocation = FVector(100.0, 0.0, 0.0);
	Params.Candidates.Add(MakeCandidate(FVector(150.0, 0.0, 0.0), 200.0f, TEXT("Pet_A")));

	TestEqual(TEXT("pawn dentro do raio dispara o encontro"), FEncounterDetector::FindTriggeredEncounter(Params), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEncounterDetectorOutsideRadiusTest,
	"BattleSquare.World.EncounterDetector.OutsideRadiusDoesNotTrigger",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEncounterDetectorOutsideRadiusTest::RunTest(const FString& Parameters)
{
	FEncounterDetectionParams Params;
	Params.PawnWorldLocation = FVector::ZeroVector;
	Params.Candidates.Add(MakeCandidate(FVector(500.0, 0.0, 0.0), 200.0f, TEXT("Pet_A")));

	TestEqual(TEXT("pawn fora do raio não dispara"), FEncounterDetector::FindTriggeredEncounter(Params), INDEX_NONE);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEncounterDetectorExactBoundaryTest,
	"BattleSquare.World.EncounterDetector.ExactRadiusBoundaryTriggers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEncounterDetectorExactBoundaryTest::RunTest(const FString& Parameters)
{
	FEncounterDetectionParams Params;
	Params.PawnWorldLocation = FVector::ZeroVector;
	Params.Candidates.Add(MakeCandidate(FVector(200.0, 0.0, 0.0), 200.0f, TEXT("Pet_A")));

	TestEqual(TEXT("distância exatamente igual ao raio conta como dentro"), FEncounterDetector::FindTriggeredEncounter(Params), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEncounterDetectorResolvedIsIgnoredTest,
	"BattleSquare.World.EncounterDetector.ResolvedCandidateIsIgnored",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEncounterDetectorResolvedIsIgnoredTest::RunTest(const FString& Parameters)
{
	FEncounterDetectionParams Params;
	Params.PawnWorldLocation = FVector::ZeroVector;
	Params.Candidates.Add(MakeCandidate(FVector(10.0, 0.0, 0.0), 200.0f, TEXT("Pet_Resolvido"), true));

	TestEqual(TEXT("candidato já resolvido não dispara mesmo em cima do pawn"),
		FEncounterDetector::FindTriggeredEncounter(Params), INDEX_NONE);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEncounterDetectorClosestWinsTest,
	"BattleSquare.World.EncounterDetector.ClosestCandidateWins",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEncounterDetectorClosestWinsTest::RunTest(const FString& Parameters)
{
	FEncounterDetectionParams Params;
	Params.PawnWorldLocation = FVector::ZeroVector;
	Params.Candidates.Add(MakeCandidate(FVector(180.0, 0.0, 0.0), 200.0f, TEXT("Pet_Longe")));
	Params.Candidates.Add(MakeCandidate(FVector(50.0, 0.0, 0.0), 200.0f, TEXT("Pet_Perto")));

	TestEqual(TEXT("entre dois candidatos no raio, o mais próximo vence"),
		FEncounterDetector::FindTriggeredEncounter(Params), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEncounterDetectorTieBreaksByLowestIndexTest,
	"BattleSquare.World.EncounterDetector.ExactTieBreaksByLowestIndex",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEncounterDetectorTieBreaksByLowestIndexTest::RunTest(const FString& Parameters)
{
	FEncounterDetectionParams Params;
	Params.PawnWorldLocation = FVector::ZeroVector;
	Params.Candidates.Add(MakeCandidate(FVector(100.0, 0.0, 0.0), 200.0f, TEXT("Pet_Primeiro")));
	Params.Candidates.Add(MakeCandidate(FVector(-100.0, 0.0, 0.0), 200.0f, TEXT("Pet_Segundo")));

	TestEqual(TEXT("empate exato de distância resolve pelo menor índice"),
		FEncounterDetector::FindTriggeredEncounter(Params), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEncounterDetectorClosestResolvedYieldsToFartherTest,
	"BattleSquare.World.EncounterDetector.ResolvedClosestYieldsToFartherUnresolved",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEncounterDetectorClosestResolvedYieldsToFartherTest::RunTest(const FString& Parameters)
{
	FEncounterDetectionParams Params;
	Params.PawnWorldLocation = FVector::ZeroVector;
	Params.Candidates.Add(MakeCandidate(FVector(20.0, 0.0, 0.0), 200.0f, TEXT("Pet_Perto_Resolvido"), true));
	Params.Candidates.Add(MakeCandidate(FVector(150.0, 0.0, 0.0), 200.0f, TEXT("Pet_Longe_Disponivel")));

	TestEqual(TEXT("o mais próximo estando resolvido, o disponível mais distante dispara"),
		FEncounterDetector::FindTriggeredEncounter(Params), 1);
	return true;
}

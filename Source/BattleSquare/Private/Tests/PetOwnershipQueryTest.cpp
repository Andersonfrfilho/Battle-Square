// Copyright 2026 Anderson. All Rights Reserved.

#include "Meta/PetOwnershipQuery.h"
#include "Misc/AutomationTest.h"

/**
 * CR1 — "de quem é este pet" responde estável, e SELVAGEM é resposta.
 *
 * A pergunta que roubo, denúncia e polícia todos fazem. O aceite tem duas
 * pontas: selvagem é uma resposta válida (não ausência), e o próprio pet
 * responde "seu" — senão a volta ao próprio pet depois de reconectar fica sem
 * dono.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPetOwnershipRelationTest,
	"BattleSquare.Meta.Posse.DeQuemEhEstePet",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPetOwnershipRelationTest::RunTest(const FString&)
{
	using PetOwnershipQuery::RelationOf;
	using PetOwnershipQuery::EOwnerRelation;

	// SELVAGEM é resposta, não ausência: dono vazio é o bicho do mundo.
	TestEqual(TEXT("sem dono e SELVAGEM"),
		static_cast<int32>(RelationOf(TEXT(""), TEXT("conta-a"))),
		static_cast<int32>(EOwnerRelation::Wild));

	// O CONTRAPESO: o proprio pet, visto pelo proprio dono, e SEU — e a
	// resposta e a mesma em duas perguntas seguidas (funcao pura).
	TestEqual(TEXT("meu pet e MEU"),
		static_cast<int32>(RelationOf(TEXT("conta-a"), TEXT("conta-a"))),
		static_cast<int32>(EOwnerRelation::Self));
	TestEqual(TEXT("e responde o mesmo de novo"),
		static_cast<int32>(RelationOf(TEXT("conta-a"), TEXT("conta-a"))),
		static_cast<int32>(EOwnerRelation::Self));

	// O de OUTRO e de outro — o unico caso em que roubo faz sentido.
	TestEqual(TEXT("pet de outra conta e OtherAccount"),
		static_cast<int32>(RelationOf(TEXT("conta-b"), TEXT("conta-a"))),
		static_cast<int32>(EOwnerRelation::OtherAccount));

	// DOIS VAZIOS nao dao "meu": o offline sem conta ve todo pet com dono
	// como de outro, e o selvagem continua selvagem. Comparar vazio com
	// vazio como "igual" faria todo bicho do mundo virar seu.
	TestEqual(TEXT("offline: pet com dono e de OUTRO, nao meu"),
		static_cast<int32>(RelationOf(TEXT("conta-b"), TEXT(""))),
		static_cast<int32>(EOwnerRelation::OtherAccount));
	TestEqual(TEXT("offline: o selvagem ainda e selvagem"),
		static_cast<int32>(RelationOf(TEXT(""), TEXT(""))),
		static_cast<int32>(EOwnerRelation::Wild));

	return true;
}

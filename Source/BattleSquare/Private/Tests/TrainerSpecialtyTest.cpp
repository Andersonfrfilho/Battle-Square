// Copyright 2026 Anderson. All Rights Reserved.

#include "Meta/TrainerSpecialtyRules.h"
#include "Misc/AutomationTest.h"
#include "World/TrainingFieldRules.h"

// A ESCASSEZ é a regra. Sem teto não haveria escolha, e o "nunca completo" que
// dá identidade ao treinador deixaria de valer.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTrainerIsNeverCompleteTest,
	"BattleSquare.Meta.Trainer.IsNeverComplete",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTrainerIsNeverCompleteTest::RunTest(const FString& Parameters)
{
	FTrainerProfile Perfil;
	TestEqual(TEXT("Começa com todas as vagas"),
		FTrainerSpecialtyRules::FreeSlots(Perfil), FTrainerSpecialtyRules::MaxSpecialties);

	TestTrue(TEXT("Aprende a primeira"),
		FTrainerSpecialtyRules::TryLearn(Perfil, TEXT("flight")));
	TestTrue(TEXT("Aprende a segunda"),
		FTrainerSpecialtyRules::TryLearn(Perfil, TEXT("musculature")));

	TestEqual(TEXT("E as vagas acabaram"), FTrainerSpecialtyRules::FreeSlots(Perfil), 0);
	TestFalse(TEXT("A terceira é recusada"),
		FTrainerSpecialtyRules::TryLearn(Perfil, TEXT("camouflage")));
	TestEqual(TEXT("E nada foi acrescentado"), Perfil.Specialties.Num(), 2);

	// O teto é menor que o número de atributos, senão não haveria escolha.
	TestTrue(TEXT("O teto deixa atributos de fora"),
		FTrainerSpecialtyRules::MaxSpecialties < 5);

	return true;
}

// Aprender de novo o mesmo NÃO gasta a segunda vaga.
//
// Sem isto, clicar duas vezes por engano custaria uma escolha que não se
// desfaz — e o jogador não teria como saber que foi isso que aconteceu.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLearningTwiceDoesNotSpendASlotTest,
	"BattleSquare.Meta.Trainer.LearningTwiceDoesNotSpendASlot",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLearningTwiceDoesNotSpendASlotTest::RunTest(const FString& Parameters)
{
	FTrainerProfile Perfil;
	TestTrue(TEXT("Aprende"), FTrainerSpecialtyRules::TryLearn(Perfil, TEXT("flight")));
	TestFalse(TEXT("Repetir é recusado"),
		FTrainerSpecialtyRules::TryLearn(Perfil, TEXT("flight")));
	TestEqual(TEXT("E a vaga continua livre"),
		FTrainerSpecialtyRules::FreeSlots(Perfil), FTrainerSpecialtyRules::MaxSpecialties - 1);

	return true;
}

// Atributo desconhecido NÃO gasta vaga.
//
// Gastar uma das duas num nome escrito errado seria punição permanente por
// erro de cadastro — e a vaga não volta.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUnknownAttributeNeverSpendsASlotTest,
	"BattleSquare.Meta.Trainer.UnknownAttributeNeverSpendsASlot",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FUnknownAttributeNeverSpendsASlotTest::RunTest(const FString& Parameters)
{
	FTrainerProfile Perfil;
	TestFalse(TEXT("'telepatia' é recusado"),
		FTrainerSpecialtyRules::TryLearn(Perfil, TEXT("telepatia")));
	TestFalse(TEXT("Vazio também"),
		FTrainerSpecialtyRules::TryLearn(Perfil, FString()));
	TestEqual(TEXT("Nenhuma vaga foi gasta"),
		FTrainerSpecialtyRules::FreeSlots(Perfil), FTrainerSpecialtyRules::MaxSpecialties);

	return true;
}

// A especialidade MULTIPLICA o treino, e só o do atributo dela (DP-atr-09).
//
// Especialista em voo que treina musculatura rende como qualquer um: o estudo
// tem valor sem tornar o pet irrelevante nem valer para tudo de uma vez.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpecialtyOnlyHelpsItsOwnAttributeTest,
	"BattleSquare.Meta.Trainer.SpecialtyOnlyHelpsItsOwnAttribute",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSpecialtyOnlyHelpsItsOwnAttributeTest::RunTest(const FString& Parameters)
{
	FTrainerProfile Perfil;
	FTrainerSpecialtyRules::TryLearn(Perfil, TEXT("flight"));

	TestTrue(TEXT("É especialista em voo"),
		FTrainerSpecialtyRules::IsSpecialistIn(Perfil, TEXT("flight")));
	TestFalse(TEXT("E NÃO em musculatura"),
		FTrainerSpecialtyRules::IsSpecialistIn(Perfil, TEXT("musculature")));

	// O rendimento diferente é o que dá sentido à especialidade.
	float RestoNoVoo = 0.0f;
	const int32 NoVoo = FTrainingFieldRules::PointsForTime(60.0f,
		FTrainerSpecialtyRules::IsSpecialistIn(Perfil, TEXT("flight")), RestoNoVoo);

	float RestoNoMusculo = 0.0f;
	const int32 NoMusculo = FTrainingFieldRules::PointsForTime(60.0f,
		FTrainerSpecialtyRules::IsSpecialistIn(Perfil, TEXT("musculature")), RestoNoMusculo);

	TestTrue(TEXT("O mesmo tempo rende mais no atributo da especialidade"),
		NoVoo > NoMusculo);

	return true;
}

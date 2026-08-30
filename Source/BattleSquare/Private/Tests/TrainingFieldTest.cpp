// Copyright 2026 Anderson. All Rights Reserved.

#include "Meta/PetAttributeProgression.h"
#include "Meta/PetMoveRequirements.h"
#include "Misc/AutomationTest.h"
#include "Balance/PetTypeCatalog.h"
#include "Misc/Paths.h"
#include "World/TrainingFieldRules.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "World/WorldTrainingField.h"

// O RESTO acumulado atravessa as visitas.
//
// Sem devolvê-lo, dez visitas de cinco segundos renderiam ZERO enquanto uma de
// cinquenta renderia oito — e duas formas de fazer a mesma coisa não podem ter
// resultados diferentes. É o defeito que faria o jogador aprender a ficar
// parado em vez de andar pelo mundo.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTrainingCarriesLeftoverTimeTest,
	"BattleSquare.World.Training.CarriesLeftoverTime",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTrainingCarriesLeftoverTimeTest::RunTest(const FString& Parameters)
{
	// Meio segundo por passagem, como o temporizador do mundo.
	float Resto = 0.0f;
	int32 Total = 0;
	for (int32 Passagem = 0; Passagem < 24; ++Passagem)
	{
		Total += FTrainingFieldRules::PointsForTime(0.5f, /*bEspecialista=*/false, Resto);
	}

	// 24 passagens de 0,5s = 12 segundos = 2 pontos a 6s cada.
	TestEqual(TEXT("Passos pequenos somam igual a um passo grande"), Total, 2);

	float RestoDeUmaVez = 0.0f;
	TestEqual(TEXT("E 12 segundos de uma vez rendem o mesmo"),
		FTrainingFieldRules::PointsForTime(12.0f, /*bEspecialista=*/false, RestoDeUmaVez), 2);

	return true;
}

// O estudo do dono MULTIPLICA (DP-atr-09): mesmo tempo, mais rendimento —
// nunca treino que acontece sem o pet.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpecialistTrainsFasterForTheSameTimeTest,
	"BattleSquare.World.Training.SpecialistTrainsFasterForTheSameTime",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSpecialistTrainsFasterForTheSameTimeTest::RunTest(const FString& Parameters)
{
	float RestoComum = 0.0f;
	const int32 Comum = FTrainingFieldRules::PointsForTime(60.0f, /*bEspecialista=*/false, RestoComum);

	float RestoEspecialista = 0.0f;
	const int32 Especialista = FTrainingFieldRules::PointsForTime(60.0f, /*bEspecialista=*/true, RestoEspecialista);

	TestTrue(TEXT("O especialista rende mais no mesmo tempo"), Especialista > Comum);
	TestEqual(TEXT("E rende exatamente o multiplicador declarado"),
		Especialista, (Comum * FTrainingFieldRules::SpecialistBonusPercent) / 100);

	return true;
}

// Tempo zero ou negativo não rende nada, e não mexe no resto.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNoTimeNoTrainingTest,
	"BattleSquare.World.Training.NoTimeNoTraining",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FNoTimeNoTrainingTest::RunTest(const FString& Parameters)
{
	float Resto = 3.0f;
	TestEqual(TEXT("Zero segundos não rende"),
		FTrainingFieldRules::PointsForTime(0.0f, false, Resto), 0);
	TestEqual(TEXT("E não mexe no que já estava acumulado"), Resto, 3.0f);

	TestEqual(TEXT("Tempo negativo também não rende"),
		FTrainingFieldRules::PointsForTime(-5.0f, false, Resto), 0);
	TestEqual(TEXT("E também não mexe no resto"), Resto, 3.0f);

	return true;
}

// O campo treina O ATRIBUTO QUE DIZ TREINAR, e nome desconhecido não mexe em
// nada — um campo mal configurado não pode subir o atributo errado calado.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTrainingHitsTheNamedAttributeTest,
	"BattleSquare.World.Training.HitsTheNamedAttribute",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTrainingHitsTheNamedAttributeTest::RunTest(const FString& Parameters)
{
	FOwnedPetInstance Pet;
	FTrainingFieldRules::ApplyPoints(TEXT("flight"), 3, Pet);

	TestEqual(TEXT("Voo subiu"), Pet.SkillProficiency[FPetAttributeProgression::Flight], 3);
	TestEqual(TEXT("E a musculatura não"), Pet.Musculature, 0);

	FTrainingFieldRules::ApplyPoints(TEXT("musculature"), 2, Pet);
	TestEqual(TEXT("Musculatura subiu"), Pet.Musculature, 2);

	FOwnedPetInstance Intocado;
	FTrainingFieldRules::ApplyPoints(TEXT("telepatia"), 99, Intocado);
	TestEqual(TEXT("Nome desconhecido não mexe na musculatura"), Intocado.Musculature, 0);
	TestEqual(TEXT("Nem na personalidade"), Intocado.Personality, 0);
	TestEqual(TEXT("Nem no voo"),
		Intocado.SkillProficiency[FPetAttributeProgression::Flight], 0);

	return true;
}

// O vocabulário do campo é o MESMO do requisito de golpe.
//
// Duas listas de nomes produziriam um campo que treina algo que nenhum golpe
// exige — treino que nunca destrava nada, e sem nada apontando a causa.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTrainingSharesVocabularyWithMoveRequirementsTest,
	"BattleSquare.World.Training.SharesVocabularyWithMoveRequirements",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTrainingSharesVocabularyWithMoveRequirementsTest::RunTest(const FString& Parameters)
{
	const TCHAR* Atributos[] = {
		TEXT("musculature"), TEXT("personality"),
		TEXT("camouflage"), TEXT("flight"), TEXT("underground"),
	};

	for (const TCHAR* Atributo : Atributos)
	{
		// Rótulo conhecido: GetAttributeLabel devolve o nome CRU quando não
		// conhece, então "traduziu" prova que os dois lados falam o mesmo.
		const FString Rotulo = FPetMoveRequirements::GetAttributeLabel(Atributo).ToString();
		TestNotEqual(
			*FString::Printf(TEXT("'%s' é reconhecido pelo requisito de golpe"), Atributo),
			Rotulo, FString(Atributo));

		// E o treino sabe aplicá-lo.
		FOwnedPetInstance Pet;
		FTrainingFieldRules::ApplyPoints(Atributo, 1, Pet);
		const bool bMudouAlgo = Pet.Musculature != 0 || Pet.Personality != 0
			|| Pet.SkillProficiency[0] != 0 || Pet.SkillProficiency[1] != 0
			|| Pet.SkillProficiency[2] != 0;
		TestTrue(*FString::Printf(TEXT("'%s' treina alguma coisa"), Atributo), bMudouAlgo);

		// E o campo tem cor própria para ele — cinza é o "não conheço".
		TestNotEqual(*FString::Printf(TEXT("'%s' tem cor própria"), Atributo),
			AWorldTrainingField::ColorForAttribute(Atributo), FLinearColor(0.5f, 0.5f, 0.5f));
	}

	return true;
}

// Estar no campo é distância NO PLANO: pular dentro dele continua dentro.
// Exigir contato com o chão faria o treino piscar a cada salto.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTrainingFieldIgnoresHeightTest,
	"BattleSquare.World.Training.FieldIgnoresHeight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTrainingFieldIgnoresHeightTest::RunTest(const FString& Parameters)
{
	const AWorldTrainingField* Padrao = GetDefault<AWorldTrainingField>();

	TestTrue(TEXT("Campo nasce com malha ATRIBUÍDA"),
		Padrao->GetFieldMesh() != nullptr && Padrao->GetFieldMesh()->GetStaticMesh() != nullptr);

	UWorld* Mundo = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/false);
	if (!Mundo)
	{
		AddError(TEXT("não foi possível criar mundo de teste"));
		return false;
	}

	AWorldTrainingField* Campo = Mundo->SpawnActor<AWorldTrainingField>(
		AWorldTrainingField::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
	Campo->FieldRadiusUnits = 400.0f;

	TestTrue(TEXT("No centro está dentro"), Campo->IsInside(FVector::ZeroVector));
	TestTrue(TEXT("Pulando alto continua dentro"), Campo->IsInside(FVector(0.0f, 0.0f, 5000.0f)));
	TestTrue(TEXT("Na borda está dentro"), Campo->IsInside(FVector(400.0f, 0.0f, 0.0f)));
	TestFalse(TEXT("Um passo além está fora"), Campo->IsInside(FVector(401.0f, 0.0f, 0.0f)));

	Mundo->DestroyWorld(false);
	return true;
}

// CRIATURA É VIVA, TERRENO É TERRA — e isto é medível, não opinião.
//
// A primeira paleta dos campos não obedecia à regra, e o resultado era
// concreto: o verde da camuflagem ficava a cinco pontos do verde do pet de
// Planta. Um pet parado no próprio campo virava duas coisas da mesma cor
// dizendo coisas diferentes.
//
// O teste compara SATURAÇÃO: todo campo precisa ser mais apagado que o
// elemento mais apagado do elenco de criaturas. Assim a regra sobrevive ao
// próximo campo e ao próximo elemento, em vez de proteger só os cinco de hoje.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTerrainIsAlwaysDullerThanCreaturesTest,
	"BattleSquare.World.Training.TerrainIsAlwaysDullerThanCreatures",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTerrainIsAlwaysDullerThanCreaturesTest::RunTest(const FString& Parameters)
{
	FPetTypeCatalog Tipos;
	const FString Caminho = FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("PetTypes.json"));
	if (!FPetTypeCatalog::LoadFromJson(Caminho, Tipos))
	{
		AddError(TEXT("Config/PetTypes.json não carregou"));
		return false;
	}

	float SaturacaoMinimaDeCriatura = 1.0f;
	for (const FPetElementDefinition& Elemento : Tipos.GetElements())
	{
		SaturacaoMinimaDeCriatura = FMath::Min(
			SaturacaoMinimaDeCriatura, Elemento.Color.LinearRGBToHSV().G);
	}

	const TCHAR* Atributos[] = {
		TEXT("musculature"), TEXT("personality"),
		TEXT("camouflage"), TEXT("flight"), TEXT("underground"),
	};

	for (const TCHAR* Atributo : Atributos)
	{
		const float SaturacaoDoCampo =
			AWorldTrainingField::ColorForAttribute(Atributo).LinearRGBToHSV().G;

		TestTrue(
			*FString::Printf(TEXT("O campo de %s (sat %.2f) é mais apagado que qualquer criatura (mín %.2f)"),
				Atributo, SaturacaoDoCampo, SaturacaoMinimaDeCriatura),
			SaturacaoDoCampo < SaturacaoMinimaDeCriatura);
	}

	return true;
}

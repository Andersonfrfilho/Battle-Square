// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattleState.h"
#include "Data/BattleDataTranslator.h"
#include "Data/PetDataLoader.h"
#include "Misc/AutomationTest.h"

/**
 * O TRADUTOR LIGA A CONDUÇÃO — e o núcleo continua sem saber o que é "Raio".
 *
 * Sem isto a F8 inteira era inerte: regra completa, testada, e que nunca
 * acontecia numa partida. É a fronteira que já vale para a efetividade de
 * tipo — o Attack chega pré-multiplicado, e o núcleo nunca aprendeu que tipo
 * existe. A bandeira do golpe é a mesma ideia.
 */

namespace ProvaDoTradutorDeConducao
{
	FLoadedPetRecord PetDe(const FString& Tipo)
	{
		FLoadedPetRecord Registro;
		Registro.Id = TEXT("teste");
		Registro.Name = TEXT("Teste");
		Registro.Type = Tipo;
		Registro.Attack = 30;
		Registro.Defense = 10;
		Registro.Speed = 20;
		Registro.MaxHealth = 100;

		FLoadedPetMove Golpe;
		Golpe.Power = 40;
		Registro.Moves.Add(Golpe);

		return Registro;
	}

	FPetState Traduzido(const FLoadedPetRecord& Registro)
	{
		FPetState Estado;
		FPetPresentationInfo Aparencia;
		FBattleDataTranslator::TranslatePet(Registro, 1, 0, 0, 0, Estado, Aparencia);
		return Estado;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTranslatorConductionLightningConductsTest,
	"BattleSquare.TranslatorConduction.LightningConducts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTranslatorConductionLightningConductsTest::RunTest(const FString& Parameters)
{
	// O ACEITE DA G2: um pet de Raio sai da tradução com o golpe conduzindo.
	const FPetState DeRaio = ProvaDoTradutorDeConducao::Traduzido(
		ProvaDoTradutorDeConducao::PetDe(TEXT("Natural/Raio")));

	TestEqual(TEXT("o golpe de um pet de Raio conduz"),
		static_cast<int32>(DeRaio.MoveConducts[0]), 1);

	// E a CAPACIDADE dele é a força desse golpe — é ela que o faz aguentar a
	// própria corrente, sem regra à parte.
	TestEqual(TEXT("a capacidade e a forca do golpe que conduz"),
		DeRaio.ConductionCapacity(), 40);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTranslatorConductionOtherElementsDoNotTest,
	"BattleSquare.TranslatorConduction.OtherElementsDoNot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTranslatorConductionOtherElementsDoNotTest::RunTest(const FString& Parameters)
{
	// O CONTRAPESO, e sem ele o teste acima passaria numa regra que ligasse a
	// bandeira em TODO golpe — e aí qualquer pancada dentro d'água viraria
	// dano em área, com a água deixando de ser escolha para virar armadilha.
	const TCHAR* Outros[] = {
		TEXT("Natural/Fogo"), TEXT("Natural/Agua"), TEXT("Natural/Terra"),
		TEXT("Natural/Ar"), TEXT("Natural/Luz")
	};

	for (const TCHAR* Tipo : Outros)
	{
		const FPetState Pet = ProvaDoTradutorDeConducao::Traduzido(
			ProvaDoTradutorDeConducao::PetDe(Tipo));

		if (Pet.MoveConducts[0] != 0)
		{
			AddError(FString::Printf(TEXT("o golpe de um pet %s conduz, e nao devia"), Tipo));
			return false;
		}

		TestEqual(*FString::Printf(TEXT("%s nao tem capacidade de conducao"), Tipo),
			Pet.ConductionCapacity(), 0);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTranslatorConductionEmptySlotsDoNotConductTest,
	"BattleSquare.TranslatorConduction.EmptySlotsDoNotConduct",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTranslatorConductionEmptySlotsDoNotConductTest::RunTest(const FString& Parameters)
{
	// O pet de Raio tem UM golpe cadastrado; os outros três slots estão
	// vazios. Ligar a bandeira neles daria capacidade a um golpe que não
	// existe — e como a capacidade é o que absorve a corrente, um pet com
	// slots vazios ficaria resistente de graça.
	const FPetState DeRaio = ProvaDoTradutorDeConducao::Traduzido(
		ProvaDoTradutorDeConducao::PetDe(TEXT("Natural/Raio")));

	for (int32 Slot = 1; Slot < 4; ++Slot)
	{
		TestEqual(*FString::Printf(TEXT("o slot vazio %d nao conduz"), Slot),
			static_cast<int32>(DeRaio.MoveConducts[Slot]), 0);
	}

	return true;
}

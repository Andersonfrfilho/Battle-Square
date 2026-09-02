// Copyright 2026 Anderson. All Rights Reserved.

#include "Balance/PetSkillCatalog.h"
#include "Balance/PetTypeCatalog.h"
#include "Balance/PetTypeIdentity.h"
#include "Balance/TypeEffectivenessTable.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"

/**
 * O RAIO chega ao jogo inteiro — não só ao arquivo.
 *
 * Um elemento acrescentado só no JSON existe, luta e sai parecendo genérico:
 * sem cor própria, sem tombo, sem skill, e com efetividade neutra contra tudo.
 * O defeito é silencioso, e é o mesmo que dois testes já pegaram em 30/08/2026
 * quando um elemento nasceu pela metade.
 *
 * Este teste é o que torna o próximo elemento barato de conferir: ele percorre
 * as quatro pontas que um elemento precisa alcançar.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRaioElementReachesTheGameTest,
	"BattleSquare.RaioElement.ReachesTheGame",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRaioElementReachesTheGameTest::RunTest(const FString& Parameters)
{
	// PONTA 1: o catálogo conhece o elemento.
	TestTrue(TEXT("o Raio esta entre os elementos"),
		FPetTypeIdentity::AllElements().Contains(TEXT("Raio")));

	// PONTA 2: ele tem os DOIS canais de silhueta. Quem não distingue matiz lê
	// a inclinação, e vice-versa — declarar só um deixa metade dos jogadores
	// sem a informação.
	const FPetElementDefinition* Raio = nullptr;
	for (const FPetElementDefinition& Elemento : FPetTypeCatalog::Get().GetElements())
	{
		if (Elemento.Name.Equals(TEXT("Raio")))
		{
			Raio = &Elemento;
			break;
		}
	}

	if (!Raio)
	{
		AddError(TEXT("o Raio nao esta no catalogo de elementos"));
		return false;
	}

	// Cor e tombo DISTINTOS de todos os outros: dois elementos iguais na tela
	// são um elemento a menos, e nenhuma contagem muda.
	for (const FPetElementDefinition& Outro : FPetTypeCatalog::Get().GetElements())
	{
		if (Outro.Name.Equals(TEXT("Raio")))
		{
			continue;
		}

		TestFalse(*FString::Printf(TEXT("o tombo do Raio difere do de %s"), *Outro.Name),
			FMath::IsNearlyEqual(Raio->TiltDegrees, Outro.TiltDegrees, 1.0f));
		TestFalse(*FString::Printf(TEXT("a cor do Raio difere da de %s"), *Outro.Name),
			Raio->Color.Equals(Outro.Color, 0.001f));
	}

	// PONTA 3: a skill dele vira AÇÃO de verdade. Nome desconhecido cai em
	// `Aguardar`, e um elemento cuja skill não existe é um elemento sem golpe.
	TestNotEqual(TEXT("eletrificar vira uma acao"),
		static_cast<int32>(FPetSkillCatalog::SkillFromName(TEXT("eletrificar"))),
		static_cast<int32>(EActionType::Aguardar));
	TestEqual(TEXT("e a acao e Eletrificar"),
		static_cast<int32>(FPetSkillCatalog::SkillFromName(TEXT("eletrificar"))),
		static_cast<int32>(EActionType::Eletrificar));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRaioElementConductsAndIsGroundedTest,
	"BattleSquare.RaioElement.ConductsAndIsGrounded",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRaioElementConductsAndIsGroundedTest::RunTest(const FString& Parameters)
{
	FTypeEffectivenessTable Tabela;
	if (!FTypeEffectivenessTable::LoadFromJson(
		FPaths::ProjectConfigDir() / TEXT("TypeEffectiveness.json"), Tabela))
	{
		AddError(TEXT("nao consegui carregar a tabela de efetividade"));
		return false;
	}

	// PONTA 4: ele tem MATCHUPS. Elemento sem par cadastrado luta neutro
	// contra tudo — existe, luta, e não significa nada.
	//
	// As duas que ele tem são as que a física dá, e ter referência de fora é o
	// que impede a tabela de virar gosto: o raio CONDUZ pela água, e a TERRA
	// o aterra antes que a descarga chegue.
	//
	// A escola é a mesma dos dois lados de propósito: assim o que se está
	// medindo é o eixo do ELEMENTO, e não a composição dos dois.
	const int32 ContraAgua = Tabela.GetComposedPercent(
		TEXT("Natural/Raio"), TEXT("Natural/Agua"));
	const int32 ContraTerra = Tabela.GetComposedPercent(
		TEXT("Natural/Raio"), TEXT("Natural/Terra"));
	const int32 Neutro = Tabela.GetComposedPercent(
		TEXT("Natural/Raio"), TEXT("Natural/Fogo"));

	TestTrue(TEXT("o raio conduz pela agua"), ContraAgua > Neutro);
	TestTrue(TEXT("e a terra aterra o raio"), ContraTerra < Neutro);

	// E o contrapeso: contra um elemento sem par cadastrado ele é NEUTRO, e
	// não vantajoso. Uma tabela que devolvesse vantagem para tudo passaria nas
	// duas linhas acima e faria o raio dominar o jogo inteiro.
	TestEqual(TEXT("contra fogo ele e neutro"), Neutro, 100);

	return true;
}

// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattleState.h"
#include "Battle/FluidRegistry.h"
#include "Data/BattleDataTranslator.h"
#include "Data/PetDataLoader.h"
#include "Misc/AutomationTest.h"

/**
 * DE ONDE VEM A RESISTÊNCIA: o traço dá a base, o item soma.
 *
 * A base vem do ELEMENTO, e não de um bit em `EPetTrait` — resistência é
 * PORCENTAGEM POR FLUIDO, e um bit por fluido daria oito traços que só sabem
 * dizer sim ou não. O caminho é o mesmo do Incorpóreo, e está escrito no
 * tradutor: "o núcleo guarda o efeito, a camada de fora guarda o significado".
 *
 * O ITEM ainda não existe como sistema. Por isso a composição é função própria
 * e testada com valor injetado: o dia em que o cadastro de itens nascer, somar
 * é uma linha e não uma descoberta.
 */

namespace ProvaDaOrigemDaResistencia
{
	FPetState Traduzido(const FString& Tipo)
	{
		FLoadedPetRecord Registro;
		Registro.Id = TEXT("teste");
		Registro.Name = TEXT("Teste");
		Registro.Type = Tipo;
		Registro.Attack = 30;
		Registro.MaxHealth = 100;

		FPetState Estado;
		FPetPresentationInfo Aparencia;
		FBattleDataTranslator::TranslatePet(Registro, 1, 0, 0, 0, Estado, Aparencia);
		return Estado;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFluidResistSourceTheTraitGivesTheBaseTest,
	"BattleSquare.FluidResistSource.TheTraitGivesTheBase",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFluidResistSourceTheTraitGivesTheBaseTest::RunTest(const FString& Parameters)
{
	// O ACEITE: um pet de FOGO sai da tradução já resistindo à lava, porque é
	// o que ele É. Ninguém equipou nada.
	const FPetState DeFogo = ProvaDaOrigemDaResistencia::Traduzido(TEXT("Natural/Fogo"));

	const int32 ContraLava = DeFogo.ResistPercentFor(EFluidKind::Lava);
	if (ContraLava <= 0)
	{
		AddError(FString::Printf(
			TEXT("um pet de Fogo saiu com %d%% de resistencia a lava — o traco ")
			TEXT("declarado no elemento nao chegou ao pet"), ContraLava));
		return false;
	}

	// E ela vale: o dano que ele leva é menor que o bruto.
	const int32 Bruto = FluidRegistry::DamagePerSlot(EFluidKind::Lava);
	TestTrue(TEXT("a resistencia reduz o dano da lava"),
		(Bruto * (100 - ContraLava)) / 100 < Bruto);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFluidResistSourceOtherElementsDoNotResistTest,
	"BattleSquare.FluidResistSource.OtherElementsDoNotResist",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFluidResistSourceOtherElementsDoNotResistTest::RunTest(const FString& Parameters)
{
	// O CONTRAPESO: sem ele, uma regra que desse resistência a todo mundo
	// passaria no teste acima, e a lava deixaria de custar para qualquer um.
	const TCHAR* Outros[] = {
		TEXT("Natural/Agua"), TEXT("Natural/Planta"), TEXT("Natural/Ar"),
		TEXT("Natural/Raio")
	};

	for (const TCHAR* Tipo : Outros)
	{
		const FPetState Pet = ProvaDaOrigemDaResistencia::Traduzido(Tipo);
		TestEqual(*FString::Printf(TEXT("%s nao resiste a lava"), Tipo),
			Pet.ResistPercentFor(EFluidKind::Lava), 0);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFluidResistSourceTheItemAddsToTheTraitTest,
	"BattleSquare.FluidResistSource.TheItemAddsToTheTrait",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFluidResistSourceTheItemAddsToTheTraitTest::RunTest(const FString& Parameters)
{
	// O ITEM AINDA NÃO EXISTE como sistema — não há cadastro, save nem
	// equipar. Esta prova injeta o valor que ele daria, para que a REGRA de
	// composição não fique sem prova até lá. Sem ela, o ponto onde o item
	// entra seria uma descoberta no dia em que alguém criasse o sistema.

	// SOMA, e não o maior dos dois: tomar o maior faria uma bota de lava não
	// valer nada num pet de Fogo, que é justamente quem mais a usaria.
	TestEqual(TEXT("traco 50 mais item 30 da 80"),
		FBattleDataTranslator::ComposeFluidResist(50, 30), 80);

	// Presa em 100: acima disso o dano viraria negativo, e curar quem pisa na
	// lava e o oposto do que a regra promete.
	TestEqual(TEXT("traco 50 mais item 80 para em 100"),
		FBattleDataTranslator::ComposeFluidResist(50, 80), 100);

	// Sem item, vale o traço; sem traço, vale o item. As duas metades sozinhas.
	TestEqual(TEXT("so o traco"), FBattleDataTranslator::ComposeFluidResist(50, 0), 50);
	TestEqual(TEXT("so o item"), FBattleDataTranslator::ComposeFluidResist(0, 30), 30);

	// E negativo não subtrai: um item mal cadastrado não pode tirar a
	// resistência que a criatura tem por natureza.
	TestEqual(TEXT("item negativo nao tira o traco"),
		FBattleDataTranslator::ComposeFluidResist(50, -40), 50);

	return true;
}

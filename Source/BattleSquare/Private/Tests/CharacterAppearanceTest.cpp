// Copyright 2026 Anderson. All Rights Reserved.

#include "Meta/CharacterAppearance.h"
#include "Misc/AutomationTest.h"

/**
 * A APARÊNCIA DO PERSONAGEM (decisão 23) — a fundação de "mudar de aparência
 * despista, mas o rosto não engana".
 */

namespace AparenciaTeste
{
	// Nome próprio (L-042): helper homônimo em outro arquivo vira sobrecarga
	// ambígua no unity build.
	FCharacterAppearance OProcurado()
	{
		FCharacterAppearance A;
		A.HairStyle = 3; A.HairColor = 2; A.Outfit = 5; A.Face = 7;
		return A;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAppearanceDisguiseVsFaceTest,
	"BattleSquare.Meta.Aparencia.DisfarceNaoEnganaORosto",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAppearanceDisguiseVsFaceTest::RunTest(const FString&)
{
	using CharacterRecognition::IsRecognized;
	const FCharacterAppearance Cartaz = AparenciaTeste::OProcurado();

	// IDÊNTICO: reconhecido, obviamente.
	TestTrue(TEXT("o mesmo rosto e roupa e reconhecido"),
		IsRecognized(Cartaz, AparenciaTeste::OProcurado()));

	// TROCOU A ROUPA, manteve rosto e cabelo: AINDA reconhecido — o disfarce
	// barato (uma peca de roupa) nao engana enquanto o rosto ancora e sobra
	// outro traco. Rosto + cabelo = 2, o bastante.
	FCharacterAppearance TrocouRoupa = AparenciaTeste::OProcurado();
	TrocouRoupa.Outfit = 0;
	TestTrue(TEXT("so trocar a roupa NAO despista — o rosto e o cabelo entregam"),
		IsRecognized(Cartaz, TrocouRoupa));

	// TROCOU O ROSTO: não reconhecido — é o disfarce que a decisao 23 aceita.
	FCharacterAppearance Irreconhecivel = AparenciaTeste::OProcurado();
	Irreconhecivel.Face = 99;
	TestFalse(TEXT("trocar o ROSTO despista — a marca deixa de identificar"),
		IsRecognized(Cartaz, Irreconhecivel));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAppearanceFaceAloneIsNotEnoughTest,
	"BattleSquare.Meta.Aparencia.RostoSozinhoNaoCondena",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAppearanceFaceAloneIsNotEnoughTest::RunTest(const FString&)
{
	using CharacterRecognition::IsRecognized;
	using CharacterRecognition::MatchingTraits;
	const FCharacterAppearance Cartaz = AparenciaTeste::OProcurado();

	// SÓ o rosto bate, todo o resto diferente: NÃO reconhecido — dois
	// desconhecidos de rosto parecido nao sao a mesma pessoa. O contrapeso do
	// falso positivo (a policia erra, decisao 26 — mas nao por rosto sozinho).
	FCharacterAppearance SoORosto;
	SoORosto.HairStyle = 99; SoORosto.HairColor = 99; SoORosto.Outfit = 99;
	SoORosto.Face = Cartaz.Face;
	TestEqual(TEXT("so um traço bate"), MatchingTraits(Cartaz, SoORosto), 1);
	TestFalse(TEXT("rosto sozinho NAO condena"), IsRecognized(Cartaz, SoORosto));

	// Rosto + mais UM traço: reconhecido.
	FCharacterAppearance RostoEMais = SoORosto;
	RostoEMais.Outfit = Cartaz.Outfit;
	TestTrue(TEXT("rosto + a roupa reconhece"), IsRecognized(Cartaz, RostoEMais));

	return true;
}

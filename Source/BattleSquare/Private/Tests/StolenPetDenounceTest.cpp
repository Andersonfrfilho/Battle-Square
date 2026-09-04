// Copyright 2026 Anderson. All Rights Reserved.

#include "Meta/StolenPetDenounce.h"
#include "Misc/AutomationTest.h"

/**
 * CR8 — o pet roubado se denuncia, e o caso NEGATIVO é o centro (invariante
 * 19): o aviso NÃO pode virar perseguição.
 */

namespace DenunciaTeste
{
	FCharacterAppearance DoLadrao()
	{
		FCharacterAppearance A;
		A.HairStyle = 1; A.HairColor = 1; A.Outfit = 1; A.Face = 42;
		return A;
	}

	// Um contexto que DEVERIA avisar: pet roubado, observador que viu a lista,
	// portador reconhecível. Os testes o desligam campo a campo.
	StolenPetDenounce::FObserverContext ContextoQueAvisa()
	{
		StolenPetDenounce::FObserverContext C;
		C.bCarrierPetIsStolen = true;
		C.bObserverIsVictim = false;
		C.bObserverSawWantedList = true;
		C.PosterAppearance = DoLadrao();
		C.CarrierAppearanceNow = DoLadrao();
		return C;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDenounceWarnsWhoCanRecognizeTest,
	"BattleSquare.Meta.Denuncia.AvisaQuemPodeReconhecer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDenounceWarnsWhoCanRecognizeTest::RunTest(const FString&)
{
	using StolenPetDenounce::ShouldWarn;

	// O CASO POSITIVO: viu a lista, pet roubado, portador reconhecível.
	TestTrue(TEXT("quem viu a lista recebe o aviso"),
		ShouldWarn(DenunciaTeste::ContextoQueAvisa()));

	// A VITIMA sempre reconhece, mesmo sem ter visto a lista.
	StolenPetDenounce::FObserverContext Vitima = DenunciaTeste::ContextoQueAvisa();
	Vitima.bObserverSawWantedList = false;
	Vitima.bObserverIsVictim = true;
	TestTrue(TEXT("a vitima sempre recebe o aviso"), ShouldWarn(Vitima));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDenounceNeverHarassesTest,
	"BattleSquare.Meta.Denuncia.NuncaVirePerseguicao",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDenounceNeverHarassesTest::RunTest(const FString&)
{
	using StolenPetDenounce::ShouldWarn;

	// O CONTRAPESO DA INVARIANTE 19, e o teste mais importante do arquivo:
	// quem NAO tem relacao com o roubo — nunca viu a lista, nao e a vitima —
	// NAO recebe aviso, mesmo com o pet roubado bem na frente.
	StolenPetDenounce::FObserverContext Estranho = DenunciaTeste::ContextoQueAvisa();
	Estranho.bObserverIsVictim = false;
	Estranho.bObserverSawWantedList = false;
	TestFalse(TEXT("um estranho ao crime NAO recebe aviso — nada de perseguicao"),
		ShouldWarn(Estranho));

	// PET NAO ROUBADO: ninguem recebe aviso, nem quem viu a lista.
	StolenPetDenounce::FObserverContext Limpo = DenunciaTeste::ContextoQueAvisa();
	Limpo.bCarrierPetIsStolen = false;
	TestFalse(TEXT("pet limpo nao denuncia ninguem"), ShouldWarn(Limpo));

	// PORTADOR DISFARCADO (trocou o rosto): a marca deixou de identificar —
	// mesmo com o pet roubado e o observador tendo visto a lista, o disfarce
	// da decisao 23 despista, e a marca nao vira perpetua.
	StolenPetDenounce::FObserverContext Disfarcado = DenunciaTeste::ContextoQueAvisa();
	Disfarcado.CarrierAppearanceNow.Face = 999;
	TestFalse(TEXT("quem trocou de rosto nao e mais reconhecido"),
		ShouldWarn(Disfarcado));

	return true;
}

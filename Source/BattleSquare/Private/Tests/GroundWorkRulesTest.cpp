// Copyright 2026 Anderson. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "World/GroundWorkRules.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorkWithoutThePetStillPaysTest,
	"BattleSquare.World.Trabalho.SemOPetAindaPaga",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorkWithoutThePetStillPaysTest::RunTest(const FString&)
{
	// O CONTRAPESO PRIMEIRO, como a task exige: o caminho SEM o pet é o que a
	// spec teme quebrar — "um trabalho que EXIGE submergir tranca o jogador
	// sem" o pet certo. Se só se testasse com o pet presente, a regra
	// "facilita, nunca habilita" nunca seria verificada de verdade.
	const int32 SemPet = GroundWorkRules::PayFor(20, 50, /*bPetHasSkill=*/false);
	TestTrue(TEXT("sem o pet, o trabalho PAGA — nunca tranca"), SemPet > 0);
	TestEqual(TEXT("e paga a base INTEIRA, nao uma base descontada"), SemPet, 20);

	// COM o pet paga MAIS — por cima da base, nunca no lugar dela (o mesmo
	// desenho do DP-atr-09: o estudo do dono multiplica, não substitui).
	const int32 ComPet = GroundWorkRules::PayFor(20, 50, /*bPetHasSkill=*/true);
	TestTrue(TEXT("com o pet paga mais"), ComPet > SemPet);
	TestEqual(TEXT("na proporcao configurada"), ComPet, 30);

	// Bônus zero é pet que não muda nada — configuração válida, nunca crash.
	TestEqual(TEXT("bonus zero paga a base"),
		GroundWorkRules::PayFor(20, 0, true), 20);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorkIsOneJobInThreePlacesTest,
	"BattleSquare.World.Trabalho.UmTrabalhoTresLugares",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorkIsOneJobInThreePlacesTest::RunTest(const FString&)
{
	// A DECISÃO 59: o jogador escolhe ONDE, e o que muda entre os três lugares
	// é qual skill facilita — não a mecânica.
	TestTrue(TEXT("na fazenda se trabalha"),
		GroundWorkRules::IsWorkPlace(EGroundUse::Fazenda));
	TestTrue(TEXT("no criadouro se trabalha"),
		GroundWorkRules::IsWorkPlace(EGroundUse::Criadouro));
	TestTrue(TEXT("no pomar se trabalha"),
		GroundWorkRules::IsWorkPlace(EGroundUse::Pomar));

	// E SÓ nos três: o bosque é mata, a loja é comércio, o cemitério é
	// respeito. Trabalho em qualquer mancha faria o mapa inteiro virar
	// expediente.
	TestFalse(TEXT("no bosque nao"), GroundWorkRules::IsWorkPlace(EGroundUse::Bosque));
	TestFalse(TEXT("na loja nao"), GroundWorkRules::IsWorkPlace(EGroundUse::Loja));
	TestFalse(TEXT("no cemiterio nao"),
		GroundWorkRules::IsWorkPlace(EGroundUse::Cemiterio));
	TestFalse(TEXT("no mercado-negro nao — vender roubado e outra feature"),
		GroundWorkRules::IsWorkPlace(EGroundUse::MercadoNegro));

	// Cada lugar tem a SUA skill, e são três diferentes — se duas coincidissem,
	// um pet cobriria dois serviços e a escolha de onde trabalhar perderia o
	// motivo.
	const EActionType Fazenda = GroundWorkRules::FacilitatingSkillFor(EGroundUse::Fazenda);
	const EActionType Pomar = GroundWorkRules::FacilitatingSkillFor(EGroundUse::Pomar);
	const EActionType Criadouro = GroundWorkRules::FacilitatingSkillFor(EGroundUse::Criadouro);

	TestTrue(TEXT("tres lugares, tres skills"),
		Fazenda != Pomar && Pomar != Criadouro && Criadouro != Fazenda);

	return true;
}

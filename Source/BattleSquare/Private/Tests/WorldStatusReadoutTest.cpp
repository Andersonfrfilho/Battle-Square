// Copyright 2026 Anderson. All Rights Reserved.

#include "Meta/PetAttributeProgression.h"
#include "Misc/AutomationTest.h"
#include "World/WorldStatusReadout.h"

namespace
{
	FWorldStatusSnapshot RetratoComPet(int32 Musculatura, int32 Voo)
	{
		FWorldStatusSnapshot Retrato;
		Retrato.bHasOwnedPet = true;
		Retrato.OwnedPet.Name = TEXT("Faísca");
		Retrato.OwnedPet.Experience = 120;
		Retrato.OwnedPet.Musculature = Musculatura;
		Retrato.OwnedPet.SkillProficiency[FPetAttributeProgression::Flight] = Voo;
		return Retrato;
	}

	bool AlgumaLinhaContem(const TArray<FWorldStatusLine>& Linhas, const TCHAR* Trecho)
	{
		for (const FWorldStatusLine& Linha : Linhas)
		{
			if (Linha.Text.ToString().Contains(Trecho))
			{
				return true;
			}
		}
		return false;
	}
}

// Os atributos aparecem FORA da batalha.
//
// Até aqui a progressão só existia numa linha que passava no fim da luta. Um
// atributo que o jogador não consegue consultar é um atributo que ele não
// planeja — e planejar é o ponto inteiro de ter atributos.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWorldReadoutShowsAttributesTest,
	"BattleSquare.World.Readout.ShowsAttributes",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FWorldReadoutShowsAttributesTest::RunTest(const FString& Parameters)
{
	const TArray<FWorldStatusLine> Linhas =
		FWorldStatusReadout::Build(RetratoComPet(/*Musculatura=*/17, /*Voo=*/9));

	TestTrue(TEXT("O nome do pet aparece"), AlgumaLinhaContem(Linhas, TEXT("Faísca")));
	TestTrue(TEXT("A experiência aparece"), AlgumaLinhaContem(Linhas, TEXT("120")));
	TestTrue(TEXT("A musculatura aparece"), AlgumaLinhaContem(Linhas, TEXT("17")));
	TestTrue(TEXT("O voo aparece"), AlgumaLinhaContem(Linhas, TEXT("9")));
	TestTrue(TEXT("Com o rótulo que o jogador lê"), AlgumaLinhaContem(Linhas, TEXT("Voo")));

	return true;
}

// SEM pet, o painel DIZ que não há.
//
// O silêncio seria indistinguível de painel quebrado — e essa dúvida exata
// custou uma investigação inteira quando a experiência não pousava em ninguém.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWorldReadoutSaysWhenThereIsNoPetTest,
	"BattleSquare.World.Readout.SaysWhenThereIsNoPet",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FWorldReadoutSaysWhenThereIsNoPetTest::RunTest(const FString& Parameters)
{
	FWorldStatusSnapshot Vazio;
	const TArray<FWorldStatusLine> Linhas = FWorldStatusReadout::Build(Vazio);

	TestTrue(TEXT("Alguma linha é escrita"), Linhas.Num() > 0);
	TestTrue(TEXT("E ela diz que não há pet"),
		AlgumaLinhaContem(Linhas, TEXT("não tem pet")));
	TestTrue(TEXT("E que não há ninguém por perto"),
		AlgumaLinhaContem(Linhas, TEXT("Nenhum adversário")));

	return true;
}

// A distância vira METROS, e a COR avisa quem está andando.
//
// Quem caminha não acompanha um número mudando; percebe a linha ficando
// amarela. O número é para conferir depois, a cor é para reagir agora.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWorldReadoutWarnsByColorWhenCloseTest,
	"BattleSquare.World.Readout.WarnsByColorWhenClose",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FWorldReadoutWarnsByColorWhenCloseTest::RunTest(const FString& Parameters)
{
	FWorldStatusSnapshot Longe = RetratoComPet(0, 0);
	Longe.EncountersAlive = 3;
	Longe.DistanceToNearestUnits = 4000.0f;

	const TArray<FWorldStatusLine> LinhasLonge = FWorldStatusReadout::Build(Longe);
	TestTrue(TEXT("4000 unidades viram 40 m"), AlgumaLinhaContem(LinhasLonge, TEXT("40 m")));
	TestTrue(TEXT("Longe não é amarelo"),
		LinhasLonge.Last().Color != FColor::Yellow);

	FWorldStatusSnapshot Perto = Longe;
	Perto.DistanceToNearestUnits = 900.0f;

	const TArray<FWorldStatusLine> LinhasPerto = FWorldStatusReadout::Build(Perto);
	TestTrue(TEXT("Perto fica amarelo"), LinhasPerto.Last().Color == FColor::Yellow);

	return true;
}

// Inimigo vivo SEM distância conhecida não vira "colado em você".
//
// Zero é uma medida, não uma ausência: mostrá-lo faria o painel gritar perigo
// máximo justamente quando não se sabe nada.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWorldReadoutNeverFakesZeroDistanceTest,
	"BattleSquare.World.Readout.NeverFakesZeroDistance",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FWorldReadoutNeverFakesZeroDistanceTest::RunTest(const FString& Parameters)
{
	FWorldStatusSnapshot SemDistancia = RetratoComPet(0, 0);
	SemDistancia.EncountersAlive = 2;
	SemDistancia.DistanceToNearestUnits = -1.0f;

	const TArray<FWorldStatusLine> Linhas = FWorldStatusReadout::Build(SemDistancia);

	TestTrue(TEXT("Diz quantos são"), AlgumaLinhaContem(Linhas, TEXT("2")));
	TestFalse(TEXT("E NÃO inventa uma distância"), AlgumaLinhaContem(Linhas, TEXT("0 m")));
	TestTrue(TEXT("Nem fica amarelo por falta de informação"),
		Linhas.Last().Color != FColor::Yellow);

	return true;
}

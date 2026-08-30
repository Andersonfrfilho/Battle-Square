// Copyright 2026 Anderson. All Rights Reserved.

#include "Environment/ScenaryPalette.h"

#include "Misc/AutomationTest.h"

namespace
{
	// Nome próprio (L-042): helper homônimo em outro arquivo de teste vira
	// sobrecarga ambígua quando o unity build junta os dois.

	/**
	 * O quanto duas cores se distinguem NA TELA.
	 *
	 * Não é distância de matiz: dois verdes de matizes vizinhos e mesmo
	 * brilho leem como uma mancha só, que foi exatamente o defeito relatado.
	 * O que separa de longe é a diferença de LUMINÂNCIA — e é ela que este
	 * teste mede.
	 */
	float SeparacaoDePaleta(const FLinearColor& Primeira, const FLinearColor& Segunda)
	{
		const auto Luminancia = [](const FLinearColor& Cor)
		{
			return 0.2126f * Cor.R + 0.7152f * Cor.G + 0.0722f * Cor.B;
		};
		return FMath::Abs(Luminancia(Primeira) - Luminancia(Segunda));
	}

	/** Abaixo disto, um degrau da escala não se lê como degrau. */
	constexpr float SeparacaoMinima = 0.02f;

	const FName SlotDeCapim(TEXT("grass"));
	const FName SlotDeFolha(TEXT("leafsGreen"));
	const FName SlotDeTerra(TEXT("dirt"));
	const FName SlotDeCasca(TEXT("woodBark"));
}

/**
 * O defeito relatado, na sua forma exata: o arbusto e o capim chegam do
 * pacote com o MESMO slot `grass`, e enquanto a cor vinha de lá eles eram o
 * mesmo verde. Chavear a paleta só pelo slot deixaria assim.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FScenaryPaletteSeparatesUndergrowthFromGrassTest,
	"BattleSquare.ScenaryPalette.SeparatesUndergrowthFromGrass",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FScenaryPaletteSeparatesUndergrowthFromGrassTest::RunTest(const FString& Parameters)
{
	const FLinearColor Capim = ScenaryPalette::ColorFor(EScenaryRole::GroundCover, SlotDeCapim);
	const FLinearColor Arbusto = ScenaryPalette::ColorFor(EScenaryRole::Undergrowth, SlotDeCapim);

	TestTrue(
		TEXT("arbusto e capim, no MESMO slot do pacote, se distinguem na tela"),
		SeparacaoDePaleta(Capim, Arbusto) >= SeparacaoMinima);

	return true;
}

/**
 * A outra metade do pedido: árvore da mata, dossel e capim são três verdes
 * DIFERENTES, e a pedra não é verde nenhum.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FScenaryPaletteGivesEveryRoleItsOwnStepTest,
	"BattleSquare.ScenaryPalette.GivesEveryRoleItsOwnStep",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FScenaryPaletteGivesEveryRoleItsOwnStepTest::RunTest(const FString& Parameters)
{
	const TArray<TPair<EScenaryRole, FName>> Degraus = {
		{ EScenaryRole::GroundCover, SlotDeCapim },
		{ EScenaryRole::Undergrowth, SlotDeCapim },
		{ EScenaryRole::ForestTree, SlotDeFolha },
		{ EScenaryRole::CanopyTree, SlotDeFolha },
	};

	for (int32 Primeiro = 0; Primeiro < Degraus.Num(); ++Primeiro)
	{
		for (int32 Segundo = Primeiro + 1; Segundo < Degraus.Num(); ++Segundo)
		{
			const FLinearColor Uma = ScenaryPalette::ColorFor(Degraus[Primeiro].Key, Degraus[Primeiro].Value);
			const FLinearColor Outra = ScenaryPalette::ColorFor(Degraus[Segundo].Key, Degraus[Segundo].Value);

			TestTrue(
				FString::Printf(TEXT("degraus %d e %d da escada de verdes se distinguem"), Primeiro, Segundo),
				SeparacaoDePaleta(Uma, Outra) >= SeparacaoMinima);
		}
	}

	// E contra o chão, que é o degrau mais escuro de todos: era ele que
	// vestia `leafsGreen`, o MESMO material das folhas, e por isso o quadro
	// inteiro lia como uma mancha verde-água.
	const FLinearColor Chao = ScenaryPalette::GroundColor();
	for (const TPair<EScenaryRole, FName>& Degrau : Degraus)
	{
		TestTrue(
			TEXT("todo degrau se destaca CONTRA o chão da mata"),
			SeparacaoDePaleta(Chao, ScenaryPalette::ColorFor(Degrau.Key, Degrau.Value)) >= SeparacaoMinima);
	}

	return true;
}

/**
 * A pedra não pode ser verde nem marrom: ela é a única coisa do cenário que
 * não é folha nem madeira, e é assim que ela se anuncia. O musgo em cima
 * dela, sim, é verde — são as duas metades do mesmo asset.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FScenaryPaletteKeepsRockOutOfTheGreensTest,
	"BattleSquare.ScenaryPalette.KeepsRockOutOfTheGreens",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FScenaryPaletteKeepsRockOutOfTheGreensTest::RunTest(const FString& Parameters)
{
	const FLinearColor Rocha = ScenaryPalette::ColorFor(EScenaryRole::Rock, SlotDeTerra);

	TestTrue(
		TEXT("a rocha e' cinza: nenhum canal se destaca dos outros"),
		FMath::Abs(Rocha.R - Rocha.G) < 0.05f && FMath::Abs(Rocha.G - Rocha.B) < 0.05f);

	const FLinearColor Musgo = ScenaryPalette::ColorFor(EScenaryRole::Rock, SlotDeCapim);
	TestTrue(TEXT("o musgo sobre a pedra e' verde"), Musgo.G > Musgo.R && Musgo.G > Musgo.B);

	return true;
}

/**
 * A madeira é madeira em QUALQUER papel.
 *
 * Um tronco caído no chão e o tronco de uma árvore do dossel são a mesma
 * matéria; pintá-los diferente porque o papel é diferente confundiria quem
 * tenta ler o que é o quê.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FScenaryPaletteKeepsBarkConsistentAcrossRolesTest,
	"BattleSquare.ScenaryPalette.KeepsBarkConsistentAcrossRoles",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FScenaryPaletteKeepsBarkConsistentAcrossRolesTest::RunTest(const FString& Parameters)
{
	const FLinearColor CascaDoTroncoCaido = ScenaryPalette::ColorFor(EScenaryRole::DeadWood, SlotDeCasca);
	const FLinearColor CascaDaArvore = ScenaryPalette::ColorFor(EScenaryRole::CanopyTree, SlotDeCasca);

	TestEqual(TEXT("a casca e' a mesma cor no tronco caido e na arvore"),
		CascaDoTroncoCaido, CascaDaArvore);

	// E ela é quente: é o contraste contra o verde que estava faltando.
	TestTrue(TEXT("a casca puxa para o vermelho, nao para o verde"),
		CascaDaArvore.R > CascaDaArvore.G && CascaDaArvore.G >= CascaDaArvore.B);

	return true;
}

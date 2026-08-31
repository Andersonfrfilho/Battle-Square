// Copyright 2026 Anderson. All Rights Reserved.

#include "World/VillageLayout.h"
#include "Environment/RegionResidency.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVilaCabeNoLoteTest,
	"BattleSquare.World.Vila.CabeNoLote",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVilaCabeNoLoteTest::RunTest(const FString&)
{
	// Prédio que passa do lote invade a floresta em volta — e a floresta é o
	// resto do bloco 0,0. Sem esta conta, a vila cresce até o bloco vizinho e
	// o jogador sai de casa direto para outro pedaço do mundo.
	//
	// Este teste JÁ PEGOU o defeito uma vez: a primeira versão do traçado
	// punha prédios a 1280 unidades num lote de 1088.
	const TArray<FVillagePlacement> Pecas = VillageLayout::Plan();
	TestTrue(TEXT("A vila tem prédios"), Pecas.Num() > 0);

	for (const FVillagePlacement& Peca : Pecas)
	{
		TestTrue(*FString::Printf(TEXT("O prédio %d cabe no lote"),
			static_cast<int32>(Peca.Building)), VillageLayout::FitsInPlot(Peca));
	}

	// E o LOTE cabe no bloco, com folga para a mata: vila que enche o bloco
	// deixa de ser um lugar dentro de um mundo e passa a ser o mundo.
	TestTrue(TEXT("O lote é uma fração do bloco"),
		VillageLayout::PlotHalfExtentUnits() * 2.0f
			< RegionResidency::ChunkSideUnits() * 0.5f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNenhumPredioInvadeOutroTest,
	"BattleSquare.World.Vila.NenhumPredioInvadeOutro",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNenhumPredioInvadeOutroTest::RunTest(const FString&)
{
	// Dois prédios no mesmo lugar não dão erro nenhum — eles se atravessam, e
	// o defeito só aparece olhando. É o tipo de coisa que este projeto
	// descobriu tarde três vezes.
	const TArray<FVillagePlacement> Pecas = VillageLayout::Plan();

	for (int32 Primeiro = 0; Primeiro < Pecas.Num(); ++Primeiro)
	{
		for (int32 Segundo = Primeiro + 1; Segundo < Pecas.Num(); ++Segundo)
		{
			TestFalse(*FString::Printf(TEXT("O prédio %d não invade o %d"),
				static_cast<int32>(Pecas[Primeiro].Building),
				static_cast<int32>(Pecas[Segundo].Building)),
				VillageLayout::Overlaps(Pecas[Primeiro], Pecas[Segundo]));
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAVilaTemOQuePrometeuTest,
	"BattleSquare.World.Vila.TemOQuePrometeu",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAVilaTemOQuePrometeuTest::RunTest(const FString&)
{
	const TArray<FVillagePlacement> Pecas = VillageLayout::Plan();

	// Os DOIS que tiram regra do console. São a razão de a vila existir: sem
	// eles, curar e especializar-se seguem inalcançáveis para quem joga, e a
	// vila vira cenário bonito.
	TestTrue(TEXT("Tem Centro de Recuperação"),
		VillageLayout::HasBuilding(Pecas, EVillageBuilding::CentroDeRecuperacao));
	TestTrue(TEXT("Tem Escola do treinador"),
		VillageLayout::HasBuilding(Pecas, EVillageBuilding::Escola));

	TestTrue(TEXT("Tem praça, que é onde fica o poste"),
		VillageLayout::HasBuilding(Pecas, EVillageBuilding::Praca));
	TestTrue(TEXT("Tem marco de retorno"),
		VillageLayout::HasBuilding(Pecas, EVillageBuilding::Marco));
	TestTrue(TEXT("Tem arena"),
		VillageLayout::HasBuilding(Pecas, EVillageBuilding::Arena));

	// E CASAS, que existem só para a vila se ler de fora — mas precisam
	// existir, senão ela é um punhado de prédios soltos num descampado.
	int32 Casas = 0;
	for (const FVillagePlacement& Peca : Pecas)
	{
		if (Peca.Building == EVillageBuilding::Casa) { ++Casas; }
	}
	TestTrue(TEXT("Tem casas fechando o quadro"), Casas >= 3);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAPracaFicaNoMeioTest,
	"BattleSquare.World.Vila.APracaFicaNoMeio",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAPracaFicaNoMeioTest::RunTest(const FString&)
{
	// O poste de anúncios vive na praça, e ele é o que responde "onde ficam as
	// coisas" — a pergunta que a primeira sessão jogada deixou sem resposta.
	// Poste num canto é poste que ninguém lê.
	const TArray<FVillagePlacement> Pecas = VillageLayout::Plan();

	const FVillagePlacement* Praca = Pecas.FindByPredicate(
		[](const FVillagePlacement& Peca)
		{ return Peca.Building == EVillageBuilding::Praca; });

	TestNotNull(TEXT("A praça existe"), Praca);
	if (!Praca) { return false; }

	TestTrue(TEXT("A praça está no centro da vila"),
		Praca->OffsetUnits.IsNearlyZero());

	// E ela é CHÃO, não prédio: uma praça alta seria um muro no meio da vila.
	for (const FVillagePlacement& Peca : Pecas)
	{
		if (Peca.Building == EVillageBuilding::Praca) { continue; }
		TestTrue(TEXT("Todo prédio é mais alto que a praça"),
			Peca.HeightUnits > Praca->HeightUnits);
	}

	// A CASA é mais baixa que o Centro de Recuperação: tamanho é hierarquia
	// sem precisar de placa, e o que importa precisa se ver de longe.
	const FVillagePlacement* Centro = Pecas.FindByPredicate(
		[](const FVillagePlacement& Peca)
		{ return Peca.Building == EVillageBuilding::CentroDeRecuperacao; });
	const FVillagePlacement* Casa = Pecas.FindByPredicate(
		[](const FVillagePlacement& Peca)
		{ return Peca.Building == EVillageBuilding::Casa; });

	if (Centro && Casa)
	{
		TestTrue(TEXT("O Centro se ergue acima das casas"),
			Centro->HeightUnits > Casa->HeightUnits);
	}

	return true;
}

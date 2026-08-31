// Copyright 2026 Anderson. All Rights Reserved.

#include "World/Village.h"
#include "World/VillageLayout.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

namespace VilaEmPeTeste
{
	UWorld* CriarMundo()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
		FWorldContext& Contexto = GEngine->CreateNewWorldContext(EWorldType::Game);
		Contexto.SetCurrentWorld(World);
		World->InitializeActorsForPlay(FURL());
		return World;
	}

	void DestruirMundo(UWorld* World)
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAVilaExisteNaTelaTest,
	"BattleSquare.World.Vila.ExisteNaTela",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAVilaExisteNaTelaTest::RunTest(const FString&)
{
	// Ator que cria componente de malha e não atribui NENHUMA passa em toda
	// bateria e não existe na tela. Aconteceu três vezes neste projeto, e é
	// por isso que a atribuição é verificada aqui e não só pela sonda.
	UWorld* Mundo = VilaEmPeTeste::CriarMundo();
	AVillage* Vila = Mundo->SpawnActor<AVillage>();
	Vila->BuildVillage();

	TestEqual(TEXT("Um prédio por peça do traçado"),
		Vila->GetBuiltCount(), VillageLayout::Plan().Num());

	for (const TObjectPtr<UStaticMeshComponent>& Predio : Vila->GetBuiltMeshes())
	{
		TestNotNull(TEXT("O prédio tem componente"), Predio.Get());
		if (!Predio) { continue; }

		TestNotNull(TEXT("E o componente tem MALHA atribuída"),
			ToRawPtr(Predio->GetStaticMesh()));

		// Escala zero é malha atribuída e invisível — o mesmo defeito com
		// outra roupa.
		const FVector Escala = Predio->GetRelativeScale3D();
		TestTrue(TEXT("E a escala não é zero"),
			Escala.X > KINDA_SMALL_NUMBER && Escala.Y > KINDA_SMALL_NUMBER
				&& Escala.Z > KINDA_SMALL_NUMBER);
	}

	VilaEmPeTeste::DestruirMundo(Mundo);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAVilaNaoSeAtravessaTest,
	"BattleSquare.World.Vila.NaoSeAtravessa",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAVilaNaoSeAtravessaTest::RunTest(const FString&)
{
	// ESTE teste nasceu de um relato: "estou atravessando blocos", "muitas
	// montanhas deixam a gente passar". Cenário sólido sem colisão é a
	// terceira ocorrência do mesmo padrão neste projeto, e nenhuma bateria o
	// pegava — o ator está lá, a lógica roda, e o jogador passa por dentro.
	UWorld* Mundo = VilaEmPeTeste::CriarMundo();
	AVillage* Vila = Mundo->SpawnActor<AVillage>();
	Vila->BuildVillage();

	for (const TObjectPtr<UStaticMeshComponent>& Predio : Vila->GetBuiltMeshes())
	{
		if (!Predio) { continue; }

		TestTrue(TEXT("O prédio tem colisão ligada"),
			Predio->GetCollisionEnabled() != ECollisionEnabled::NoCollision);
		TestEqual(TEXT("E BLOQUEIA quem anda"),
			Predio->GetCollisionResponseToChannel(ECC_Pawn), ECR_Block);
	}

	VilaEmPeTeste::DestruirMundo(Mundo);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAVilaNasceOndeOJogadorNasceTest,
	"BattleSquare.World.Vila.NasceOndeOJogadorNasce",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAVilaNasceOndeOJogadorNasceTest::RunTest(const FString&)
{
	// A vila fica DEBAIXO do jogador, não ao lado. Sair de casa é o primeiro
	// passo do jogo; nascer a duzentos metros da vila faria o primeiro passo
	// ser uma caminhada até o começo.
	//
	// E o lote inteiro precisa caber no bloco 0,0 — senão a vila transborda
	// para o bloco vizinho, que é conteúdo de outro pedaço do mundo.
	const float MeioLote = VillageLayout::PlotHalfExtentUnits();

	for (const FVillagePlacement& Peca : VillageLayout::Plan())
	{
		const float MaisLonge = FMath::Max(
			FMath::Abs(Peca.OffsetUnits.X) + Peca.HalfExtentUnits.X,
			FMath::Abs(Peca.OffsetUnits.Y) + Peca.HalfExtentUnits.Y);

		TestTrue(TEXT("Nenhum prédio sai do lote em volta da origem"),
			MaisLonge <= MeioLote + KINDA_SMALL_NUMBER);
	}

	// E o jogador nasce DENTRO do lote: na origem, que é o centro da praça.
	TestTrue(TEXT("A origem cai dentro da vila"), MeioLote > 0.0f);

	return true;
}

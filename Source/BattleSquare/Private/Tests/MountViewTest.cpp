// Copyright 2026 Anderson. All Rights Reserved.

#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "World/MountView.h"

/**
 * MT5 — o pet montado EXISTE NA TELA: a prova é a ATRIBUIÇÃO de malha no
 * construtor, a regra três vezes paga (pet, inimigo do mundo, o proprio
 * jogador nasceram invisiveis por passar em toda logica sem asset).
 */

namespace MontariaNaTelaTeste
{
	UWorld* CriarMundoParaAMontaria()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
		FWorldContext& Contexto = GEngine->CreateNewWorldContext(EWorldType::Game);
		Contexto.SetCurrentWorld(World);
		World->InitializeActorsForPlay(FURL());
		return World;
	}

	void DestruirMundoDaMontaria(UWorld* World)
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMountViewHasMeshTest,
	"BattleSquare.World.Montaria.OMontadoExisteNaTela",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMountViewHasMeshTest::RunTest(const FString&)
{
	UWorld* Mundo = MontariaNaTelaTeste::CriarMundoParaAMontaria();
	AMountView* Montaria = Mundo->SpawnActor<AMountView>();
	if (!TestNotNull(TEXT("a montaria nasce"), Montaria))
	{
		MontariaNaTelaTeste::DestruirMundoDaMontaria(Mundo);
		return false;
	}

	// A PROVA que importa: o corpo tem malha ATRIBUIDA no construtor. Sem isto,
	// a montaria passaria em toda logica e nao existiria na tela.
	if (TestNotNull(TEXT("o corpo da montaria existe"), Montaria->GetBody()))
	{
		TestNotNull(TEXT("o corpo tem malha atribuida — nao e invisivel"),
			ToRawPtr(Montaria->GetBody()->GetStaticMesh()));
	}

	MontariaNaTelaTeste::DestruirMundoDaMontaria(Mundo);
	return true;
}

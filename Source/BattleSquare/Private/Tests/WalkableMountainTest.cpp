// Copyright 2026 Anderson. All Rights Reserved.

#include "Environment/WalkableMountain.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "PhysicsEngine/BodySetup.h"

namespace
{
	// Nome próprio (L-042): helper homônimo em outro arquivo de teste vira
	// sobrecarga ambígua quando o unity build junta os dois.
	UWorld* CreateWalkableMountainTestWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
		FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
		WorldContext.SetCurrentWorld(World);
		World->InitializeActorsForPlay(FURL());
		return World;
	}

	void DestroyWalkableMountainTestWorld(UWorld* World)
	{
		if (!World)
		{
			return;
		}
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
	}

	constexpr uint32 SementeDaEncosta = 20260831u;

	/** O comprimento tangencial que um patamar realmente tem, em unidades. */
	float ComprimentoDoPatamar(const AWalkableMountain& Montanha, const FTransform& Patamar)
	{
		const UStaticMesh* Malha = Montanha.GetTrail()
			? Montanha.GetTrail()->GetStaticMesh().Get() : nullptr;
		if (!Malha)
		{
			return 0.0f;
		}

		return static_cast<float>(Patamar.GetScale3D().Y * Malha->GetBoundingBox().GetSize().Y);
	}

	/** A espessura que um patamar realmente tem, em unidades. */
	float EspessuraDoPatamar(const AWalkableMountain& Montanha, const FTransform& Patamar)
	{
		const UStaticMesh* Malha = Montanha.GetTrail()
			? Montanha.GetTrail()->GetStaticMesh().Get() : nullptr;
		if (!Malha)
		{
			return 0.0f;
		}

		return static_cast<float>(Patamar.GetScale3D().Z * Malha->GetBoundingBox().GetSize().Z);
	}
}

/**
 * O padrão que já custou três defeitos a este projeto: componente criado,
 * asset nunca atribuído — passa em toda verificação de lógica e não existe na
 * tela. O que se afirma aqui é a ATRIBUIÇÃO.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWalkableMountainAssignsMeshesTest,
	"BattleSquare.WalkableMountain.AssignsMeshes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWalkableMountainAssignsMeshesTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateWalkableMountainTestWorld();
	AWalkableMountain* Montanha = World->SpawnActor<AWalkableMountain>();

	TestNotNull(TEXT("a montanha nasce"), Montanha);
	TestNotNull(TEXT("o corpo existe"), Montanha->GetBody());
	TestNotNull(TEXT("a trilha existe"), Montanha->GetTrail());
	TestNotNull(TEXT("o corpo tem malha atribuída"),
		Montanha->GetBody()->GetStaticMesh().Get());
	TestNotNull(TEXT("a trilha tem malha atribuída"),
		Montanha->GetTrail()->GetStaticMesh().Get());

	DestroyWalkableMountainTestWorld(World);
	return true;
}

/**
 * Bloquear é o que separa esta montanha da serra do horizonte.
 *
 * E colisão LIGADA não basta: o corpo precisa de geometria de colisão de
 * verdade, senão o bloqueio é nominal e o jogador atravessa a rocha.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWalkableMountainBlocksTest,
	"BattleSquare.WalkableMountain.Blocks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWalkableMountainBlocksTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateWalkableMountainTestWorld();
	AWalkableMountain* Montanha = World->SpawnActor<AWalkableMountain>();
	Montanha->BuildMountain(SementeDaEncosta);

	TestTrue(TEXT("o corpo colide"),
		Montanha->GetBody()->GetCollisionEnabled() == ECollisionEnabled::QueryAndPhysics);
	TestTrue(TEXT("o corpo bloqueia o pino do jogador"),
		Montanha->GetBody()->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Block);
	TestTrue(TEXT("a trilha colide"),
		Montanha->GetTrail()->GetCollisionEnabled() == ECollisionEnabled::QueryAndPhysics);
	TestTrue(TEXT("a trilha bloqueia o pino do jogador"),
		Montanha->GetTrail()->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Block);

	UStaticMesh* Corpo = Montanha->GetBody()->GetStaticMesh().Get();
	UBodySetup* Setup = Corpo ? Corpo->GetBodySetup() : nullptr;
	TestNotNull(TEXT("o corpo tem corpo de colisão"), Setup);
	if (Setup)
	{
		TestTrue(TEXT("o corpo tem geometria de colisão, não só a permissão"),
			Setup->AggGeom.GetElementCount() > 0);
	}

	UStaticMesh* Patamar = Montanha->GetTrail()->GetStaticMesh().Get();
	UBodySetup* SetupDoPatamar = Patamar ? Patamar->GetBodySetup() : nullptr;
	TestNotNull(TEXT("o patamar tem corpo de colisão"), SetupDoPatamar);
	if (SetupDoPatamar)
	{
		TestTrue(TEXT("o patamar tem geometria de colisão"),
			SetupDoPatamar->AggGeom.GetElementCount() > 0);
	}

	DestroyWalkableMountainTestWorld(World);
	return true;
}

/**
 * A razão de a trilha existir: cada degrau tem que caber no passo.
 *
 * Um degrau acima de `MaxStepHeight` não deixa a subida mais difícil — ele a
 * interrompe, e a montanha volta a ser parede a partir dali. Este é o teste
 * que quebra quando alguém dobrar a altura do patamar "para subir mais rápido".
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWalkableMountainTrailIsClimbableTest,
	"BattleSquare.WalkableMountain.TrailIsClimbable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWalkableMountainTrailIsClimbableTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateWalkableMountainTestWorld();
	AWalkableMountain* Montanha = World->SpawnActor<AWalkableMountain>();
	Montanha->BuildMountain(SementeDaEncosta);

	const TArray<FTransform>& Patamares = Montanha->GetTrailSteps();
	TestTrue(TEXT("a trilha tem patamares"), Patamares.Num() > 2);

	float MaiorDegrau = 0.0f;
	bool bSempreSobe = true;
	for (int32 Passo = 1; Passo < Patamares.Num(); ++Passo)
	{
		const float Degrau = static_cast<float>(
			Patamares[Passo].GetLocation().Z - Patamares[Passo - 1].GetLocation().Z);
		MaiorDegrau = FMath::Max(MaiorDegrau, Degrau);
		bSempreSobe &= (Degrau >= 0.0f);
	}

	TestTrue(TEXT("nenhum degrau passa do passo da engine"),
		MaiorDegrau <= AWalkableMountain::MaxWalkableRiseUnits);
	TestTrue(TEXT("a trilha nunca desce no meio da subida"), bSempreSobe);

	TestEqual(TEXT("a trilha tem uma instância por patamar"),
		Montanha->GetTrail()->GetInstanceCount(), Patamares.Num());

	DestroyWalkableMountainTestWorld(World);
	return true;
}

/**
 * Dois patamares que não se alcançam deixam uma fresta, e a fresta numa
 * espiral é por onde se cai de volta ao chão. A contagem de patamares não
 * enxerga isso: só a distância enxerga.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWalkableMountainTrailHasNoGapTest,
	"BattleSquare.WalkableMountain.TrailHasNoGap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWalkableMountainTrailHasNoGapTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateWalkableMountainTestWorld();
	AWalkableMountain* Montanha = World->SpawnActor<AWalkableMountain>();
	Montanha->BuildMountain(SementeDaEncosta);

	const TArray<FTransform>& Patamares = Montanha->GetTrailSteps();
	const int32 Cume = Montanha->GetSummitStepIndex();

	int32 Frestas = 0;
	for (int32 Passo = 1; Passo < Cume; ++Passo)
	{
		const float Distancia = static_cast<float>(FVector::Dist2D(
			Patamares[Passo].GetLocation(), Patamares[Passo - 1].GetLocation()));
		const float Alcance =
			0.5f * ComprimentoDoPatamar(*Montanha, Patamares[Passo])
			+ 0.5f * ComprimentoDoPatamar(*Montanha, Patamares[Passo - 1]);

		if (Distancia > Alcance)
		{
			++Frestas;
		}
	}

	TestEqual(TEXT("nenhum vão entre patamares consecutivos"), Frestas, 0);

	DestroyWalkableMountainTestWorld(World);
	return true;
}

/**
 * A trilha é BEIRADA: ela mora no raio que a rocha tem naquela altura.
 *
 * Solta do cone, ela vira ponte flutuante; enterrada, some. O teste mede
 * contra o raio do próprio ator, e não contra um número transcrito — cópia de
 * fórmula concorda com o original até a primeira edição (L-032/L-033).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWalkableMountainTrailHugsTheRockTest,
	"BattleSquare.WalkableMountain.TrailHugsTheRock",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWalkableMountainTrailHugsTheRockTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateWalkableMountainTestWorld();
	AWalkableMountain* Montanha = World->SpawnActor<AWalkableMountain>();
	Montanha->BuildMountain(SementeDaEncosta);

	const TArray<FTransform>& Patamares = Montanha->GetTrailSteps();
	const int32 Cume = Montanha->GetSummitStepIndex();

	float MaiorAfastamento = 0.0f;
	for (int32 Passo = 0; Passo < Cume; ++Passo)
	{
		const FVector Onde = Patamares[Passo].GetLocation();
		const float RaioDoPatamar = static_cast<float>(FVector2D(Onde.X, Onde.Y).Size());
		// A altura guardada é o centro do patamar; a superfície fica meia
		// espessura acima, e é dela que sai o raio da rocha.
		const float AlturaDaSuperficie = static_cast<float>(Onde.Z)
			+ 0.5f * EspessuraDoPatamar(*Montanha, Patamares[Passo]);
		const float RaioDaRocha = Montanha->RadiusAtHeight(AlturaDaSuperficie);

		MaiorAfastamento = FMath::Max(MaiorAfastamento,
			FMath::Abs(RaioDoPatamar - RaioDaRocha));
	}

	// A tolerância é o meio patamar: mais que isso e a beirada deixou de
	// encostar na rocha.
	TestTrue(TEXT("todo patamar encosta na encosta"), MaiorAfastamento <= 130.0f);

	DestroyWalkableMountainTestWorld(World);
	return true;
}

/**
 * Trilha que termina onde não se pode parar não terminou em lugar nenhum: a
 * ponta do cone não se pisa.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWalkableMountainSummitIsAPlatformTest,
	"BattleSquare.WalkableMountain.SummitIsAPlatform",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWalkableMountainSummitIsAPlatformTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateWalkableMountainTestWorld();
	AWalkableMountain* Montanha = World->SpawnActor<AWalkableMountain>();
	Montanha->BuildMountain(SementeDaEncosta);

	const TArray<FTransform>& Patamares = Montanha->GetTrailSteps();
	const int32 Cume = Montanha->GetSummitStepIndex();
	TestTrue(TEXT("existe um patamar de cume"), Cume > 0);

	const FVector Onde = Patamares[Cume].GetLocation();
	TestTrue(TEXT("o cume fica no eixo da montanha"),
		FVector2D(Onde.X, Onde.Y).Size() < 1.0);

	// A trilha precisa chegar ALTO. Uma que parasse no meio passaria em todos
	// os testes de degrau e mesmo assim não levaria a lugar nenhum.
	TestTrue(TEXT("o cume fica na parte de cima da montanha"),
		Onde.Z >= 0.8 * Montanha->GetSummitUnits());

	DestroyWalkableMountainTestWorld(World);
	return true;
}

/**
 * Mesma semente, mesma montanha — a ilha não pode mudar de forma entre duas
 * partidas.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWalkableMountainIsDeterministicTest,
	"BattleSquare.WalkableMountain.IsDeterministic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWalkableMountainIsDeterministicTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateWalkableMountainTestWorld();

	AWalkableMountain* Primeira = World->SpawnActor<AWalkableMountain>();
	Primeira->BuildMountain(SementeDaEncosta);
	const TArray<FTransform> Referencia = Primeira->GetTrailSteps();

	AWalkableMountain* Segunda = World->SpawnActor<AWalkableMountain>();
	Segunda->BuildMountain(SementeDaEncosta);

	TestEqual(TEXT("mesmo número de patamares"),
		Segunda->GetTrailSteps().Num(), Referencia.Num());

	bool bIguais = true;
	for (int32 Passo = 0; Passo < Referencia.Num() && Passo < Segunda->GetTrailSteps().Num(); ++Passo)
	{
		bIguais &= Segunda->GetTrailSteps()[Passo].GetLocation().Equals(
			Referencia[Passo].GetLocation(), 0.01);
	}
	TestTrue(TEXT("os patamares caem no mesmo lugar"), bIguais);

	AWalkableMountain* Outra = World->SpawnActor<AWalkableMountain>();
	Outra->BuildMountain(SementeDaEncosta + 1u);

	bool bAlgoMudou = false;
	for (int32 Passo = 0; Passo < Referencia.Num() && Passo < Outra->GetTrailSteps().Num(); ++Passo)
	{
		bAlgoMudou |= !Outra->GetTrailSteps()[Passo].GetLocation().Equals(
			Referencia[Passo].GetLocation(), 0.01);
	}
	TestTrue(TEXT("semente diferente serpenteia diferente"), bAlgoMudou);

	DestroyWalkableMountainTestWorld(World);
	return true;
}

// Copyright 2026 Anderson. All Rights Reserved.

#include "Environment/Volcano.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/AutomationTest.h"

namespace
{
	// Nome próprio (L-042): helper homônimo em outro arquivo de teste vira
	// sobrecarga ambígua quando o unity build junta os dois.
	UWorld* CreateVolcanoTestWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
		FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
		WorldContext.SetCurrentWorld(World);
		World->InitializeActorsForPlay(FURL());
		return World;
	}

	void DestroyVolcanoTestWorld(UWorld* World)
	{
		if (!World)
		{
			return;
		}
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
	}

	constexpr uint32 SementeDaCratera = 20260830u;
	constexpr uint32 OutraSementeDaCratera = 71717171u;

	/** O tamanho real que uma instância ocupa num eixo, já com a escala. */
	float MedidaDaInstancia(
		const UStaticMeshComponent* Componente, const FTransform& Onde, int32 Eixo)
	{
		const UStaticMesh* Malha = Componente ? Componente->GetStaticMesh().Get() : nullptr;
		if (!Malha)
		{
			return 0.0f;
		}

		const FVector Tamanho = Malha->GetBoundingBox().GetSize();
		return static_cast<float>(Onde.GetScale3D()[Eixo] * Tamanho[Eixo]);
	}

	bool EstaPintadoDeCenario(const UPrimitiveComponent* Componente)
	{
		return Componente && Cast<UMaterialInstanceDynamic>(Componente->GetMaterial(0)) != nullptr;
	}
}

/**
 * O padrão que já custou três defeitos a este projeto: componente criado,
 * asset nunca atribuído — passa em toda verificação de lógica e não existe na
 * tela. O que se afirma aqui é a ATRIBUIÇÃO, e a tinta em cima dela.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FVolcanoAssignsMeshesAndPaintTest,
	"BattleSquare.Volcano.AssignsMeshesAndPaint",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVolcanoAssignsMeshesAndPaintTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateVolcanoTestWorld();
	AVolcano* Vulcao = World->SpawnActor<AVolcano>();

	TestNotNull(TEXT("o vulcão nasce"), Vulcao);
	TestNotNull(TEXT("a encosta tem malha atribuída"),
		Vulcao->GetSlopes()->GetStaticMesh().Get());
	TestNotNull(TEXT("o anel da cratera tem malha atribuída"),
		Vulcao->GetRim()->GetStaticMesh().Get());
	TestNotNull(TEXT("a lava tem malha atribuída"), Vulcao->GetLava()->GetStaticMesh().Get());
	TestNotNull(TEXT("os derrames têm malha atribuída"),
		Vulcao->GetFlows()->GetStaticMesh().Get());

	Vulcao->BuildVolcano(SementeDaCratera);

	TestTrue(TEXT("a encosta está pintada de rocha vulcânica"),
		EstaPintadoDeCenario(Vulcao->GetSlopes()));
	TestTrue(TEXT("o anel está pintado de rocha vulcânica"),
		EstaPintadoDeCenario(Vulcao->GetRim()));
	TestTrue(TEXT("a lava está pintada de lava"), EstaPintadoDeCenario(Vulcao->GetLava()));
	TestTrue(TEXT("os derrames estão pintados de lava"),
		EstaPintadoDeCenario(Vulcao->GetFlows()));

	DestroyVolcanoTestWorld(World);
	return true;
}

/**
 * A cratera é o que separa um vulcão de um morro preto.
 *
 * Um cone termina em ponta, e no cume o raio dele é zero. Aqui o cume tem raio
 * POSITIVO, e é esse número que é o poço. Se um dia alguém trocar a pilha de
 * discos por um cone "para simplificar", é este teste que reclama.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FVolcanoKeepsAnOpenCraterTest,
	"BattleSquare.Volcano.KeepsAnOpenCrater",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVolcanoKeepsAnOpenCraterTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateVolcanoTestWorld();
	AVolcano* Vulcao = World->SpawnActor<AVolcano>();

	const float Altura = Vulcao->GetHeightUnits();
	const float Base = Vulcao->GetBaseRadiusUnits();
	const float Poco = Vulcao->GetCraterRadiusUnits();

	TestTrue(TEXT("o poço da cratera tem raio positivo"), Poco > 0.0f);
	TestTrue(TEXT("o poço é menor que a base"), Poco < Base);
	TestEqual(TEXT("no chão o raio é o da base"), Vulcao->RadiusAtHeight(0.0f), Base);
	TestEqual(TEXT("no cume o raio é o do poço"), Vulcao->RadiusAtHeight(Altura), Poco);

	float Anterior = Base + 1.0f;
	for (int32 Amostra = 0; Amostra <= 10; ++Amostra)
	{
		const float Z = Altura * static_cast<float>(Amostra) / 10.0f;
		const float Agora = Vulcao->RadiusAtHeight(Z);
		TestTrue(TEXT("a encosta só afina para cima"), Agora < Anterior);
		Anterior = Agora;
	}

	DestroyVolcanoTestWorld(World);
	return true;
}

/**
 * A lava fica CONTIDA: abaixo da borda e sem encostar nela.
 *
 * Lava no nível do cume lê como uma tampa laranja, e lava do tamanho exato do
 * poço encosta no anel e some dentro dele. As duas falhas apagam a cratera.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FVolcanoHoldsLavaBelowTheRimTest,
	"BattleSquare.Volcano.HoldsLavaBelowTheRim",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVolcanoHoldsLavaBelowTheRimTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateVolcanoTestWorld();
	AVolcano* Vulcao = World->SpawnActor<AVolcano>();
	Vulcao->BuildVolcano(SementeDaCratera);

	const float Altura = Vulcao->GetHeightUnits();
	const float Superficie = Vulcao->GetLavaSurfaceUnits();

	TestTrue(TEXT("a lava está acima do chão"), Superficie > 0.0f);
	TestTrue(TEXT("a lava está abaixo do cume"), Superficie < Altura);

	const FTransform DaLava = Vulcao->GetLava()->GetRelativeTransform();
	const float Espessura = MedidaDaInstancia(Vulcao->GetLava(), DaLava, 2);
	const float TopoDaLava = static_cast<float>(DaLava.GetLocation().Z) + 0.5f * Espessura;

	TestTrue(TEXT("a lava tem espessura"), Espessura > 0.0f);
	TestTrue(TEXT("a face de cima da lava é a superfície da lava"),
		FMath::IsNearlyEqual(TopoDaLava, Superficie, 1.0f));

	const float RaioDaLava = 0.5f * MedidaDaInstancia(Vulcao->GetLava(), DaLava, 0);
	TestTrue(TEXT("a lava cabe dentro do poço sem encostar no anel"),
		RaioDaLava > 0.0f && RaioDaLava < Vulcao->GetCraterRadiusUnits());

	// A encosta é sólida: se ela subisse até o cume, entupiria o poço e a lava
	// ficaria enterrada dentro da própria montanha.
	float TopoDaEncosta = 0.0f;
	for (int32 Camada = 0; Camada < Vulcao->GetSlopes()->GetInstanceCount(); ++Camada)
	{
		FTransform Disco;
		Vulcao->GetSlopes()->GetInstanceTransform(Camada, Disco, /*bWorldSpace=*/false);
		const float Topo = static_cast<float>(Disco.GetLocation().Z)
			+ 0.5f * MedidaDaInstancia(Vulcao->GetSlopes(), Disco, 2);
		TopoDaEncosta = FMath::Max(TopoDaEncosta, Topo);
	}

	TestTrue(TEXT("a encosta sólida para na superfície da lava"),
		FMath::IsNearlyEqual(TopoDaEncosta, Superficie, 1.0f));

	DestroyVolcanoTestWorld(World);
	return true;
}

/** O anel fecha em volta do poço, todo na mesma altura e todo no mesmo raio. */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FVolcanoRingClosesAroundTheCraterTest,
	"BattleSquare.Volcano.RingClosesAroundTheCrater",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVolcanoRingClosesAroundTheCraterTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateVolcanoTestWorld();
	AVolcano* Vulcao = World->SpawnActor<AVolcano>();
	Vulcao->BuildVolcano(SementeDaCratera);

	const TArray<FTransform>& Anel = Vulcao->GetRimBlocks();
	TestTrue(TEXT("o anel tem blocos"), Anel.Num() > 0);

	// Bloco guardado na lista mas nunca somado ao HISM e um anel que existe
	// so na memoria: a cratera fica aberta na tela e todo teste de geometria
	// continua passando, porque ele mede a lista.
	TestEqual(TEXT("todo bloco guardado tambem foi desenhado"),
		Vulcao->GetRim()->GetInstanceCount(), Anel.Num());

	if (Anel.Num() == 0)
	{
		DestroyVolcanoTestWorld(World);
		return false;
	}

	const float Poco = Vulcao->GetCraterRadiusUnits();
	const float ZDoPrimeiro = static_cast<float>(Anel[0].GetLocation().Z);
	const float RaioDoPrimeiro = static_cast<float>(
		FVector2D(Anel[0].GetLocation().X, Anel[0].GetLocation().Y).Size());

	for (const FTransform& Pedra : Anel)
	{
		TestTrue(TEXT("todo bloco do anel está na mesma altura"),
			FMath::IsNearlyEqual(static_cast<float>(Pedra.GetLocation().Z), ZDoPrimeiro, 1.0f));

		const float Raio = static_cast<float>(
			FVector2D(Pedra.GetLocation().X, Pedra.GetLocation().Y).Size());
		TestTrue(TEXT("todo bloco do anel está no mesmo raio"),
			FMath::IsNearlyEqual(Raio, RaioDoPrimeiro, 1.0f));
		TestTrue(TEXT("o anel fica por fora do poço"), Raio > Poco);
	}

	// A soma dos comprimentos passa da volta inteira: anel que só encosta deixa
	// fresta, e fresta no anel é lava escorrendo pela lateral do vulcão.
	const float Comprimento = MedidaDaInstancia(Vulcao->GetRim(), Anel[0], 1);
	TestTrue(TEXT("os blocos se sobrepõem em vez de só se encostarem"),
		Comprimento * static_cast<float>(Anel.Num()) > 2.0f * PI * RaioDoPrimeiro);

	DestroyVolcanoTestWorld(World);
	return true;
}

/**
 * Os derrames descem, e descem diferente a cada semente.
 *
 * Semente igual dando desenho diferente quebraria a promessa de o mundo ser o
 * mesmo em toda máquina; semente diferente dando o mesmo desenho faria todo
 * vulcão da ilha ser o mesmo vulcão copiado.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FVolcanoPoursSeededFlowsTest,
	"BattleSquare.Volcano.PoursSeededFlows",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVolcanoPoursSeededFlowsTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateVolcanoTestWorld();
	AVolcano* Primeiro = World->SpawnActor<AVolcano>();
	AVolcano* Igual = World->SpawnActor<AVolcano>();
	AVolcano* Outro = World->SpawnActor<AVolcano>();

	Primeiro->BuildVolcano(SementeDaCratera);
	Igual->BuildVolcano(SementeDaCratera);
	Outro->BuildVolcano(OutraSementeDaCratera);

	const TArray<FTransform>& Derrames = Primeiro->GetFlowSteps();
	TestTrue(TEXT("há derrame na encosta"), Derrames.Num() > 0);
	TestEqual(TEXT("todo pedaço de derrame também foi desenhado"),
		Primeiro->GetFlows()->GetInstanceCount(), Derrames.Num());
	TestEqual(TEXT("a mesma semente dá o mesmo número de pedaços"),
		Igual->GetFlowSteps().Num(), Derrames.Num());

	bool bTodosIguais = true;
	bool bAlgumDiferente = false;
	for (int32 Indice = 0; Indice < Derrames.Num(); ++Indice)
	{
		if (!Igual->GetFlowSteps()[Indice].GetLocation().Equals(
				Derrames[Indice].GetLocation(), 0.1))
		{
			bTodosIguais = false;
		}
		if (Indice < Outro->GetFlowSteps().Num()
			&& !Outro->GetFlowSteps()[Indice].GetLocation().Equals(
				Derrames[Indice].GetLocation(), 0.1))
		{
			bAlgumDiferente = true;
		}
	}

	TestTrue(TEXT("a mesma semente dá o mesmo derrame"), bTodosIguais);
	TestTrue(TEXT("semente diferente dá derrame diferente"), bAlgumDiferente);

	// Lava sobe é lava errada: dentro de um derrame, cada pedaço fica abaixo do
	// anterior. É o que garante que o passo zero é a boca e não o pé.
	const int32 Pedacos = Primeiro->GetFlowSteps().Num() / FMath::Max(1, Primeiro->GetFlowCount());
	for (int32 Derrame = 0; Derrame < Primeiro->GetFlowCount(); ++Derrame)
	{
		for (int32 Passo = 1; Passo < Pedacos; ++Passo)
		{
			const int32 Indice = Derrame * Pedacos + Passo;
			if (Indice >= Derrames.Num())
			{
				break;
			}
			TestTrue(TEXT("o derrame desce a encosta"),
				Derrames[Indice].GetLocation().Z < Derrames[Indice - 1].GetLocation().Z);
		}
	}

	DestroyVolcanoTestWorld(World);
	return true;
}

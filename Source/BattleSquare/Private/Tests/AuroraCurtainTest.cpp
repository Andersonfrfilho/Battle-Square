// Copyright 2026 Anderson. All Rights Reserved.

#include "Environment/AuroraCurtain.h"
#include "IslandBiomeTestHelper.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Environment/IslandGeography.h"
#include "Environment/ScenaryPalette.h"
#include "Environment/WorldNightSky.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	// Nome próprio (L-042): helper homônimo em outro arquivo de teste vira
	// sobrecarga ambígua quando o unity build junta os dois.
	UWorld* CreateAuroraTestWorld()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
		FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
		WorldContext.SetCurrentWorld(World);
		World->InitializeActorsForPlay(FURL());
		return World;
	}

	void DestroyAuroraTestWorld(UWorld* World)
	{
		if (!World)
		{
			return;
		}
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
	}

	constexpr uint32 SementeDaAurora = 20260831u;
	constexpr uint32 OutraSementeDaAurora = 44553311u;

	/** A cor que a faixa está REALMENTE mostrando, e não a que se pediu. */
	FLinearColor CorPintadaNaAurora(const UPrimitiveComponent* Faixa)
	{
		const UMaterialInstanceDynamic* Tinta =
			Faixa ? Cast<UMaterialInstanceDynamic>(Faixa->GetMaterial(0)) : nullptr;

		return Tinta
			? const_cast<UMaterialInstanceDynamic*>(Tinta)->K2_GetVectorParameterValue(
				TEXT("Color"))
			: FLinearColor::Black;
	}

	/** O maior e o menor Z entre os pedaços de uma faixa. */
	void FaixaDaAuroraEmZ(const TArray<FTransform>& Pedacos, float& MaisBaixo, float& MaisAlto)
	{
		MaisBaixo = TNumericLimits<float>::Max();
		MaisAlto = -TNumericLimits<float>::Max();

		for (const FTransform& Pedaco : Pedacos)
		{
			const float Z = static_cast<float>(Pedaco.GetLocation().Z);
			MaisBaixo = FMath::Min(MaisBaixo, Z);
			MaisAlto = FMath::Max(MaisAlto, Z);
		}
	}
}

/**
 * O padrão que já custou três defeitos a este projeto: componente criado, asset
 * nunca atribuído — passa em toda verificação de lógica e não existe na tela.
 * O que se afirma aqui é a ATRIBUIÇÃO, e a tinta em cima dela.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAuroraCurtainAssignsMeshAndPaintTest,
	"BattleSquare.Environment.AuroraCurtain.AtribuiMalhaEPintura",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAuroraCurtainAssignsMeshAndPaintTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateAuroraTestWorld();
	AAuroraCurtain* Cortina = World->SpawnActor<AAuroraCurtain>();

	TestNotNull(TEXT("a cortina nasce"), Cortina);
	TestNotNull(TEXT("o verde tem malha atribuída"), Cortina->GetVeil()->GetStaticMesh().Get());
	TestNotNull(TEXT("o roxo tem malha atribuída"), Cortina->GetCrown()->GetStaticMesh().Get());
	TestNotNull(TEXT("a cortina tem luz"), Cortina->GetGlow());

	Cortina->BuildCurtain(SementeDaAurora);
	Cortina->SetStrength(1.0f);

	TestEqual(TEXT("o verde está pintado com o verde da paleta"),
		CorPintadaNaAurora(Cortina->GetVeil()),
		ScenaryPalette::ColorFor(EScenaryRole::AuroraVeil, NAME_None));
	TestEqual(TEXT("o roxo está pintado com o roxo da paleta"),
		CorPintadaNaAurora(Cortina->GetCrown()),
		ScenaryPalette::ColorFor(EScenaryRole::AuroraCrown, NAME_None));

	// Duas matizes, e não a mesma duas vezes: sem topo de cor diferente a
	// cortina é um retângulo verde chapado, que não pende de nada.
	TestNotEqual(TEXT("o alto da cortina não é da cor da base"),
		CorPintadaNaAurora(Cortina->GetCrown()), CorPintadaNaAurora(Cortina->GetVeil()));

	DestroyAuroraTestWorld(World);
	return true;
}

/**
 * A cortina CURVA e ONDULA.
 *
 * Uma faixa reta no céu lê como parede pintada — foi essa a reclamação que
 * derrubou a primeira geração de montanhas. O que prova a curva é o rumo dos
 * pedaços variar; o que prova a onda é o raio variar.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAuroraCurtainHangsCurvedRibbonsTest,
	"BattleSquare.Environment.AuroraCurtain.PenduraFitasCurvas",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAuroraCurtainHangsCurvedRibbonsTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateAuroraTestWorld();
	AAuroraCurtain* Cortina = World->SpawnActor<AAuroraCurtain>();
	Cortina->BuildCurtain(SementeDaAurora);

	const TArray<FTransform>& Verde = Cortina->GetVeilSegments();
	const TArray<FTransform>& Roxo = Cortina->GetCrownSegments();

	TestTrue(TEXT("o verde tem pedaços"), Verde.Num() > 0);
	TestTrue(TEXT("o roxo tem pedaços"), Roxo.Num() > 0);

	// Pedaço guardado na lista mas nunca somado ao HISM é cortina que existe só
	// na memória: toda medida de geometria continua passando, porque ela mede a
	// lista, e o céu fica vazio.
	TestEqual(TEXT("todo pedaço do verde também foi desenhado"),
		Cortina->GetVeil()->GetInstanceCount(), Verde.Num());
	TestEqual(TEXT("todo pedaço do roxo também foi desenhado"),
		Cortina->GetCrown()->GetInstanceCount(), Roxo.Num());
	TestEqual(TEXT("as duas faixas têm o mesmo recorte"), Roxo.Num(), Verde.Num());

	if (Verde.Num() == 0)
	{
		DestroyAuroraTestWorld(World);
		return false;
	}

	float MenorRumo = TNumericLimits<float>::Max();
	float MaiorRumo = -TNumericLimits<float>::Max();
	float MenorRaio = TNumericLimits<float>::Max();
	float MaiorRaio = 0.0f;

	for (const FTransform& Pedaco : Verde)
	{
		const float Rumo = static_cast<float>(Pedaco.GetRotation().Rotator().Yaw);
		MenorRumo = FMath::Min(MenorRumo, Rumo);
		MaiorRumo = FMath::Max(MaiorRumo, Rumo);

		const float Raio = static_cast<float>(
			FVector2D(Pedaco.GetLocation().X, Pedaco.GetLocation().Y).Size());
		MenorRaio = FMath::Min(MenorRaio, Raio);
		MaiorRaio = FMath::Max(MaiorRaio, Raio);
	}

	TestTrue(TEXT("o rumo dos pedaços varia, então a fita não é reta"),
		MaiorRumo - MenorRumo > 10.0f);
	TestTrue(TEXT("o raio varia, então a fita serpenteia"), MaiorRaio - MenorRaio > 100.0f);

	// Espessura contra comprimento: um pedaço tão grosso quanto longo é um
	// bloco, e uma fila de blocos não é cortina.
	TestTrue(TEXT("o pedaço é mais longo que grosso"),
		Verde[0].GetScale3D().Y > Verde[0].GetScale3D().X);

	DestroyAuroraTestWorld(World);
	return true;
}

/**
 * O roxo fica em cima do verde, e os dois se encostam.
 *
 * Folga entre as faixas apareceria como uma linha preta atravessando a cortina
 * no céu; sobreposição faria o roxo cobrir o verde e apagar o degradê.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAuroraCurtainStacksCrownAboveVeilTest,
	"BattleSquare.Environment.AuroraCurtain.RoxoAcimaDoVerde",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAuroraCurtainStacksCrownAboveVeilTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateAuroraTestWorld();
	AAuroraCurtain* Cortina = World->SpawnActor<AAuroraCurtain>();
	Cortina->BuildCurtain(SementeDaAurora);

	float BaseDoVerde = 0.0f;
	float TopoDoVerde = 0.0f;
	float BaseDoRoxo = 0.0f;
	float TopoDoRoxo = 0.0f;
	FaixaDaAuroraEmZ(Cortina->GetVeilSegments(), BaseDoVerde, TopoDoVerde);
	FaixaDaAuroraEmZ(Cortina->GetCrownSegments(), BaseDoRoxo, TopoDoRoxo);

	TestTrue(TEXT("o roxo está acima do verde"), BaseDoRoxo > TopoDoVerde - 1.0f);
	TestTrue(TEXT("as duas faixas estão cada uma na sua altura"),
		FMath::IsNearlyEqual(BaseDoVerde, TopoDoVerde, 1.0f)
			&& FMath::IsNearlyEqual(BaseDoRoxo, TopoDoRoxo, 1.0f));

	// A cortina precisa passar bem por cima do maior cume da ilha. Aurora que
	// passa rasante por uma montanha lê como neblina presa no morro.
	TestTrue(TEXT("a cortina começa muito acima do chão"),
		Cortina->GetAltitudeUnits() > 5000.0f);
	TestTrue(TEXT("o topo da cortina está acima da base dela"),
		Cortina->GetTopUnits() > Cortina->GetAltitudeUnits());

	// A luz mora na BASE: no meio, metade do alcance se gasta iluminando o céu
	// vazio acima da cortina.
	TestEqual(TEXT("a luz mora na base da cortina"),
		static_cast<float>(Cortina->GetGlow()->GetRelativeLocation().Z),
		Cortina->GetAltitudeUnits());
	TestTrue(TEXT("a luz alcança para além da altura da cortina"),
		Cortina->GetGlow()->AttenuationRadius > Cortina->GetAltitudeUnits());

	DestroyAuroraTestWorld(World);
	return true;
}

/**
 * A cortina fica sobre a GELEIRA, e a geleira é fria.
 *
 * As duas metades importam. Escrever 252° aqui faria a aurora ficar onde a
 * geleira estava, no dia em que alguém mexesse nos setores; e uma geleira que
 * deixasse de ser de clima frio apagaria a aurora para sempre, em silêncio,
 * porque `AuroraStrength` só acende no frio.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAuroraCurtainSitsOverTheGlacierTest,
	"BattleSquare.Environment.AuroraCurtain.FicaSobreAGeleira",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAuroraCurtainSitsOverTheGlacierTest::RunTest(const FString& Parameters)
{
	// UMA ILHA, UM BIOMA: "o ponto na geleira" deixou de ser uma direção e
	// passou a ser uma ILHA de geleira. O teste continua querendo geleira —
	// ele só pede de outro jeito.
	const IlhaDeTeste::FBiomaTemporario IlhaDeGeleira(TEXT("Glacier"));

	const FVector2D Centro = AAuroraCurtain::SkyCenterUnits();

	TestTrue(TEXT("o centro da aurora está em terra"), IslandGeography::IsOnLand(Centro));
	TestEqual(TEXT("o centro da aurora cai no setor da geleira"),
		static_cast<int32>(IslandGeography::BiomeAt(Centro)),
		static_cast<int32>(EIslandBiome::Glacier));

	// Meia-noite: o mais escuro que a noite fica.
	TestTrue(TEXT("a geleira é fria, então há aurora para acender"),
		WorldNightSky::AuroraStrength(
			IslandGeography::ClimateOf(EIslandBiome::Glacier), 0.0f) > 0.0f);
	TestEqual(TEXT("ao meio-dia não há aurora nenhuma"),
		WorldNightSky::AuroraStrength(
			IslandGeography::ClimateOf(EIslandBiome::Glacier), 12.0f),
		0.0f);

	return true;
}

/**
 * Ela ACENDE e APAGA com a noite, e a matiz nunca muda.
 *
 * A paleta pinta cor, não brilho, então a modulação é a QUANTIDADE do mesmo
 * verde. Escolher um segundo verde para o meio-brilho daria duas auroras de
 * cores diferentes no dia em que alguém editasse uma (L-032). E abaixo do
 * mínimo ela desaparece em vez de escurecer: mancha verde no céu ao meio-dia é
 * pior que aurora nenhuma.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAuroraCurtainFadesWithTheNightTest,
	"BattleSquare.Environment.AuroraCurtain.AcendeEApagaComANoite",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAuroraCurtainFadesWithTheNightTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateAuroraTestWorld();
	AAuroraCurtain* Cortina = World->SpawnActor<AAuroraCurtain>();
	Cortina->BuildCurtain(SementeDaAurora);

	Cortina->SetStrength(0.0f);
	TestEqual(TEXT("apagada, a força é zero"), Cortina->GetStrength(), 0.0f);
	TestFalse(TEXT("apagada, o verde não se vê"), Cortina->GetVeil()->IsVisible());
	TestFalse(TEXT("apagada, o roxo não se vê"), Cortina->GetCrown()->IsVisible());
	TestEqual(TEXT("apagada, a luz não acende"), Cortina->GetGlow()->Intensity, 0.0f);

	Cortina->SetStrength(1.0f);
	TestTrue(TEXT("acesa, o verde se vê"), Cortina->GetVeil()->IsVisible());
	TestTrue(TEXT("acesa, o roxo se vê"), Cortina->GetCrown()->IsVisible());
	TestTrue(TEXT("acesa, a luz acende"), Cortina->GetGlow()->Intensity > 0.0f);

	const FLinearColor Cheia = CorPintadaNaAurora(Cortina->GetVeil());

	Cortina->SetStrength(0.4f);
	const FLinearColor Meia = CorPintadaNaAurora(Cortina->GetVeil());

	TestTrue(TEXT("a meio brilho a cor escurece"), Meia.G < Cheia.G);
	TestTrue(TEXT("a meio brilho a matiz é a mesma"),
		FMath::IsNearlyEqual(Meia.G * Cheia.R, Meia.R * Cheia.G, 0.001f)
			&& FMath::IsNearlyEqual(Meia.G * Cheia.B, Meia.B * Cheia.G, 0.001f));

	// Força fora da faixa não é erro do relógio: é o que acontece quando alguém
	// soma dois efeitos. Ela se prende, em vez de estourar a cor.
	Cortina->SetStrength(3.0f);
	TestEqual(TEXT("força acima de um se prende em um"), Cortina->GetStrength(), 1.0f);
	Cortina->SetStrength(-2.0f);
	TestEqual(TEXT("força abaixo de zero se prende em zero"), Cortina->GetStrength(), 0.0f);

	DestroyAuroraTestWorld(World);
	return true;
}

/**
 * A mesma semente dá a mesma cortina; semente diferente dá outra.
 *
 * Sem a primeira metade, a aurora muda de forma entre duas visitas ao mesmo
 * lugar. Sem a segunda, toda aurora do jogo é a mesma cortina copiada.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAuroraCurtainKeepsItsSeedTest,
	"BattleSquare.Environment.AuroraCurtain.GuardaASemente",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAuroraCurtainKeepsItsSeedTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreateAuroraTestWorld();
	AAuroraCurtain* Primeira = World->SpawnActor<AAuroraCurtain>();
	AAuroraCurtain* Igual = World->SpawnActor<AAuroraCurtain>();
	AAuroraCurtain* Outra = World->SpawnActor<AAuroraCurtain>();

	Primeira->BuildCurtain(SementeDaAurora);
	Igual->BuildCurtain(SementeDaAurora);
	Outra->BuildCurtain(OutraSementeDaAurora);

	const TArray<FTransform>& Referencia = Primeira->GetVeilSegments();
	TestEqual(TEXT("a mesma semente dá o mesmo número de pedaços"),
		Igual->GetVeilSegments().Num(), Referencia.Num());

	bool bTodosIguais = true;
	bool bAlgumDiferente = false;
	for (int32 Indice = 0; Indice < Referencia.Num(); ++Indice)
	{
		if (!Igual->GetVeilSegments()[Indice].GetLocation().Equals(
				Referencia[Indice].GetLocation(), 0.1))
		{
			bTodosIguais = false;
		}
		if (Indice < Outra->GetVeilSegments().Num()
			&& !Outra->GetVeilSegments()[Indice].GetLocation().Equals(
				Referencia[Indice].GetLocation(), 0.1))
		{
			bAlgumDiferente = true;
		}
	}

	TestTrue(TEXT("a mesma semente dá a mesma cortina"), bTodosIguais);
	TestTrue(TEXT("semente diferente dá outra cortina"), bAlgumDiferente);

	// Reconstruir não empilha: montar em cima do que já estava dobraria os
	// pedaços a cada vez que o mundo se remontasse.
	const int32 Antes = Primeira->GetVeil()->GetInstanceCount();
	Primeira->BuildCurtain(SementeDaAurora);
	TestEqual(TEXT("remontar não empilha pedaços"),
		Primeira->GetVeil()->GetInstanceCount(), Antes);

	DestroyAuroraTestWorld(World);
	return true;
}

#endif

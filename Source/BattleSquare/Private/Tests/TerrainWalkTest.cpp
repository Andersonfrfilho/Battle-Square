// Copyright 2026 Anderson. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Engine/World.h"
#include "Environment/IslandGeography.h"
#include "World/IslandBakedPlan.h"
#include "World/TerrainMesh.h"

/**
 * O jogador ANDA no relevo — e andar é a prova de que ele é chão.
 *
 * A caminhada é medida sobre a superfície ERGUIDA, não sobre o plano: entre o
 * que o plano calcula e o que a malha desenha cabe justamente o degrau que
 * faria o jogador travar ou cair. Medir no plano seria medir a intenção.
 */

namespace ProvaDaCaminhada
{
	/** Quantos passos do mar ao barranco. Passo fino acha degrau que passo grosso pula. */
	constexpr int32 Passos = 400;

	UWorld* MundoDeTeste()
	{
		return UWorld::CreateWorld(EWorldType::Game, false);
	}

	ATerrainMesh* RelevoErguido(UWorld* Mundo, const UIslandBakedPlan& Assado)
	{
		ATerrainMesh* Relevo = Mundo->SpawnActor<ATerrainMesh>();
		Relevo->BuildFrom(Assado);
		return Relevo;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTerrainWalkHasNoStepTallerThanACellTest,
	"BattleSquare.TerrainWalk.HasNoStepTallerThanACell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTerrainWalkHasNoStepTallerThanACellTest::RunTest(const FString& Parameters)
{
	const UIslandBakedPlan* Assado = IslandBakedPlan::Load();
	if (!Assado)
	{
		AddError(TEXT("o assado nao existe — rode ./Tools/bake_island.sh"));
		return false;
	}

	UWorld* Mundo = ProvaDaCaminhada::MundoDeTeste();
	ATerrainMesh* Relevo = ProvaDaCaminhada::RelevoErguido(Mundo, *Assado);

	// Caminha em VÁRIOS rumos: um rumo só mediria uma fatia da ilha, e o
	// barranco não tem a mesma cara em toda a volta — a rampa é uma fresta.
	constexpr int32 Rumos = 24;
	float PiorDegrau = 0.0f;
	float OndePior = 0.0f;
	int32 RumoDoPior = 0;

	for (int32 Rumo = 0; Rumo < Rumos; ++Rumo)
	{
		const float Angulo = (2.0f * PI * Rumo) / Rumos;
		const FVector2D Direcao(FMath::Cos(Angulo), FMath::Sin(Angulo));

		// Do mar (fora da terra) até o platô, passando pela praia e pelo barranco.
		const float DoMar = IslandGeography::LandRadiusAt(Angulo);
		const float AteODentro = 0.0f;

		float Anterior = Relevo->BuiltHeightAt(Direcao * DoMar);
		for (int32 Passo = 1; Passo <= ProvaDaCaminhada::Passos; ++Passo)
		{
			const float Fracao = static_cast<float>(Passo) / ProvaDaCaminhada::Passos;
			const float Raio = FMath::Lerp(DoMar, AteODentro, Fracao);
			const float Agora = Relevo->BuiltHeightAt(Direcao * Raio);

			const float Degrau = FMath::Abs(Agora - Anterior);
			if (Degrau > PiorDegrau)
			{
				PiorDegrau = Degrau;
				OndePior = Raio;
				RumoDoPior = Rumo;
			}
			Anterior = Agora;
		}
	}

	// O teto é a casa da grade. Degrau maior que ela é parede vertical dentro
	// do chão: o personagem trava nela, ou cai por dentro dela.
	const float Teto = Relevo->GetCellSizeUnits();
	if (PiorDegrau > Teto)
	{
		AddError(FString::Printf(
			TEXT("degrau de %.1f u no rumo %d, raio %.0f — teto e a casa da grade (%.1f u)"),
			PiorDegrau, RumoDoPior, OndePior, Teto));
		Mundo->DestroyWorld(false);
		return false;
	}

	// Degrau zero em toda a volta seria ilha plana, e passaria na linha acima.
	TestTrue(TEXT("a caminhada subiu alguma coisa"), PiorDegrau > 0.0f);

	Mundo->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTerrainWalkClimbsFromTheSeaToThePlateauTest,
	"BattleSquare.TerrainWalk.ClimbsFromTheSeaToThePlateau",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTerrainWalkClimbsFromTheSeaToThePlateauTest::RunTest(const FString& Parameters)
{
	const UIslandBakedPlan* Assado = IslandBakedPlan::Load();
	if (!Assado)
	{
		AddError(TEXT("o assado nao existe — rode ./Tools/bake_island.sh"));
		return false;
	}

	UWorld* Mundo = ProvaDaCaminhada::MundoDeTeste();
	ATerrainMesh* Relevo = ProvaDaCaminhada::RelevoErguido(Mundo, *Assado);

	// A caminhada tem de SUBIR o barranco em ALGUM rumo.
	//
	// Na forma EXISTENCIAL, e medindo o ponto mais alto do percurso — nunca o
	// centro da ilha. Escrevi primeiro "o centro e mais alto que a praia" e o
	// teste reprovou terreno certo: MEDIDO, o centro está a 1.540 e o anel em
	// volta a 5.028. A ilha não é um domo, e eu tinha afirmado que era.
	//
	// Eleger um ponto ("o centro", "o mais perto") é o que faz um teste medir o
	// desempate em vez da propriedade. Aqui não há ponto eleito: basta existir
	// um rumo em que a subida acontece.
	constexpr int32 Rumos = 24;
	float MaiorSubida = 0.0f;

	for (int32 Rumo = 0; Rumo < Rumos; ++Rumo)
	{
		const float Angulo = (2.0f * PI * Rumo) / Rumos;
		const FVector2D Direcao(FMath::Cos(Angulo), FMath::Sin(Angulo));

		const float DoMar = IslandGeography::LandRadiusAt(Angulo);
		const float NaPraia = Relevo->BuiltHeightAt(Direcao * (DoMar - 1.0f));

		float MaisAltoNoPercurso = NaPraia;
		for (int32 Passo = 1; Passo <= ProvaDaCaminhada::Passos; ++Passo)
		{
			const float Raio = FMath::Lerp(DoMar, 0.0f,
				static_cast<float>(Passo) / ProvaDaCaminhada::Passos);
			MaisAltoNoPercurso =
				FMath::Max(MaisAltoNoPercurso, Relevo->BuiltHeightAt(Direcao * Raio));
		}

		MaiorSubida = FMath::Max(MaiorSubida, MaisAltoNoPercurso - NaPraia);
	}

	if (MaiorSubida < IslandGeography::PlateauHeightUnits() * 0.5f)
	{
		AddError(FString::Printf(
			TEXT("a maior subida do mar para dentro foi %.0f u, e o plato tem %.0f u — ")
			TEXT("nenhum rumo sobe o barranco"),
			MaiorSubida, IslandGeography::PlateauHeightUnits()));
		Mundo->DestroyWorld(false);
		return false;
	}

	Mundo->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTerrainWalkSurfaceMatchesTheBakedGridTest,
	"BattleSquare.TerrainWalk.SurfaceMatchesTheBakedGrid",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTerrainWalkSurfaceMatchesTheBakedGridTest::RunTest(const FString& Parameters)
{
	const UIslandBakedPlan* Assado = IslandBakedPlan::Load();
	if (!Assado)
	{
		AddError(TEXT("o assado nao existe — rode ./Tools/bake_island.sh"));
		return false;
	}

	UWorld* Mundo = ProvaDaCaminhada::MundoDeTeste();
	ATerrainMesh* Relevo = ProvaDaCaminhada::RelevoErguido(Mundo, *Assado);

	// Em cima de um vértice, a interpolação tem de devolver o vértice. Um erro
	// de meia casa no índice passaria despercebido em qualquer teste de forma,
	// e poria o mundo inteiro deslocado meio quadrado.
	const float Raio = Assado->LandRadiusUnits;
	const float Casa = Relevo->GetCellSizeUnits();

	for (int32 Linha = 0; Linha < Assado->HeightGridSide; Linha += 13)
	{
		for (int32 Coluna = 0; Coluna < Assado->HeightGridSide; Coluna += 17)
		{
			const FVector2D EmCimaDoVertice(-Raio + Coluna * Casa, -Raio + Linha * Casa);

			TestEqual(*FString::Printf(TEXT("a superficie no vertice (%d,%d)"), Coluna, Linha),
				Relevo->BuiltHeightAt(EmCimaDoVertice),
				Assado->HeightAtCell(Coluna, Linha), 0.5f);
		}
	}

	Mundo->DestroyWorld(false);
	return true;
}

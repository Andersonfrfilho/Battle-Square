// Copyright 2026 Anderson. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Battle/FluidRegistry.h"
#include "Environment/FreshWater.h"
#include "World/IslandBakedPlan.h"
#include "World/WaterFooting.h"

/**
 * O MUNDO DECLARA SEUS FLUIDOS.
 *
 * A ilha tem seis, e até aqui tratava todos como uma água só: quem entra numa
 * fonte termal na saia do vulcão sentia o mesmo que quem atravessa um córrego
 * de montanha. O registro existe para separá-los, e não chegava até aqui.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorldFluidTheThermalWaterIsRecognisedTest,
	"BattleSquare.WorldFluid.TheThermalWaterIsRecognised",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldFluidTheThermalWaterIsRecognisedTest::RunTest(const FString& Parameters)
{
	const UIslandBakedPlan* Assado = IslandBakedPlan::Load();
	if (!Assado)
	{
		AddError(TEXT("o assado nao existe — rode ./Tools/bake_island.sh"));
		return false;
	}

	// O ACEITE DA F6. Procura no traçado um ponto de água que o gerador diz
	// ser termal, e cobra que o mundo o reconheça como termal — não como doce.
	//
	// O ponto não é escrito à mão: ele sai do próprio traçado, e por isso
	// continua certo quando a ilha mudar de tamanho ou o vulcão de lugar.
	FVector2D NoTermal = FVector2D::ZeroVector;
	bool bAchou = false;

	for (const FBakedRiver& Curso : Assado->Rivers)
	{
		for (int32 Ponto = 0; Ponto < Curso.PointsUnits.Num(); ++Ponto)
		{
			if (Curso.FluidByPoint.IsValidIndex(Ponto)
				&& static_cast<EFluidKind>(Curso.FluidByPoint[Ponto])
					== EFluidKind::AguaTermal)
			{
				NoTermal = Curso.PointsUnits[Ponto];
				bAchou = true;
				break;
			}
		}
		if (bAchou)
		{
			break;
		}
	}

	if (!bAchou)
	{
		AddError(TEXT("nenhum ponto de agua termal no assado — ou o tracado ")
			TEXT("deixou de produzir agua termal, ou o assado perdeu o fluido"));
		return false;
	}

	// O gerador concorda: é a prova de que o assado não inventou o termal.
	TestTrue(TEXT("o gerador tambem diz que ali e termal"),
		FreshWater::IsThermalAt(NoTermal));

	TestEqual(TEXT("o mundo reconhece a agua termal"),
		static_cast<int32>(WaterFooting::FluidAt(*Assado, NoTermal)),
		static_cast<int32>(EFluidKind::AguaTermal));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorldFluidTheIslandHasMoreThanOneWaterTest,
	"BattleSquare.WorldFluid.TheIslandHasMoreThanOneWater",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldFluidTheIslandHasMoreThanOneWaterTest::RunTest(const FString& Parameters)
{
	const UIslandBakedPlan* Assado = IslandBakedPlan::Load();
	if (!Assado)
	{
		AddError(TEXT("o assado nao existe — rode ./Tools/bake_island.sh"));
		return false;
	}

	// O CONTRAPESO, e o teste que carrega o peso desta task: se tudo saísse
	// termal, ou tudo doce, a prova acima passaria numa das duas direções e o
	// mundo continuaria com uma água só — que é o estado de onde se partiu.
	TSet<int32> Fluidos;
	for (const FBakedRiver& Curso : Assado->Rivers)
	{
		for (const uint8 Qual : Curso.FluidByPoint)
		{
			Fluidos.Add(Qual);
		}
	}

	if (Fluidos.Num() < 2)
	{
		AddError(FString::Printf(
			TEXT("a agua corrente da ilha tem %d fluido(s) — o mundo continua ")
			TEXT("tratando toda agua como uma so"), Fluidos.Num()));
		return false;
	}

	// E toda água declarada É água: um ponto de rio que saísse como lava seria
	// defeito de mapeamento, e ninguém mais notaria.
	for (const int32 Qual : Fluidos)
	{
		TestTrue(*FString::Printf(TEXT("o fluido %d de um rio e agua"), Qual),
			FluidRegistry::IsWater(static_cast<EFluidKind>(Qual)));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorldFluidBeyondTheCoastIsSaltTest,
	"BattleSquare.WorldFluid.BeyondTheCoastIsSalt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldFluidBeyondTheCoastIsSaltTest::RunTest(const FString& Parameters)
{
	const UIslandBakedPlan* Assado = IslandBakedPlan::Load();
	if (!Assado)
	{
		AddError(TEXT("o assado nao existe — rode ./Tools/bake_island.sh"));
		return false;
	}

	// O MAR não está no traçado como curso — ele é o que sobra depois que a
	// terra acaba. A pergunta se responde pela linha da costa, que o assado
	// tem, e é o que impede o mar de sair como água doce.
	for (int32 Rumo = 0; Rumo < 8; ++Rumo)
	{
		const float Angulo = (2.0f * PI * Rumo) / 8.0f;
		const FVector2D Direcao(FMath::Cos(Angulo), FMath::Sin(Angulo));
		const float NaCosta = Assado->CoastRadiusAt(Angulo);

		TestEqual(*FString::Printf(TEXT("fora da costa no rumo %d e salgada"), Rumo),
			static_cast<int32>(
				WaterFooting::FluidAt(*Assado, Direcao * (NaCosta + 5000.0f))),
			static_cast<int32>(EFluidKind::AguaSalgada));
	}

	// E o centro da ilha NÃO é mar: uma regra que devolvesse salgada em todo
	// lugar passaria no laço acima com a ilha inteira submersa.
	TestNotEqual(TEXT("o centro da ilha nao e mar"),
		static_cast<int32>(WaterFooting::FluidAt(*Assado, FVector2D::ZeroVector)),
		static_cast<int32>(EFluidKind::AguaSalgada));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorldFluidDryLandHasNoFluidTest,
	"BattleSquare.WorldFluid.DryLandHasNoFluid",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldFluidDryLandHasNoFluidTest::RunTest(const FString& Parameters)
{
	const UIslandBakedPlan* Assado = IslandBakedPlan::Load();
	if (!Assado)
	{
		AddError(TEXT("o assado nao existe — rode ./Tools/bake_island.sh"));
		return false;
	}

	// Onde o pé está SECO, não há fluido — as duas perguntas têm de concordar.
	// Se divergissem, o painel diria "seco em água doce", e o jogador
	// aprenderia a não confiar nele.
	constexpr int32 Amostras = 30;
	int32 Conferidas = 0;

	for (int32 Linha = 0; Linha < Amostras; ++Linha)
	{
		for (int32 Coluna = 0; Coluna < Amostras; ++Coluna)
		{
			const float X = ((static_cast<float>(Coluna) / (Amostras - 1)) * 2.0f - 1.0f)
				* Assado->LandRadiusUnits * 0.7f;
			const float Y = ((static_cast<float>(Linha) / (Amostras - 1)) * 2.0f - 1.0f)
				* Assado->LandRadiusUnits * 0.7f;
			const FVector2D Onde(X, Y);

			if (WaterFooting::At(*Assado, Onde) != EWaterFooting::Seco)
			{
				continue;
			}

			++Conferidas;
			if (WaterFooting::FluidAt(*Assado, Onde) != EFluidKind::Nenhum)
			{
				AddError(FString::Printf(
					TEXT("o pe esta seco em (%.0f,%.0f) e o fluido nao e nenhum"), X, Y));
				return false;
			}
		}
	}

	TestTrue(TEXT("houve chao seco para conferir"), Conferidas > 0);

	return true;
}

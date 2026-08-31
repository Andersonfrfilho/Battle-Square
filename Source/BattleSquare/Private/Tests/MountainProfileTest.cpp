// Copyright 2026 Anderson. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Environment/MountainProfile.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace PerfilDaMontanhaTeste
{
	/** Quantas alturas se amostra ao varrer a encosta inteira. */
	constexpr int32 AmostrasDaEncosta = 200;

	/** O cone que o perfil precisa superar em toda altura interna. */
	float RaioDoCone(float Fracao)
	{
		return 1.0f - Fracao;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMountainProfileIsFatterThanAConeTest,
	"BattleSquare.Environment.MountainProfile.OPerfilEMaisGordoQueUmCone",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMountainProfileIsFatterThanAConeTest::RunTest(const FString& Parameters)
{
	using namespace PerfilDaMontanhaTeste;

	TestEqual(TEXT("A base é o raio inteiro"), MountainProfile::RadiusFractionAt(0.0f), 1.0f);
	TestEqual(TEXT("Acima do cume não há montanha"), MountainProfile::RadiusFractionAt(1.0f), 0.0f);

	// ESTA é a asserção que impede a montanha de voltar a ser um chapéu de
	// festa. Um cone passa em toda verificação numérica de altura, raio e
	// colisão; o que ele não passa é em ser mais gordo que ele mesmo.
	for (int32 Amostra = 1; Amostra < AmostrasDaEncosta; ++Amostra)
	{
		const float Fracao = static_cast<float>(Amostra) / static_cast<float>(AmostrasDaEncosta);
		const float Raio = MountainProfile::RadiusFractionAt(Fracao);

		TestTrue(
			FString::Printf(TEXT("Em %.3f da altura a encosta é mais larga que a do cone"), Fracao),
			Raio > RaioDoCone(Fracao));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMountainProfileNarrowsWithoutEverGrowingTest,
	"BattleSquare.Environment.MountainProfile.AEncostaSoAfina",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMountainProfileNarrowsWithoutEverGrowingTest::RunTest(const FString& Parameters)
{
	using namespace PerfilDaMontanhaTeste;

	float Anterior = MountainProfile::RadiusFractionAt(0.0f);
	for (int32 Amostra = 1; Amostra <= AmostrasDaEncosta; ++Amostra)
	{
		const float Fracao = static_cast<float>(Amostra) / static_cast<float>(AmostrasDaEncosta);
		const float Raio = MountainProfile::RadiusFractionAt(Fracao);

		TestTrue(
			FString::Printf(TEXT("Em %.3f a montanha não engorda subindo"), Fracao),
			Raio <= Anterior);

		Anterior = Raio;
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMountainProfileEndsInACapNotANeedleTest,
	"BattleSquare.Environment.MountainProfile.OCumeECalotaNaoAgulha",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMountainProfileEndsInACapNotANeedleTest::RunTest(const FString& Parameters)
{
	const int32 Ultima = MountainProfile::SliceCount - 1;
	const float RaioDoCume = MountainProfile::SliceRadiusFraction(Ultima);

	// Agulha é raio que chega a zero na última fatia. Calota é a fatia de cima
	// ainda ter largura: é ela que vira o patamar onde a trilha termina, e sem
	// ela o topo não seria alcançável, só apontado.
	TestTrue(TEXT("A fatia do cume ainda tem largura"), RaioDoCume > 0.05f);
	TestTrue(TEXT("A fatia do cume é bem mais estreita que a base"),
		RaioDoCume < MountainProfile::SliceRadiusFraction(0) * 0.5f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMountainProfileSlicesCoverEveryHeightTest,
	"BattleSquare.Environment.MountainProfile.TodaAlturaCaiNumaFatia",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMountainProfileSlicesCoverEveryHeightTest::RunTest(const FString& Parameters)
{
	using namespace PerfilDaMontanhaTeste;

	TestEqual(TEXT("A base cai na primeira fatia"), MountainProfile::SliceIndexAt(0.0f), 0);
	TestEqual(TEXT("O cume cai na última fatia"),
		MountainProfile::SliceIndexAt(1.0f), MountainProfile::SliceCount - 1);

	// Fora da faixa o índice ainda precisa existir: quem pergunta o raio numa
	// altura acima do cume recebe zero por outro caminho, e não um estouro.
	TestEqual(TEXT("Abaixo da base grampeia na primeira"),
		MountainProfile::SliceIndexAt(-3.0f), 0);
	TestEqual(TEXT("Acima do cume grampeia na última"),
		MountainProfile::SliceIndexAt(4.0f), MountainProfile::SliceCount - 1);

	int32 AnteriorIndice = 0;
	for (int32 Amostra = 0; Amostra <= AmostrasDaEncosta; ++Amostra)
	{
		const float Fracao = static_cast<float>(Amostra) / static_cast<float>(AmostrasDaEncosta);
		const int32 Indice = MountainProfile::SliceIndexAt(Fracao);

		TestTrue(TEXT("O índice da fatia é sempre válido"),
			Indice >= 0 && Indice < MountainProfile::SliceCount);
		TestTrue(TEXT("Subindo, a fatia nunca volta"), Indice >= AnteriorIndice);

		AnteriorIndice = Indice;
	}

	TestEqual(TEXT("As fatias somam a altura inteira"),
		MountainProfile::SliceHeightFraction() * static_cast<float>(MountainProfile::SliceCount),
		1.0f);

	return true;
}

#endif

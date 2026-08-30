// Copyright 2026 Anderson. All Rights Reserved.

#include "Environment/WorldWeather.h"

#include "Battle/BattleTypes.h"
#include "Environment/WorldTimeOfDay.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Uma semente qualquer de mundo, estável para os testes lerem sempre o mesmo céu. */
	constexpr uint32 SementeDoTempo = 20260830u;

	/** Quantos blocos varrer quando o teste quer medir frequência, e não um caso. */
	constexpr int32 BlocosAmostradosDoTempo = 400;

	const EWeather CeusDoTempo[] = {
		EWeather::Clear, EWeather::Cloudy, EWeather::Overcast, EWeather::Rain
	};

	const EScenaryClimate ClimasDoTempo[] = {
		EScenaryClimate::Temperate, EScenaryClimate::Cold,
		EScenaryClimate::Mild, EScenaryClimate::Desert
	};

	/** Em quantos dos blocos varridos choveu, neste clima. */
	int32 BlocosDeChuvaDoTempo(EScenaryClimate Clima)
	{
		int32 Chuvosos = 0;
		for (int32 Bloco = 0; Bloco < BlocosAmostradosDoTempo; ++Bloco)
		{
			const float Hora = Bloco * WorldWeather::HoursPerSpell;
			if (WorldWeather::IsRaining(WorldWeather::WeatherAt(SementeDoTempo, Clima, Hora)))
			{
				++Chuvosos;
			}
		}
		return Chuvosos;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWorldWeatherSpellLastsItsBlock,
	"BattleSquare.Environment.WorldWeather.OCeuDuraOBloco",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldWeatherSpellLastsItsBlock::RunTest(const FString& Parameters)
{
	// Dentro do mesmo bloco o céu não muda: é isso que separa tempo de ruído.
	for (int32 Bloco = 0; Bloco < 24; ++Bloco)
	{
		const float Comeco = Bloco * WorldWeather::HoursPerSpell;
		const EWeather NoComeco = WorldWeather::WeatherAt(
			SementeDoTempo, EScenaryClimate::Temperate, Comeco);

		for (int32 Passo = 1; Passo < 10; ++Passo)
		{
			const float Dentro = Comeco + WorldWeather::HoursPerSpell * Passo / 10.0f;
			TestTrue(FString::Printf(TEXT("bloco %d, passo %d: mesmo ceu"), Bloco, Passo),
				WorldWeather::WeatherAt(SementeDoTempo, EScenaryClimate::Temperate, Dentro) == NoComeco);
		}
	}

	// E ao longo do tempo ele MUDA — um céu que nunca muda também não é tempo.
	bool bMudouAlgumaVez = false;
	EWeather Anterior = WorldWeather::WeatherAt(SementeDoTempo, EScenaryClimate::Temperate, 0.0f);
	for (int32 Bloco = 1; Bloco < BlocosAmostradosDoTempo; ++Bloco)
	{
		const EWeather Agora = WorldWeather::WeatherAt(
			SementeDoTempo, EScenaryClimate::Temperate, Bloco * WorldWeather::HoursPerSpell);
		if (Agora != Anterior)
		{
			bMudouAlgumaVez = true;
			break;
		}
		Anterior = Agora;
	}
	TestTrue(TEXT("o ceu muda ao longo dos blocos"), bMudouAlgumaVez);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWorldWeatherIsReproducible,
	"BattleSquare.Environment.WorldWeather.MesmaSementeMesmoCeu",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldWeatherIsReproducible::RunTest(const FString& Parameters)
{
	// Dois jogadores no mesmo mundo veem a mesma chuva, e uma investigação
	// consegue voltar à hora exata.
	for (int32 Bloco = 0; Bloco < 50; ++Bloco)
	{
		const float Hora = Bloco * WorldWeather::HoursPerSpell + 0.7f;
		TestTrue(FString::Printf(TEXT("bloco %d: repete"), Bloco),
			WorldWeather::WeatherAt(SementeDoTempo, EScenaryClimate::Temperate, Hora)
			== WorldWeather::WeatherAt(SementeDoTempo, EScenaryClimate::Temperate, Hora));
	}

	// E mundos diferentes têm céus diferentes, senão a semente não serve para nada.
	bool bDiferiuAlgumaVez = false;
	for (int32 Bloco = 0; Bloco < BlocosAmostradosDoTempo; ++Bloco)
	{
		const float Hora = Bloco * WorldWeather::HoursPerSpell;
		if (WorldWeather::WeatherAt(SementeDoTempo, EScenaryClimate::Temperate, Hora)
			!= WorldWeather::WeatherAt(SementeDoTempo + 1u, EScenaryClimate::Temperate, Hora))
		{
			bDiferiuAlgumaVez = true;
			break;
		}
	}
	TestTrue(TEXT("outra semente, outro tempo"), bDiferiuAlgumaVez);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWorldWeatherDesertRarelyRains,
	"BattleSquare.Environment.WorldWeather.NoDesertoQuaseNuncaChove",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldWeatherDesertRarelyRains::RunTest(const FString& Parameters)
{
	const int32 NoDeserto = BlocosDeChuvaDoTempo(EScenaryClimate::Desert);
	const int32 NaMata = BlocosDeChuvaDoTempo(EScenaryClimate::Temperate);
	const int32 NaSerra = BlocosDeChuvaDoTempo(EScenaryClimate::Cold);

	TestTrue(TEXT("no deserto chove menos que na mata"), NoDeserto < NaMata);
	TestTrue(TEXT("na serra fria chove mais que na mata"), NaSerra > NaMata);

	// Raro, mas EXISTE: chuva no deserto é o evento que vale alguma coisa
	// justamente por ser rara. Zero seria um `if` disfarçado de tabela.
	TestTrue(TEXT("no deserto chove alguma vez"), NoDeserto > 0);

	// A tabela promete 3%; a amostra tem de ficar perto disso, não em 40%.
	const int32 Teto = BlocosAmostradosDoTempo * WorldWeather::RainChancePercent(EScenaryClimate::Desert) * 3 / 100;
	TestTrue(FString::Printf(TEXT("chuva no deserto rara (%d de %d)"), NoDeserto, BlocosAmostradosDoTempo),
		NoDeserto <= Teto);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWorldWeatherCloudDimsTheSun,
	"BattleSquare.Environment.WorldWeather.NuvemEscureceOSol",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldWeatherCloudDimsTheSun::RunTest(const FString& Parameters)
{
	// Quanto mais nuvem, menos sol — e é essa ordem que faz o tempo APARECER
	// na tela sem uma única partícula.
	TestTrue(TEXT("limpo passa mais que nublado"),
		WorldWeather::SunDimming(EWeather::Clear) > WorldWeather::SunDimming(EWeather::Cloudy));
	TestTrue(TEXT("nublado passa mais que encoberto"),
		WorldWeather::SunDimming(EWeather::Cloudy) > WorldWeather::SunDimming(EWeather::Overcast));
	TestTrue(TEXT("encoberto passa mais que chuva"),
		WorldWeather::SunDimming(EWeather::Overcast) > WorldWeather::SunDimming(EWeather::Rain));

	for (const EWeather Ceu : CeusDoTempo)
	{
		const float Passa = WorldWeather::SunDimming(Ceu);
		TestTrue(FString::Printf(TEXT("%s: nunca apaga o dia"), WorldWeather::WeatherDebugName(Ceu)),
			Passa > 0.0f && Passa <= 1.0f);

		const float Cobertura = WorldWeather::CloudCover(Ceu);
		TestTrue(FString::Printf(TEXT("%s: cobertura entre 0 e 1"), WorldWeather::WeatherDebugName(Ceu)),
			Cobertura >= 0.0f && Cobertura <= 1.0f);
	}

	// Mais nuvem e mais sol ao mesmo tempo seria a tabela se contradizendo.
	TestTrue(TEXT("chuva cobre mais que limpo"),
		WorldWeather::CloudCover(EWeather::Rain) > WorldWeather::CloudCover(EWeather::Clear));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWorldWeatherRainMakesMud,
	"BattleSquare.Environment.WorldWeather.ChuvaEnlameiaOCampo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldWeatherRainMakesMud::RunTest(const FString& Parameters)
{
	for (const EScenaryClimate Clima : ClimasDoTempo)
	{
		const int32 NaChuva = WorldWeather::HumidityPercent(Clima, EWeather::Rain);
		const int32 NoLimpo = WorldWeather::HumidityPercent(Clima, EWeather::Clear);

		TestTrue(TEXT("chovendo o chao fica mais umido que no sol"), NaChuva > NoLimpo);
		TestTrue(TEXT("umidade fica entre 0 e 100"),
			NaChuva >= 0 && NaChuva <= 100 && NoLimpo >= 0 && NoLimpo <= 100);
	}

	// O clima ameno é o caso que prova que a chuva vira REGRA e não enfeite:
	// seco ele não faz lama, chovendo ele faz. `MudMinHumidity` é o mesmo
	// limiar que o `BattleSim` usa para decidir o terreno da arena.
	TestTrue(TEXT("clima ameno seco nao enlameia"),
		WorldWeather::HumidityPercent(EScenaryClimate::Mild, EWeather::Clear) < MudMinHumidity);
	TestTrue(TEXT("clima ameno na chuva enlameia"),
		WorldWeather::HumidityPercent(EScenaryClimate::Mild, EWeather::Rain) >= MudMinHumidity);

	// E o deserto continua seco mesmo debaixo da chuva rara dele.
	TestTrue(TEXT("deserto nao enlameia nem chovendo"),
		WorldWeather::HumidityPercent(EScenaryClimate::Desert, EWeather::Rain) < MudMinHumidity);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWorldWeatherSunnyNeedsAHighSun,
	"BattleSquare.Environment.WorldWeather.EnsolaradoEhLimpoComSolAlto",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldWeatherSunnyNeedsAHighSun::RunTest(const FString& Parameters)
{
	constexpr float MeioDia = 12.0f;
	constexpr float MeiaNoite = 0.0f;

	TestTrue(TEXT("meio-dia limpo e ensolarado"), WorldWeather::IsSunny(EWeather::Clear, MeioDia));

	// Céu limpo de madrugada é onde se veem as estrelas, não sol nenhum. Sem
	// esta metade, "ensolarado" seria só outro nome para "limpo".
	TestFalse(TEXT("meia-noite limpa nao e ensolarada"),
		WorldWeather::IsSunny(EWeather::Clear, MeiaNoite));

	TestFalse(TEXT("meio-dia chovendo nao e ensolarado"),
		WorldWeather::IsSunny(EWeather::Rain, MeioDia));
	TestFalse(TEXT("meio-dia encoberto nao e ensolarado"),
		WorldWeather::IsSunny(EWeather::Overcast, MeioDia));

	// E quem decide a altura do sol é o relógio, não uma segunda conta aqui.
	TestTrue(TEXT("o meio-dia do relogio tem sol alto"),
		WorldTimeOfDay::SunElevationDegrees(MeioDia) > WorldTimeOfDay::SunElevationDegrees(MeiaNoite));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWorldWeatherSurvivesNegativeHours,
	"BattleSquare.Environment.WorldWeather.HoraAntesDoComecoNaoQuebra",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldWeatherSurvivesNegativeHours::RunTest(const FString& Parameters)
{
	// Um relógio que começa atrasado pergunta o tempo de horas negativas. A
	// resposta tem de ser um céu, não um estouro.
	for (int32 Passo = 1; Passo <= 20; ++Passo)
	{
		const float Hora = -WorldWeather::HoursPerSpell * Passo;
		const EWeather Ceu = WorldWeather::WeatherAt(SementeDoTempo, EScenaryClimate::Temperate, Hora);
		TestTrue(FString::Printf(TEXT("hora %.1f: ceu valido"), Hora),
			Ceu == EWeather::Clear || Ceu == EWeather::Cloudy
			|| Ceu == EWeather::Overcast || Ceu == EWeather::Rain);
	}

	return true;
}

#endif

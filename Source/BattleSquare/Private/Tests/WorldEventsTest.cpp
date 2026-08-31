// Copyright 2026 Anderson. All Rights Reserved.

#include "Environment/WorldEvents.h"

#include "Environment/IslandFeatureLayout.h"
#include "Environment/IslandGeography.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	constexpr uint32 SementeDosEventosDaTerra = 20260901u;
	constexpr uint32 OutraSementeDosEventosDaTerra = 777003u;
	constexpr int32 PeriodosVarridosDosEventosDaTerra = 12;
	constexpr float PassoEmHorasDosEventosDaTerra = 0.05f;

	const EWorldEvent NomesDosEventosDaTerra[] = {
		EWorldEvent::None, EWorldEvent::Earthquake, EWorldEvent::Hurricane, EWorldEvent::Tsunami };

	/** Um ponto no mar, no rumo pedido. */
	FVector2D NoMarDosEventosDaTerra(float RumoGraus)
	{
		const float Radianos = FMath::DegreesToRadians(RumoGraus);
		const float Raio = IslandGeography::LandRadiusUnits() + 1500.0f;
		return FVector2D(FMath::Cos(Radianos), FMath::Sin(Radianos)) * Raio;
	}

	/** Um ponto bem no miolo da ilha, longe de qualquer costa. */
	FVector2D NoMioloDosEventosDaTerra()
	{
		return FVector2D(600.0f, -400.0f);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorldEventsFaultLivesOnTheVolcano,
	"BattleSquare.Environment.WorldEvents.AFalhaMoraNoVulcao",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldEventsFaultLivesOnTheVolcano::RunTest(const FString& Parameters)
{
	FVector2D DoPlano = FVector2D::ZeroVector;
	bool bAchouVulcao = false;
	for (const IslandFeatureLayout::FFeaturePlacement& Peca : IslandFeatureLayout::Plan())
	{
		if (Peca.Feature == IslandFeatureLayout::EIslandFeature::Volcano)
		{
			DoPlano = Peca.CenterUnits();
			bAchouVulcao = true;
		}
	}

	TestTrue(TEXT("o plano da ilha tem um vulcao"), bAchouVulcao);
	TestTrue(TEXT("o vulcao nao esta no centro da ilha"), DoPlano.Size() > 1000.0);

	// A falha SAI do vulcão em vez de repetir a coordenada dele: mover o vulcão
	// e ver o terremoto ficar onde estava seria descobrir tarde que havia duas
	// cópias do mesmo lugar (L-032).
	TestTrue(TEXT("a falha esta no vulcao"),
		WorldEvents::FaultCenterUnits().Equals(DoPlano, 1.0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorldEventsQuakeComesOncePerPeriod,
	"BattleSquare.Environment.WorldEvents.OTremorVemUmaVezPorPeriodo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldEventsQuakeComesOncePerPeriod::RunTest(const FString& Parameters)
{
	// Uma janela por período, nem zero nem duas. Zero daria mundos inteiros sem
	// tremor nenhum; duas fariam a onda de uma pegar a hora da outra.
	for (int32 Periodo = 0; Periodo < PeriodosVarridosDosEventosDaTerra; ++Periodo)
	{
		int32 Aberturas = 0;
		bool bTremendo = false;

		const int32 Passos = FMath::RoundToInt(
			WorldEvents::EarthquakePeriodHours / PassoEmHorasDosEventosDaTerra);
		for (int32 Passo = 0; Passo < Passos; ++Passo)
		{
			const float Horas = Periodo * WorldEvents::EarthquakePeriodHours
				+ Passo * PassoEmHorasDosEventosDaTerra;
			const bool bAgora = WorldEvents::EarthquakeMagnitude(SementeDosEventosDaTerra, Horas) > 0.0f;
			if (bAgora && !bTremendo)
			{
				++Aberturas;
			}
			bTremendo = bAgora;
		}

		TestEqual(TEXT("um tremor por periodo"), Aberturas, 1);
		TestFalse(TEXT("o tremor termina dentro do periodo"), bTremendo);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorldEventsShakingFadesWithDistance,
	"BattleSquare.Environment.WorldEvents.OTremorSeSenteMenosLonge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldEventsShakingFadesWithDistance::RunTest(const FString& Parameters)
{
	// Acha uma hora em que a terra esteja tremendo, em vez de cravar uma: a
	// hora é sorteada dentro do período, e cravar seria medir a sorte.
	float HoraDoTremor = -1.0f;
	const int32 Passos = FMath::RoundToInt(
		WorldEvents::EarthquakePeriodHours / PassoEmHorasDosEventosDaTerra);
	for (int32 Passo = 0; Passo < Passos; ++Passo)
	{
		const float Horas = Passo * PassoEmHorasDosEventosDaTerra;
		if (WorldEvents::EarthquakeMagnitude(SementeDosEventosDaTerra, Horas) > 0.5f)
		{
			HoraDoTremor = Horas;
			break;
		}
	}

	TestTrue(TEXT("houve um tremor forte no primeiro periodo"), HoraDoTremor >= 0.0f);
	if (HoraDoTremor < 0.0f)
	{
		return false;
	}

	const FVector2D Epicentro =
		WorldEvents::EarthquakeEpicenterUnits(SementeDosEventosDaTerra, HoraDoTremor);
	const float NoEpicentro =
		WorldEvents::EarthquakeShaking(SementeDosEventosDaTerra, Epicentro, HoraDoTremor);
	const float AMeioCaminho = WorldEvents::EarthquakeShaking(
		SementeDosEventosDaTerra,
		Epicentro + FVector2D(WorldEvents::EarthquakeReachUnits * 0.5f, 0.0f),
		HoraDoTremor);
	const float ForaDoAlcance = WorldEvents::EarthquakeShaking(
		SementeDosEventosDaTerra,
		Epicentro + FVector2D(WorldEvents::EarthquakeReachUnits * 1.2f, 0.0f),
		HoraDoTremor);

	TestTrue(TEXT("no epicentro se sente inteiro"), NoEpicentro > 0.0f);
	TestTrue(TEXT("mais longe se sente menos"), AMeioCaminho < NoEpicentro);
	TestEqual(TEXT("fora do alcance nao se sente"), ForaDoAlcance, 0.0f);

	// O epicentro cai PERTO do vulcão e não em cima dele: uma falha pontual
	// faria todo terremoto do mundo ser sentido igual, no mesmo lugar.
	TestTrue(TEXT("o epicentro cai na vizinhanca da falha"),
		FVector2D::Distance(Epicentro, WorldEvents::FaultCenterUnits())
			<= WorldEvents::FaultSpreadUnits + 1.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorldEventsTsunamiFollowsTheQuake,
	"BattleSquare.Environment.WorldEvents.OTsunamiVemDepoisDoTremor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldEventsTsunamiFollowsTheQuake::RunTest(const FString& Parameters)
{
	int32 PeriodosComOnda = 0;
	int32 PeriodosSemOnda = 0;

	const int32 Passos = FMath::RoundToInt(
		WorldEvents::EarthquakePeriodHours / PassoEmHorasDosEventosDaTerra);

	for (int32 Periodo = 0; Periodo < PeriodosVarridosDosEventosDaTerra; ++Periodo)
	{
		float UltimoTremor = -1.0f;
		float PrimeiraOnda = -1.0f;

		for (int32 Passo = 0; Passo < Passos; ++Passo)
		{
			const float Horas = Periodo * WorldEvents::EarthquakePeriodHours
				+ Passo * PassoEmHorasDosEventosDaTerra;
			if (WorldEvents::EarthquakeMagnitude(SementeDosEventosDaTerra, Horas) > 0.0f)
			{
				UltimoTremor = Horas;
			}
			const float Subiu = WorldEvents::TsunamiRiseUnits(SementeDosEventosDaTerra, Horas);
			if (Subiu > 0.0f && PrimeiraOnda < 0.0f)
			{
				PrimeiraOnda = Horas;
			}
			if (Subiu > 0.0f)
			{
				// Onda alguma sem tremor forte antes dela: o tsunami é
				// CONSEQUÊNCIA, e sortear os dois em separado permitiria a onda
				// vir sozinha.
				TestTrue(TEXT("a onda so vem de tremor forte"),
					WorldEvents::TsunamiRises(SementeDosEventosDaTerra, Horas));
			}
		}

		if (PrimeiraOnda < 0.0f)
		{
			++PeriodosSemOnda;
			continue;
		}

		++PeriodosComOnda;
		TestTrue(TEXT("a onda comeca depois do tremor acabar"), PrimeiraOnda > UltimoTremor);
		TestTrue(TEXT("a espera da onda e respeitada"),
			PrimeiraOnda - UltimoTremor >= WorldEvents::TsunamiDelayHours - PassoEmHorasDosEventosDaTerra * 2.0f);
	}

	// Nem todo tremor levanta onda, e é isso que faz o tsunami valer o susto.
	TestTrue(TEXT("houve periodo com onda"), PeriodosComOnda > 0);
	TestTrue(TEXT("houve periodo sem onda"), PeriodosSemOnda > 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorldEventsHurricaneStaysOverTheWater,
	"BattleSquare.Environment.WorldEvents.OFuracaoNaoEntraNaTerra",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldEventsHurricaneStaysOverTheWater::RunTest(const FString& Parameters)
{
	bool bViuFuracaoNoMar = false;
	float MaiorNoMiolo = 0.0f;

	const int32 Passos = FMath::RoundToInt(
		WorldEvents::HurricanePeriodHours * 2.0f / PassoEmHorasDosEventosDaTerra);
	for (int32 Passo = 0; Passo < Passos; ++Passo)
	{
		const float Horas = Passo * PassoEmHorasDosEventosDaTerra;

		MaiorNoMiolo = FMath::Max(MaiorNoMiolo,
			WorldEvents::HurricaneStrength(SementeDosEventosDaTerra, NoMioloDosEventosDaTerra(), Horas));

		const float Olho = WorldEvents::HurricaneEyeAngleDegrees(SementeDosEventosDaTerra, Horas);
		if (WorldEvents::HurricaneStrength(
				SementeDosEventosDaTerra, NoMarDosEventosDaTerra(Olho), Horas) > 0.0f)
		{
			bViuFuracaoNoMar = true;
		}
	}

	TestTrue(TEXT("o furacao aparece no mar"), bViuFuracaoNoMar);
	// Furacão morre em cima de terra seca. É o que responde ao "em certas
	// áreas que faz sentido": não é um `if` proibindo, é o mecanismo.
	TestEqual(TEXT("o furacao nao chega ao miolo da ilha"), MaiorNoMiolo, 0.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorldEventsHurricaneKeepsToItsEye,
	"BattleSquare.Environment.WorldEvents.OFuracaoAndaNoSetorDoOlho",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldEventsHurricaneKeepsToItsEye::RunTest(const FString& Parameters)
{
	float HoraDoFuracao = -1.0f;
	const int32 Passos = FMath::RoundToInt(
		WorldEvents::HurricanePeriodHours / PassoEmHorasDosEventosDaTerra);
	for (int32 Passo = 0; Passo < Passos; ++Passo)
	{
		const float Horas = Passo * PassoEmHorasDosEventosDaTerra;
		const float Olho = WorldEvents::HurricaneEyeAngleDegrees(SementeDosEventosDaTerra, Horas);
		if (WorldEvents::HurricaneStrength(
				SementeDosEventosDaTerra, NoMarDosEventosDaTerra(Olho), Horas) > 0.3f)
		{
			HoraDoFuracao = Horas;
			break;
		}
	}

	TestTrue(TEXT("houve furacao no primeiro periodo"), HoraDoFuracao >= 0.0f);
	if (HoraDoFuracao < 0.0f)
	{
		return false;
	}

	const float Olho = WorldEvents::HurricaneEyeAngleDegrees(SementeDosEventosDaTerra, HoraDoFuracao);
	const float NoOlho = WorldEvents::HurricaneStrength(
		SementeDosEventosDaTerra, NoMarDosEventosDaTerra(Olho), HoraDoFuracao);
	const float NaBorda = WorldEvents::HurricaneStrength(
		SementeDosEventosDaTerra,
		NoMarDosEventosDaTerra(Olho + WorldEvents::HurricaneHalfWidthDegrees * 0.5f),
		HoraDoFuracao);
	const float DoOutroLadoDaIlha = WorldEvents::HurricaneStrength(
		SementeDosEventosDaTerra, NoMarDosEventosDaTerra(Olho + 180.0f), HoraDoFuracao);

	TestTrue(TEXT("no olho bate mais forte"), NoOlho > NaBorda);
	TestTrue(TEXT("na borda do setor ainda bate"), NaBorda > 0.0f);
	TestEqual(TEXT("do outro lado da ilha nao bate"), DoOutroLadoDaIlha, 0.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorldEventsWaveReachesTheShoreAlone,
	"BattleSquare.Environment.WorldEvents.AOndaAlcancaAPraiaENaoOMiolo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldEventsWaveReachesTheShoreAlone::RunTest(const FString& Parameters)
{
	const float Raio = IslandGeography::LandRadiusUnits();
	const float Praia = IslandGeography::BeachWidthUnits();

	TestTrue(TEXT("a onda alcanca o mar"), WorldEvents::TsunamiReaches(FVector2D(Raio + 2000.0f, 0.0f)));
	TestTrue(TEXT("a onda alcanca a praia"), WorldEvents::TsunamiReaches(FVector2D(Raio - Praia * 0.5f, 0.0f)));
	TestFalse(TEXT("a onda nao alcanca o interior"),
		WorldEvents::TsunamiReaches(FVector2D(Raio - Praia * 2.0f, 0.0f)));
	TestFalse(TEXT("a onda nao alcanca o miolo"),
		WorldEvents::TsunamiReaches(NoMioloDosEventosDaTerra()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorldEventsShowsWhatIsActuallyHappening,
	"BattleSquare.Environment.WorldEvents.OQueSeVeEOQueEstaAcontecendo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldEventsShowsWhatIsActuallyHappening::RunTest(const FString& Parameters)
{
	// `EventAt` e `EventStrength` são duas portas para a mesma pergunta, e o
	// jeito de elas discordarem é um `case` esquecido no `switch`: a tela
	// escreveria "TERREMOTO" com força zero, ou "calmo" com a terra tremendo.
	const int32 Passos = FMath::RoundToInt(
		WorldEvents::EarthquakePeriodHours * 3.0f / PassoEmHorasDosEventosDaTerra);
	TSet<EWorldEvent> Vistos;

	for (int32 Passo = 0; Passo < Passos; ++Passo)
	{
		const float Horas = Passo * PassoEmHorasDosEventosDaTerra;

		const FVector2D Lugares[] = {
			NoMioloDosEventosDaTerra(),
			WorldEvents::FaultCenterUnits(),
			NoMarDosEventosDaTerra(WorldEvents::HurricaneEyeAngleDegrees(SementeDosEventosDaTerra, Horas)),
			FVector2D(IslandGeography::LandRadiusUnits(), 0.0f)
		};

		for (const FVector2D& Onde : Lugares)
		{
			const EWorldEvent Evento = WorldEvents::EventAt(SementeDosEventosDaTerra, Onde, Horas);
			const float Forca = WorldEvents::EventStrength(SementeDosEventosDaTerra, Onde, Horas);
			Vistos.Add(Evento);

			if (Evento == EWorldEvent::None)
			{
				TestEqual(TEXT("calmo tem forca zero"), Forca, 0.0f);
			}
			else
			{
				TestTrue(TEXT("evento nomeado tem forca"), Forca > 0.0f);
			}
		}
	}

	// Os três acontecem de fato em três períodos de mundo. Um teste que só
	// afirmasse "algum evento aconteceu" passaria calado com um deles morto.
	TestTrue(TEXT("houve terremoto"), Vistos.Contains(EWorldEvent::Earthquake));
	TestTrue(TEXT("houve furacao"), Vistos.Contains(EWorldEvent::Hurricane));
	TestTrue(TEXT("houve tsunami"), Vistos.Contains(EWorldEvent::Tsunami));
	TestTrue(TEXT("houve calma"), Vistos.Contains(EWorldEvent::None));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorldEventsSameWorldShakesTheSame,
	"BattleSquare.Environment.WorldEvents.OMesmoMundoTremeIgual",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldEventsSameWorldShakesTheSame::RunTest(const FString& Parameters)
{
	const FVector2D Onde = WorldEvents::FaultCenterUnits();
	bool bAlgumaDiferenca = false;

	const int32 Passos = FMath::RoundToInt(
		WorldEvents::EarthquakePeriodHours * 2.0f / PassoEmHorasDosEventosDaTerra);
	for (int32 Passo = 0; Passo < Passos; ++Passo)
	{
		const float Horas = Passo * PassoEmHorasDosEventosDaTerra;

		const float Uma = WorldEvents::EarthquakeShaking(SementeDosEventosDaTerra, Onde, Horas);
		const float Outra = WorldEvents::EarthquakeShaking(SementeDosEventosDaTerra, Onde, Horas);
		TestEqual(TEXT("a mesma pergunta da a mesma resposta"), Uma, Outra);

		if (!FMath::IsNearlyEqual(
				Uma, WorldEvents::EarthquakeShaking(OutraSementeDosEventosDaTerra, Onde, Horas)))
		{
			bAlgumaDiferenca = true;
		}
	}

	// Semente diferente, mundo diferente: sem isso o "determinismo" seria só
	// uma constante, e todo jogador viveria o mesmo terremoto na mesma hora.
	TestTrue(TEXT("mundo diferente treme diferente"), bAlgumaDiferenca);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWorldEventsGivesEveryNameItsOwnCase,
	"BattleSquare.Environment.WorldEvents.CadaNomeTemSeuCaso",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWorldEventsGivesEveryNameItsOwnCase::RunTest(const FString& Parameters)
{
	TSet<FString> Nomes;
	for (const EWorldEvent Evento : NomesDosEventosDaTerra)
	{
		Nomes.Add(FString(WorldEvents::EventDebugName(Evento)));
	}

	TestEqual(TEXT("cada evento tem nome proprio"),
		Nomes.Num(), static_cast<int32>(UE_ARRAY_COUNT(NomesDosEventosDaTerra)));
	return true;
}

#endif

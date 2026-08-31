// Copyright 2026 Anderson. All Rights Reserved.

#include "World/WorldMapPins.h"
#include "UI/WorldMapProjection.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarcarDeNovoApagaTest,
	"BattleSquare.World.Pins.MarcarDeNovoApaga",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMarcarDeNovoApagaTest::RunTest(const FString&)
{
	// UM gesto para as duas coisas. Apagar por um segundo gesto — um modo, uma
	// tecla, um menu — seria construir a porta que ninguém acha, e este
	// projeto já perdeu três rodadas exatamente assim.
	FWorldMapPins Marcas;

	TestEqual(TEXT("A primeira põe"),
		Marcas.ToggleAt(FVector2D(1000.0f, 1000.0f), EWorldPinKind::Interesse),
		FWorldMapPins::EResult::Posta);
	TestEqual(TEXT("E fica uma"), Marcas.Pins.Num(), 1);

	TestEqual(TEXT("A segunda no MESMO lugar apaga"),
		Marcas.ToggleAt(FVector2D(1000.0f, 1000.0f), EWorldPinKind::Interesse),
		FWorldMapPins::EResult::Apagada);
	TestEqual(TEXT("E não sobra nenhuma"), Marcas.Pins.Num(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FApagarNaoDependeDoTipoTest,
	"BattleSquare.World.Pins.ApagarNaoDependeDoTipo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FApagarNaoDependeDoTipoTest::RunTest(const FString&)
{
	// Marcar em cima apaga MESMO com outro tipo. A alternativa — trocar o
	// tipo — faria quem quer apagar ter de marcar três vezes até dar a volta
	// nos tipos, sem nada explicando por que a marca não some.
	FWorldMapPins Marcas;
	Marcas.ToggleAt(FVector2D::ZeroVector, EWorldPinKind::Perigo);

	TestEqual(TEXT("Outro tipo, mesmo lugar: apaga"),
		Marcas.ToggleAt(FVector2D::ZeroVector, EWorldPinKind::Destino),
		FWorldMapPins::EResult::Apagada);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPertoEOMesmoLugarLongeNaoTest,
	"BattleSquare.World.Pins.PertoEOMesmoLugarLongeNao",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPertoEOMesmoLugarLongeNaoTest::RunTest(const FString&)
{
	// "O mesmo lugar" não pode exigir a mesma unidade de mundo: ninguém volta
	// ao centímetro exato, e um apagar que exigisse isso nunca funcionaria.
	FWorldMapPins Marcas;
	Marcas.ToggleAt(FVector2D::ZeroVector, EWorldPinKind::Interesse);

	const float Meio = FWorldMapPins::SamePlaceUnits * 0.5f;
	TestEqual(TEXT("Meio raio adiante ainda é o mesmo lugar"),
		Marcas.ToggleAt(FVector2D(Meio, 0.0f), EWorldPinKind::Interesse),
		FWorldMapPins::EResult::Apagada);

	// E longe é OUTRO lugar: um raio generoso demais faria a segunda marcação
	// apagar a primeira sem o jogador entender por quê.
	Marcas.ToggleAt(FVector2D::ZeroVector, EWorldPinKind::Interesse);
	const float Longe = FWorldMapPins::SamePlaceUnits * 3.0f;
	TestEqual(TEXT("Três raios adiante é lugar novo"),
		Marcas.ToggleAt(FVector2D(Longe, 0.0f), EWorldPinKind::Interesse),
		FWorldMapPins::EResult::Posta);
	TestEqual(TEXT("E as duas coexistem"), Marcas.Pins.Num(), 2);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMapaCheioRecusaEmVezDePerderTest,
	"BattleSquare.World.Pins.MapaCheioRecusaEmVezDePerder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMapaCheioRecusaEmVezDePerderTest::RunTest(const FString&)
{
	FWorldMapPins Marcas;
	const float Passo = FWorldMapPins::SamePlaceUnits * 2.0f;

	for (int32 Indice = 0; Indice < FWorldMapPins::MaxPins; ++Indice)
	{
		Marcas.ToggleAt(FVector2D(Indice * Passo, 0.0f), EWorldPinKind::Interesse);
	}
	TestEqual(TEXT("Encheu"), Marcas.Pins.Num(), FWorldMapPins::MaxPins);

	// RECUSA, e não troca pela mais antiga: perder uma marcação que se pôs de
	// propósito, em silêncio, é o pior dos desfechos possíveis aqui.
	const FVector2D Excedente(FWorldMapPins::MaxPins * Passo, 0.0f);
	TestEqual(TEXT("A que passa do teto é recusada"),
		Marcas.ToggleAt(Excedente, EWorldPinKind::Interesse),
		FWorldMapPins::EResult::Cheio);
	TestEqual(TEXT("E nenhuma antiga se perdeu"),
		Marcas.Pins.Num(), FWorldMapPins::MaxPins);

	// Cheio ainda deixa APAGAR — senão o teto vira armadilha sem saída.
	TestEqual(TEXT("Com o mapa cheio ainda dá para apagar"),
		Marcas.ToggleAt(FVector2D::ZeroVector, EWorldPinKind::Interesse),
		FWorldMapPins::EResult::Apagada);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMarcaSaltaDoFundoTest,
	"BattleSquare.World.Pins.MarcaSaltaDoFundo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMarcaSaltaDoFundoTest::RunTest(const FString&)
{
	// A marcação é o que o jogador PROCURA no mapa. Se ela fosse tão surda
	// quanto o terreno, o mapa teria uma anotação que só se acha sabendo onde
	// ela está — o que anula a anotação.
	// Varre ATÉ `Count`, e não uma lista à mão. A lista à mão parava em cinco
	// terrenos enquanto o mapa já tinha oito: deserto, glaciar e vulcão nunca
	// entravam na conta, e um terreno novo mais vivo que uma marcação apagaria
	// a marcação sem nada aqui reclamar.
	float TerrenoMaisForte = 0.0f;
	for (int32 Indice = 0; Indice < static_cast<int32>(EWorldMapTerrain::Count); ++Indice)
	{
		const EWorldMapTerrain Terreno = static_cast<EWorldMapTerrain>(Indice);
		const FLinearColor Cor = FWorldMapProjection::ColorForTerrain(Terreno);
		TerrenoMaisForte = FMath::Max(TerrenoMaisForte, Cor.R + Cor.G + Cor.B);

		TestFalse(TEXT("Todo terreno tem nome na legenda"),
			FWorldMapProjection::LabelForTerrain(Terreno).ToString().IsEmpty());
	}

	const EWorldPinKind Pinos[] = {
		EWorldPinKind::Interesse, EWorldPinKind::Perigo, EWorldPinKind::Destino };

	TSet<FString> Cores;
	for (const EWorldPinKind Pino : Pinos)
	{
		const FLinearColor Cor = FWorldMapProjection::ColorForPin(Pino);
		TestTrue(TEXT("Toda marcação é mais viva que qualquer terreno"),
			(Cor.R + Cor.G + Cor.B) > TerrenoMaisForte);

		TestFalse(TEXT("E tem nome na legenda"),
			FWorldMapProjection::LabelForPin(Pino).ToString().IsEmpty());
		Cores.Add(Cor.ToString());
	}

	TestEqual(TEXT("Os três tipos têm cores distintas"),
		Cores.Num(), static_cast<int32>(UE_ARRAY_COUNT(Pinos)));

	return true;
}

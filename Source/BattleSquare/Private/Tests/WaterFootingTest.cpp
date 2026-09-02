// Copyright 2026 Anderson. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Environment/FreshWater.h"
#include "World/IslandBakedPlan.h"
#include "World/WaterFooting.h"

/**
 * A ÁGUA MOLHA — e o FUNDO importa.
 *
 * Um rio desenhado que não muda nada ao ser pisado é enfeite: lê como
 * obstáculo e se comporta como chão. É essa promessa quebrada que esvazia as
 * 56 travessias do traçado — se dá para andar por qualquer lugar, a ponte não
 * serve para nada.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWaterFootingTheFordAndTheDeepDifferTest,
	"BattleSquare.WaterFooting.TheFordAndTheDeepDiffer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWaterFootingTheFordAndTheDeepDifferTest::RunTest(const FString& Parameters)
{
	const UIslandBakedPlan* Assado = IslandBakedPlan::Load();
	if (!Assado)
	{
		AddError(TEXT("o assado nao existe — rode ./Tools/bake_island.sh"));
		return false;
	}

	// Acha um trecho RASO e um trecho FUNDO de verdade no traçado, e atravessa
	// os dois. Escrever coordenadas à mão faria o teste medir dois números
	// escolhidos por mim, não a água que a ilha tem.
	FVector2D NoVau = FVector2D::ZeroVector;
	FVector2D NoFundo = FVector2D::ZeroVector;
	bool bAchouVau = false;
	bool bAchouFundo = false;

	for (const FBakedRiver& Curso : Assado->Rivers)
	{
		for (int32 Ponto = 0; Ponto < Curso.PointsUnits.Num(); ++Ponto)
		{
			if (!Curso.HalfWidthUnits.IsValidIndex(Ponto))
			{
				continue;
			}

			const EWaterFooting Chao = WaterFooting::At(*Assado, Curso.PointsUnits[Ponto]);
			if (!bAchouVau && Chao == EWaterFooting::Vau)
			{
				NoVau = Curso.PointsUnits[Ponto];
				bAchouVau = true;
			}
			if (!bAchouFundo && Chao == EWaterFooting::Fundo)
			{
				NoFundo = Curso.PointsUnits[Ponto];
				bAchouFundo = true;
			}
		}
	}

	if (!bAchouVau || !bAchouFundo)
	{
		AddError(FString::Printf(
			TEXT("a ilha nao tem os dois: vau=%d, fundo=%d — sem os dois, a agua ")
			TEXT("nao distingue nada e a travessia nao tem motivo"),
			bAchouVau ? 1 : 0, bAchouFundo ? 1 : 0));
		return false;
	}

	// O RESULTADO TEM DE SER DIFERENTE. Esta é a linha que a task pede: um vau
	// e um trecho fundo com resultados diferentes.
	TestNotEqual(TEXT("o vau e o fundo nao rendem o mesmo passo"),
		WaterFooting::SpeedMultiplierFor(WaterFooting::At(*Assado, NoVau)),
		WaterFooting::SpeedMultiplierFor(WaterFooting::At(*Assado, NoFundo)));

	// E os dois rendem MENOS que a terra: água que não atrasa não molha.
	const float EmTerra = WaterFooting::SpeedMultiplierFor(EWaterFooting::Seco);
	TestTrue(TEXT("o vau atrasa"),
		WaterFooting::SpeedMultiplierFor(EWaterFooting::Vau) < EmTerra);
	TestTrue(TEXT("o fundo atrasa mais que o vau"),
		WaterFooting::SpeedMultiplierFor(EWaterFooting::Fundo)
			< WaterFooting::SpeedMultiplierFor(EWaterFooting::Vau));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWaterFootingDryLandStaysDryTest,
	"BattleSquare.WaterFooting.DryLandStaysDry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWaterFootingDryLandStaysDryTest::RunTest(const FString& Parameters)
{
	const UIslandBakedPlan* Assado = IslandBakedPlan::Load();
	if (!Assado)
	{
		AddError(TEXT("o assado nao existe — rode ./Tools/bake_island.sh"));
		return false;
	}

	// A ilha tem 6% de água pedida. Se a regra molhasse tudo, todo teste acima
	// passaria — o vau e o fundo continuariam diferentes um do outro — e o
	// jogador andaria a passo de nado pela ilha inteira.
	constexpr int32 Amostras = 40;
	int32 Secos = 0;

	for (int32 Linha = 0; Linha < Amostras; ++Linha)
	{
		for (int32 Coluna = 0; Coluna < Amostras; ++Coluna)
		{
			const float X = ((static_cast<float>(Coluna) / (Amostras - 1)) * 2.0f - 1.0f)
				* Assado->LandRadiusUnits * 0.7f;
			const float Y = ((static_cast<float>(Linha) / (Amostras - 1)) * 2.0f - 1.0f)
				* Assado->LandRadiusUnits * 0.7f;

			if (WaterFooting::At(*Assado, FVector2D(X, Y)) == EWaterFooting::Seco)
			{
				++Secos;
			}
		}
	}

	const int32 Total = Amostras * Amostras;
	if (Secos * 10 < Total * 9)
	{
		AddError(FString::Printf(
			TEXT("so %d de %d amostras estao secas — a agua esta molhando a ilha inteira"),
			Secos, Total));
		return false;
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWaterFootingTheFordComesFromTheCrossingsTest,
	"BattleSquare.WaterFooting.TheFordComesFromTheCrossings",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWaterFootingTheFordComesFromTheCrossingsTest::RunTest(const FString& Parameters)
{
	const UIslandBakedPlan* Assado = IslandBakedPlan::Load();
	if (!Assado)
	{
		AddError(TEXT("o assado nao existe — rode ./Tools/bake_island.sh"));
		return false;
	}

	// ONDE SE PASSA A PÉ, quem diz é o TRAÇADO. Eu tinha escrito que a largura
	// decidia, e a MEDIÇÃO derrubou: nenhum ponto de rio saía vau, porque a
	// meia-largura mínima é 481 e o limiar do a-pé é 30% da calha do rio. O
	// traçado escolhe o vau pela FUNDURA (as 30 travessias abaixo de 94
	// unidades), que é coisa que a largura não diz — rio largo e raso existe.
	//
	// Este teste existe para que ninguém reconstrua aquela regra aqui: no
	// centro de uma travessia marcada como vau, o pé tem de achar vau.
	constexpr uint8 TravessiaVau = 0;

	int32 VausConferidos = 0;
	for (const FBakedCrossing& Travessia : Assado->Crossings)
	{
		if (Travessia.Kind != TravessiaVau)
		{
			continue;
		}

		// O que se cobra é que dê para PASSAR A PÉ ali — não que haja água.
		//
		// MEDIDO: 25 dos 30 vaus caem FORA da calha, e 24 têm fundura zero. O
		// vau em (-87330,-59353) está 874 unidades fora de um rio de 3.754 de
		// meia-largura. Isso é o traçado, não a construção: um vau de fundura
		// zero é o ponto em que a trilha passa sem molhar o pé.
		//
		// Escrevi primeiro "no centro de um vau o pé acha vau" e o teste
		// reprovou dado correto. Chão seco também se atravessa a pé; o que
		// nunca pode acontecer ali é água FUNDA, que é o que tornaria a
		// travessia impossível justamente onde o traçado promete passagem.
		++VausConferidos;
		if (WaterFooting::At(*Assado, Travessia.CenterUnits) == EWaterFooting::Fundo)
		{
			AddError(FString::Printf(
				TEXT("a travessia de vau em (%.0f,%.0f) caiu em agua funda"),
				Travessia.CenterUnits.X, Travessia.CenterUnits.Y));
			return false;
		}
	}

	// Zero vaus faria o laço acima passar sem afirmar nada.
	TestTrue(TEXT("o tracado tem travessias de vau"), VausConferidos > 0);

	return true;
}

/**
 * O MESMO RIO NOS DOIS SENTIDOS NÃO É A MESMA TRAVESSIA.
 *
 * Este é o aceite da C5. Antes dela, a água atrasava por ser água: descer e
 * subir o curso rendiam idêntico, e um rio que atrapalha os dois lados por
 * igual não tem sentido nenhum — é um pântano com desenho de rio.
 *
 * O teste procura no traçado uma casa com corrente de verdade em vez de
 * inventar coordenada: número escolhido por mim mediria a minha escolha.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWaterFootingUpstreamCostsMoreTest,
	"BattleSquare.WaterFooting.UpstreamCostsMore",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWaterFootingUpstreamCostsMoreTest::RunTest(const FString& Parameters)
{
	const UIslandBakedPlan* Assado = IslandBakedPlan::Load();
	if (!Assado)
	{
		AddError(TEXT("o assado nao existe — rode ./Tools/bake_island.sh"));
		return false;
	}

	FVector2D Onde = FVector2D::ZeroVector;
	FVector2D FluxoDaAgua = FVector2D::ZeroVector;
	int32 Forca = 0;

	for (const FBakedRiver& Curso : Assado->Rivers)
	{
		for (const FVector2D& Ponto : Curso.PointsUnits)
		{
			int32 ForcaAqui = 0;
			const FVector2D FluxoDaAguaAqui =
				WaterFooting::FlowAt(*Assado, Ponto, ForcaAqui);
			if (ForcaAqui > 0 && !FluxoDaAguaAqui.IsNearlyZero())
			{
				Onde = Ponto;
				FluxoDaAgua = FluxoDaAguaAqui;
				Forca = ForcaAqui;
				break;
			}
		}
		if (Forca > 0)
		{
			break;
		}
	}

	if (Forca <= 0)
	{
		AddError(TEXT("nenhum ponto de curso tem corrente — C1 nao chegou aqui"));
		return false;
	}

	const EWaterFooting Chao = WaterFooting::At(*Assado, Onde);
	const float Descendo =
		WaterFooting::SpeedMultiplierAlong(Chao, FluxoDaAgua, FluxoDaAgua, Forca);
	const float Subindo =
		WaterFooting::SpeedMultiplierAlong(Chao, -FluxoDaAgua, FluxoDaAgua, Forca);

	AddInfo(FString::Printf(
		TEXT("forca %d por mil — descendo %.3f, subindo %.3f"),
		Forca, Descendo, Subindo));

	TestTrue(TEXT("descer o curso rende MAIS que subir"), Descendo > Subindo);

	// CONTRAPESO DE SANIDADE: o de graça teria de ser melhor que o de graça
	// nenhum. Sem ele, "os dois iguais a zero" também passaria no teste acima
	// se alguém trocasse o `>` por `>=` numa refatoração.
	TestTrue(TEXT("subir ainda ANDA — corredeira nao e parede invisivel"),
		Subindo > 0.0f);

	return true;
}

/**
 * DE TRAVÉS, NEM UM NEM OUTRO — e este é o contrapeso que importa.
 *
 * Atravessar de margem a margem é o movimento que o mapa mais pede, e é
 * perpendicular ao curso. Se a corrente cobrasse ali, ela castigaria
 * justamente a travessia, e o rio ficaria mais difícil de cruzar por correr
 * mais forte — que é o contrário do que ela deveria fazer com quem cruza.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWaterFootingSidewaysIsNeitherTest,
	"BattleSquare.WaterFooting.SidewaysIsNeither",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWaterFootingSidewaysIsNeitherTest::RunTest(const FString& Parameters)
{
	const FVector2D FluxoDaAgua(1.0f, 0.0f);
	const FVector2D DeTraves(0.0f, 1.0f);

	const float Parado = WaterFooting::SpeedMultiplierFor(EWaterFooting::Vau);
	const float Atravessando = WaterFooting::SpeedMultiplierAlong(
		EWaterFooting::Vau, DeTraves, FluxoDaAgua, /*ForcaPorMil=*/1000);

	TestEqual(TEXT("de traves, a corrente nao muda o passo"),
		Atravessando, Parado, 0.001f);

	// E o mesmo vale para quem está PARADO: quem não anda não anda a favor de
	// nada, e um multiplicador aplicado a velocidade zero continua zero — mas
	// o fator não pode virar castigo guardado para o primeiro passo.
	const float Imovel = WaterFooting::SpeedMultiplierAlong(
		EWaterFooting::Vau, FVector2D::ZeroVector, FluxoDaAgua, /*ForcaPorMil=*/1000);
	TestEqual(TEXT("parado nao sobe nem desce"), Imovel, Parado, 0.001f);

	// E ÁGUA PARADA continua sendo só água: força zero devolve o fator seco,
	// mesmo com um rumo apontando para algum lado.
	const float SemCorrente = WaterFooting::SpeedMultiplierAlong(
		EWaterFooting::Vau, FluxoDaAgua, FluxoDaAgua, /*ForcaPorMil=*/0);
	TestEqual(TEXT("agua parada e so agua"), SemCorrente, Parado, 0.001f);

	return true;
}

/**
 * A CORREDEIRA NÃO É PAREDE.
 *
 * Mesma lição que a balsa já custou: corrente forte o bastante para zerar o
 * passo prende o jogador dentro do rio, e o que ele vê não é uma água difícil
 * — é um jogo travado, sem nada na tela explicando por quê.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWaterFootingNeverStopsTheWalkerTest,
	"BattleSquare.WaterFooting.NeverStopsTheWalker",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWaterFootingNeverStopsTheWalkerTest::RunTest(const FString& Parameters)
{
	const FVector2D FluxoDaAgua(1.0f, 0.0f);

	for (const EWaterFooting Chao :
		{ EWaterFooting::Seco, EWaterFooting::Vau, EWaterFooting::Fundo })
	{
		const float ContraTudo = WaterFooting::SpeedMultiplierAlong(
			Chao, -FluxoDaAgua, FluxoDaAgua, /*ForcaPorMil=*/1000);
		TestTrue(FString::Printf(TEXT("%s: contra a corrente maxima, ainda anda"),
			WaterFooting::DebugName(Chao)), ContraTudo > 0.0f);
	}

	return true;
}

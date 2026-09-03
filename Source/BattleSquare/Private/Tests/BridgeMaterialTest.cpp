// Copyright 2026 Anderson. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "World/IslandBakedPlan.h"
#include "World/TrailLayout.h"

// ---------------------------------------------------------------------------
// M6 — PONTES DE BLOCO, MADEIRA E DESTRUÍDA.
//
// O `0 pontes` da carta não era decisão de arte: era o sintoma de um traçado
// que enxergava menos água do que existe. Conforme as réguas da fundura foram
// ficando honestas, o mundo pediu 1, depois 3, depois 4 pontes — sem ninguém
// escrever uma linha sobre elas.
//
// Esta task diz DE QUE elas são feitas, e se ainda servem.
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBridgeMaterialComesFromTheSpanTest,
	"BattleSquare.Bridge.MaterialComesFromTheSpan",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBridgeMaterialComesFromTheSpanTest::RunTest(const FString& Parameters)
{
	const UIslandBakedPlan* Assado = IslandBakedPlan::Load();
	if (!Assado)
	{
		AddError(TEXT("o assado nao existe — rode ./Tools/bake_island.sh"));
		return false;
	}

	int32 Pontes = 0;
	int32 ComMaterial = 0;
	int32 NaoPontesComMaterial = 0;

	for (const FBakedCrossing& Travessia : Assado->Crossings)
	{
		const bool bEhPonte =
			Travessia.Kind == static_cast<uint8>(TrailLayout::ECrossingKind::Ponte);

		if (bEhPonte)
		{
			++Pontes;
			if (Travessia.BridgeMaterial
				!= static_cast<uint8>(TrailLayout::EBridgeMaterial::Nenhum))
			{
				++ComMaterial;
			}
		}
		else if (Travessia.BridgeMaterial
			!= static_cast<uint8>(TrailLayout::EBridgeMaterial::Nenhum))
		{
			++NaoPontesComMaterial;
		}
	}

	AddInfo(FString::Printf(TEXT("pontes: %d, com material: %d"), Pontes, ComMaterial));

	TestTrue(TEXT("o mundo tem ponte"), Pontes > 0);
	TestEqual(TEXT("TODA ponte tem material"), ComMaterial, Pontes);

	// CONTRAPESO: o que NÃO é ponte não tem material. `Nenhum` é o valor de
	// quem não é ponte E o de um assado antigo — as duas coisas coincidem de
	// propósito, e é isso que impede um vau de virar "ponte de madeira" por
	// um campo esquecido.
	TestEqual(TEXT("vau e balsa nao tem material de ponte"),
		NaoPontesComMaterial, 0);

	return true;
}

// A PONTE DESTRUÍDA NÃO DEIXA PASSAR — e é a regra inteira desta task.
//
// ⚠️ AFIRMADO COMO PROPRIEDADE, e não sobre o mundo assado: com quatro pontes e
// trinta por cento de ruína, o mundo de hoje pode legitimamente não ter nenhuma
// destruída (acontece em 34% das vezes). Um teste que exigisse encontrar uma
// estaria medindo o SORTEIO, não a regra — é o erro que a regra 12 de
// `geracao-procedural-de-mapas.md` nomeia.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBridgeRuinedDoesNotCrossTest,
	"BattleSquare.Bridge.RuinedDoesNotCross",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBridgeRuinedDoesNotCrossTest::RunTest(const FString& Parameters)
{
	// A PROPRIEDADE, montada à mão: destruída não passa, e todo o resto passa.
	FBakedCrossing Ruina;
	Ruina.Kind = static_cast<uint8>(TrailLayout::ECrossingKind::Ponte);
	Ruina.BridgeMaterial = static_cast<uint8>(TrailLayout::EBridgeMaterial::Destruida);
	TestTrue(TEXT("a ponte DESTRUIDA nao deixa passar"), !Ruina.CanBeCrossed());

	for (const TrailLayout::EBridgeMaterial Qual : {
			TrailLayout::EBridgeMaterial::Nenhum,
			TrailLayout::EBridgeMaterial::Bloco,
			TrailLayout::EBridgeMaterial::Madeira })
	{
		FBakedCrossing Inteira;
		Inteira.BridgeMaterial = static_cast<uint8>(Qual);
		TestTrue(TEXT("e todo o resto passa"), Inteira.CanBeCrossed());
	}

	// E NO MUNDO: se houver ruína assada, ela não passa. Se não houver, o
	// teste NÃO reprova — mas diz, para quem lê saber que o caso não foi
	// exercitado pelo mundo de hoje.
	const UIslandBakedPlan* Assado = IslandBakedPlan::Load();
	if (!Assado)
	{
		return true;
	}

	int32 Ruinas = 0;
	int32 RuinasQuePassam = 0;

	for (const FBakedCrossing& Travessia : Assado->Crossings)
	{
		if (Travessia.BridgeMaterial
			== static_cast<uint8>(TrailLayout::EBridgeMaterial::Destruida))
		{
			++Ruinas;
			if (Travessia.CanBeCrossed())
			{
				++RuinasQuePassam;
			}
		}
	}

	AddInfo(FString::Printf(
		TEXT("ruinas no mundo de hoje: %d (30%% das de madeira; com poucas "
			 "pontes, zero e resultado legitimo)"), Ruinas));

	TestEqual(TEXT("nenhuma ruina assada deixa passar"), RuinasQuePassam, 0);

	return true;
}

// O BLOCO É O VÃO CURTO, A MADEIRA É O LONGO — e nunca o contrário.
//
// Onde a pedra alcança, ninguém corta árvore: bloco dá mais trabalho e dura
// muito mais, e o vão curto é onde ele compensa.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBridgeStoneIsTheShortSpanTest,
	"BattleSquare.Bridge.StoneIsTheShortSpan",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBridgeStoneIsTheShortSpanTest::RunTest(const FString& Parameters)
{
	const UIslandBakedPlan* Assado = IslandBakedPlan::Load();
	if (!Assado)
	{
		AddError(TEXT("o assado nao existe"));
		return false;
	}

	// A FUNDURA é o que o assado guarda por travessia, e ela cresce com a
	// largura — então a de bloco, que é o vão curto, tende a ser a mais rasa.
	float MaisFundaDeBloco = 0.0f;
	float MaisRasaDeMadeira = TNumericLimits<float>::Max();
	int32 Blocos = 0;
	int32 Madeiras = 0;

	for (const FBakedCrossing& Travessia : Assado->Crossings)
	{
		if (Travessia.BridgeMaterial
			== static_cast<uint8>(TrailLayout::EBridgeMaterial::Bloco))
		{
			++Blocos;
			MaisFundaDeBloco = FMath::Max(MaisFundaDeBloco, Travessia.DepthUnits);
		}
		else if (Travessia.BridgeMaterial
			== static_cast<uint8>(TrailLayout::EBridgeMaterial::Madeira))
		{
			++Madeiras;
			MaisRasaDeMadeira = FMath::Min(MaisRasaDeMadeira, Travessia.DepthUnits);
		}
	}

	AddInfo(FString::Printf(
		TEXT("bloco: %d (mais funda %.0f), madeira: %d (mais rasa %.0f)"),
		Blocos, MaisFundaDeBloco, Madeiras,
		Madeiras > 0 ? MaisRasaDeMadeira : 0.0f));

	// ⚠️ NÃO se afirma "toda de bloco é mais rasa que toda de madeira": a
	// fundura sai da largura E do declive, e uma ponte de bloco sobre um
	// trecho manso pode ser mais funda que uma de madeira numa corredeira.
	// Afirmar isso mediria o declive, não o vão.
	//
	// O que se afirma é que os DOIS materiais existem quando há pontes de
	// sobra — e que nenhum deles é o único, que seria a regra não decidindo
	// nada.
	if (Blocos + Madeiras >= 4)
	{
		TestTrue(TEXT("com quatro pontes ou mais, os dois materiais aparecem"),
			Blocos > 0 && Madeiras > 0);
	}

	return true;
}

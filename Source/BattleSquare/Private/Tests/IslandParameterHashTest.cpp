#include "Misc/AutomationTest.h"

#include "World/IslandBakedPlan.h"

/**
 * O resumo dos parâmetros só serve se TODO parâmetro estiver dentro dele.
 *
 * Um campo de fora é uma mudança de mundo que o assado velho aprova em
 * silêncio — e silêncio aqui é pior que erro nenhum, porque o mundo passa a ser
 * de uma configuração que não existe mais.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandParameterHashChangesWithEveryParameterTest,
	"BattleSquare.IslandParameterHash.ChangesWithEveryParameter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandParameterHashChangesWithEveryParameterTest::RunTest(const FString& Parameters)
{
	const FIslandParameters Original = IslandBakedPlan::GatherParameters();
	const uint32 ResumoOriginal = IslandBakedPlan::HashParameters(Original);

	// Percorre a reflexão em vez de listar os campos: campo acrescentado à
	// struct passa a ser cobrado por este teste sem ninguém lembrar dele.
	int32 Conferidos = 0;
	for (TFieldIterator<FProperty> Campo(FIslandParameters::StaticStruct()); Campo; ++Campo)
	{
		FIslandParameters Mexido = Original;
		void* Valor = Campo->ContainerPtrToValuePtr<void>(&Mexido);

		// Cada tipo muda do seu jeito. Somar 1 num `uint8` que já vale 255
		// voltaria a zero, então o passo é XOR: ele sempre muda o valor.
		if (const FFloatProperty* Flutuante = CastField<FFloatProperty>(*Campo))
		{
			Flutuante->SetPropertyValue(Valor, Flutuante->GetPropertyValue(Valor) + 1.0f);
		}
		else if (const FIntProperty* Inteiro = CastField<FIntProperty>(*Campo))
		{
			Inteiro->SetPropertyValue(Valor, Inteiro->GetPropertyValue(Valor) ^ 1);
		}
		else if (const FByteProperty* Byte = CastField<FByteProperty>(*Campo))
		{
			Byte->SetPropertyValue(Valor, Byte->GetPropertyValue(Valor) ^ 1);
		}
		else if (const FStructProperty* Estrutura = CastField<FStructProperty>(*Campo))
		{
			// O único struct entre os parâmetros é a posição do vulcão. Mexer
			// nela é mover o vulcão, que é mudança de ilha como qualquer outra.
			if (Estrutura->Struct == TBaseStructure<FVector2D>::Get())
			{
				static_cast<FVector2D*>(Valor)->X += 1000.0f;
			}
			else
			{
				AddError(FString::Printf(
					TEXT("o campo %s e um struct que este teste nao sabe mexer"),
					*Campo->GetName()));
				return false;
			}
		}
		else
		{
			AddError(FString::Printf(
				TEXT("o campo %s tem tipo que este teste nao sabe mexer — ")
				TEXT("ensine-o aqui, senao o parametro fica fora da prova"),
				*Campo->GetName()));
			return false;
		}

		if (IslandBakedPlan::HashParameters(Mexido) == ResumoOriginal)
		{
			AddError(FString::Printf(
				TEXT("mudar %s nao mudou o resumo — este parametro esta fora da guarda"),
				*Campo->GetName()));
			return false;
		}

		++Conferidos;
	}

	// Zero campo percorrido passaria no laço acima sem afirmar nada.
	TestTrue(TEXT("a struct de parametros tem campos"), Conferidos > 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandParameterHashIsStableTest,
	"BattleSquare.IslandParameterHash.IsStable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandParameterHashIsStableTest::RunTest(const FString& Parameters)
{
	// Resumo que muda sozinho entre duas leituras faria a guarda acusar
	// divergencia a cada carga, e uma guarda que grita sempre e ignorada.
	const uint32 Primeiro = IslandBakedPlan::HashParameters(IslandBakedPlan::GatherParameters());
	const uint32 Segundo = IslandBakedPlan::HashParameters(IslandBakedPlan::GatherParameters());

	TestEqual(TEXT("o resumo dos mesmos parametros e o mesmo"), Segundo, Primeiro);
	TestTrue(TEXT("o resumo nao e zero"), Primeiro != 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandParameterHashIsBakedIntoThePlanTest,
	"BattleSquare.IslandParameterHash.IsBakedIntoThePlan",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandParameterHashIsBakedIntoThePlanTest::RunTest(const FString& Parameters)
{
	const UIslandBakedPlan* Assado = IslandBakedPlan::Load();
	if (!Assado)
	{
		AddError(FString::Printf(
			TEXT("o assado nao existe em %s — rode ./Tools/bake_island.sh"),
			IslandBakedPlan::AssetPath()));
		return false;
	}

	// O assado guarda o resumo E os valores: o resumo detecta, os valores
	// nomeiam. Resumo gravado como zero passaria despercebido na comparação.
	TestTrue(TEXT("o assado gravou o resumo"), Assado->ParameterHash != 0);

	if (Assado->ParameterHash != IslandBakedPlan::HashParameters(Assado->Parameters))
	{
		// Nomeia o que divergiu em vez de dizer só "nao sao iguais": foi
		// exatamente a mensagem muda que fez esta falha custar uma rodada.
		AddError(FString::Printf(
			TEXT("o resumo gravado (%u) nao bate com os parametros gravados (%u); ")
			TEXT("divergencia contra os de agora: [%s]"),
			Assado->ParameterHash, IslandBakedPlan::HashParameters(Assado->Parameters),
			*FString::Join(IslandBakedPlan::DescribeParameterDivergence(
				Assado->Parameters, IslandBakedPlan::GatherParameters()), TEXT("; "))));
		return false;
	}

	return true;
}

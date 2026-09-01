// Copyright 2026 Anderson. All Rights Reserved.

#include "World/PlanReentryGuard.h"

namespace
{
	/**
	 * Quais planos estão sendo montados AGORA.
	 *
	 * Uma lista, e não um contador: a mensagem precisa dizer o CAMINHO do
	 * ciclo, e não só que houve um. "A água chamou a região, que chamou a
	 * água" é acionável; "houve reentrada" manda ler o rastro de pilha, que é
	 * o que já custava dez minutos.
	 */
	TArray<const TCHAR*>& EmConstrucao()
	{
		static TArray<const TCHAR*> Pilha;
		return Pilha;
	}
}

FPlanReentryGuard::FPlanReentryGuard(const TCHAR* PlanName)
	: Name(PlanName)
{
	TArray<const TCHAR*>& Pilha = EmConstrucao();

	for (const TCHAR* Ja : Pilha)
	{
		if (FCString::Strcmp(Ja, PlanName) != 0)
		{
			continue;
		}

		FString Caminho;
		for (const TCHAR* Qual : Pilha)
		{
			Caminho += FString::Printf(TEXT("%s -> "), Qual);
		}

		// `checkf` e não `ensure`: seguir em frente devolveria um plano pela
		// metade, e um mundo montado sobre meio plano é pior que um que não
		// sobe — ele fica errado em silêncio.
		checkf(false,
			TEXT("Ciclo entre planos do mundo: %s%s.\n")
			TEXT("Camada de baixo nunca pergunta a de cima. ")
			TEXT("Quem precisa de altura durante o plano da agua ")
			TEXT("pergunta a ROCHA (BedrockHeightAt), nunca ao relevo acabado."),
			*Caminho, PlanName);
	}

	Pilha.Add(PlanName);
}

FPlanReentryGuard::~FPlanReentryGuard()
{
	TArray<const TCHAR*>& Pilha = EmConstrucao();
	if (Pilha.Num() > 0)
	{
		Pilha.Pop();
	}
}

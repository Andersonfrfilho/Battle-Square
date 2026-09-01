// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Impede que um plano em construção seja perguntado de novo — e FALA quando é.
 *
 * O mundo é feito de planos que se calculam uma vez e ficam guardados: a
 * região, os rios, as trilhas, o uso do solo. Eles se consultam entre si, e
 * basta um consultar alguém que (mais abaixo) consulta ele de volta para o
 * inicializador estático ser reentrado — o que em C++ é `abort()`.
 *
 * O custo disso não é o erro; é a FORMA dele. O processo morre depois de
 * minutos escrevendo dump de pilha, sem dizer nada útil, e a rodada inteira se
 * perde. Aconteceu duas vezes seguidas aqui, e na primeira eu "consertei" um
 * trecho do ciclo e dei o problema por resolvido — o segundo trecho passava
 * pela mesa da cidade, e só o rastro de pilha mostrou.
 *
 * A guarda troca o travamento por uma mensagem que NOMEIA o plano reentrado.
 *
 * Uso:
 *
 *     const TArray<FCoisa>& Plan()
 *     {
 *         static TArray<FCoisa> Guardado = []()
 *         {
 *             const FPlanReentryGuard Guarda(TEXT("MinhaCoisa::Plan"));
 *             ...
 *         }();
 *
 *         return Guardado;
 *     }
 *
 * E a regra que ela protege: **camada de baixo nunca pergunta à de cima.**
 *
 *     rocha  ->  água  ->  região  ->  relevo com lotes  ->  trilhas  ->  solo
 *
 * O leito do rio é anterior à vila; a mesa da cidade está onde a cidade está,
 * e portanto não é rocha. Quem precisar de altura durante o plano da água
 * pergunta à ROCHA, nunca ao relevo acabado.
 */
struct BATTLESQUARE_API FPlanReentryGuard
{
	explicit FPlanReentryGuard(const TCHAR* PlanName);
	~FPlanReentryGuard();

	FPlanReentryGuard(const FPlanReentryGuard&) = delete;
	FPlanReentryGuard& operator=(const FPlanReentryGuard&) = delete;

private:
	const TCHAR* Name = nullptr;
};

// Copyright 2026 Anderson. All Rights Reserved.

#include "World/NatureBalance.h"

NatureBalance::FNatureCorrecao NatureBalance::Correct(
	const FNatureCenso& Censo, const FNatureFaixaAlvo& Faixa)
{
	FNatureCorrecao Correcao;
	Correcao.Tap = Censo.Tap;

	// Faixa invertida = sem alvo confiavel: nao mexe (nunca um ajuste doido).
	if (Faixa.Min > Faixa.Max)
	{
		return Correcao;
	}

	if (Censo.CurrentLevel < Faixa.Min)
	{
		// Falta: abre a torneira ate o minimo.
		Correcao.TapAdjustment = Faixa.Min - Censo.CurrentLevel;
	}
	else if (Censo.CurrentLevel > Faixa.Max)
	{
		// Sobra: fecha a torneira ate o maximo.
		Correcao.TapAdjustment = Faixa.Max - Censo.CurrentLevel;
	}
	// Dentro da faixa: correcao zero — nao mexe no que esta saudavel.
	return Correcao;
}

bool NatureBalance::ApplyAndLog(
	const FNatureCorrecao& Correcao, float CensoLevel, TArray<FNatureLogEntry>& OutLog)
{
	// Correcao nula nao gera registro — nao houve intervencao a delatar.
	if (Correcao.IsZero())
	{
		return false;
	}

	// Nao-nula: DELATA sempre. Este e o unico caminho de aplicacao, entao nao
	// existe aplicar sem registrar (MN3).
	FNatureLogEntry Linha;
	Linha.Tap = Correcao.Tap;
	Linha.FromLevel = CensoLevel;
	Linha.Adjustment = Correcao.TapAdjustment;
	OutLog.Add(Linha);
	return true;
}

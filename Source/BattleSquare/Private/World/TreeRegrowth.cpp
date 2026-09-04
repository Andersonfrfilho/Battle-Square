// Copyright 2026 Anderson. All Rights Reserved.

#include "World/TreeRegrowth.h"

bool TreeRegrowth::HasRegrown(
	int32 CutAtWorldAgeDays, int32 CurrentWorldAgeDays, int32 DeadlineDays)
{
	// Prazo não-positivo rebrota na hora: config degenerada nunca deixa a mata
	// cortada presa para sempre.
	if (DeadlineDays <= 0)
	{
		return true;
	}

	const int32 Decorrido = CurrentWorldAgeDays - CutAtWorldAgeDays;

	// Relógio incoerente (agora antes do corte): na dúvida, continua cortada —
	// o corte é fato que alguém causou; a rebrota é que precisa do prazo cheio.
	if (Decorrido < 0)
	{
		return false;
	}

	return Decorrido >= DeadlineDays;
}


int32 TreeRegrowth::DaysUntilRegrowth(
	int32 CutAtWorldAgeDays, int32 CurrentWorldAgeDays, int32 DeadlineDays)
{
	if (HasRegrown(CutAtWorldAgeDays, CurrentWorldAgeDays, DeadlineDays))
	{
		return 0;
	}
	// Ainda cortada: falta o prazo menos o que ja decorreu. HasRegrown ja
	// garantiu prazo positivo e decorrido nao-negativo aqui.
	const int32 Decorrido = CurrentWorldAgeDays - CutAtWorldAgeDays;
	return FMath::Max(0, DeadlineDays - Decorrido);
}

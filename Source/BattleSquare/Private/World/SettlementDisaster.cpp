// Copyright 2026 Anderson. All Rights Reserved.

#include "World/SettlementDisaster.h"

bool SettlementDisaster::IsAbandoned(float DisasterMagnitude, float Threshold)
{
	// Limiar não-positivo nunca abandona: config degenerada deixa a vila de pé,
	// não a some por um zero esquecido.
	if (Threshold <= 0.0f)
	{
		return false;
	}
	return DisasterMagnitude >= Threshold;
}

bool SettlementDisaster::OffersAfterDisaster(
	ESettlementKind Kind, ESettlementService Service, bool bAbandoned)
{
	// Abandonada, nenhum serviço. De pé, a tabela de sempre — sem segunda fonte.
	if (bAbandoned)
	{
		return false;
	}
	return SettlementEconomy::Offers(Kind, Service);
}

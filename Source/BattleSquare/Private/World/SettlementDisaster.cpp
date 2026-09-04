// Copyright 2026 Anderson. All Rights Reserved.

#include "World/SettlementDisaster.h"

bool SettlementDisaster::IsDisrupted(float DisasterMagnitude, float Threshold)
{
	// Limiar não-positivo nunca interrompe: config degenerada deixa a vila de
	// pé, nao a some por um zero esquecido.
	if (Threshold <= 0.0f)
	{
		return false;
	}
	// Temporario por construcao: verdadeiro so enquanto a magnitude (janela do
	// desastre) esta alta. Passado o evento, cai — a vila se reconstroi (52).
	return DisasterMagnitude >= Threshold;
}

bool SettlementDisaster::OffersDuringDisaster(
	ESettlementKind Kind, ESettlementService Service, bool bDisrupted)
{
	// Interrompida, nenhum serviço enquanto o desastre passa. Fora disso, a
	// tabela de sempre — sem segunda fonte.
	if (bDisrupted)
	{
		return false;
	}
	return SettlementEconomy::Offers(Kind, Service);
}

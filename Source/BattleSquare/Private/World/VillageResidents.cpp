// Copyright 2026 Anderson. All Rights Reserved.

#include "World/VillageResidents.h"

#include "Battle/DeterministicSpread.h"

namespace
{
	/**
	 * Os nomes de quem mora aqui. Lista curta de propósito: numa vila de
	 * quatro casas, dezesseis nomes bastam para os vizinhos não se repetirem
	 * — e o repetido, quando vier, é homônimo de cidade pequena, não defeito.
	 */
	const TCHAR* NomesDeMorador[] = {
		TEXT("Dona Iraci"), TEXT("Seu Waldemar"), TEXT("Zilda"), TEXT("Tonho"),
		TEXT("Dona Benedita"), TEXT("Seu Ataliba"), TEXT("Marilda"), TEXT("Juvenal"),
		TEXT("Dona Percilia"), TEXT("Seu Norato"), TEXT("Cotinha"), TEXT("Doriva"),
		TEXT("Dona Filomena"), TEXT("Seu Elpidio"), TEXT("Nazare"), TEXT("Quincas"),
	};

	/**
	 * O QUE SE OUVE NUMA VISITA — e cada fala é uma mecânica com teste.
	 *
	 * A regra é a mesma do quadro da Escola: o morador só afirma o que a
	 * batalha (ou o mundo) cobra. Fala sem mecânica por trás não entra — e
	 * nenhuma fala aponta segredo.
	 */
	const TCHAR* FalasDeMorador[] = {
		TEXT("o Centro de Recuperacao cura de graca — sempre curou, sempre vai curar"),
		TEXT("atras da Escola tem o patio: e la que se treina e se destrava golpe"),
		TEXT("a Arena tem um campeao... e sempre o MESMO. Da para aprender o jeito dele"),
		TEXT("fazenda, criadouro e pomar pagam por trabalho — e o pet certo rende mais"),
		TEXT("pet capturado se vende no Mercado; a cidade grande paga menos, viu"),
		TEXT("ponte destruida NAO passa — olhe o aviso antes de gastar caminhada"),
		TEXT("rio fundo se NADA; olhe a fundura no painel antes de atravessar"),
		TEXT("quem cai em batalha acorda no hospital mais perto — curado, de graca"),
	};
}

VillageResidents::FResident VillageResidents::ResidentFor(
	ESettlementKind Kind, int32 DoorIndex)
{
	// A semente sai do LUGAR (tipo da vila + porta), nunca do relógio nem de
	// estado global — regra 5 da geração procedural, a mesma do campeão.
	const uint32 Semente = BattleSpread::SeedFromText(FString::Printf(
		TEXT("morador-%s-%d"), RegionLayout::KindDebugName(Kind), DoorIndex));

	FResident Morador;
	Morador.Name = NomesDeMorador[
		BattleSpread::Below(Semente, 0, UE_ARRAY_COUNT(NomesDeMorador))];
	Morador.TipLine = FalasDeMorador[
		BattleSpread::Below(Semente, 1, UE_ARRAY_COUNT(FalasDeMorador))];

	// A janela de estar em casa: começa em qualquer hora e dura de 10 a 16 —
	// nunca o dia inteiro, nunca dia nenhum. Todo morador tem hora de estar e
	// hora de não estar, e é isso que faz a visita ser visita.
	Morador.HomeStartHour = BattleSpread::Below(Semente, 2, 24);
	Morador.HomeEndHour =
		(Morador.HomeStartHour + 10 + BattleSpread::Below(Semente, 3, 7)) % 24;
	return Morador;
}

bool VillageResidents::IsHomeAtHour(const FResident& Morador, float Hora)
{
	const int32 Agora = FMath::FloorToInt(FMath::Fmod(FMath::Max(0.0f, Hora), 24.0f));

	// A janela pode CRUZAR a meia-noite: "das 20 às 6" é morador de dia fora.
	if (Morador.HomeStartHour <= Morador.HomeEndHour)
	{
		return Agora >= Morador.HomeStartHour && Agora < Morador.HomeEndHour;
	}

	return Agora >= Morador.HomeStartHour || Agora < Morador.HomeEndHour;
}

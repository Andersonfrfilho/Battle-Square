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

	/**
	 * OS ARCOS DE HISTÓRIA (decisão 15), três estágios cada: quem sou, o que
	 * houve, a confidência.
	 *
	 * Todo arco é ancorado no que EXISTE no mundo — a fazenda, a ponte
	 * destruída que não passa, o campeão que não muda, as grutas que se
	 * ligam, o vau, os preços do Mercado. Memória de morador é flavor; fato
	 * mecânico FALSO não entra nem como lembrança.
	 */
	struct FArcoDeHistoria
	{
		const TCHAR* QuemSou;
		const TCHAR* OQueHouve;
		const TCHAR* Confidencia;
	};

	const FArcoDeHistoria ArcosDeHistoria[] = {
		{ TEXT("trabalho na fazenda desde crianca — a terra daqui e boa"),
		  TEXT("teve um ano que o rio baixou e quase levou a lavoura junto"),
		  TEXT("guardo as sementes da primeira colheita. um dia te mostro") },
		{ TEXT("meu avo atravessava a ponte do rio todo santo dia"),
		  TEXT("quando ela caiu, ninguem consertou — e ela NAO passa, ve la"),
		  TEXT("as vezes vou ate o vao so para olhar. caminho que ja foi") },
		{ TEXT("ja fui desafiante da Arena, sabia?"),
		  TEXT("perdi tres vezes para o campeao — o jeito dele NUNCA muda"),
		  TEXT("se voce o vencer, volte aqui e me conte COMO") },
		{ TEXT("criei um pet de agua quando o lago era mais cheio"),
		  TEXT("ele nadava fundo onde hoje da pe, acredita?"),
		  TEXT("do que sinto falta e do barulho dele na agua, de noite") },
		{ TEXT("minha mae dizia para nao entrar nas grutas"),
		  TEXT("dizem que ALGUMAS se ligam por baixo — algumas, nao todas"),
		  TEXT("nunca achei a passagem. mas sei que existe... e voce?") },
		{ TEXT("vendi meu primeiro pet no Mercado, faz tempo"),
		  TEXT("na cidade grande pagam menos — aprendi na pele"),
		  TEXT("dinheiro vai, a historia fica. por isso te conto as minhas") },
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

FString VillageResidents::ResidentKeyFor(ESettlementKind Kind, int32 DoorIndex)
{
	return FString::Printf(TEXT("%s-%d"),
		RegionLayout::KindDebugName(Kind), DoorIndex);
}

FString VillageResidents::StoryLineFor(const FResident& Morador,
	ESettlementKind Kind, int32 DoorIndex, int32 Meetings)
{
	// A PRIMEIRA visita é apresentação + a dica: o morador ainda não te
	// conhece, e estranho não ganha história — ganha conselho.
	if (Meetings <= 1)
	{
		return FString::Printf(TEXT("prazer, sou %s. %s"),
			*Morador.Name, *Morador.TipLine);
	}

	// O arco sai da MESMA semente do morador (índice novo): a história é
	// DELE, estável como o nome — regra 5, a de sempre.
	const uint32 Semente = BattleSpread::SeedFromText(FString::Printf(
		TEXT("historia-%s"), *ResidentKeyFor(Kind, DoorIndex)));
	const FArcoDeHistoria& Arco = ArcosDeHistoria[
		BattleSpread::Below(Semente, 0, UE_ARRAY_COUNT(ArcosDeHistoria))];

	// Segunda visita: quem sou. Terceira: o que houve. Da quarta em diante, a
	// confidência — quem chegou ao fim é amigo, e amigo repete causo. Isso é
	// gente, não defeito.
	switch (Meetings)
	{
	case 2:  return Arco.QuemSou;
	case 3:  return Arco.OQueHouve;
	default: return Arco.Confidencia;
	}
}

FString VillageResidents::StoryLineReacting(const FResident& Morador,
	ESettlementKind Kind, int32 DoorIndex, int32 Meetings,
	const FPlayerDeeds& Feitos)
{
	const FString Fala = StoryLineFor(Morador, Kind, DoorIndex, Meetings);

	// O GANCHO é a fala, não um índice: se alguém reordenar os arcos, o
	// pedido e a reação continuam colados — índice solto os separaria na
	// primeira edição.
	if (Feitos.bBeatChampion && Fala.Contains(TEXT("se voce o vencer")))
	{
		return TEXT("voce O VENCEU?! entao era verdade... me conta TUDO, do primeiro golpe ao ultimo");
	}

	// O arco do Mercado, dito a quem TAMBÉM já vendeu: a confidência vira
	// cumplicidade — a frase é a mesma lição, agora entre iguais.
	if (Feitos.bHasSoldAPet && Fala.Contains(TEXT("dinheiro vai, a historia fica")))
	{
		return TEXT("soube que voce tambem ja vendeu um... entao sabe: dinheiro vai, a historia fica. me conta a sua?");
	}

	// O arco do pet de água, dito a quem ANDA com um: a saudade reconhece o
	// bicho do visitante antes de falar do próprio.
	if (Feitos.bHasWaterPet && Fala.Contains(TEXT("barulho dele na agua")))
	{
		return TEXT("o SEU e de agua, eu vi de longe! cuida bem dele... o barulho deles na agua, de noite, e a melhor parte");
	}

	return Fala;
}

// Copyright 2026 Anderson. All Rights Reserved.

#include "Environment/IslandGeography.h"

#include "Misc/ConfigCacheIni.h"
#include "World/RegionLayout.h"
#include "World/VillageLayout.h"

namespace GeografiaDaIlha
{
	/** O raio de hoje. Crescer a ilha é mudar ESTE número, e só ele. */
	constexpr float RaioDaTerraPadrao = 6000.0f;

	/** Seção e chave do `.ini`, ao lado das outras medidas do mundo. */
	const TCHAR* SecaoDoMundo = TEXT("/Script/BattleSquare.BattleSquareGameMode");
	const TCHAR* ChaveDoRaio = TEXT("WorldIslandRadiusUnits");

	/**
	 * A praia é FRAÇÃO do raio, não um número fixo.
	 *
	 * Com número fixo, a mesma faixa que é uma praia na ilha de hoje viraria
	 * um risco de areia quando a ilha crescer três vezes.
	 */
	constexpr float FracaoDaPraia = 0.08f;

	/**
	 * O pântano é mais largo que a praia porque ele é o que se ATRAVESSA.
	 *
	 * A praia é uma orla: passa-se por ela para chegar na água. O brejo é
	 * caminho, e uma faixa tão fina quanto a areia seria atravessada sem
	 * ninguém reparar que mudou de lugar.
	 */
	constexpr float FracaoDoPantano = 0.10f;

	/**
	 * O PISO do miolo de mata, em unidades.
	 *
	 * Fixo porque ele existe para caber os campos de treino, que têm tamanho
	 * próprio e não crescem com a ilha. Quem garante que cabe é
	 * `IslandGeographyTest`, não este comentário.
	 */
	constexpr float RaioMinimoDaCasa = 2600.0f;

	/**
	 * E a fração, que é quem manda quando a ilha é grande.
	 *
	 * Só o número fixo é o que fez a mata "desaparecer": os setores de bioma
	 * são fatias que se encontram no CENTRO, então fora do miolo eles chegam
	 * todos juntos. Com 2600 numa ilha de 20000, o jogador dava sessenta
	 * metros e já estava em rocha vulcânica — e o pedaço de mundo tem 6400 de
	 * lado, ou seja, nem UM pedaço inteiro de mata cabia no miolo.
	 *
	 * Trinta e cinco por cento dá 7000 na ilha de hoje: um punhado de pedaços
	 * de mata em toda direção antes da primeira divisa. A mata é a casa, e
	 * casa se atravessa andando, não em três passos.
	 */
	constexpr float FracaoDaCasa = 0.35f;

	constexpr float GrausDaVolta = 360.0f;

	/**
	 * Que bioma tem cada setor.
	 *
	 * Dois de mata porque a mata é a casa, e porque é o único bioma que hoje
	 * tem povoamento autoral — os outros nascem com chão e cor antes de terem
	 * plantas próprias.
	 *
	 * Glaciar entre vulcão e mata não é engano: é a Islândia, e é o arranjo em
	 * que a aurora tem onde aparecer sem ficar longe de tudo.
	 */
	const EIslandBiome BiomaDoSetor[IslandGeography::SectorCount] = {
		EIslandBiome::Forest,
		EIslandBiome::Desert,
		EIslandBiome::Volcano,
		EIslandBiome::Glacier,
		EIslandBiome::Forest,
	};
}


/**
 * O relevo. Puro, determinístico e sem estado.
 *
 * Cada função responde por UMA camada do chão, e `GroundHeightAt` as compõe
 * numa ordem que é a regra. Separadas assim, "o lote é plano" e "a rampa sobe
 * o barranco" se leem uma de cada vez.
 */
namespace Relevo
{
	/**
	 * Altura da terra firme sobre o mar, e a amplitude dos morros.
	 *
	 * A relação entre os dois é a regra, não os valores: **a ondulação tem de
	 * ser menor que a altura da terra.** Da primeira vez ela era maior, e o
	 * despejo do mapa mostrou altura mínima negativa — vales abaixo do nível
	 * do mar, ou seja, buracos de água dentro da ilha. Há teste.
	 */
	constexpr float FracaoDaTerra = 0.026f;
	constexpr float FracaoDaOndulacao = 0.015f;

	/** Tamanho de um morro. Grande demais vira planície; pequeno, arrepio. */
	constexpr float FracaoDaCelula = 0.11f;

	/**
	 * A MESA da cidade grande. Ela é o barranco de cidade que barra o passo.
	 *
	 * Centrada na CIDADE, e não na ilha — foi assim que eu errei da primeira
	 * vez, e o despejo do mapa mostrou: planalto centrado na ilha separava o
	 * interior da fronteira, que o ranking já separa. Portão duplo não barra
	 * duas vezes; ele só faz o primeiro portão não significar nada.
	 */
	constexpr float FracaoDoPlanalto = 0.020f;
	constexpr float FracaoDaBordaInterna = 0.025f;
	constexpr float FracaoDaBordaExterna = 0.043f;

	constexpr float MeiaLarguraDaRampa = 16.0f;

	/** Onde o degrau do barranco começa e quanto da faixa ele ocupa. */
	constexpr float InicioDoDegrau = 0.40f;
	constexpr float LarguraDoDegrau = 0.20f;

	/** O cone do vulcão sobe bem mais que qualquer morro: ele é O marco. */
	constexpr float FracaoDoCone = 0.10f;

	float AlturaDaTerra() { return IslandGeography::LandRadiusUnits() * FracaoDaTerra; }

	/** Passo da medida de inclinação: um metro. */
	float PassoDaMedida() { return 100.0f; }

	FVector2D CentroDoPlanalto()
	{
		for (const FSettlementPlacement& Assentamento : RegionLayout::Plan())
		{
			if (Assentamento.Kind == ESettlementKind::CidadeGrande)
			{
				return Assentamento.CenterUnits;
			}
		}

		return FVector2D::ZeroVector;
	}

	/**
	 * A rampa olha para CASA.
	 *
	 * Não é um número: é o rumo da cidade para a vila inicial, calculado. Quem
	 * chega de casa encontra a subida de frente, e não depois de contornar a
	 * mesa inteira. Escrito como constante, ele mentiria no dia em que a
	 * cidade mudasse de lugar.
	 */
	float RumoDaRampa()
	{
		const FVector2D ParaCasa = FVector2D::ZeroVector - CentroDoPlanalto();
		if (ParaCasa.IsNearlyZero())
		{
			return 0.0f;
		}

		return FMath::RadiansToDegrees(FMath::Atan2(ParaCasa.Y, ParaCasa.X));
	}

	/** Hash inteiro determinístico. Mesma entrada, mesma saída, sempre. */
	uint32 Embaralhar(int32 X, int32 Y)
	{
		uint32 Semente = static_cast<uint32>(X) * 374761393u
			+ static_cast<uint32>(Y) * 668265263u;
		Semente = (Semente ^ (Semente >> 13)) * 1274126177u;
		return Semente ^ (Semente >> 16);
	}

	/** O valor de um canto da grade, entre -1 e 1. */
	float ValorDoCanto(int32 X, int32 Y)
	{
		return (static_cast<float>(Embaralhar(X, Y) & 0xFFFFu) / 32768.0f) - 1.0f;
	}

	float Suavizar(float T) { return T * T * (3.0f - 2.0f * T); }

	/**
	 * Ruído de valor sobre uma grade. Não é `FMath::Rand`: é conta sobre a
	 * POSIÇÃO, e por isso o mesmo lugar tem sempre a mesma altura — o mundo
	 * nasce por pedaço, e relevo que muda entre visitas é o chão se mexendo.
	 */
	float Ondulacao(const FVector2D& Onde)
	{
		const float Lado = IslandGeography::LandRadiusUnits() * FracaoDaCelula;
		const float EmX = Onde.X / Lado;
		const float EmY = Onde.Y / Lado;

		const int32 X0 = FMath::FloorToInt(EmX);
		const int32 Y0 = FMath::FloorToInt(EmY);

		const float FracaoX = Suavizar(EmX - static_cast<float>(X0));
		const float FracaoY = Suavizar(EmY - static_cast<float>(Y0));

		const float Baixo = FMath::Lerp(ValorDoCanto(X0, Y0), ValorDoCanto(X0 + 1, Y0), FracaoX);
		const float Cima = FMath::Lerp(ValorDoCanto(X0, Y0 + 1), ValorDoCanto(X0 + 1, Y0 + 1), FracaoX);

		return FMath::Lerp(Baixo, Cima, FracaoY)
			* IslandGeography::LandRadiusUnits() * FracaoDaOndulacao;
	}

	float ConeDoVulcao(const FVector2D& Centro, float RaioQueimado, const FVector2D& Onde)
	{
		const float Distancia = FVector2D::Distance(Onde, Centro);
		if (Distancia >= RaioQueimado)
		{
			return 0.0f;
		}

		const float Fracao = 1.0f - (Distancia / RaioQueimado);
		return Fracao * Fracao * IslandGeography::LandRadiusUnits() * FracaoDoCone;
	}

	bool EstaNaRampa(const FVector2D& Centro, const FVector2D& Onde)
	{
		const FVector2D Daqui = Onde - Centro;
		if (Daqui.IsNearlyZero())
		{
			return false;
		}

		const float Graus = FMath::RadiansToDegrees(FMath::Atan2(Daqui.Y, Daqui.X));
		const float Diferenca = FMath::Abs(FMath::FindDeltaAngleDegrees(Graus, RumoDaRampa()));
		return Diferenca <= MeiaLarguraDaRampa;
	}

	/**
	 * O planalto da cidade, e o barranco por borda dele.
	 *
	 * O barranco é o que impede resolver a região correndo em linha reta: ou
	 * se sobe pela rampa da trilha, ou se escala devagar. **Ele nunca é
	 * parede** — a regra é que todo destino é alcançável a pé, e barranco
	 * intransponível a transformaria em chave de porta.
	 */
	float ComPlanalto(float Altura, const FVector2D& Onde)
	{
		const float Interno = IslandGeography::BluffInnerRadiusUnits();
		const float Externo = IslandGeography::BluffOuterRadiusUnits();
		const float Distancia = FVector2D::Distance(Onde, CentroDoPlanalto());

		if (Distancia >= Externo)
		{
			return Altura;
		}

		const float DoPlanalto = IslandGeography::PlateauHeightUnits();

		if (Distancia <= Interno)
		{
			return Altura + DoPlanalto;
		}

		// Na faixa do barranco a subida é ABRUPTA — é o que faz custar. Na
		// rampa ela é suave, e é por ali que a trilha entra.
		//
		// O barranco é um DEGRAU concentrado no meio da faixa, não uma curva.
		// Da primeira vez usei a cúbica, e o teste mostrou que ela é mais
		// MANSA que a rampa no meio: cúbica é suave onde eu queria penhasco,
		// e íngreme só encostada no topo. Concentrar a subida em um quinto da
		// faixa dá cinco vezes a inclinação da rampa, que é o que "barranco"
		// quer dizer.
		const float Fracao = (Externo - Distancia) / (Externo - Interno);
		const float Perfil = EstaNaRampa(CentroDoPlanalto(), Onde)
			? Fracao
			: FMath::Clamp((Fracao - Relevo::InicioDoDegrau) / Relevo::LarguraDoDegrau, 0.0f, 1.0f);

		return Altura + DoPlanalto * Perfil;
	}

	/**
	 * Os lotes são PLANOS, e este passo vem por último.
	 *
	 * Prédio em chão inclinado fica com meia parede enterrada — a mesma
	 * família de defeito do pet afundando no tabuleiro, que só apareceu quando
	 * um humano olhou a tela.
	 */
	float ComLotesPlanos(float Altura, const FVector2D& Onde)
	{
		for (const FSettlementPlacement& Assentamento : RegionLayout::Plan())
		{
			const float Lote = VillageLayout::PlotHalfExtentUnitsFor(Assentamento.Kind);
			const float Clareira = VillageLayout::ClearingHalfExtentUnitsFor(Assentamento.Kind);

			const FVector2D Daqui = Onde - Assentamento.CenterUnits;
			const float Longe = FMath::Max(FMath::Abs(Daqui.X), FMath::Abs(Daqui.Y));
			if (Longe >= Clareira)
			{
				continue;
			}

			// A altura do lote é a do CENTRO dele sem os lotes — senão a conta
			// se chamaria de volta sem fim.
			float DoCentro = AlturaDaTerra();
			DoCentro += Ondulacao(Assentamento.CenterUnits);
			DoCentro += ConeDoVulcao(IslandGeography::VolcanoCenterUnits(),
				IslandGeography::VolcanoScorchedRadiusUnits(), Assentamento.CenterUnits);
			DoCentro = ComPlanalto(DoCentro, Assentamento.CenterUnits);

			if (Longe <= Lote)
			{
				return DoCentro;
			}

			// Entre o lote e a clareira, o chão volta ao natural aos poucos:
			// degrau na saída da vila seria um muro invisível.
			const float Mistura = (Longe - Lote) / (Clareira - Lote);
			return FMath::Lerp(DoCentro, Altura, Suavizar(Mistura));
		}

		return Altura;
	}
}

namespace IslandGeography
{
	float LandRadiusUnits()
	{
		float Configurado = GeografiaDaIlha::RaioDaTerraPadrao;
		if (GConfig)
		{
			GConfig->GetFloat(GeografiaDaIlha::SecaoDoMundo, GeografiaDaIlha::ChaveDoRaio,
				Configurado, GGameIni);
		}
		return Configurado > 0.0f ? Configurado : GeografiaDaIlha::RaioDaTerraPadrao;
	}

	float BeachWidthUnits()
	{
		return LandRadiusUnits() * GeografiaDaIlha::FracaoDaPraia;
	}

	float SwampWidthUnits()
	{
		return LandRadiusUnits() * GeografiaDaIlha::FracaoDoPantano;
	}

	float HomeRadiusUnits()
	{
		// Numa ilha pequena demais para ter miolo E borda, a casa cede: ela
		// nunca pode empurrar a praia para dentro do próprio centro.
		const float Pedido = FMath::Max(GeografiaDaIlha::RaioMinimoDaCasa,
			LandRadiusUnits() * GeografiaDaIlha::FracaoDaCasa);
		return FMath::Min(Pedido, LandRadiusUnits() - BeachWidthUnits());
	}

	int32 SectorAt(const FVector2D& PositionUnits)
	{
		const float Graus = FMath::RadiansToDegrees(
			FMath::Atan2(PositionUnits.Y, PositionUnits.X));
		const float Voltado = Graus < 0.0f ? Graus + GeografiaDaIlha::GrausDaVolta : Graus;

		const float GrausPorSetor = GeografiaDaIlha::GrausDaVolta / SectorCount;
		return FMath::Clamp(FMath::FloorToInt(Voltado / GrausPorSetor), 0, SectorCount - 1);
	}

	EIslandBiome IslandBiome()
	{
		// UMA ILHA, UM BIOMA.
		//
		// A ilha era fatiada em seis setores de pizza, e cada bioma ficava com
		// 0,39 km² — um deserto desse tamanho não é deserto, é caixa de areia.
		// Com uma ilha por bioma, cada um passa a ter a ilha inteira: 6 km².
		//
		// E é o que faz o resto do desenho fechar. A fronteira exige o ranking,
		// o que faz sentido ENTRE ilhas e seria travar a caminhada para o norte
		// dentro de uma. As espécies são próprias da região, o que só significa
		// alguma coisa se a região for um lugar inteiro.
		FString Escrito;
		if (GConfig && GConfig->GetString(GeografiaDaIlha::SecaoDoMundo,
			TEXT("WorldIslandBiome"), Escrito, GGameIni))
		{
			const FName Nome(*Escrito);
			if (Nome == TEXT("Desert"))  { return EIslandBiome::Desert; }
			if (Nome == TEXT("Glacier")) { return EIslandBiome::Glacier; }
			if (Nome == TEXT("Volcano")) { return EIslandBiome::Volcano; }
			if (Nome == TEXT("Swamp"))   { return EIslandBiome::Swamp; }
		}

		// A mata é o padrão, e é a ilha que existe hoje.
		return EIslandBiome::Forest;
	}

	EIslandBiome BiomeOfSector(int32 Sector)
	{
		// O setor deixou de decidir bioma: a ilha inteira é um só. A função
		// continua existindo porque o MAPA e o clima ainda perguntam por
		// posição, e mudar todas as chamadas de uma vez seria trocar duas
		// coisas no mesmo passo.
		(void)Sector;
		return IslandBiome();
	}

	float VolcanoAngleDegrees() { return 180.0f; }

	float VolcanoRingUnits() { return LandRadiusUnits() * 0.75f; }

	FVector2D VolcanoCenterUnits()
	{
		const float Radianos = FMath::DegreesToRadians(VolcanoAngleDegrees());
		return FVector2D(FMath::Cos(Radianos), FMath::Sin(Radianos)) * VolcanoRingUnits();
	}

	float VolcanoHeatRadiusUnits() { return LandRadiusUnits() * 0.30f; }

	float VolcanoScorchedRadiusUnits() { return LandRadiusUnits() * 0.10f; }

	EIslandBiome BiomeAt(const FVector2D& PositionUnits)
	{
		const float Distancia = PositionUnits.Size();


		// A ordem é a regra. Trocá-la põe deserto encostado no mar e glaciar
		// em cima dos campos de treino.
		// O MIOLO é o bioma da ilha, e não mata fixa. Numa ilha de geleira, a
		// casa do jogador é geleira — ele nasce no lugar onde vive.
		if (Distancia <= HomeRadiusUnits())
		{
			return IslandBiome();
		}
		if (Distancia >= LandRadiusUnits() - BeachWidthUnits())
		{
			return EIslandBiome::Beach;
		}

		// O chão em volta da cratera é queimado em qualquer ilha: o vulcão é
		// marco, não bioma. Vem DEPOIS da praia — a areia da borda é o que a
		// pessoa vê primeiro, e lava chegando no mar é outra coisa.
		if (FVector2D::Distance(PositionUnits, VolcanoCenterUnits()) <= VolcanoScorchedRadiusUnits())
		{
			return EIslandBiome::Volcano;
		}

		const EIslandBiome DoSetor = IslandBiome();

		// Brejo é mata que não drena. Onde o setor já é seco, a mesma faixa
		// baixa continua sendo o que o setor diz: encostar o deserto no mar
		// dá salina, não pântano.
		const bool bAtrasDaPraia = Distancia
			>= LandRadiusUnits() - BeachWidthUnits() - SwampWidthUnits();
		if (bAtrasDaPraia && DoSetor == EIslandBiome::Forest)
		{
			return EIslandBiome::Swamp;
		}

		return DoSetor;
	}

	EScenaryClimate ClimateOf(EIslandBiome Biome)
	{
		switch (Biome)
		{
		case EIslandBiome::Desert:  return EScenaryClimate::Desert;
		case EIslandBiome::Volcano: return EScenaryClimate::Desert;
		case EIslandBiome::Glacier: return EScenaryClimate::Cold;
		case EIslandBiome::Beach:   return EScenaryClimate::Mild;
		case EIslandBiome::Swamp:   return EScenaryClimate::Humid;
		case EIslandBiome::Forest:  break;
		}

		// Sem `default:`, e é o ponto. Com ele, o pântano teria nascido
		// temperado sem ninguém escrever nada errado — foi assim que o vulcão
		// quase nasceu caverna. Bioma novo sem caso aqui não compila.
		return EScenaryClimate::Temperate;
	}

	EScenaryClimate ClimateAt(const FVector2D& PositionUnits)
	{
		return ClimateOf(BiomeAt(PositionUnits));
	}

	EScenaryClimate SectorClimateAt(const FVector2D& PositionUnits)
	{
		return ClimateOf(BiomeOfSector(SectorAt(PositionUnits)));
	}

	bool IsOnLand(const FVector2D& PositionUnits)
	{
		return PositionUnits.Size() <= LandRadiusUnits();
	}

	const TCHAR* BiomeDebugName(EIslandBiome Biome)
	{
		switch (Biome)
		{
		case EIslandBiome::Desert:  return TEXT("deserto");
		case EIslandBiome::Volcano: return TEXT("vulcao");
		case EIslandBiome::Glacier: return TEXT("geleira");
		case EIslandBiome::Beach:   return TEXT("praia");
		case EIslandBiome::Swamp:   return TEXT("pantano");
		case EIslandBiome::Forest:  break;
		}

		return TEXT("mata");
	}

	// ---------------------------------------------------------------- relevo

	float PlateauHeightUnits() { return LandRadiusUnits() * Relevo::FracaoDoPlanalto; }

	float BluffInnerRadiusUnits() { return LandRadiusUnits() * Relevo::FracaoDaBordaInterna; }

	float BluffOuterRadiusUnits() { return LandRadiusUnits() * Relevo::FracaoDaBordaExterna; }

	float BluffRampAngleDegrees() { return Relevo::RumoDaRampa(); }

	float BluffRampHalfWidthDegrees() { return Relevo::MeiaLarguraDaRampa; }

	bool IsOnBluffRamp(const FVector2D& PositionUnits)
	{
		return Relevo::EstaNaRampa(Relevo::CentroDoPlanalto(), PositionUnits);
	}

	float GroundHeightAt(const FVector2D& PositionUnits)
	{
		// A ORDEM é a regra, e cada passo pode sobrescrever o anterior. Trocá-la
		// põe morro dentro da praça.
		const float Distancia = PositionUnits.Size();

		// 1. Fora da terra é mar, e o mar é o zero de tudo.
		if (Distancia >= LandRadiusUnits())
		{
			return 0.0f;
		}

		// 2. A praia sobe do mar até o nível da terra. Sem esta rampa, a ilha
		//    seria um prato com parede — que foi exatamente o relato de jogo:
		//    "ao chegar na água eu afundo para sempre".
		const float Borda = LandRadiusUnits() - BeachWidthUnits();
		const float DaOrla = (Distancia > Borda)
			? FMath::Clamp((LandRadiusUnits() - Distancia) / BeachWidthUnits(), 0.0f, 1.0f)
			: 1.0f;

		float Altura = Relevo::AlturaDaTerra() * DaOrla;

		// 3. As ondulações, ENCOLHIDAS pela mesma orla. É o que faz a caminhada
		//    ter custo diferente por caminho, e é de onde a rota nasce.
		//
		//    O encolhimento não é enfeite: sem ele o vale de um morro na faixa
		//    de praia descia abaixo do nível do mar, e a ilha ganhava buracos
		//    de água. O teste mediu; eu tinha olhado só o miolo.
		Altura += Relevo::Ondulacao(PositionUnits) * DaOrla;

		// 4. O cone do vulcão, e ele vem antes do planalto porque o vulcão é o
		//    marco mais alto: nada o achata.
		Altura += Relevo::ConeDoVulcao(VolcanoCenterUnits(), VolcanoScorchedRadiusUnits(),
			PositionUnits);

		// 5. O PLANALTO da cidade grande, com o barranco por borda. É ele que
		//    impede resolver a região correndo em linha reta: ou se sobe pela
		//    rampa da trilha, ou se escala devagar.
		Altura = Relevo::ComPlanalto(Altura, PositionUnits);

		// 6. Os LOTES são planos, e este passo é o último de propósito: prédio
		//    em chão inclinado fica com meia parede enterrada, que é a mesma
		//    família de defeito do pet afundando no tabuleiro.
		return Relevo::ComLotesPlanos(Altura, PositionUnits);
	}

	float GroundSlopeAt(const FVector2D& PositionUnits)
	{
		// Diferença nas quatro vizinhas, não derivada: o campo tem degrau de
		// propósito (o barranco), e derivada de degrau é infinito.
		const float Passo = Relevo::PassoDaMedida();

		const float Leste = GroundHeightAt(PositionUnits + FVector2D(Passo, 0.0f));
		const float Oeste = GroundHeightAt(PositionUnits - FVector2D(Passo, 0.0f));
		const float Norte = GroundHeightAt(PositionUnits + FVector2D(0.0f, Passo));
		const float Sul = GroundHeightAt(PositionUnits - FVector2D(0.0f, Passo));

		const float PorX = (Leste - Oeste) / (2.0f * Passo);
		const float PorY = (Norte - Sul) / (2.0f * Passo);

		return FMath::Sqrt(PorX * PorX + PorY * PorY);
	}
}

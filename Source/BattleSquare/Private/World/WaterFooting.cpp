// Copyright 2026 Anderson. All Rights Reserved.

#include "World/WaterFooting.h"

#include "Environment/FreshWater.h"
#include "World/IslandBakedPlan.h"
#include "World/TrailLayout.h"

namespace ChaoMolhado
{
	/**
	 * Quanto o passo rende em cada chão.
	 *
	 * A tabela, num lugar só. Espalhada pelos chamadores, ela viraria três
	 * velocidades diferentes para a mesma água na terceira edição.
	 */
	constexpr float PassoEmTerra = 1.0f;
	constexpr float PassoNoVau = 0.55f;
	constexpr float PassoNoFundo = 0.25f;

	/**
	 * O quanto a corrente cheia mexe no passo, para mais ou para menos.
	 *
	 * 0,8 e não 1,0: em 1,0 a correnteza máxima subtrairia o passo inteiro, e
	 * o piso abaixo passaria a ser o único responsável por o jogador andar —
	 * regra de verdade escondida atrás de uma salvaguarda.
	 */
	constexpr float EmpurraoDaCorrente = 0.8f;

	/**
	 * Nunca menos que um quinto do que se andaria naquela água parada.
	 *
	 * Não é generosidade: é o que impede a corredeira de virar uma parede
	 * invisível, em que o jogador aperta para a frente e a tela não muda.
	 */
	constexpr float PisoContraACorrente = 0.2f;

	/**
	 * O pé encontra água quando está DENTRO da calha, não perto dela.
	 *
	 * Mede contra os SEGMENTOS do curso, nunca contra os pontos amostrados. A
	 * distância entre duas amostras é o comprimento do curso dividido por 40, e
	 * isso é MAIOR que a calha na maior parte da ilha: medindo por pontos, o
	 * meio de cada segmento sai seco, e um vau inteiro pode cair nessa fresta.
	 *
	 * Foi assim que uma travessia de vau apareceu em terra firme — a grade de
	 * medição era mais grossa que a coisa que ela precisava enxergar, que é o
	 * defeito que neste projeto já se disfarçou de quatro outros.
	 */
	/**
	 * Dentro da calha, e QUANTO FUNDO no trecho que molhou.
	 *
	 * A fundura sai do MESMO laço da largura, e não de uma segunda varredura:
	 * duas varreduras poderiam achar trechos diferentes como "o mais perto", e
	 * aí a largura falaria de um lugar e a fundura de outro — as duas
	 * verdadeiras, e a conclusão errada.
	 *
	 * `FunduraNoPonto` pode devolver zero: é o córrego e a fonte, que não têm
	 * fundura por ponto. Zero ali significa "não sei", e quem lê decide — não
	 * "raso", que seria uma resposta inventada.
	 */
	bool DentroDaCalha(const TArray<FVector2D>& Pontos, const FVector2D& Onde,
		TFunctionRef<float(int32)> MeiaLarguraNoPonto, float& MeiaLarguraAchada,
		TFunctionRef<float(int32)> FunduraNoPonto, float& FunduraAchada)
	{
		bool bMolhou = false;
		for (int32 Ponto = 0; Ponto + 1 < Pontos.Num(); ++Ponto)
		{
			// A calha do trecho é a MAIOR das duas pontas: usar a menor
			// afinaria o rio justo onde ele engrossa.
			const float Meia =
				FMath::Max(MeiaLarguraNoPonto(Ponto), MeiaLarguraNoPonto(Ponto + 1));
			if (Meia <= 0.0f)
			{
				continue;
			}

			const FVector2D NoTrecho = FMath::ClosestPointOnSegment2D(
				Onde, Pontos[Ponto], Pontos[Ponto + 1]);

			if (FVector2D::DistSquared(NoTrecho, Onde) <= Meia * Meia)
			{
				MeiaLarguraAchada = FMath::Max(MeiaLarguraAchada, Meia);

				// A FUNDURA DA PONTA MAIS PERTO, e não a maior das duas.
				//
				// Eu tinha escrito "a mais funda das duas pontas, pelo mesmo
				// motivo da largura", e a MEDIÇÃO derrubou: o traçado escolhe
				// a travessia pela fundura NO PONTO, e o pé lia o trecho. As
				// duas leituras são defensáveis, e é por isso que ter as duas
				// é o defeito — a travessia de vau em (40544,-44884) está a
				// 1812 unidades de um rio de 1808 de meia-largura, ou seja
				// DENTRO DA CALHA POR QUATRO UNIDADES, e a outra ponta do
				// trecho é bem mais funda.
				//
				// A largura fica com o MÁXIMO porque ela decide se molhou —
				// e molhar na beira do trecho mais largo é molhar. A fundura
				// fica com o ponto porque ela decide se PASSA, e quem passa
				// pisa onde está, não na outra ponta.
				const float ProporcaoNoTrecho = FVector2D::Distance(
					Pontos[Ponto], NoTrecho)
					/ FMath::Max(1.0f, FVector2D::Distance(Pontos[Ponto], Pontos[Ponto + 1]));

				FunduraAchada = FMath::Max(FunduraAchada,
					FMath::Lerp(FunduraNoPonto(Ponto), FunduraNoPonto(Ponto + 1),
						FMath::Clamp(ProporcaoNoTrecho, 0.0f, 1.0f)));

				bMolhou = true;
			}
		}
		return bMolhou;
	}
}

namespace WaterFooting
{
	/**
	 * O valor de `TrailLayout::ECrossingKind::Vau` como número.
	 *
	 * O tipo viaja assado como `uint8` porque `ECrossingKind` mora dentro de um
	 * `namespace`, onde `UENUM` não existe. A conversão fica num lugar só.
	 */
	constexpr uint8 ETravessiaVau =
		static_cast<uint8>(TrailLayout::ECrossingKind::Vau);

	float DefaultHeightUnits()
	{
		// 176 unidades — a cápsula do jogador tem 88 de meia-altura, e é a
		// única altura que este mundo mede hoje. Quando os pets tiverem
		// altura própria, ela entra por `AtForHeight` sem mexer aqui.
		return 176.0f;
	}

	float WaistDepthUnitsFor(float HeightUnits)
	{
		// QUARENTA POR CENTO, e a fração é o conserto de uma briga.
		//
		// Havia três âncoras e nenhuma podia ganhar: 100 (o limiar do
		// traçado), 88 (a meia-altura da cápsula) e "abaixo de 94" (a fundura
		// das trinta travessias de vau). Escolher uma tornaria as outras duas
		// erradas.
		//
		// Como fração da altura, as três viram a mesma regra medida em pessoas
		// diferentes — e uma criatura miúda passa a se molhar onde uma
		// corpulenta passa seca, que é o que se vê num rio de verdade.
		constexpr float DaAlturaAteACintura = 0.40f;

		return FMath::Max(0.0f, HeightUnits) * DaAlturaAteACintura;
	}

	EWaterFooting At(const UIslandBakedPlan& Assado, const FVector2D& Onde)
	{
		return AtForHeight(Assado, Onde, DefaultHeightUnits());
	}

	EWaterFooting AtForHeight(const UIslandBakedPlan& Assado, const FVector2D& Onde,
		float Altura)
	{
		// FORA DA COSTA É MAR, e mar é FUNDO.
		//
		// Isto faltava, e quem achou foi a prova de que "onde o pé está seco
		// não há fluido": fora da terra, `FluidAt` dizia água salgada e `At`
		// dizia seco — as duas perguntas discordando sobre o mesmo lugar. O
		// painel teria dito "seco em água salgada", e o jogador aprenderia a
		// não confiar nele.
		//
		// O mar não está no traçado como curso; ele é o que sobra depois que a
		// terra acaba, e por isso nenhum laço sobre rios jamais o encontraria.
		if (Onde.Size() > Assado.CoastRadiusAt(FMath::Atan2(Onde.Y, Onde.X)))
		{
			return EWaterFooting::Fundo;
		}

		// A água mais FUNDA ganha: quem está no encontro de um córrego com um
		// rio está no rio. Parar na primeira água encontrada faria o resultado
		// depender da ordem do assado, que é desempate disfarçado de regra.
		float MaiorMeiaLargura = 0.0f;
		float MaiorFundura = 0.0f;

		// DE QUE ÁGUA veio a largura, e não só quanto ela mede.
		//
		// Sem isto, córrego e poço de fonte caem na mesma regra de largura — e
		// os dois são coisas opostas: um fio que se pisa, e uma bacia funda.
		bool bAchouCorrego = false;
		bool bAchouFonte = false;

		for (const FBakedRiver& Curso : Assado.Rivers)
		{
			float Meia = 0.0f;
			float Fundo = 0.0f;
			if (ChaoMolhado::DentroDaCalha(Curso.PointsUnits, Onde,
				[&Curso](int32 Ponto)
				{
					return Curso.HalfWidthUnits.IsValidIndex(Ponto)
						? Curso.HalfWidthUnits[Ponto] : 0.0f;
				}, Meia,
				[&Curso](int32 Ponto)
				{
					return Curso.DepthUnits.IsValidIndex(Ponto)
						? Curso.DepthUnits[Ponto] : 0.0f;
				}, Fundo))
			{
				MaiorMeiaLargura = FMath::Max(MaiorMeiaLargura, Meia);
				MaiorFundura = FMath::Max(MaiorFundura, Fundo);
			}

			// LAGO E POÇO SÃO ÁGUA PARADA E FUNDA — e agora eles dizem QUANTO.
			//
			// Antes contribuíam largura e nenhuma fundura, e caíam na regra de
			// largura junto com o córrego. Isso passou despercebido enquanto a
			// consulta às travessias respondia por eles; ao removê-la (M5), o
			// buraco apareceu: a travessia de vau em (40544,-44884) está a
			// 6122 unidades do lago do rio 52, cuja meia-largura é 7000 — ou
			// seja, DENTRO do lago — e nenhum rio a molha (o mais perto passa a
			// 1812 de um trecho de 1808).
			//
			// O poço tem fundura assada (`PlungePoolDepthUnits`); o lago não
			// tem uma própria, e a do curso no progresso do lago já a carrega —
			// é lá que a barriga funda entra em `DepthAtProgress`.
			if (Curso.bHasLake
				&& FVector2D::Distance(Onde, Curso.LakeCenterUnits)
					<= FreshWater::LakeHalfWidthUnits())
			{
				MaiorMeiaLargura =
					FMath::Max(MaiorMeiaLargura, FreshWater::LakeHalfWidthUnits());

				// A fundura do PONTO DO CURSO mais perto do centro do lago: a
				// barriga do lago já está em `DepthAtProgress`, e ler dali é
				// ler a mesma fonte que o resto.
				float NoLago = 0.0f;
				for (int32 Ponto = 0; Ponto < Curso.DepthUnits.Num(); ++Ponto)
				{
					if (Curso.PointsUnits.IsValidIndex(Ponto)
						&& FVector2D::Distance(Curso.PointsUnits[Ponto],
							Curso.LakeCenterUnits) <= FreshWater::LakeHalfWidthUnits())
					{
						NoLago = FMath::Max(NoLago, Curso.DepthUnits[Ponto]);
					}
				}

				MaiorFundura = FMath::Max(MaiorFundura, NoLago);
			}

			if (Curso.bHasFall
				&& FVector2D::Distance(Onde, Curso.FallCenterUnits)
					<= Curso.PlungePoolHalfWidthUnits)
			{
				MaiorMeiaLargura =
					FMath::Max(MaiorMeiaLargura, Curso.PlungePoolHalfWidthUnits);
				MaiorFundura =
					FMath::Max(MaiorFundura, Curso.PlungePoolDepthUnits);
			}
		}

		for (const FBakedBrook& Corrego : Assado.Brooks)
		{
			float Meia = 0.0f;
			float Fundo = 0.0f;
			if (ChaoMolhado::DentroDaCalha(Corrego.PointsUnits, Onde,
				[&Corrego](int32) { return Corrego.HalfWidthUnits; }, Meia,
				// O córrego NÃO TEM fundura por ponto, e zero aqui é "não
				// sei" — quem lê cai na largura, como o traçado já faz.
				[](int32) { return 0.0f; }, Fundo))
			{
				MaiorMeiaLargura = FMath::Max(MaiorMeiaLargura, Meia);
				bAchouCorrego = true;
			}
		}

		for (const FBakedSpring& Fonte : Assado.Springs)
		{
			if (FVector2D::Distance(Onde, Fonte.CenterUnits) <= Fonte.PoolHalfWidthUnits)
			{
				MaiorMeiaLargura = FMath::Max(MaiorMeiaLargura, Fonte.PoolHalfWidthUnits);
				bAchouFonte = true;
			}
		}

		if (MaiorMeiaLargura <= 0.0f)
		{
			return EWaterFooting::Seco;
		}

		// A FUNDURA DECIDE, quando ela é conhecida.
		//
		// ## O que estava aqui, e por que sai
		//
		// Antes, quem dizia onde se passa a pé era o TRAÇADO: percorria-se as
		// travessias marcadas como vau e perguntava-se se este ponto era uma
		// delas. O comentário explicava a escolha — *"o traçado escolhe o vau
		// pela FUNDURA no ponto, que é coisa que a largura não diz"*.
		//
		// Ou seja: a consulta às travessias era uma PROCURAÇÃO da fundura,
		// feita quando a fundura não existia. Agora ela existe (M3), e vem no
		// assado ponto a ponto.
		//
		// Manter as duas seria ter duas respostas para "posso passar aqui?" —
		// e a do traçado só sabe responder nos 37 pontos que ele marcou,
		// enquanto o jogador pisa em qualquer lugar do rio.
		//
		// ## E agora a resposta depende de QUEM pergunta
		//
		// A água não é funda: ela é funda para alguém. Passou da cintura,
		// nada; abaixo dela, anda.
		if (MaiorFundura > 0.0f)
		{
			return MaiorFundura <= WaistDepthUnitsFor(Altura)
				? EWaterFooting::Vau
				: EWaterFooting::Fundo;
		}

		// SEM FUNDURA CONHECIDA, e aqui está o CÓRREGO e a FONTE.
		//
		// Nenhum dos dois tem fundura por ponto, porque nenhum dos dois tem
		// curso com progresso e declive — não há o que variar neles.
		//
		// **E o córrego se atravessa a pé em qualquer ponto.** É o que o separa
		// do rio, que precisa de obra. Isto estava escrito no comentário e NÃO
		// no código: a afirmação vivia à sombra da consulta às travessias, que
		// devolvia vau ali por outro caminho. Ao remover aquela consulta —
		// procuração da fundura, morta na M5 —, tirei junto a única coisa que
		// dizia "aqui se passa".
		//
		// Medido: a travessia de vau em (13577,-37678) está a 187 unidades de
		// um córrego e a 3149 do rio mais perto. O pé achou água funda num
		// lugar onde o traçado promete passagem, e a culpa era desta omissão.
		// O CÓRREGO É VAU, e ponto. Não é a largura dele que decide: é o que ele
		// É. Um fio que liga uma fonte a um rio se atravessa em qualquer lugar,
		// e foi por isso que o traçado marcou vau ali.
		if (bAchouCorrego)
		{
			return EWaterFooting::Vau;
		}

		// O POÇO DA FONTE é o oposto: bacia parada, e funda. Cair na regra de
		// largura junto com o córrego faria a nascente e o fio que sai dela
		// serem a mesma coisa.
		if (bAchouFonte)
		{
			return EWaterFooting::Fundo;
		}

		// E o que sobra é água sem fundura conhecida que não é nem córrego nem
		// fonte — assado antigo, gravado antes da M3. A largura é o que resta,
		// e é a mesma medida que o mundo usava antes desta feature.
		return FreshWater::NavigabilityForHalfWidth(MaiorMeiaLargura)
			== FreshWater::ENavigability::APe
			? EWaterFooting::Vau
			: EWaterFooting::Fundo;
	}

	EFluidKind FluidAt(const UIslandBakedPlan& Assado, const FVector2D& Onde)
	{
		// FORA DA COSTA É MAR. Ele não está no traçado como curso — é o que
		// sobra depois que a terra acaba —, então a pergunta se responde pela
		// linha da costa, que o assado tem.
		if (Onde.Size() > Assado.CoastRadiusAt(FMath::Atan2(Onde.Y, Onde.X)))
		{
			return EFluidKind::AguaSalgada;
		}

		// A água mais FUNDA ganha, pela mesma razão do `At`: parar na primeira
		// encontrada faria o resultado depender da ordem do assado, que é
		// desempate disfarçado de regra.
		float MaiorMeiaLargura = 0.0f;
		float MaiorFundura = 0.0f;

		// DE QUE ÁGUA veio a largura, e não só quanto ela mede.
		//
		// Sem isto, córrego e poço de fonte caem na mesma regra de largura — e
		// os dois são coisas opostas: um fio que se pisa, e uma bacia funda.
		bool bAchouCorrego = false;
		bool bAchouFonte = false;
		EFluidKind Achado = EFluidKind::Nenhum;

		for (const FBakedRiver& Curso : Assado.Rivers)
		{
			for (int32 Ponto = 0; Ponto + 1 < Curso.PointsUnits.Num(); ++Ponto)
			{
				const float Meia = FMath::Max(
					Curso.HalfWidthUnits.IsValidIndex(Ponto)
						? Curso.HalfWidthUnits[Ponto] : 0.0f,
					Curso.HalfWidthUnits.IsValidIndex(Ponto + 1)
						? Curso.HalfWidthUnits[Ponto + 1] : 0.0f);

				if (Meia <= 0.0f || Meia <= MaiorMeiaLargura)
				{
					continue;
				}

				const FVector2D NoTrecho = FMath::ClosestPointOnSegment2D(
					Onde, Curso.PointsUnits[Ponto], Curso.PointsUnits[Ponto + 1]);

				if (FVector2D::DistSquared(NoTrecho, Onde) <= Meia * Meia)
				{
					MaiorMeiaLargura = Meia;

					// O fluido do PONTO, não do curso: termal é propriedade da
					// posição, e um rio nasce fervendo e chega frio ao mar.
					Achado = Curso.FluidByPoint.IsValidIndex(Ponto)
						? static_cast<EFluidKind>(Curso.FluidByPoint[Ponto])
						: EFluidKind::AguaDoce;
				}
			}
		}

		for (const FBakedBrook& Corrego : Assado.Brooks)
		{
			float Meia = 0.0f;
			// `FluidAt` pergunta DE QUE fluido é a água, não quanto ela funda.
			// A fundura entra e sai ignorada — dar-lhe um nome de variável
			// seria fingir que alguém a lê.
			float FunduraIgnorada = 0.0f;
			if (ChaoMolhado::DentroDaCalha(Corrego.PointsUnits, Onde,
				[&Corrego](int32) { return Corrego.HalfWidthUnits; }, Meia,
				[](int32) { return 0.0f; }, FunduraIgnorada)
				&& Meia > MaiorMeiaLargura)
			{
				MaiorMeiaLargura = Meia;
				Achado = static_cast<EFluidKind>(Corrego.Fluid);
			}
		}

		for (const FBakedSpring& Fonte : Assado.Springs)
		{
			if (FVector2D::Distance(Onde, Fonte.CenterUnits) <= Fonte.PoolHalfWidthUnits
				&& Fonte.PoolHalfWidthUnits > MaiorMeiaLargura)
			{
				MaiorMeiaLargura = Fonte.PoolHalfWidthUnits;
				Achado = static_cast<EFluidKind>(Fonte.Fluid);
			}
		}

		return Achado;
	}

	FVector2D FlowAt(const UIslandBakedPlan& Assado, const FVector2D& Onde,
		int32& ForcaPorMil)
	{
		ForcaPorMil = 0;

		// A água mais LARGA ganha, pela mesma razão de `At` e `FluidAt`: parar
		// na primeira encontrada faria o resultado depender da ordem do
		// assado, que é desempate disfarçado de regra.
		float MaiorMeiaLargura = 0.0f;
		float MaiorFundura = 0.0f;

		// DE QUE ÁGUA veio a largura, e não só quanto ela mede.
		//
		// Sem isto, córrego e poço de fonte caem na mesma regra de largura — e
		// os dois são coisas opostas: um fio que se pisa, e uma bacia funda.
		bool bAchouCorrego = false;
		bool bAchouFonte = false;
		FVector2D Rumo = FVector2D::ZeroVector;

		for (const FBakedRiver& Curso : Assado.Rivers)
		{
			for (int32 Ponto = 0; Ponto + 1 < Curso.PointsUnits.Num(); ++Ponto)
			{
				const float Meia = FMath::Max(
					Curso.HalfWidthUnits.IsValidIndex(Ponto)
						? Curso.HalfWidthUnits[Ponto] : 0.0f,
					Curso.HalfWidthUnits.IsValidIndex(Ponto + 1)
						? Curso.HalfWidthUnits[Ponto + 1] : 0.0f);

				if (Meia <= 0.0f || Meia <= MaiorMeiaLargura)
				{
					continue;
				}

				const FVector2D NoTrecho = FMath::ClosestPointOnSegment2D(
					Onde, Curso.PointsUnits[Ponto], Curso.PointsUnits[Ponto + 1]);

				if (FVector2D::DistSquared(NoTrecho, Onde) > Meia * Meia)
				{
					continue;
				}

				// O RUMO É O DO TRECHO, lido da ordem da polilinha — a mesma
				// fonte que a grade quantiza em oito. Deduzi-lo do raio ou do
				// declive seria a segunda verdade que a invariante proíbe.
				const FVector2D Passo =
					Curso.PointsUnits[Ponto + 1] - Curso.PointsUnits[Ponto];
				if (Passo.IsNearlyZero())
				{
					continue;
				}

				MaiorMeiaLargura = Meia;
				Rumo = Passo.GetSafeNormal();
				ForcaPorMil = Curso.FlowStrengthByPoint.IsValidIndex(Ponto)
					? Curso.FlowStrengthByPoint[Ponto]
					: 0;
			}
		}

		// Força sem rumo é zero, como na grade: uma corrente que empurra para
		// lugar nenhum sairia MUDA em vez de errada.
		if (Rumo.IsNearlyZero())
		{
			ForcaPorMil = 0;
		}

		return Rumo;
	}

	float SpeedMultiplierFor(EWaterFooting Chao)
	{
		switch (Chao)
		{
		case EWaterFooting::Vau:   return ChaoMolhado::PassoNoVau;
		case EWaterFooting::Fundo: return ChaoMolhado::PassoNoFundo;
		default: break;
		}
		return ChaoMolhado::PassoEmTerra;
	}

	float SpeedMultiplierAlong(EWaterFooting Chao, const FVector2D& Rumo,
		const FVector2D& Fluxo, int32 ForcaPorMil)
	{
		const float Parado = SpeedMultiplierFor(Chao);
		if (ForcaPorMil <= 0 || Rumo.IsNearlyZero() || Fluxo.IsNearlyZero())
		{
			return Parado;
		}

		// SÓ A COMPONENTE AO LONGO DO RUMO. Atravessar de través não é subir
		// nem descer o rio, e cobrar atraso ali castigaria justamente a
		// travessia — que é o movimento que o mapa mais pede.
		const float AoLongo = static_cast<float>(Rumo.GetSafeNormal()
			| Fluxo.GetSafeNormal());

		const float Fator = 1.0f + AoLongo
			* (static_cast<float>(ForcaPorMil) / 1000.0f) * ChaoMolhado::EmpurraoDaCorrente;

		return FMath::Max(Parado * ChaoMolhado::PisoContraACorrente, Parado * Fator);
	}

	const TCHAR* DebugName(EWaterFooting Chao)
	{
		switch (Chao)
		{
		case EWaterFooting::Vau:   return TEXT("vau");
		case EWaterFooting::Fundo: return TEXT("agua funda");
		default: break;
		}
		return TEXT("seco");
	}
}

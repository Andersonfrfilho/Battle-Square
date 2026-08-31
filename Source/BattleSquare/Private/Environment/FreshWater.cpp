// Copyright 2026 Anderson. All Rights Reserved.

#include "Environment/FreshWater.h"

#include "Battle/DeterministicSpread.h"
#include "Environment/CaveSystem.h"
#include "Environment/IslandFeatureLayout.h"
#include "Environment/IslandGeography.h"

namespace FreshWater
{
	namespace
	{
		/**
		 * A calha do rio comum.
		 *
		 * Larga o bastante para ser um rio visto de cima, e estreita o bastante
		 * para ainda se atravessar a pé: um rio que corta a ilha em duas sem
		 * ponte não é paisagem, é muro.
		 *
		 * A largura é FRAÇÃO do raio, e não unidades escritas à mão — a mesma
		 * armadilha que deixou os anéis das peças da ilha para trás quando o
		 * raio cresceu. Com 170 fixos numa ilha de 1,4 km, o rio tinha 1,7 m:
		 * um córrego que o passo do traçado de trilha nem enxergava.
		 */
		constexpr float FracaoDaCalhaDoRio = 0.0055f;

		/** O lago no auge: seis vezes o rio, o que já é lago e não poça. */
		constexpr float FracaoDaCalhaDoLago = 0.033f;

		float MeiaCalhaDoRio() { return IslandGeography::LandRadiusUnits() * FracaoDaCalhaDoRio; }
		float MeiaCalhaDoLago() { return IslandGeography::LandRadiusUnits() * FracaoDaCalhaDoLago; }

		/**
		 * Quantos rios desce cada monte.
		 *
		 * Dois, em flancos opostos: a montanha derrama para os dois lados, e
		 * um rio só por monte deixava a ilha inteira com três fios de água.
		 */
		constexpr int32 RiosPorMonte = 2;

		/** Quanto os dois rios de um monte se afastam, em graus. */
		constexpr float AberturaEntreIrmaos = 26.0f;





		/**
		 * Sobre quantas unidades de raio o rio engorda até virar lago.
		 *
		 * A transição é longa de propósito. Alargamento curto lê como um
		 * quadrado grudado no rio; longo lê como a água parando.
		 *
		 * Como a largura, ele é FRAÇÃO da calha do lago — e maior que ela, para
		 * o lago ser mais COMPRIDO que largo. Alargar a água sem alongar junto
		 * fez o lago virar uma bolha que engoliu a cachoeira do próprio rio, e
		 * três das seis grutas ficaram sem chão seco para nascer.
		 */
		float AlcanceDoLago() { return MeiaCalhaDoLago() * 1.7f; }

		/** Metade do comprimento da queda, medido no raio. */
		float MeiaQueda() { return MeiaCalhaDoRio() * 0.32f; }

		/** Da margem até o meio da trilha. */
		constexpr float AfastamentoDaTrilha = 260.0f;

		/** Metade da largura da trilha de beira-rio. */
		constexpr float MeiaTrilha = 95.0f;

		/**
		 * Onde a nascente fica, medida em saias de monte.
		 *
		 * No cume não pode: o monte é sólido e a água sairia de dentro da
		 * pedra. Na base também não: o rio começaria já na planície e ninguém
		 * ligaria uma coisa à outra. Meio caminho na encosta é o único ponto em
		 * que se vê o monte atrás da nascente.
		 */
		constexpr float SaiaDaNascente = 0.6f;

		/** Quanto o rumo abre para cada lado, no mínimo e no máximo. */
		constexpr float MenorSerpente = 0.045f;
		constexpr float MaiorSerpente = 0.085f;

		/** Quantas curvas completas da nascente à foz. */
		constexpr float MenosCurvas = 1.15f;
		constexpr float MaisCurvas = 1.85f;

		/** Onde o lago alaga, em fração do percurso. */
		constexpr float PrimeiroLago = 0.32f;
		constexpr float UltimoLago = 0.46f;

		/** Quanto depois do fim do lago a água despenca. */
		float MenorDegrau() { return MeiaCalhaDoLago() * 0.65f; }
		float MaiorDegrau() { return MeiaCalhaDoLago() * 2.4f; }

		/**
		 * A primeira distância tentada entre a queda e o centro da gruta, e o
		 * passo com que ela cresce quando a mais perto não serve.
		 *
		 * Começar perto e crescer é o que faz a gruta ficar ENCOSTADA na
		 * cachoeira: o primeiro lugar que serve ganha, e o primeiro lugar
		 * tentado é o mais próximo. Sorteando a distância, metade das grutas
		 * nasceria longe da única coisa que explica por que ela está ali.
		 *
		 * As distâncias saem da LARGURA DO RIO, não de unidades escritas à mão
		 * — e é do RIO, não do lago. A gruta é da CACHOEIRA, e na queda o rio
		 * já voltou a ser estreito; escalar pelo lago mandava a busca começar
		 * mais longe da queda do que o teste permite a gruta ficar.
		 *
		 * Errei as duas vezes pelo mesmo motivo em dois passos: primeiro o
		 * número era absoluto, depois era relativo à água errada. Relativo não
		 * basta — tem de ser relativo à coisa CERTA.
		 */
		float PrimeiraDistanciaDaGruta() { return MeiaCalhaDoRio() * 1.6f; }
		float PassoDaDistanciaDaGruta() { return MeiaCalhaDoRio() * 0.32f; }
		constexpr int32 DistanciasDaGruta = 16;

		/**
		 * Em quantas direções se procura ao redor da queda.
		 *
		 * Doze, como as horas de um relógio: perto do degrau o rio vem torto de
		 * tanto serpentear, e as duas perpendiculares à corrente — que era o
		 * que se tentava antes — apontam para a água em metade dos casos, porque
		 * a torção dá componente RADIAL à perpendicular e joga a gruta rio
		 * acima, dentro do lago.
		 */
		constexpr int32 RumosDaGruta = 12;

		/**
		 * A folga que um lugar precisa ter para ser aceito, em cima de caber.
		 *
		 * Sem ela a busca aceita o primeiro lugar que cabe, e "cabe" chegou a
		 * ser cinco unidades de margem: uma gruta encostada na calha, que
		 * qualquer mexida na serpentina do rio põe dentro da água. Caber não é
		 * critério; caber com sobra é.
		 */
		constexpr float FolgaDaGruta = 300.0f;

		/**
		 * O passo com que se percorre o rio ao medir a distância até a água.
		 *
		 * O curso é uma curva, e não há fórmula fechada para a distância de um
		 * ponto a ela: mede-se por amostragem. Passo curto custa tempo de
		 * planejamento; passo longo passa por cima de uma volta da serpentina.
		 */
		constexpr float PassoDeSondagemDoRio = 60.0f;
	}

	float RiverHalfWidthUnits() { return MeiaCalhaDoRio(); }
	float LakeHalfWidthUnits() { return MeiaCalhaDoLago(); }
	float FallHalfLengthUnits() { return MeiaQueda(); }
	float TrailOffsetUnits() { return AfastamentoDaTrilha; }
	float TrailHalfWidthUnits() { return MeiaTrilha; }

	int32 RiversPerMountain() { return RiosPorMonte; }

	namespace
	{
		/**
		 * Quantas fontes, e onde elas cabem.
		 *
		 * No MIOLO, entre a casa e a saia dos montes: é a faixa que os rios não
		 * alcançam, porque eles descem para fora. Sem fonte, essa faixa — que é
		 * justamente onde todo mundo mora — não tem uma gota.
		 */
		constexpr int32 QuantasFontes = 5;
		constexpr float PrimeiroAnelDeFonte = 0.22f;
		constexpr float UltimoAnelDeFonte = 0.52f;

		/** A poça da fonte: bem menor que um lago, e maior que a calha. */
		constexpr float FracaoDaPoca = 0.010f;

		/** O córrego é fio de água: atravessa-se a pé, e por isso não pede ponte. */
		constexpr float FracaoDoCorrego = 0.0022f;

		/** Em quantos trechos o córrego é descrito. */
		constexpr int32 TrechosDoCorrego = 12;

		/** Quanto ele serpenteia, em fração do próprio comprimento. */
		constexpr float SerpenteDoCorrego = 0.13f;
	}

	TArray<FSpring> PlanSprings()
	{
		TArray<FSpring> Fontes;

		for (int32 Indice = 0; Indice < QuantasFontes; ++Indice)
		{
			const uint32 Semente = BattleSpread::SeedFromText(
				FString::Printf(TEXT("fonte-do-miolo-%d"), Indice));

			// O rumo é espalhado por igual e depois EMPURRADO pelo sorteio: só
			// sorteado, cinco fontes cairiam em duas do mesmo lado, e o miolo
			// continuaria seco de um dos lados.
			const float Rumo = (360.0f * Indice / QuantasFontes)
				+ BattleSpread::Between(-24.0f, 24.0f, BattleSpread::Fraction(Semente, 0));

			const float Anel = IslandGeography::LandRadiusUnits() * BattleSpread::Between(
				PrimeiroAnelDeFonte, UltimoAnelDeFonte, BattleSpread::Fraction(Semente, 1));

			const float Radianos = FMath::DegreesToRadians(Rumo);

			FSpring Fonte;
			Fonte.CenterUnits = FVector2D(FMath::Cos(Radianos), FMath::Sin(Radianos)) * Anel;
			Fonte.PoolHalfWidthUnits = IslandGeography::LandRadiusUnits() * FracaoDaPoca;
			Fontes.Add(Fonte);
		}

		return Fontes;
	}

	TArray<FBrook> PlanBrooks()
	{
		TArray<FBrook> Corregos;
		const TArray<FRiverCourse> Rios = Plan();
		if (Rios.Num() == 0)
		{
			return Corregos;
		}

		const float MeiaCalha = IslandGeography::LandRadiusUnits() * FracaoDoCorrego;

		/** O ponto do rio mais perto de onde a fonte está. */
		auto NoRioMaisPerto = [&Rios](const FVector2D& Daqui)
		{
			FVector2D Melhor = PointAt(Rios[0], Rios[0].SourceRadiusUnits);
			float Menor = TNumericLimits<float>::Max();

			for (const FRiverCourse& Rio : Rios)
			{
				const float Passo = (Rio.MouthRadiusUnits - Rio.SourceRadiusUnits) / 60.0f;
				for (float Raio = Rio.SourceRadiusUnits; Raio <= Rio.MouthRadiusUnits; Raio += Passo)
				{
					const FVector2D Ali = PointAt(Rio, Raio);
					const float Daqui2 = FVector2D::DistSquared(Daqui, Ali);
					if (Daqui2 < Menor)
					{
						Menor = Daqui2;
						Melhor = Ali;
					}
				}
			}

			return Melhor;
		};

		/**
		 * Traça o fio de água. Ele SERPENTEIA: córrego reto é vala, e vala é
		 * obra de gente — a água nunca desce em linha.
		 */
		auto Fio = [MeiaCalha](const FVector2D& Daqui, const FVector2D& Prali, uint32 Semente)
		{
			FBrook Corrego;
			Corrego.HalfWidthUnits = MeiaCalha;

			const FVector2D AoLongo = Prali - Daqui;
			const FVector2D DeLado = FVector2D(-AoLongo.Y, AoLongo.X).GetSafeNormal();
			const float Amplitude = AoLongo.Size() * SerpenteDoCorrego
				* BattleSpread::Between(0.5f, 1.0f, BattleSpread::Fraction(Semente, 0));

			for (int32 Trecho = 0; Trecho <= TrechosDoCorrego; ++Trecho)
			{
				const float Onde = static_cast<float>(Trecho) / TrechosDoCorrego;

				// Meia onda: sai da fonte e chega no rio sem desvio, e a barriga
				// fica no meio. Onda inteira faria o córrego nascer torto.
				const float Desvio = FMath::Sin(Onde * PI) * Amplitude;
				Corrego.PointsUnits.Add(FMath::Lerp(Daqui, Prali, Onde) + DeLado * Desvio);
			}

			return Corrego;
		};

		int32 Indice = 0;
		for (const FSpring& Fonte : PlanSprings())
		{
			const uint32 Semente = BattleSpread::SeedFromText(
				FString::Printf(TEXT("corrego-da-fonte-%d"), Indice));
			Corregos.Add(Fio(Fonte.CenterUnits, NoRioMaisPerto(Fonte.CenterUnits), Semente));
			++Indice;
		}

		// E os córregos que ligam RIO A RIO, no meio do percurso: sem eles a
		// ilha tem seis fios paralelos e nada os une. Um por par vizinho.
		for (int32 Rio = 0; Rio + 1 < Rios.Num(); ++Rio)
		{
			const float NoMeio = (Rios[Rio].SourceRadiusUnits + Rios[Rio].MouthRadiusUnits) * 0.5f;
			const float NoMeioDoOutro =
				(Rios[Rio + 1].SourceRadiusUnits + Rios[Rio + 1].MouthRadiusUnits) * 0.5f;

			const uint32 Semente = BattleSpread::SeedFromText(
				FString::Printf(TEXT("corrego-entre-rios-%d"), Rio));

			Corregos.Add(Fio(PointAt(Rios[Rio], NoMeio),
				PointAt(Rios[Rio + 1], NoMeioDoOutro), Semente));
		}

		return Corregos;
	}

	TArray<FRiverCourse> Plan()
	{
		TArray<FRiverCourse> Cursos;
		const float Foz = IslandGeography::LandRadiusUnits();

		int32 Indice = 0;
		for (const IslandFeatureLayout::FFeaturePlacement& Peca : IslandFeatureLayout::Plan())
		{
			if (Peca.Feature != IslandFeatureLayout::EIslandFeature::WalkableMountain)
			{
				continue;
			}

			// DOIS rios por monte, em flancos opostos: a montanha derrama para
			// os dois lados, e um por monte deixava a ilha com três fios de
			// água que nenhuma trilha jamais cruzava.
			for (int32 Irmao = 0; Irmao < RiosPorMonte; ++Irmao)
			{
				FRiverCourse Curso;
				Curso.SourceRadiusUnits = Peca.RadiusUnits + Peca.ClearanceUnits * SaiaDaNascente;
				Curso.MouthRadiusUnits = Foz;

				const float Desvio = (static_cast<float>(Irmao)
					- (RiosPorMonte - 1) * 0.5f) * AberturaEntreIrmaos;
				Curso.BearingRadians =
					FMath::DegreesToRadians(Peca.AngleDegrees + Desvio);

				// A semente sai do ÍNDICE, não do ângulo: ângulo é float, e
				// float virando semente é a receita de o rio mudar de curva
				// porque alguém mexeu na terceira casa decimal do anel.
				const uint32 Semente = BattleSpread::SeedFromText(
					FString::Printf(TEXT("rio-da-montanha-%d"), Indice));

				Curso.MeanderRadians = BattleSpread::Between(MenorSerpente, MaiorSerpente,
					BattleSpread::Fraction(Semente, 0));
				Curso.MeanderTurns = BattleSpread::Between(MenosCurvas, MaisCurvas,
					BattleSpread::Fraction(Semente, 1));

				const float Percurso =
					FMath::Max(0.0f, Curso.MouthRadiusUnits - Curso.SourceRadiusUnits);
				Curso.LakeRadiusUnits = Curso.SourceRadiusUnits + Percurso * BattleSpread::Between(
					PrimeiroLago, UltimoLago, BattleSpread::Fraction(Semente, 2));

				// A queda vem SEMPRE depois do lago, e por soma em vez de
				// sorteio independente: sorteados à parte, um dia sairia uma
				// cachoeira no meio do lago — água caindo dentro de água parada.
				Curso.FallRadiusUnits = Curso.LakeRadiusUnits + AlcanceDoLago() + BattleSpread::Between(
					MenorDegrau(), MaiorDegrau(), BattleSpread::Fraction(Semente, 3));

				Cursos.Add(Curso);
				++Indice;
			}
		}

		return Cursos;
	}

	FVector2D PointAt(const FRiverCourse& Course, float RadiusUnits)
	{
		const float Percurso = Course.MouthRadiusUnits - Course.SourceRadiusUnits;
		const float Andado = (Percurso > KINDA_SMALL_NUMBER)
			? (RadiusUnits - Course.SourceRadiusUnits) / Percurso
			: 0.0f;

		const float Rumo = Course.BearingRadians + Course.MeanderRadians
			* FMath::Sin(2.0f * PI * Course.MeanderTurns * Andado);

		return FVector2D(FMath::Cos(Rumo) * RadiusUnits, FMath::Sin(Rumo) * RadiusUnits);
	}

	float HalfWidthAt(const FRiverCourse& Course, float RadiusUnits)
	{
		const float Distancia = FMath::Abs(RadiusUnits - Course.LakeRadiusUnits);
		if (Distancia >= AlcanceDoLago())
		{
			return MeiaCalhaDoRio();
		}

		const float Subida = 1.0f - Distancia / AlcanceDoLago();
		const float Suave = Subida * Subida * (3.0f - 2.0f * Subida);
		return MeiaCalhaDoRio() + (MeiaCalhaDoLago() - MeiaCalhaDoRio()) * Suave;
	}

	bool IsFallAt(const FRiverCourse& Course, float RadiusUnits)
	{
		return FMath::Abs(RadiusUnits - Course.FallRadiusUnits) <= MeiaQueda();
	}

	namespace
	{
		/** A menor distância entre um ponto e a água doce de toda a ilha. */
		float MargemDaAgua(const TArray<FRiverCourse>& Cursos, const FVector2D& Ponto)
		{
			float Menor = TNumericLimits<float>::Max();
			for (const FRiverCourse& Curso : Cursos)
			{
				for (float Raio = Curso.SourceRadiusUnits; Raio <= Curso.MouthRadiusUnits;
					Raio += PassoDeSondagemDoRio)
				{
					const float Daqui = static_cast<float>(
						FVector2D::Distance(Ponto, PointAt(Curso, Raio))) - HalfWidthAt(Curso, Raio);
					Menor = FMath::Min(Menor, Daqui);
				}
			}

			return Menor;
		}
	}

	TArray<IslandFeatureLayout::FFeaturePlacement> PlanGrottoes()
	{
		TArray<IslandFeatureLayout::FFeaturePlacement> Grutas;

		const TArray<FRiverCourse> Cursos = Plan();
		const TArray<IslandFeatureLayout::FFeaturePlacement> PecasDaIlha = IslandFeatureLayout::Plan();
		const IslandFeatureLayout::FIslandBounds Limites;

		for (int32 Indice = 0; Indice < Cursos.Num(); ++Indice)
		{
			const FVector2D NaQueda = PointAt(Cursos[Indice], Cursos[Indice].FallRadiusUnits);

			IslandFeatureLayout::FFeaturePlacement Gruta;
			Gruta.Feature = IslandFeatureLayout::EIslandFeature::Cave;
			Gruta.CaveSide = ACaveSystem::GrottoCaveSide;
			Gruta.ClearanceUnits = IslandFeatureLayout::CaveClearanceUnits(Gruta.CaveSide);

			// Água, sempre, e sem passar pelo temperador do plano da ilha: ele
			// dá água a quem está perto da ORLA, e a gruta da cachoeira tem água
			// por causa da cachoeira. O motivo é outro, e o raio não o conhece.
			Gruta.CaveFlavor = ECaveFlavor::Water;

			const uint32 Semente = BattleSpread::SeedFromText(
				FString::Printf(TEXT("gruta-da-queda-%d"), Indice));
			const int32 PrimeiroRumo = BattleSpread::Below(Semente, 0, RumosDaGruta);

			// O sorteio escolhe por onde COMEÇAR a procurar, não onde ficar. Era
			// ele que decidia o lado antes, e decidir sem olhar é como a gruta
			// ia para dentro do lago: a semente não sabe onde está a água.
			bool bAchou = false;
			for (int32 Tentativa = 0; Tentativa < DistanciasDaGruta && !bAchou; ++Tentativa)
			{
				const float Distancia = PrimeiraDistanciaDaGruta()
					+ PassoDaDistanciaDaGruta() * static_cast<float>(Tentativa);

				for (int32 Passo = 0; Passo < RumosDaGruta && !bAchou; ++Passo)
				{
					const float Rumo = 2.0f * PI
						* static_cast<float>((PrimeiroRumo + Passo) % RumosDaGruta)
						/ static_cast<float>(RumosDaGruta);

					const FVector2D Centro = NaQueda
						+ FVector2D(FMath::Cos(Rumo), FMath::Sin(Rumo)) * Distancia;

					Gruta.AngleDegrees = FMath::RadiansToDegrees(
						static_cast<float>(FMath::Atan2(Centro.Y, Centro.X)));
					Gruta.RadiusUnits = static_cast<float>(Centro.Size());

					// A folga entra ENGORDANDO a gruta para as perguntas, não
					// afrouxando as perguntas: assim quem decide se duas coisas
					// se tocam continua sendo o plano da ilha, uma vez só, e a
					// gruta não carrega uma segunda cópia dessas contas (L-032).
					IslandFeatureLayout::FFeaturePlacement Inflada = Gruta;
					Inflada.ClearanceUnits = Gruta.ClearanceUnits + FolgaDaGruta;

					if (MargemDaAgua(Cursos, Centro) <= Inflada.ClearanceUnits)
					{
						continue;
					}
					if (!IslandFeatureLayout::FitsOnLand(Inflada, Limites))
					{
						continue;
					}
					if (!IslandFeatureLayout::ClearsTrainingFields(Inflada, Limites))
					{
						continue;
					}

					bool bEncosta = false;
					for (const IslandFeatureLayout::FFeaturePlacement& Peca : PecasDaIlha)
					{
						bEncosta = bEncosta || IslandFeatureLayout::Overlaps(Inflada, Peca);
					}
					for (const IslandFeatureLayout::FFeaturePlacement& Outra : Grutas)
					{
						bEncosta = bEncosta || IslandFeatureLayout::Overlaps(Inflada, Outra);
					}
					if (bEncosta)
					{
						continue;
					}

					bAchou = true;
				}
			}

			// Sem lugar, sem gruta. Plantar onde não cabe devolveria o defeito
			// que a busca existe para não ter: caverna com a quina na água.
			if (bAchou)
			{
				Grutas.Add(Gruta);
			}
		}

		return Grutas;
	}
}

FreshWater::ENavigability FreshWater::NavigabilityForHalfWidth(float HalfWidthUnits)
{
	// Os cortes saem da largura do RIO comum, que é a régua natural: acima
	// dela passa barco grande, abaixo só o pequeno, e um terço dela já é
	// travessia a pé. Números soltos aqui perderiam o sentido no dia em que a
	// ilha mudasse de tamanho.
	const float DoRio = RiverHalfWidthUnits();

	if (HalfWidthUnits >= DoRio)
	{
		return ENavigability::BarcoGrande;
	}
	if (HalfWidthUnits >= DoRio * 0.30f)
	{
		return ENavigability::BarcoPequeno;
	}

	return ENavigability::APe;
}

TArray<FreshWater::FUnderwaterLink> FreshWater::PlanUnderwaterLinks()
{
	TArray<FUnderwaterLink> Passagens;

	// Cada gruta de cachoeira vira BOCA de passagem, ligada à gruta mais
	// próxima que ainda não tem par. Ligar todas a todas faria um queijo; uma
	// por gruta faz uma rede.
	const TArray<IslandFeatureLayout::FFeaturePlacement> Grutas = PlanGrottoes();

	TArray<bool> JaLigada;
	JaLigada.Init(false, Grutas.Num());

	for (int32 Daqui = 0; Daqui < Grutas.Num(); ++Daqui)
	{
		if (JaLigada[Daqui])
		{
			continue;
		}

		int32 Escolhida = INDEX_NONE;
		float Menor = TNumericLimits<float>::Max();

		for (int32 Prali = 0; Prali < Grutas.Num(); ++Prali)
		{
			if (Prali == Daqui || JaLigada[Prali])
			{
				continue;
			}

			const float Ate = FVector2D::DistSquared(
				Grutas[Daqui].CenterUnits(), Grutas[Prali].CenterUnits());
			if (Ate < Menor)
			{
				Menor = Ate;
				Escolhida = Prali;
			}
		}

		if (Escolhida == INDEX_NONE)
		{
			continue;
		}

		FUnderwaterLink Passagem;
		Passagem.FromUnits = Grutas[Daqui].CenterUnits();
		Passagem.ToUnits = Grutas[Escolhida].CenterUnits();

		// Passagem de pedra é apertada, sempre: se ela coubesse barco grande,
		// o atalho de baixo tornaria o rio de cima decorativo.
		Passagem.Navigability = ENavigability::BarcoPequeno;

		Passagens.Add(Passagem);
		JaLigada[Daqui] = true;
		JaLigada[Escolhida] = true;
	}

	return Passagens;
}

bool FreshWater::IsWaterNetworkConnected()
{
	const TArray<FRiverCourse> Rios = Plan();
	if (Rios.Num() == 0)
	{
		return true;
	}

	// Conjuntos disjuntos: cada rio começa sozinho, e cada ligação junta dois.
	// No fim, ou sobrou um conjunto — e dá para ir de barco a todo lugar — ou
	// sobrou mais de um, e há água que não se alcança.
	TArray<int32> Dono;
	Dono.Reserve(Rios.Num());
	for (int32 Indice = 0; Indice < Rios.Num(); ++Indice)
	{
		Dono.Add(Indice);
	}

	TFunction<int32(int32)> Raiz = [&Dono, &Raiz](int32 Qual)
	{
		return Dono[Qual] == Qual ? Qual : (Dono[Qual] = Raiz(Dono[Qual]));
	};

	auto Juntar = [&Dono, &Raiz](int32 Um, int32 Outro)
	{
		Dono[Raiz(Um)] = Raiz(Outro);
	};

	/** De que rio este ponto está mais perto. */
	auto RioDoPonto = [&Rios](const FVector2D& Onde)
	{
		int32 Melhor = 0;
		float Menor = TNumericLimits<float>::Max();

		for (int32 Indice = 0; Indice < Rios.Num(); ++Indice)
		{
			const float Passo =
				(Rios[Indice].MouthRadiusUnits - Rios[Indice].SourceRadiusUnits) / 40.0f;

			for (float Raio = Rios[Indice].SourceRadiusUnits;
				Raio <= Rios[Indice].MouthRadiusUnits; Raio += Passo)
			{
				const float Ate = FVector2D::DistSquared(Onde, PointAt(Rios[Indice], Raio));
				if (Ate < Menor)
				{
					Menor = Ate;
					Melhor = Indice;
				}
			}
		}

		return Melhor;
	};

	// Os CÓRREGOS ligam por cima.
	for (const FBrook& Corrego : PlanBrooks())
	{
		if (Corrego.PointsUnits.Num() < 2)
		{
			continue;
		}

		Juntar(RioDoPonto(Corrego.PointsUnits[0]), RioDoPonto(Corrego.PointsUnits.Last()));
	}

	// E as PASSAGENS ligam por baixo.
	for (const FUnderwaterLink& Passagem : PlanUnderwaterLinks())
	{
		Juntar(RioDoPonto(Passagem.FromUnits), RioDoPonto(Passagem.ToUnits));
	}

	for (int32 Indice = 1; Indice < Rios.Num(); ++Indice)
	{
		if (Raiz(Indice) != Raiz(0))
		{
			return false;
		}
	}

	return true;
}

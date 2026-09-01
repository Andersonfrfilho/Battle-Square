// Copyright 2026 Anderson. All Rights Reserved.

#include "World/TrailLayout.h"
#include "World/PlanReentryGuard.h"
#include "World/VillageLayout.h"
#include "Environment/IslandGeography.h"
#include "Battle/DeterministicSpread.h"
#include "Misc/ConfigCacheIni.h"
#include "Environment/FreshWater.h"
#include "Environment/IslandFeatureLayout.h"

namespace
{
	/**
	 * O passo do traçado, em fração do raio.
	 *
	 * Ele decide duas coisas ao mesmo tempo: a resolução do caminho e o custo
	 * de calculá-lo. Passo grande dá trilha angulosa; passo pequeno dá uma
	 * grade que não cabe no tempo de um teste.
	 *
	 * Ele foi de 0.012 para 0.0055 — de 1.680 unidades para 770.
	 *
	 * O passo grosso era o motivo real de a trilha subir o barranco de frente:
	 * a faixa tinha uma casa e meia, e não cabe ziguezague em uma casa e meia.
	 * Não adiantava encarecer o declive; o traçado não tinha onde zigzaguear.
	 *
	 * Custa: a grade quadruplica de casas. É o que a tabela de alturas paga —
	 * sem ela, isto não terminaria.
	 */
	constexpr float FracaoDoPasso = 0.0055f;

	/** Quanto a trilha limpa para cada lado. */
	constexpr float FracaoDaLargura = 0.55f;

	/**
	 * O que a trilha NÃO atravessa, e por que cada um.
	 *
	 * Não é lista de gosto: cada penalidade impede um caminho que seria
	 * legível como defeito na tela.
	 */
	constexpr float PenalidadeDaRochaQueimada = 40.0f;
	constexpr float PenalidadeDoBarranco = 25.0f;
	/**
	 * Atravessar o rio custa, mas NÃO é proibido — é aí que a ponte nasce.
	 *
	 * O número quase não importou, e descobrir isso foi o que consertou o
	 * defeito: com 12 a região não tinha ponte nenhuma, e baixar para 3 não
	 * mudou nada. **A penalidade nunca era aplicada.** O rio tem meia largura
	 * de 170 a 1.250 unidades e o passo do traçado é 1.680 — o caminho passava
	 * POR CIMA do rio sem nunca amostrar dentro dele.
	 *
	 * A correção não foi o peso: foi medir a água no TRECHO em vez de no
	 * ponto. Ponto amostrado não vê o que é mais estreito que o passo.
	 */
	constexpr float PenalidadeDaAgua = 3.0f;

	/** Quantas amostras dentro de um trecho, para ver o que é estreito. */
	constexpr int32 AmostrasPorTrecho = 6;

	/**
	 * Quantas vezes cortar os cantos. Duas já tiram a escadinha; mais que isso
	 * começa a encurtar a curva e a trilha deixa de passar por onde o traçado
	 * decidiu que ela deveria.
	 */
	constexpr int32 PassadasDeArredondamento = 2;

	/** Quantas vezes tentar entortar, e quanto a amplitude cresce a cada vez. */
	constexpr int32 TentativasDeTorcao = 6;
	constexpr float PassoDaTorcao = 1.6f;

	/** Quantos passos tem uma perna de ziguezague em encosta mansa, e o teto. */
	constexpr float PassosPorPerna = 9.0f;
	constexpr float MaiorPerna = 5.0f;

	/**
	 * O declive que uma trilha aguenta, e o que ela paga por passar dele.
	 *
	 * Dez por cento é o limite sustentável de quem constrói trilha de verdade:
	 * acima disso a água desce pelo caminho e o caminho vira valeta. Medi a
	 * nossa e deu **607%** no barranco — a trilha subia a mesa de frente, como
	 * ninguém sobe um morro.
	 *
	 * O limite não PROÍBE: ele encarece. Proibir criaria lugar inalcançável a
	 * pé, e a regra da região é que todo destino se alcança andando. Encarecer
	 * faz o traçado achar o ziguezague sozinho — que é o que a serra do mundo
	 * real faz, e ninguém desenhou.
	 */
	constexpr float DeclivelSustentavel = 0.10f;
	constexpr float PenalidadePorDeclive = 60.0f;

	/**
	 * A REGRA DA METADE: a trilha nunca passa de metade do declive da encosta
	 * que ela contorna. É o que faz o caminho abraçar a curva de nível em vez
	 * de cortá-la — e é o segundo motivo pelo qual trilha de montanha serpenteia.
	 */
	constexpr float FracaoDaEncosta = 0.5f;
	constexpr float PenalidadeDaMetade = 8.0f;

	float Passo() { return IslandGeography::LandRadiusUnits() * FracaoDoPasso; }

	int32 LadoDaGrade()
	{
		return FMath::CeilToInt((IslandGeography::LandRadiusUnits() * 2.0f) / Passo()) + 1;
	}

	FVector2D PontoDaCelula(int32 Coluna, int32 Linha)
	{
		const float Raio = IslandGeography::LandRadiusUnits();
		return FVector2D(-Raio + Coluna * Passo(), -Raio + Linha * Passo());
	}

	bool DentroDaGrade(int32 Coluna, int32 Linha)
	{
		const int32 Lado = LadoDaGrade();
		return Coluna >= 0 && Linha >= 0 && Coluna < Lado && Linha < Lado;
	}

	/** Perto de um rio, e por isso caro: só se atravessa onde há ponte. */
	bool NaAgua(const FVector2D& Onde)
	{
		// Uma pergunta só, e é a certa: a que distância da margem este ponto
		// está. A varredura por raio que estava aqui pulava os trechos que
		// correm de lado — e numa bacia de verdade quase todos correm.
		return FreshWater::IsFreshWaterAt(Onde);
	}

	/**
	 * O trecho inteiro cruza água.
	 *
	 * Perguntar só pelas PONTAS não serve: o rio é mais estreito que o passo
	 * do traçado, e um trecho salta por cima dele com as duas pontas secas.
	 */
	bool TrechoCruzaAgua(const FVector2D& Daqui, const FVector2D& Prali)
	{
		for (int32 Amostra = 0; Amostra <= AmostrasPorTrecho; ++Amostra)
		{
			const float Onde = static_cast<float>(Amostra) / static_cast<float>(AmostrasPorTrecho);
			if (NaAgua(FMath::Lerp(Daqui, Prali, Onde)))
			{
				return true;
			}
		}

		return false;
	}

	/**
	 * O custo de um passo. É `TravelCostBetween` mais o que a trilha evita.
	 *
	 * A conta base é a MESMA que cobra o cansaço de quem anda — é isso que faz
	 * a trilha ser um conselho honesto em vez de uma linha bonita.
	 */
	/**
	 * A altura de cada casa da grade, calculada UMA vez.
	 *
	 * Sem isto o traçado ficou lento demais para terminar: o custo de um passo
	 * pergunta a altura das duas pontas e a inclinação do destino — e a
	 * inclinação são mais quatro alturas. Seis consultas por aresta, oito
	 * arestas por casa, e cada consulta percorre os assentamentos da ilha.
	 *
	 * A tabela é do TAMANHO DA GRADE, não do mundo: ela existe enquanto o
	 * caminho é traçado e some depois.
	 */
	const TArray<float>& AlturasDaGrade()
	{
		static TArray<float> Alturas = []()
		{
			const int32 Lado = LadoDaGrade();
			TArray<float> Tudo;
			Tudo.Reserve(Lado * Lado);

			for (int32 Linha = 0; Linha < Lado; ++Linha)
			{
				for (int32 Coluna = 0; Coluna < Lado; ++Coluna)
				{
					Tudo.Add(IslandGeography::GroundHeightAt(PontoDaCelula(Coluna, Linha)));
				}
			}

			return Tudo;
		}();

		return Alturas;
	}

	float AlturaDaCelula(int32 Coluna, int32 Linha)
	{
		const int32 Lado = LadoDaGrade();
		const int32 C = FMath::Clamp(Coluna, 0, Lado - 1);
		const int32 L = FMath::Clamp(Linha, 0, Lado - 1);
		return AlturasDaGrade()[L * Lado + C];
	}

	/** Inclinação lida da tabela, por diferença nas quatro vizinhas. */
	float InclinacaoDaCelula(int32 Coluna, int32 Linha)
	{
		const float PorX =
			(AlturaDaCelula(Coluna + 1, Linha) - AlturaDaCelula(Coluna - 1, Linha))
				/ (2.0f * Passo());
		const float PorY =
			(AlturaDaCelula(Coluna, Linha + 1) - AlturaDaCelula(Coluna, Linha - 1))
				/ (2.0f * Passo());

		return FMath::Sqrt(PorX * PorX + PorY * PorY);
	}

	float CustoDoPasso(int32 ColunaDaqui, int32 LinhaDaqui, int32 ColunaPrali, int32 LinhaPrali)
	{
		const FVector2D Daqui = PontoDaCelula(ColunaDaqui, LinhaDaqui);
		const FVector2D Prali = PontoDaCelula(ColunaPrali, LinhaPrali);

		const float NoChao = FVector2D::Distance(Daqui, Prali);
		const float Desnivel = AlturaDaCelula(ColunaPrali, LinhaPrali)
			- AlturaDaCelula(ColunaDaqui, LinhaDaqui);

		// A conta base é a MESMA de `TravelCostBetween`, lida da tabela: é isso
		// que faz a trilha ser um conselho honesto em vez de uma linha bonita.
		// Subir custa muito; descer custa pouco, e nunca nada.
		float Custo = NoChao + ((Desnivel > 0.0f)
			? Desnivel * IslandGeography::UphillCostWeight()
			: (-Desnivel) * IslandGeography::DownhillCostWeight());

		const float AteOVulcao =
			FVector2D::Distance(Prali, IslandGeography::VolcanoCenterUnits());
		if (AteOVulcao <= IslandGeography::VolcanoScorchedRadiusUnits())
		{
			Custo *= PenalidadeDaRochaQueimada;
		}

		const float DaEncosta = InclinacaoDaCelula(ColunaPrali, LinhaPrali);

		// O barranco tem RAMPA, e a trilha deve achá-la sozinha. Penalizar a
		// faixa íngreme menos a rampa é o que faz o caminho contornar até
		// encontrar a subida — em vez de eu apontar a rampa para ele.
		if (!IslandGeography::IsOnBluffRamp(Prali) && DaEncosta > 0.25f)
		{
			Custo *= PenalidadeDoBarranco;
		}

		if (TrechoCruzaAgua(Daqui, Prali))
		{
			Custo *= PenalidadeDaAgua;
		}

		// O DECLIVE DO PASSO, e não a inclinação do lugar: é subir que custa, e
		// subir DE LADO numa encosta íngreme é justamente o que a trilha faz
		// para não subir de frente. É daqui que sai o ziguezague.
		if (NoChao > KINDA_SMALL_NUMBER)
		{
			const float Declive = FMath::Abs(Desnivel) / NoChao;

			if (Declive > DeclivelSustentavel)
			{
				Custo *= 1.0f + (Declive / DeclivelSustentavel - 1.0f) * PenalidadePorDeclive;
			}

			// E a regra da metade, medida contra a encosta do próprio lugar.
			if (DaEncosta > KINDA_SMALL_NUMBER && Declive > DaEncosta * FracaoDaEncosta)
			{
				Custo *= 1.0f + PenalidadeDaMetade
					* (Declive / (DaEncosta * FracaoDaEncosta) - 1.0f);
			}
		}

		return Custo;
	}

	/**
	 * Arredonda os cantos do caminho.
	 *
	 * O traçado anda em GRADE, com oito vizinhos, e por isso produz escadinha
	 * de 45° em terreno plano. Isso é artefato da grade, não do mundo: nenhuma
	 * trilha pisada por gente tem canto reto.
	 *
	 * O corte de cantos é aplicado em passadas: cada uma substitui cada canto
	 * por dois pontos a um quarto e a três quartos do trecho. As PONTAS ficam
	 * onde estavam, porque elas são os centros dos assentamentos.
	 *
	 * Arredondar move a linha para dentro da curva, então ela pode encostar em
	 * algo que o traçado evitava — e é por isso que os testes de "não corta a
	 * rocha queimada" e "não sai da terra" olham o caminho DEPOIS disto.
	 */
	TArray<FVector2D> Arredondar(const TArray<FVector2D>& Caminho)
	{
		TArray<FVector2D> Atual = Caminho;

		for (int32 Passada = 0; Passada < PassadasDeArredondamento; ++Passada)
		{
			if (Atual.Num() < 3)
			{
				break;
			}

			TArray<FVector2D> Macio;
			Macio.Reserve(Atual.Num() * 2);
			Macio.Add(Atual[0]);

			for (int32 Ponto = 0; Ponto + 1 < Atual.Num(); ++Ponto)
			{
				Macio.Add(FMath::Lerp(Atual[Ponto], Atual[Ponto + 1], 0.25f));
				Macio.Add(FMath::Lerp(Atual[Ponto], Atual[Ponto + 1], 0.75f));
			}

			Macio.Add(Atual.Last());
			Atual = MoveTemp(Macio);
		}

		return Atual;
	}

	/** A sinuosidade de um caminho: quanto ele anda dividido pela reta. */
	float SinuosidadeDe(const TArray<FVector2D>& Caminho)
	{
		if (Caminho.Num() < 2)
		{
			return 1.0f;
		}

		float Andado = 0.0f;
		for (int32 Ponto = 1; Ponto < Caminho.Num(); ++Ponto)
		{
			Andado += static_cast<float>(FVector2D::Distance(Caminho[Ponto - 1], Caminho[Ponto]));
		}

		const float EmLinhaReta =
			static_cast<float>(FVector2D::Distance(Caminho[0], Caminho.Last()));

		return (EmLinhaReta > KINDA_SMALL_NUMBER) ? Andado / EmLinhaReta : 1.0f;
	}

	/**
	 * SERPENTEIA um caminho que saiu reto demais.
	 *
	 * O traçado devolve reta quando o terreno é plano — e está certo, é o
	 * caminho mais barato. Mas caminho pisado por gente não sai reto nem em
	 * campo aberto: desvia-se de uma pedra, de uma poça, do sol.
	 *
	 * O desvio é LATERAL e vem de ruído coerente na posição, com meia onda ao
	 * longo do percurso para as pontas ficarem onde estavam — elas são a praça
	 * e o destino, e mexer nelas seria mudar o caminho, não entortá-lo.
	 *
	 * As pontas ficam FIXAS e o meio nunca sai da terra nem entra na rocha
	 * queimada: entortar não pode desfazer o que o traçado garantiu.
	 */
	TArray<FVector2D> Serpentear(const TArray<FVector2D>& Caminho, float Amplitude)
	{
		if (Caminho.Num() < 3)
		{
			return Caminho;
		}

		TArray<FVector2D> Torto;
		Torto.Reserve(Caminho.Num());
		Torto.Add(Caminho[0]);

		const float Escala = IslandGeography::LandRadiusUnits() * 0.05f;

		for (int32 Ponto = 1; Ponto + 1 < Caminho.Num(); ++Ponto)
		{
			const FVector2D AoLongo =
				(Caminho[Ponto + 1] - Caminho[Ponto - 1]).GetSafeNormal();
			const FVector2D DeLado(-AoLongo.Y, AoLongo.X);

			const float Quanto = static_cast<float>(Ponto) / static_cast<float>(Caminho.Num() - 1);
			const float Meia = FMath::Sin(Quanto * PI);

			const uint32 Semente = BattleSpread::SeedFromText(FString::Printf(
				TEXT("torce-trilha-%d-%d"),
				FMath::FloorToInt(Caminho[Ponto].X / Escala),
				FMath::FloorToInt(Caminho[Ponto].Y / Escala)));

			const float Desvio = BattleSpread::Between(-1.0f, 1.0f,
				BattleSpread::Fraction(Semente, 0)) * Amplitude * Meia;

			const FVector2D Candidato = Caminho[Ponto] + DeLado * Desvio;

			const bool bServe = IslandGeography::IsOnLand(Candidato)
				&& FVector2D::Distance(Candidato, IslandGeography::VolcanoCenterUnits())
					> IslandGeography::VolcanoScorchedRadiusUnits();

			Torto.Add(bServe ? Candidato : Caminho[Ponto]);
		}

		Torto.Add(Caminho.Last());
		return Torto;
	}

	/**
	 * ZIGUEZAGUEIA os trechos íngremes. É o que uma trilha de serra é.
	 *
	 * E não é enfeite: é a única forma de subir dentro do declive sustentável.
	 * Para vencer uma altura H a dez por cento o caminho precisa de dez vezes H
	 * de percurso, e esse percurso só cabe na encosta indo DE LADO.
	 *
	 * A geometria sai da regra, não de um desenho: numa encosta de declive G,
	 * atravessá-la num ângulo θ com a curva de nível dá declive G·sen(θ). Para
	 * o resultado ser o sustentável S, sen(θ) = S/G — quanto mais íngreme a
	 * encosta, mais rasante a perna, e mais fechado o zigue.
	 *
	 * O lado ALTERNA a cada volta. Ziguezague que não alterna é uma diagonal.
	 */
	TArray<FVector2D> Ziguezaguear(const TArray<FVector2D>& Caminho)
	{
		if (Caminho.Num() < 3)
		{
			return Caminho;
		}

		TArray<FVector2D> Torto;
		Torto.Reserve(Caminho.Num());
		Torto.Add(Caminho[0]);

		float ParaQualLado = 1.0f;
		int32 DesdeAVirada = 0;

		for (int32 Ponto = 1; Ponto + 1 < Caminho.Num(); ++Ponto)
		{
			const float DaEncosta = IslandGeography::GroundSlopeAt(Caminho[Ponto]);

			if (DaEncosta <= DeclivelSustentavel)
			{
				// Chão manso não pede ziguezague: a trilha vai reta e quem sobe
				// nem percebe. Zigzaguear no plano é rodeio sem motivo.
				Torto.Add(Caminho[Ponto]);
				DesdeAVirada = 0;
				continue;
			}

			// O seno do ângulo com a curva de nível, e daí o quanto a perna se
			// afasta da linha direta.
			const float SenoDaPerna =
				FMath::Clamp(DeclivelSustentavel / DaEncosta, 0.05f, 1.0f);
			const float Afastar = Passo() * (1.0f / SenoDaPerna - 1.0f);

			const FVector2D AoLongo =
				(Caminho[Ponto + 1] - Caminho[Ponto - 1]).GetSafeNormal();
			const FVector2D DeLado(-AoLongo.Y, AoLongo.X);

			const FVector2D Candidato = Caminho[Ponto]
				+ DeLado * (ParaQualLado * FMath::Min(Afastar, Passo() * MaiorPerna));

			// Entortar não pode desfazer o que o traçado garantiu.
			const bool bServe = IslandGeography::IsOnLand(Candidato)
				&& FVector2D::Distance(Candidato, IslandGeography::VolcanoCenterUnits())
					> IslandGeography::VolcanoScorchedRadiusUnits();

			Torto.Add(bServe ? Candidato : Caminho[Ponto]);

			// Vira o lado a cada perna cumprida. A perna é mais longa em
			// encosta mansa e mais curta em encosta brava, que é o que faz o
			// ziguezague apertar onde precisa.
			++DesdeAVirada;
			const int32 PernaEmPassos = FMath::Max(2,
				FMath::RoundToInt(PassosPorPerna * SenoDaPerna));

			if (DesdeAVirada >= PernaEmPassos)
			{
				ParaQualLado = -ParaQualLado;
				DesdeAVirada = 0;
			}
		}

		Torto.Add(Caminho.Last());
		return Torto;
	}

	/** Menor custo entre dois pontos, por Dijkstra sobre a grade. */
	TArray<FVector2D> CaminhoBarato(const FVector2D& Daqui, const FVector2D& Prali, bool& bOutDesistiu)
	{
		const int32 Lado = LadoDaGrade();
		const float MeioPasso = Passo() * 0.5f;
		const float Raio = IslandGeography::LandRadiusUnits();

		auto ParaCelula = [&](const FVector2D& Onde)
		{
			return FIntPoint(
				FMath::Clamp(FMath::RoundToInt((Onde.X + Raio) / Passo()), 0, Lado - 1),
				FMath::Clamp(FMath::RoundToInt((Onde.Y + Raio) / Passo()), 0, Lado - 1));
		};

		const FIntPoint Origem = ParaCelula(Daqui);
		const FIntPoint Destino = ParaCelula(Prali);

		TArray<float> Melhor;
		Melhor.Init(TNumericLimits<float>::Max(), Lado * Lado);
		TArray<int32> DeOndeVeio;
		DeOndeVeio.Init(INDEX_NONE, Lado * Lado);

		auto Indice = [Lado](const FIntPoint& Celula) { return Celula.Y * Lado + Celula.X; };

		// Fila de prioridade simples: o heap da engine, ordenado por custo.
		struct FNaFila
		{
			float Custo = 0.0f;
			int32 Onde = 0;
			bool operator<(const FNaFila& Outro) const { return Custo < Outro.Custo; }
		};

		TArray<FNaFila> Fila;
		Melhor[Indice(Origem)] = 0.0f;
		Fila.HeapPush(FNaFila{ 0.0f, Indice(Origem) });

		const int32 NoDestino = Indice(Destino);

		while (Fila.Num() > 0)
		{
			FNaFila Atual;
			Fila.HeapPop(Atual, EAllowShrinking::No);

			if (Atual.Onde == NoDestino)
			{
				break;
			}
			if (Atual.Custo > Melhor[Atual.Onde])
			{
				continue;
			}

			const int32 Coluna = Atual.Onde % Lado;
			const int32 Linha = Atual.Onde / Lado;

			for (int32 dY = -1; dY <= 1; ++dY)
			{
				for (int32 dX = -1; dX <= 1; ++dX)
				{
					if (dX == 0 && dY == 0 || !DentroDaGrade(Coluna + dX, Linha + dY))
					{
						continue;
					}

					const FVector2D Vizinho = PontoDaCelula(Coluna + dX, Linha + dY);

					// Fora da terra não há trilha, e a praia é o limite: o
					// caminho não passa pela água salgada.
					// A trilha ENTRA na areia, e para perto da água.
					//
					// Ela parava na linha da praia, e foi isso que deixou o
					// cais inalcançável quando ele desceu para a areia — o
					// mesmo defeito que produziu sete linhas retas.
					if (Vizinho.Size()
						>= Raio - IslandGeography::BeachWidthUnits() * 0.25f + MeioPasso)
					{
						const bool bEhOFim = (Indice(FIntPoint(Coluna + dX, Linha + dY)) == NoDestino);
						if (!bEhOFim)
						{
							continue;
						}
					}

					const int32 Ali = Indice(FIntPoint(Coluna + dX, Linha + dY));
					const float Custo = Atual.Custo
						+ CustoDoPasso(Coluna, Linha, Coluna + dX, Linha + dY);

					if (Custo < Melhor[Ali])
					{
						Melhor[Ali] = Custo;
						DeOndeVeio[Ali] = Atual.Onde;
						Fila.HeapPush(FNaFila{ Custo, Ali });
					}
				}
			}
		}

		TArray<FVector2D> Caminho;
		if (Melhor[NoDestino] == TNumericLimits<float>::Max())
		{
			// Sem caminho: a linha reta, E O AVISO. Devolver vazio faria a
			// trilha sumir sem dizer por quê; devolver a reta calada faz pior,
			// porque ela se parece com um caminho bom.
			bOutDesistiu = true;
			Caminho.Add(Daqui);
			Caminho.Add(Prali);
			return Caminho;
		}

		for (int32 No = NoDestino; No != INDEX_NONE; No = DeOndeVeio[No])
		{
			Caminho.Add(PontoDaCelula(No % Lado, No / Lado));
		}
		Algo::Reverse(Caminho);

		// As pontas são os CENTROS dos assentamentos, não o centro da célula:
		// a trilha tem de encostar na praça, não parar a meio passo dela.
		if (Caminho.Num() > 0)
		{
			Caminho[0] = Daqui;
			Caminho.Last() = Prali;
		}

		return Arredondar(Caminho);
	}

	FVector2D CentroDe(ESettlementKind Tipo)
	{
		for (const FSettlementPlacement& Assentamento : RegionLayout::Plan())
		{
			if (Assentamento.Kind == Tipo)
			{
				return Assentamento.CenterUnits;
			}
		}

		return FVector2D::ZeroVector;
	}

	/**
	 * O nome é ESPECÍFICO de propósito.
	 *
	 * `Tracar` existia aqui e em `LandUseLayout.cpp`, os dois em namespace
	 * anônimo — e o unity build junta os arquivos numa unidade só, onde os
	 * dois viram sobrecarga que difere apenas no retorno. É a L-042, que a
	 * gente conhecia de HELPER DE TESTE, aparecendo em código de produção.
	 *
	 * A sonda `audit_test_helper_names.sh` não pega isto: ela olha testes.
	 */
	TArray<FTrailRoute> TracarTrilhas()
	{
		TArray<FTrailRoute> Trilhas;

		auto Ligar = [&Trilhas](ESettlementKind Daqui, const FVector2D& Origem,
			ESettlementKind Prali, const FVector2D& Destino,
			ETrailDestination Tipo = ETrailDestination::Assentamento)
		{
			FTrailRoute Trilha;
			Trilha.From = Daqui;
			Trilha.To = Prali;
			Trilha.Destination = Tipo;
			Trilha.PointsUnits = CaminhoBarato(Origem, Destino,
				Trilha.bFellBackToStraightLine);

			// PRIMEIRO o ziguezague das encostas: onde a subida é brava, a
			// trilha vai de lado, e isso muda o comprimento dela.
			if (!Trilha.bFellBackToStraightLine)
			{
				const TArray<FVector2D> Direto = Trilha.PointsUnits;
				Trilha.PointsUnits = Ziguezaguear(Direto);

				// E o TETO. Sem ele o ziguezague deu 8,7 vezes a linha reta —
				// escada de caracol, não trilha. Passando do teto, volta-se ao
				// caminho direto: melhor uma subida franca que um rodeio que
				// ninguém percorre.
				if (SinuosidadeDe(Trilha.PointsUnits) > TrailLayout::MaximumSinuosity())
				{
					Trilha.PointsUnits = Direto;
				}
			}

			// DEPOIS identifica a trilha que saiu reta e a entorta.
			//
			// Nesta ordem porque o ziguezague já pode ter dado sinuosidade de
			// sobra — serpentear em cima dele seria curva sobre curva.
			//
			// A amplitude cresce até a sinuosidade passar do mínimo, e para no
			// primeiro valor que serve — mais que isso vira serpentina, e
			// serpentina é tão artificial quanto a reta.
			//
			// O caminho de EMERGÊNCIA não é entortado: ele não passou pelo
			// terreno, e curvá-lo faria uma falha parecer um caminho bom.
			if (!Trilha.bFellBackToStraightLine && !TrailLayout::AllowsStraightTrails())
			{
				for (int32 Tentativa = 1; Tentativa <= TentativasDeTorcao; ++Tentativa)
				{
					if (SinuosidadeDe(Trilha.PointsUnits) >= TrailLayout::MinimumSinuosity())
					{
						break;
					}

					Trilha.PointsUnits = Serpentear(Trilha.PointsUnits,
						TrailLayout::HalfWidthUnits() * Tentativa * PassoDaTorcao);
				}
			}

			Trilhas.Add(MoveTemp(Trilha));
		};

		/**
		 * Quem PARTE para o marco natural é o assentamento mais perto dele.
		 *
		 * Puxar tudo de casa faria seis trilhas saindo da mesma praça e
		 * atravessando a ilha inteira. O caminho até a cachoeira sai de onde
		 * alguém que a queira ver estaria.
		 */
		auto MaisPertoDe = [](const FVector2D& Alvo)
		{
			FSettlementPlacement Escolhido = RegionLayout::Plan()[0];
			float Menor = TNumericLimits<float>::Max();

			for (const FSettlementPlacement& Assentamento : RegionLayout::Plan())
			{
				if (Assentamento.Kind == ESettlementKind::PostoDeFronteira)
				{
					continue;
				}

				const float Daqui = FVector2D::Distance(Assentamento.CenterUnits, Alvo);
				if (Daqui < Menor)
				{
					Menor = Daqui;
					Escolhido = Assentamento;
				}
			}

			return Escolhido;
		};

		const FVector2D DeCasa = CentroDe(ESettlementKind::VilaInicial);
		const FVector2D DaAcademia = CentroDe(ESettlementKind::VilaDaAcademia);
		const FVector2D DoMercado = CentroDe(ESettlementKind::VilaDoMercado);
		const FVector2D DaCidade = CentroDe(ESettlementKind::CidadeGrande);

		// ESTRELA a partir de casa: o primeiro caminho que se aprende é o de
		// sair de casa e voltar.
		Ligar(ESettlementKind::VilaInicial, DeCasa, ESettlementKind::VilaDaAcademia, DaAcademia);
		Ligar(ESettlementKind::VilaInicial, DeCasa, ESettlementKind::VilaDoMercado, DoMercado);
		Ligar(ESettlementKind::VilaInicial, DeCasa, ESettlementKind::CidadeGrande, DaCidade);

		// A ligação direta entre as duas vilas, porque a spec da região diz que
		// elas ficam a quatro minutos de casa E UMA DA OUTRA. Sem ela, ir de
		// uma à outra seria passar por casa.
		Ligar(ESettlementKind::VilaDaAcademia, DaAcademia, ESettlementKind::VilaDoMercado, DoMercado);

		// Os postos saem da CIDADE, não de casa: é a cidade que dá o ranking
		// que abre a porta, e o caminho deve dizer isso.
		for (const FSettlementPlacement& Assentamento : RegionLayout::Plan())
		{
			if (Assentamento.Kind == ESettlementKind::PostoDeFronteira)
			{
				Ligar(ESettlementKind::CidadeGrande, DaCidade,
					ESettlementKind::PostoDeFronteira, Assentamento.CenterUnits);
			}
		}

		// AS CACHOEIRAS. Cada uma ganha caminho, e é aqui que a ponte nasce: um
		// caminho até a queda tem de chegar na margem do rio.
		//
		// A trilha para na MARGEM, afastada pela largura da água mais a folga
		// da faixa limpa. Mirar o ponto exato da queda mandaria o traçado para
		// dentro do rio, e ele pagaria a penalidade da água até o fim.
		// TRONCOS, e não todo curso. O galho de cabeceira não tem queda, e o
		// raio que eu escolhi para dizer "não tem" é uma posição FORA da ilha
		// — a trilha foi mirar o mar aberto a 740 metros da costa.
		//
		// Sinalizar ausência com um valor fora de faixa só funciona enquanto
		// ninguém lê o valor. Três lugares leram.
		// TODOS os cursos, não só os troncos.
		//
		// A queda deixou de morar no tronco quando passou a ser escolhida pelo
		// leito — ela está onde a água despenca, que é rio acima. Percorrer só
		// os troncos deixava a maioria das cachoeiras sem caminho, e cachoeira
		// sem caminho é a que o relato de jogo disse nunca ter visto.
		for (const FreshWater::FRiverCourse& Rio : FreshWater::Plan())
		{
			// Só quem TEM queda ganha caminho. Prometer trilha para uma
			// cachoeira que não existe foi o que gerou sete linhas retas.
			if (!Rio.HasFall())
			{
				continue;
			}

			const FVector2D NaQueda = FreshWater::PointAtProgress(Rio, Rio.FallAtProgress);
			const float Afastar = FreshWater::HalfWidthAtProgress(Rio, Rio.FallAtProgress)
				+ TrailLayout::HalfWidthUnits() * 2.0f;

			const FSettlementPlacement Parte = MaisPertoDe(NaQueda);

			// PERPENDICULAR ao rio, e não na direção da vila.
			//
			// Afastar rumo à vila parecia óbvio e estava errado: essa direção
			// pode correr AO LONGO da água em vez de para longe dela, e aí a
			// ponta da trilha fica dentro do rio um pouco mais acima. O teste
			// pegou, e a geometria certa é sair de lado.
			//
			// O lado escolhido é o da vila — das duas margens, a trilha para na
			// que fica do lado de quem chega.
			const FVector2D UmPoucoAcima = FreshWater::PointAtProgress(Rio,
				Rio.FallAtProgress + FreshWater::FallHalfLengthUnits()
					/ FMath::Max(1.0f, FreshWater::CourseLengthUnits(Rio)));
			const FVector2D AoLongo = (UmPoucoAcima - NaQueda).GetSafeNormal();
			const FVector2D DeLado(-AoLongo.Y, AoLongo.X);

			const float ParaQualLado = FVector2D::DotProduct(
				Parte.CenterUnits - NaQueda, DeLado) >= 0.0f ? 1.0f : -1.0f;

			Ligar(Parte.Kind, Parte.CenterUnits, Parte.Kind,
				NaQueda + DeLado * (ParaQualLado * Afastar), ETrailDestination::Cachoeira);
		}

		// OS MONTES. O caminho para na saia, não no cume: o monte é sólido, e
		// uma trilha mirando o topo entraria na pedra.
		for (const IslandFeatureLayout::FFeaturePlacement& Peca : IslandFeatureLayout::Plan())
		{
			if (Peca.Feature != IslandFeatureLayout::EIslandFeature::WalkableMountain)
			{
				continue;
			}

			const FVector2D NoCume = Peca.CenterUnits();
			const FSettlementPlacement Parte = MaisPertoDe(NoCume);
			const FVector2D ParaASaia = (Parte.CenterUnits - NoCume).GetSafeNormal();

			Ligar(Parte.Kind, Parte.CenterUnits, Parte.Kind,
				NoCume + ParaASaia * (Peca.ClearanceUnits + TrailLayout::HalfWidthUnits()),
				ETrailDestination::Monte);
		}

		return Trilhas;
	}
}

float TrailLayout::MinimumSinuosity()
{
	// 1,06 é pouco e é de propósito: o alvo não é fazer serpentina, é tirar a
	// reta. Uma trilha de mil metros com 1,06 anda sessenta a mais que a reta,
	// o que é uma curva por quilômetro — que é o que uma trilha real faz.
	return 1.06f;
}

float TrailLayout::MaximumSinuosity()
{
	return 4.0f;
}

bool TrailLayout::AllowsStraightTrails()
{
	bool bDeixa = false;
	if (GConfig)
	{
		GConfig->GetBool(TEXT("/Script/BattleSquare.BattleArena"),
			TEXT("WorldAllowStraightTrails"), bDeixa, GGameIni);
	}

	return bDeixa;
}

float TrailLayout::StepUnits()
{
	return Passo();
}

float TrailLayout::HalfWidthUnits()
{
	return VillageLayout::PlotHalfExtentUnits() * FracaoDaLargura;
}

const TArray<FTrailRoute>& TrailLayout::Plan()
{
	// Traçado uma vez. Dijkstra sobre a ilha inteira não é conta para se
	// refazer a cada árvore plantada — e `BlocksPlanting` pergunta por ponto.
	static TArray<FTrailRoute> Trilhas = []()
	{
		const FPlanReentryGuard Guarda(TEXT("TrailLayout::Plan"));
		return TracarTrilhas();
	}();
	return Trilhas;
}

bool TrailLayout::IsOnTrail(const FVector2D& PositionUnits)
{
	const float Metade = HalfWidthUnits();
	const float MetadeAoQuadrado = Metade * Metade;

	for (const FTrailRoute& Trilha : Plan())
	{
		for (int32 Ponto = 1; Ponto < Trilha.PointsUnits.Num(); ++Ponto)
		{
			const FVector2D Daqui = Trilha.PointsUnits[Ponto - 1];
			const FVector2D Prali = Trilha.PointsUnits[Ponto];
			const FVector2D Trecho = Prali - Daqui;

			const float Comprimento = Trecho.SizeSquared();
			if (Comprimento <= 0.0f)
			{
				continue;
			}

			const float Onde = FMath::Clamp(
				FVector2D::DotProduct(PositionUnits - Daqui, Trecho) / Comprimento, 0.0f, 1.0f);
			const FVector2D MaisPerto = Daqui + Trecho * Onde;

			if (FVector2D::DistSquared(PositionUnits, MaisPerto) <= MetadeAoQuadrado)
			{
				return true;
			}
		}
	}

	return false;
}

float TrailLayout::WadableDepthUnits()
{
	// Um metro. Acima disso ninguém atravessa a pé carregando coisa, e é o
	// mesmo limite que o combate usa para dizer que a água deixou de ser poça.
	return 100.0f;
}

namespace
{
	/**
	 * A fundura da água num ponto, estimada pela LARGURA da calha.
	 *
	 * Rio largo é rio fundo — a relação vale em qualquer escala, e é a única
	 * medida que este mundo tem sem modelar o leito. Um córrego de três metros
	 * dá dez centímetros; um tronco de vinte e oito metros dá quase dois.
	 *
	 * Estimar não é inventar: a alternativa seria um campo de profundidade
	 * separado, que é uma segunda fonte da mesma verdade (L-032).
	 */
	constexpr float FunduraSobreLargura = 0.065f;

	float FunduraEm(const FVector2D& Onde)
	{
		float MaisFunda = 0.0f;

		for (const FreshWater::FRiverCourse& Curso : FreshWater::Plan())
		{
			float NoCurso = 0.0f;
			const float Ate = FreshWater::NearestOn(Curso, Onde, NoCurso);
			const float Meia = FreshWater::HalfWidthAtProgress(Curso, NoCurso);

			if (Ate <= Meia)
			{
				MaisFunda = FMath::Max(MaisFunda, Meia * 2.0f * FunduraSobreLargura);
			}
		}

		for (const FreshWater::FBrook& Corrego : FreshWater::PlanBrooks())
		{
			for (int32 Ponto = 1; Ponto < Corrego.PointsUnits.Num(); ++Ponto)
			{
				const FVector2D Trecho = Corrego.PointsUnits[Ponto] - Corrego.PointsUnits[Ponto - 1];
				const float Comprimento = static_cast<float>(Trecho.SizeSquared());
				if (Comprimento <= KINDA_SMALL_NUMBER)
				{
					continue;
				}

				const float Aonde = FMath::Clamp(static_cast<float>(FVector2D::DotProduct(
					Onde - Corrego.PointsUnits[Ponto - 1], Trecho)) / Comprimento, 0.0f, 1.0f);
				const FVector2D MaisPerto = Corrego.PointsUnits[Ponto - 1] + Trecho * Aonde;

				if (FVector2D::Distance(Onde, MaisPerto) <= Corrego.HalfWidthUnits)
				{
					MaisFunda = FMath::Max(MaisFunda,
						Corrego.HalfWidthUnits * 2.0f * FunduraSobreLargura);
				}
			}
		}

		return MaisFunda;
	}

	/** A partir de que inclinação a margem é degrau, e não beira. */
	constexpr float MargemVirouBarranco = 0.35f;

	/**
	 * O maior vão que uma ponte deste mundo vence.
	 *
	 * Vinte e cinco metros. Acima disso a obra deixa de ser tronco atravessado
	 * e vira engenharia — e engenharia é uma civilização que este mundo não
	 * tem. O que se faz num rio largo é atravessar flutuando.
	 */
	float MaiorVaoDePonte() { return 2500.0f; }

	/** A largura da água num ponto, para saber se a ponte alcança a outra margem. */
	float LarguraDaAguaEm(const FVector2D& Onde)
	{
		float MaisLarga = 0.0f;

		for (const FreshWater::FRiverCourse& Curso : FreshWater::Plan())
		{
			float NoCurso = 0.0f;
			const float Ate = FreshWater::NearestOn(Curso, Onde, NoCurso);
			const float Meia = FreshWater::HalfWidthAtProgress(Curso, NoCurso);

			if (Ate <= Meia)
			{
				MaisLarga = FMath::Max(MaisLarga, Meia * 2.0f);
			}
		}

		return MaisLarga;
	}
}

TArray<TrailLayout::FCrossing> TrailLayout::Crossings()
{
	TArray<FCrossing> Travessias;

	for (const FTrailRoute& Trilha : Plan())
	{
		bool bEstavaNaAgua = false;

		// Amostrado FINO dentro de cada trecho: a água é mais estreita que o
		// passo do traçado, e olhar só os vértices não vê a travessia.
		for (int32 Ponto = 1; Ponto < Trilha.PointsUnits.Num(); ++Ponto)
		{
			for (int32 Amostra = 0; Amostra <= AmostrasPorTrecho; ++Amostra)
			{
				const float Aonde =
					static_cast<float>(Amostra) / static_cast<float>(AmostrasPorTrecho);
				const FVector2D Aqui = FMath::Lerp(
					Trilha.PointsUnits[Ponto - 1], Trilha.PointsUnits[Ponto], Aonde);

				const bool bAgora = FreshWater::IsFreshWaterAt(Aqui);

				// Uma travessia por ENTRADA na água. Uma por amostra molhada
				// poria seis obras empilhadas no mesmo rio.
				if (bAgora && !bEstavaNaAgua)
				{
					FCrossing Travessia;
					Travessia.CenterUnits = Aqui;
					Travessia.DepthUnits = FunduraEm(Aqui);

					// A ORDEM decide, e é a regra inteira:
					//
					// 1. margem alta manda, por mais rasa que a água seja —
					//    ponte precisa de dois apoios no mesmo nível;
					// 2. raso o bastante se atravessa andando;
					// 3. o resto é ponte.
					// A ORDEM decide, e é a regra inteira:
					//
					// 1. margem alta manda, por mais rasa que a água seja —
					//    ponte precisa de dois apoios no mesmo nível;
					// 2. raso o bastante se atravessa andando;
					// 3. largo demais para ponte se atravessa FLUTUANDO;
					// 4. o resto é ponte.
					const float Largura = LarguraDaAguaEm(Aqui);

					if (IslandGeography::GroundSlopeAt(Aqui) >= MargemVirouBarranco)
					{
						Travessia.Kind = ECrossingKind::Barranco;
					}
					else if (Travessia.DepthUnits <= WadableDepthUnits())
					{
						Travessia.Kind = ECrossingKind::Vau;
					}
					else if (Largura >= MaiorVaoDePonte()
						&& FreshWater::NavigabilityForHalfWidth(Largura * 0.5f)
							== FreshWater::ENavigability::BarcoGrande)
					{
						Travessia.Kind = ECrossingKind::Balsa;
					}
					else
					{
						Travessia.Kind = ECrossingKind::Ponte;
					}

					Travessias.Add(Travessia);
				}

				bEstavaNaAgua = bAgora;
			}
		}
	}

	return Travessias;
}

TArray<FVector2D> TrailLayout::BridgePoints()
{
	TArray<FVector2D> Pontes;

	for (const FCrossing& Travessia : Crossings())
	{
		if (Travessia.Kind == ECrossingKind::Ponte)
		{
			Pontes.Add(Travessia.CenterUnits);
		}
	}

	return Pontes;
}

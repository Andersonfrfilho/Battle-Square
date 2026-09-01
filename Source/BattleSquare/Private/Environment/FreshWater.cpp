// Copyright 2026 Anderson. All Rights Reserved.

#include "Environment/FreshWater.h"
#include "World/PlanReentryGuard.h"

#include "Battle/DeterministicSpread.h"
#include "Environment/CaveSystem.h"
#include "Environment/IslandFeatureLayout.h"
#include "Environment/IslandGeography.h"
#include "World/WorldBudget.h"

namespace FreshWater
{
	/**
	 * Declarada antes de usar: a sonda de cobertura precisa dos córregos para
	 * medir, e os córregos precisam dos rios — que a sonda ainda está ajudando
	 * a escolher. Uma declaração adiantada resolve sem circularidade real,
	 * porque nada aqui chama `Plan()`.
	 */
	TArray<FBrook> MontarCorregos(const TArray<FRiverCourse>& Rios);

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
		 * Quantos cursos desce cada monte.
		 *
		 * Um TRONCO por monte, e os galhos morrem nele.
		 *
		 * TRÊS ordens, e é o que faz o desenho ler como raiz.
		 *
		 * Com um tronco e dois galhos entrando no MESMO ponto, o resultado era
		 * um Y — e um Y não é uma raiz. Raiz tem muitos fiapos entrando em
		 * ordens sucessivas, e as junções em alturas diferentes.
		 *
		 * Um tronco, dois galhos, e dois fiapos por galho: sete cursos por
		 * monte, vinte e um na ilha.
		 */
		constexpr int32 TroncosPorMonte = 1;
		constexpr int32 GalhosPorTronco = 2;
		constexpr int32 FiaposPorGalho = 2;
		constexpr int32 CursosPorMonte =
			TroncosPorMonte + GalhosPorTronco + GalhosPorTronco * FiaposPorGalho;

		/**
		 * Onde cada galho encontra o tronco, em fração do percurso.
		 *
		 * DIFERENTES de propósito: confluência real não é simétrica, e dois
		 * galhos entrando no mesmo raio desenham uma flecha, não uma bacia.
		 */
		constexpr float PrimeiroEncontroDeGalho = 0.30f;
		constexpr float SegundoEncontroDeGalho = 0.52f;

		/** E o fiapo entra no galho bem antes disso. */
		constexpr float EncontroDoFiapo = 0.14f;

		/** Quanto cada ordem se abre do curso que a recebe. */
		constexpr float AberturaDoGalho = 26.0f;
		constexpr float AberturaDoFiapo = 24.0f;

		/**
		 * SPACE COLONIZATION — o algoritmo de Runions, o mesmo que gera árvore,
		 * nervura de folha e raiz.
		 *
		 * Ele existe aqui porque as duas tentativas anteriores desenharam a
		 * mesma coisa errada. A primeira parametrizava o curso pelo raio, então
		 * todo galho corria do centro para fora; a segunda cresceu da foz para
		 * dentro mas OBRIGAVA cada passo a entrar, e obrigar a entrar desenha
		 * um círculo. Nenhuma quantidade de galhos conserta uma regra que
		 * proíbe o galho de ir para o lado.
		 *
		 * A ideia do algoritmo é o contrário de mandar: espalha-se ATRAÇÃO pelo
		 * espaço, e o galho cresce para onde há atração ainda não atendida. A
		 * forma sai do espaço a preencher, não de uma fórmula — e é por isso
		 * que ela parece orgânica.
		 *
		 * O laço é: cada atrator escolhe o nó mais próximo dentro do alcance;
		 * cada nó escolhido cresce um passo na média das direções dos seus
		 * atratores; atrator alcançado morre. Onde dois conjuntos de atratores
		 * puxam o mesmo nó para lados diferentes, ele BIFURCA sozinho — a
		 * ramificação não é programada, ela acontece.
		 */

		/**
		 * A COBERTURA de referência e quantos atratores ela custa.
		 *
		 * Os atratores deixaram de ser um número escolhido: eles saem da
		 * cobertura de água pedida pelo bioma, proporcionalmente.
		 *
		 * O par abaixo é MEDIDO, e a primeira vez eu o escrevi por estimativa:
		 * supus que 420 atratores dariam 6% de terra molhada, e a máscara disse
		 * **35%**. Um terço da ilha era água. Estimar o par de calibração é o
		 * mesmo erro de olhar o desenho em vez de medir, só que escondido num
		 * número que parece inofensivo.
		 *
		 * E ela NÃO É LINEAR. Dois pontos medidos na máscara:
		 *
		 *     72 atratores  ->  1,83% de terra molhada
		 *    420 atratores  -> 35,1%
		 *
		 * Isso dá expoente ~1,67, e o motivo é geométrico: mais atratores não
		 * só alongam a rede, eles criam mais CONFLUÊNCIA — e confluência sobe a
		 * ordem de Strahler, que alarga a calha. A água cresce em comprimento e
		 * em largura ao mesmo tempo.
		 *
		 * Tratar como linear foi o meu segundo erro seguido de estimativa: a
		 * primeira vez errei o par de calibração, a segunda errei a forma da
		 * curva entre os pares.
		 */
		constexpr float CoberturaDeReferencia = 0.0183f;
		constexpr int32 AtratoresDaReferencia = 72;
		/**
		 * A resolução da grade de água, usada pela máscara do jogo E pela sonda
		 * de calibragem. Uma só, pelo motivo acima.
		 *
		 * Precisa ser mais FINA que o rio mais estreito — grade grossa não vê o
		 * que é fino, e este mundo já pagou por isso quatro vezes.
		 */
	}

	constexpr int32 LadoDaGradeDeAgua = 700;

	namespace
	{

		/** Os limites e o esforço da busca. */

		/**
		 * Fartos de propósito: eles não controlam a densidade, só marcam onde a
		 * rede ainda pode crescer. Quem controla é a distância de morte.
		 */
		constexpr int32 AtratoresFartos = 1400;

		/**
		 * Quantas vezes o declive de corredeira para virar QUEDA.
		 *
		 * Corredeira é água que quebra descendo; cachoeira é água que perde o
		 * chão. Sem a distinção, todo trecho rápido viraria cachoeira e a ilha
		 * teria dezenas — o que é o mesmo que não ter nenhuma.
		 *
		 * Baixado para 1,15 depois de medir: com 2,2 nenhum curso passava, e a
		 * ilha ficou sem cachoeira nenhuma. O declive medido ao LONGO do leito
		 * e uma media sobre duas calhas de rio — bem menor que a inclinacao
		 * instantanea da encosta, e eu tinha calibrado pensando na segunda.
		 */
		constexpr float DeclivePorQueda = 1.15f;

		/**
		 * A faixa da distância de morte, em fração do raio.
		 *
		 * O teto era 0,090 e a busca encostava nele: mesmo na rede mais esparsa
		 * que ela sabia fazer, o mundo dava 11,6% para 6% pedidos. Alvo fora do
		 * alcance parece imprecisao e e outra coisa — a diferenca aparece
		 * olhando se a busca terminou colada num extremo.
		 */
		constexpr float MorreMaisPerto = 0.016f;
		constexpr float MorreMaisLonge = 0.26f;

		/**
		 * Doze, e não oito: com oito o intervalo final ainda tinha uns três
		 * atratores de largura, e perto do trecho íngreme da curva três
		 * atratores mexem na cobertura mais que o erro que se quer.
		 */
		constexpr int32 TentativasDeCalibragem = 12;

		/**
		 * A cobertura de uma lista de cursos, medida numa grade grossa.
		 *
		 * Grossa de propósito: ela roda uma vez por tentativa da busca, e o que
		 * se quer dela é a PROPORÇÃO, não a borda exata. A máscara fina do jogo
		 * é outra coisa e vem depois.
		 */
		/**
		 * DESENHA a água numa grade. A ÚNICA rasterização que existe.
		 *
		 * Ela nasceu duplicada — uma para a máscara que o jogo consulta, outra
		 * para a sonda que calibra a cobertura — e as duas discordaram por
		 * quase o dobro, o que fez a busca acertar um alvo e o mundo entregar
		 * outro. Eu escrevi o comentário sobre L-032 três vezes nesta sessão e
		 * só na terceira extraí a função.
		 */
	}

	TArray<bool> RasterizarAgua(const TArray<FRiverCourse>& Cursos,
			const TArray<FBrook>& Corregos, const TArray<FSpring>& Fontes, int32 Lado)
		{
			const float Raio = IslandGeography::LandRadiusUnits();
			const float Casa = (Raio * 2.0f) / static_cast<float>(Lado - 1);

			TArray<bool> Molhado;
			Molhado.Init(false, Lado * Lado);

			auto Carimbar = [&Molhado, Raio, Casa, Lado](const FVector2D& Onde, float Meia)
			{
				const int32 Alcance = FMath::CeilToInt(Meia / Casa);
				const int32 Coluna = FMath::RoundToInt((Onde.X + Raio) / Casa);
				const int32 Linha = FMath::RoundToInt((Onde.Y + Raio) / Casa);

				for (int32 dY = -Alcance; dY <= Alcance; ++dY)
				{
					for (int32 dX = -Alcance; dX <= Alcance; ++dX)
					{
						const int32 C = Coluna + dX;
						const int32 L = Linha + dY;
						if (C < 0 || L < 0 || C >= Lado || L >= Lado)
						{
							continue;
						}

						if (FMath::Square(dX * Casa) + FMath::Square(dY * Casa) <= Meia * Meia)
						{
							Molhado[L * Lado + C] = true;
						}
					}
				}
			};

			auto CarimbarTrecho = [&Carimbar, Casa](const FVector2D& Daqui,
				const FVector2D& Prali, float Meia)
			{
				const int32 Quantos = FMath::Max(1, FMath::CeilToInt(
					FVector2D::Distance(Daqui, Prali) / (Casa * 0.5f)));

				for (int32 Passo = 0; Passo <= Quantos; ++Passo)
				{
					Carimbar(FMath::Lerp(Daqui, Prali,
						static_cast<float>(Passo) / Quantos), Meia);
				}
			};

			for (const FRiverCourse& Curso : Cursos)
			{
				const float Comprimento = FMath::Max(1.0f, CourseLengthUnits(Curso));
				float Andado = 0.0f;

				for (int32 Ponto = 1; Ponto < Curso.PointsUnits.Num(); ++Ponto)
				{
					Andado += static_cast<float>(FVector2D::Distance(
						Curso.PointsUnits[Ponto - 1], Curso.PointsUnits[Ponto]));

					CarimbarTrecho(Curso.PointsUnits[Ponto - 1], Curso.PointsUnits[Ponto],
						HalfWidthAtProgress(Curso, Andado / Comprimento));
				}
			}

			for (const FBrook& Corrego : Corregos)
			{
				for (int32 Ponto = 1; Ponto < Corrego.PointsUnits.Num(); ++Ponto)
				{
					CarimbarTrecho(Corrego.PointsUnits[Ponto - 1], Corrego.PointsUnits[Ponto],
						Corrego.HalfWidthUnits);
				}
			}

			for (const FSpring& Fonte : Fontes)
			{
				Carimbar(Fonte.CenterUnits, Fonte.PoolHalfWidthUnits);
			}

			return Molhado;
		}

	/** Que fração da TERRA a grade diz estar molhada. */
	float CoberturaDaGrade(const TArray<bool>& Molhado, int32 Lado)
		{
			const float Raio = IslandGeography::LandRadiusUnits();
			const float Casa = (Raio * 2.0f) / static_cast<float>(Lado - 1);

			int32 EmTerra = 0;
			int32 Molhadas = 0;

			for (int32 Linha = 0; Linha < Lado; ++Linha)
			{
				for (int32 Coluna = 0; Coluna < Lado; ++Coluna)
				{
					const FVector2D Onde(-Raio + Coluna * Casa, -Raio + Linha * Casa);
					if (Onde.Size() > Raio)
					{
						continue;
					}

					++EmTerra;
					Molhadas += Molhado[Linha * Lado + Coluna] ? 1 : 0;
				}
			}

			return (EmTerra > 0) ? static_cast<float>(Molhadas) / EmTerra : 0.0f;
		}

	float CoberturaDosCursos(const TArray<FRiverCourse>& Cursos)
	{
		return CoberturaDaGrade(
			RasterizarAgua(Cursos, MontarCorregos(Cursos), PlanSprings(),
				LadoDaGradeDeAgua),
			LadoDaGradeDeAgua);
	}

	namespace
	{


		/** De onde até onde a bacia enche. O miolo fica seco: é onde a vila mora. */
		constexpr float MioloSemRio = 0.16f;
		constexpr float BordaSemRio = 0.02f;

		/** Até onde um atrator enxerga um nó, e a que distância ele morre. */
		constexpr float AlcanceDoAtrator = 0.20f;
		constexpr float MorreAPerto = 0.055f;

		/** O passo de crescimento. */
		constexpr float PassoDoGalho = 0.019f;

		/** Teto de voltas, para um caso patológico não travar o mundo. */
		constexpr int32 VoltasNoMaximo = 220;

		/**
		 * A rede do SUBSOLO: mais rala e mais curta que a de cima.
		 *
		 * O subsolo acompanha a superfície: bioma seco tem pouca galeria, porque
		 * galeria é água que dissolveu pedra. Ele é uma FRAÇÃO da superfície,
		 * não um número próprio — dois números soltos divergiriam na primeira
		 * vez que alguém mexesse num.
		 */
		constexpr float SubsoloSobreSuperficie = 0.62f;
		constexpr int32 VoltasNoSubsolo = 140;
		constexpr float AlcanceDoAtratorNoSubsolo = 0.24f;
		constexpr float MorreAPertoNoSubsolo = 0.045f;
		constexpr float PassoDaGaleria = 0.016f;

		/** Quanto os atratores se espalham em volta da linha entre duas grutas. */
		/** Quanto o ruído torce o rumo do passo. Acima disso vira nó, não curva. */
		constexpr float TorceOGalho = 0.42f;

		/** Quanto o galho segue a subida ao crescer rio acima. */
		constexpr float SegueAEncosta = 0.55f;
		constexpr float TorceAGaleria = 0.55f;

		/** Até onde duas galerias de cavernas diferentes se emendam. */
		constexpr float EmendaNoMaximo = 0.09f;

		/**
		 * Quanto o poço da queda afunda por unidade de altura caída, e quantas
		 * vezes ele aprofunda mais depressa do que alarga.
		 *
		 * O dez vem da medição de poços reais — e eu o li ERRADO da primeira
		 * vez. A literatura diz que a incisão VERTICAL supera a lateral em uma
		 * ordem de grandeza: isso é uma razão de VELOCIDADE de erosão, não a
		 * forma do buraco. Poço de cachoeira é mais largo que fundo, como
		 * qualquer poço; o que o dez explica é por que ele afunda tão rápido.
		 *
		 * Escrevi um teste afirmando "mais fundo que largo" e ele reprovou o
		 * código — a asserção é que estava errada, não a conta.
		 */
		constexpr float ProfundidadePorAltura = 0.55f;
		constexpr float AlargaSobreAprofunda = 1.6f;

		/** Quanto a calha engrossa a cada ordem. */
		constexpr float EngrossaPorOrdem = 1.7f;

		/**
		 * E quanto ela engrossa da nascente à foz, mesmo sem galho entrando.
		 *
		 * Rio real ganha água o percurso inteiro, não só nas junções: a chuva
		 * que cai na bacia entra pela margem. Sem isto a calha era um degrau em
		 * cada junção e reta entre elas.
		 */
		constexpr float EngrossaAteAFoz = 1.6f;







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

		/**
		 * Quanto o rumo abre para cada lado, no mínimo e no máximo.
		 *
		 * Baixado depois de MEDIR: com a onda de Leopold no lugar, a amplitude
		 * antiga deu sinuosidade 2,0 a 2,65. Pela mesma literatura, rio
		 * meândrico fica entre 1,5 e 2,0 — acima disso é rio tortuoso, quase
		 * um oxbow atrás do outro. Corrigi o comprimento de onda e a amplitude
		 * ficou sobrando: são dois botões para o mesmo efeito, e mexer num sem
		 * remedir o outro trocou um exagero por outro.
		 */
		constexpr float MenorSerpente = 0.030f;
		constexpr float MaiorSerpente = 0.052f;

		/**
		 * O comprimento de onda do meandro, em LARGURAS DE CALHA.
		 *
		 * Onze é o número de Leopold — rios reais meandram com onda de dez a
		 * catorze larguras. O nosso fazia de uma a duas ondas no percurso
		 * inteiro, e medindo deu sinuosidade 1,26: pela mesma literatura isso é
		 * rio RETO, não meândrico (o corte é 1,5).
		 *
		 * Sai da largura e do comprimento, e não de um número de curvas: assim
		 * um rio mais largo faz curvas mais abertas, que é o que a natureza
		 * faz. Fixar as curvas dava o contrário — o rio largo curvava apertado.
		 */
		constexpr float LargurasPorOnda = 11.0f;

		/** A variação em torno da onda de Leopold, não a contagem de curvas. */
		constexpr float MenosCurvas = 0.85f;
		constexpr float MaisCurvas = 1.20f;

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

	TArray<FRiverCourse> PlanTrunks()
	{
		TArray<FRiverCourse> Troncos;
		for (const FRiverCourse& Curso : Plan())
		{
			if (Curso.FlowsToTheSea())
			{
				Troncos.Add(Curso);
			}
		}

		return Troncos;
	}

	namespace
	{
		/** O comprimento acumulado de cada ponto do curso. */
		void MedirCurso(const FRiverCourse& Course, TArray<float>& OutAcumulado, float& OutTotal)
		{
			OutAcumulado.Reset();
			OutTotal = 0.0f;

			if (Course.PointsUnits.Num() == 0)
			{
				return;
			}

			OutAcumulado.Add(0.0f);
			for (int32 Ponto = 1; Ponto < Course.PointsUnits.Num(); ++Ponto)
			{
				OutTotal += static_cast<float>(FVector2D::Distance(
					Course.PointsUnits[Ponto - 1], Course.PointsUnits[Ponto]));
				OutAcumulado.Add(OutTotal);
			}
		}
	}

	/** O comprimento total do curso. */
	float CourseLengthUnits(const FRiverCourse& Course)
	{
		TArray<float> Acumulado;
		float Total = 0.0f;
		MedirCurso(Course, Acumulado, Total);
		return Total;
	}


	namespace
	{
		/**
		 * Atratores numa GRADE SACUDIDA — uniforme por área, sem estrutura
		 * angular para a rede copiar.
		 *
		 * Cada casa da grade ganha um ponto no centro dela mais um empurrão
		 * sorteado de até meia casa. É o jeito barato de ter distribuição sem
		 * grumo e sem alinhamento: sorteio puro faz grumo, grade pura faz
		 * fileira, e a sacudida tira as duas coisas.
		 */
		TArray<FVector2D> AtratoresEmGrade(int32 Quantos, float Dentro, float Fora,
			const TCHAR* Assunto)
		{
			TArray<FVector2D> Atratores;
			if (Quantos <= 0 || Fora <= 0.0f)
			{
				return Atratores;
			}

			const int32 Lado = FMath::Max(2, FMath::CeilToInt(FMath::Sqrt(
				static_cast<float>(Quantos) * 4.0f / PI)));
			const float Casa = (Fora * 2.0f) / static_cast<float>(Lado);

			for (int32 Linha = 0; Linha < Lado; ++Linha)
			{
				for (int32 Coluna = 0; Coluna < Lado; ++Coluna)
				{
					const uint32 Semente = BattleSpread::SeedFromText(
						FString::Printf(TEXT("atrator-%s-%d-%d"), Assunto, Coluna, Linha));

					const FVector2D Onde(
						-Fora + (Coluna + 0.5f) * Casa + BattleSpread::Between(
							-Casa * 0.5f, Casa * 0.5f, BattleSpread::Fraction(Semente, 0)),
						-Fora + (Linha + 0.5f) * Casa + BattleSpread::Between(
							-Casa * 0.5f, Casa * 0.5f, BattleSpread::Fraction(Semente, 1)));

					const float Distancia = static_cast<float>(Onde.Size());
					if (Distancia < Dentro || Distancia > Fora)
					{
						continue;
					}

					Atratores.Add(Onde);
				}
			}

			return Atratores;
		}

		/**
		 * O empurrão ORGÂNICO do passo, vindo de ruído coerente na posição.
		 *
		 * Sem ele o galho anda em segmentos retos entre nós e sai com cara de
		 * esqueleto de arame. E o ruído tem de ser COERENTE — função da
		 * posição, não sorteio por passo: sorteio independente dá tremor, e
		 * tremor não é curva.
		 */
		/**
		 * Para onde a ROCHA sobe, num ponto.
		 *
		 * Medida por diferença nas quatro vizinhas, na rocha e não no relevo
		 * acabado: o leito é anterior à vila, e perguntar a altura com os lotes
		 * e a mesa já postos fecha o ciclo que já abortou o processo.
		 */
		FVector2D SubidaDaRocha(const FVector2D& Onde, float Passo)
		{
			const float Leste = IslandGeography::BedrockHeightAt(Onde + FVector2D(Passo, 0.0f));
			const float Oeste = IslandGeography::BedrockHeightAt(Onde - FVector2D(Passo, 0.0f));
			const float Norte = IslandGeography::BedrockHeightAt(Onde + FVector2D(0.0f, Passo));
			const float Sul = IslandGeography::BedrockHeightAt(Onde - FVector2D(0.0f, Passo));

			return FVector2D(Leste - Oeste, Norte - Sul).GetSafeNormal();
		}

		FVector2D EmpurraoOrganico(const FVector2D& Onde, float Amplitude)
		{
			const float Escala = IslandGeography::LandRadiusUnits() * 0.06f;
			const uint32 Semente = BattleSpread::SeedFromText(FString::Printf(
				TEXT("ruido-%d-%d"),
				FMath::FloorToInt(Onde.X / Escala), FMath::FloorToInt(Onde.Y / Escala)));

			const float Angulo = BattleSpread::Between(0.0f, 2.0f * PI,
				BattleSpread::Fraction(Semente, 0));

			return FVector2D(FMath::Cos(Angulo), FMath::Sin(Angulo)) * Amplitude;
		}
	}

	int32 RiversPerMountain() { return CursosPorMonte; }

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

	TArray<FBrook> MontarCorregos(const TArray<FRiverCourse>& Rios)
	{
		TArray<FBrook> Corregos;
		if (Rios.Num() == 0)
		{
			return Corregos;
		}

		const float MeiaCalha = IslandGeography::LandRadiusUnits() * FracaoDoCorrego;

		/** O ponto do rio mais perto de onde a fonte está. */
		auto NoRioMaisPerto = [&Rios](const FVector2D& Daqui)
		{
			FVector2D Melhor = PointAtProgress(Rios[0], 0.0f);
			float Menor = TNumericLimits<float>::Max();

			for (const FRiverCourse& Rio : Rios)
			{
				for (int32 Passo = 0; Passo <= 60; ++Passo)
				{
					const FVector2D Ali = PointAtProgress(Rio, static_cast<float>(Passo) / 60.0f);
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

		// NÃO HÁ MAIS CÓRREGO LIGANDO RIO A RIO.
		//
		// A regra era "liga o curso i ao i+1", e ela nasceu quando a ilha tinha
		// três rios paralelos que precisavam se encontrar. Com a bacia
		// dendrítica são centenas de cursos, e a regra passou a cavar centenas
		// de canais retos atravessando a ilha — inventando mais água do que
		// todos os rios juntos, e sujando o mapa.
		//
		// A bacia já é conexa por construção: todo curso desemboca no seguinte
		// até o mar. O que liga uma BACIA à outra é a rede subterrânea, e é o
		// lugar certo para isso.

		return Corregos;
	}

	TArray<FBrook> PlanBrooks()
	{
		// Os córregos da bacia REAL. `MontarCorregos` existe separado porque a
		// sonda de cobertura precisa medi-los durante a busca, quando `Plan()`
		// ainda não existe — chamar `Plan()` de lá seria recursão.
		return MontarCorregos(Plan());
	}


		/**
		 * A bacia inteira, por space colonization.
		 *
		 * Devolve um curso por SEGMENTO da árvore: do nó em que ele nasceu até
		 * a próxima bifurcação ou até a ponta. É essa fatia que o resto do
		 * mundo chama de "rio".
		 *
		 * `static` e com nome específico: L-042 já apareceu neste módulo em
		 * código de produção, quando `Tracar` existia em dois arquivos e o
		 * unity build os juntou.
		 */
		/**
		 * Põe a queda de cada curso no ponto mais íngreme do leito dele.
		 *
		 * Só em curso de ORDEM 2 para cima — fio de cabeceira que despenca é
		 * gotejamento, não cachoeira — e só onde o leito passa do declive de
		 * corredeira, porque abaixo disso a água escorre em vez de cair.
		 *
		 * E em terra ALCANÇÁVEL: cachoeira que nenhuma trilha atinge é a
		 * cachoeira que o relato de jogo disse nunca ter visto.
		 */
		/**
		 * Põe o LAGO de cada curso no trecho mais MANSO do leito.
		 *
		 * Mesmo conserto que a cachoeira precisou, e pelo mesmo motivo: o lago
		 * estava preso ao tronco, e tronco é o pedaço curto e costeiro entre a
		 * última bifurcação e o mar. Todos os lagos nasciam na foz, e o Mercado
		 * do Lago ficou a 84 metros de qualquer água.
		 *
		 * Água para onde o chão achata — é o que um lago É. E em curso de
		 * ordem 2 para cima, porque fio de cabeceira não tem volume para
		 * alagar nada.
		 */
		void ColocarOsLagos(TArray<FRiverCourse>& Cursos)
		{
			const float Limite = IslandGeography::LandRadiusUnits()
				- IslandGeography::BeachWidthUnits() * 1.9f;

			for (FRiverCourse& Curso : Cursos)
			{
				Curso.LakeAtProgress = -1.0f;

				if (Curso.Order < 2 || Curso.PointsUnits.Num() < 3)
				{
					continue;
				}

				float MaisManso = TNumericLimits<float>::Max();
				float Aonde = -1.0f;

				for (int32 Passo = 3; Passo <= 17; ++Passo)
				{
					const float Onde = static_cast<float>(Passo) / 20.0f;
					if (PointAtProgress(Curso, Onde).Size() > Limite)
					{
						continue;
					}

					const float Declive = BedGradientAtProgress(Curso, Onde);
					if (Declive < MaisManso)
					{
						MaisManso = Declive;
						Aonde = Onde;
					}
				}

				// E só alaga se for manso DE VERDADE: num leito que despenca o
				// tempo todo não há lago, há corredeira do começo ao fim.
				if (Aonde > 0.0f && MaisManso <= RapidsGradient() * 0.5f)
				{
					Curso.LakeAtProgress = Aonde;
				}
			}
		}

		void ColocarAsQuedas(TArray<FRiverCourse>& Cursos)
		{
			const float Limite = IslandGeography::LandRadiusUnits()
				- IslandGeography::BeachWidthUnits() * 1.6f;

			for (FRiverCourse& Curso : Cursos)
			{
				Curso.FallAtProgress = -1.0f;

				if (Curso.Order < 2 || Curso.PointsUnits.Num() < 3)
				{
					continue;
				}

				float MaisIngreme = 0.0f;
				float Aonde = -1.0f;

				// DEPOIS do lago, sempre. Os dois eram escolhidos de forma
				// independente, e num curso a queda saiu antes — água caindo
				// para dentro de um lago que ela ainda vai formar.
				const int32 Comeca = Curso.HasLake()
					? FMath::Max(2, FMath::CeilToInt(Curso.LakeAtProgress * 20.0f) + 1)
					: 2;

				for (int32 Passo = Comeca; Passo <= 18; ++Passo)
				{
					const float Onde = static_cast<float>(Passo) / 20.0f;
					if (PointAtProgress(Curso, Onde).Size() > Limite)
					{
						continue;
					}

					const float Declive = BedGradientAtProgress(Curso, Onde);
					if (Declive > MaisIngreme)
					{
						MaisIngreme = Declive;
						Aonde = Onde;
					}
				}

				if (Aonde > 0.0f && MaisIngreme >= RapidsGradient() * DeclivePorQueda)
				{
					Curso.FallAtProgress = Aonde;
				}
			}
		}

		static TArray<FRiverCourse> ColonizarBacia(const TArray<FVector2D>& Bocas,
			int32 Quantos, float MorreA)
		{
			TArray<FRiverCourse> Cursos;
			if (Bocas.Num() == 0)
			{
				return Cursos;
			}

			const float Raio = IslandGeography::LandRadiusUnits();
			const float Alcance = Raio * AlcanceDoAtrator;
			const float Morte = Raio * MorreA;
			const float Passo = Raio * PassoDoGalho;

			// OS ATRATORES, espalhados na coroa que a bacia deve encher.
			//
			// Em disco por RAIZ do sorteio: sem a raiz eles se acumulam no
			// centro da coroa e a bacia nasce densa no meio e rala nas pontas.
			// EM GRADE SACUDIDA, e nunca em coordenada polar.
			//
			// Espalhar por ângulo e raio parece natural e é a causa do defeito
			// mais visível deste mundo: uma rede que cresce para atratores
			// numa COROA herda a coroa. Os galhos param nas bordas dela, e o
			// desenho vira arco — "fluxo de água formando círculos".
			//
			// Grade sacudida é uniforme por ÁREA e não tem estrutura angular
			// nenhuma para a raiz copiar.
			TArray<FVector2D> Atratores = AtratoresEmGrade(
				Quantos, Raio * MioloSemRio, Raio * (1.0f - BordaSemRio),
				TEXT("bacia"));

			// A árvore: posições e de quem cada nó veio.
			TArray<FVector2D> Nos;
			TArray<int32> Pai;

			for (const FVector2D& Boca : Bocas)
			{
				Nos.Add(Boca);
				Pai.Add(INDEX_NONE);
			}

			for (int32 Volta = 0; Volta < VoltasNoMaximo && Atratores.Num() > 0; ++Volta)
			{
				// Cada atrator vota no nó mais perto dentro do alcance.
				TMap<int32, FVector2D> Puxao;
				TMap<int32, int32> Votos;

				for (const FVector2D& Atrator : Atratores)
				{
					int32 MaisPerto = INDEX_NONE;
					float Menor = Alcance * Alcance;

					for (int32 No = 0; No < Nos.Num(); ++No)
					{
						const float Ate = FVector2D::DistSquared(Atrator, Nos[No]);
						if (Ate < Menor)
						{
							Menor = Ate;
							MaisPerto = No;
						}
					}

					if (MaisPerto == INDEX_NONE)
					{
						continue;
					}

					Puxao.FindOrAdd(MaisPerto) += (Atrator - Nos[MaisPerto]).GetSafeNormal();
					Votos.FindOrAdd(MaisPerto) += 1;
				}

				if (Puxao.Num() == 0)
				{
					break;
				}

				// Cada nó votado cresce UM passo na média dos seus atratores.
				//
				// É aqui que a bifurcação acontece SOZINHA: quando dois grupos
				// de atratores puxam o mesmo nó para lados opostos, a média cai
				// no meio, o galho avança, e na volta seguinte os dois grupos
				// já enxergam nós diferentes. Ninguém programou o "abre em Y".
				const int32 QuantosAntes = Nos.Num();
				for (const TPair<int32, FVector2D>& Quem : Puxao)
				{
					if (Quem.Key >= QuantosAntes)
					{
						continue;
					}

					// E O EMPURRÃO MORRO ACIMA.
					//
					// A árvore cresce da foz para DENTRO, ou seja, rio acima —
					// e rio acima é subida. Sem este termo o galho seguia só o
					// atrator, e quinze cursos acabaram perdendo altura da
					// nascente para a foz: água correndo morro acima.
					//
					// O peso é pequeno de propósito: ele CORRIGE o rumo, não o
					// manda. Grande, a bacia vira um leque radial subindo o
					// morro pelo caminho mais curto, e some a ramificação.
					const FVector2D ParaCima =
						SubidaDaRocha(Nos[Quem.Key], Passo) * SegueAEncosta;

					const FVector2D Rumo = (Quem.Value.GetSafeNormal()
						+ EmpurraoOrganico(Nos[Quem.Key], TorceOGalho)
						+ ParaCima).GetSafeNormal();
					if (Rumo.IsNearlyZero())
					{
						continue;
					}

					const FVector2D Novo = Nos[Quem.Key] + Rumo * Passo;
					if (Novo.Size() > Raio)
					{
						continue;
					}

					Nos.Add(Novo);
					Pai.Add(Quem.Key);
				}

				if (Nos.Num() == QuantosAntes)
				{
					break;
				}

				// Atrator alcançado morre. Sem isso a árvore cresce para sempre
				// na direção do mesmo ponto e vira um fio único e reto.
				//
				// Só os nós NOVOS são conferidos: os velhos já foram, na volta
				// em que nasceram. Comparar todos contra todos a cada volta é
				// quadrático em cima de quadrático, e foi o que travou a
				// primeira versão — ela não terminava.
				Atratores.RemoveAll([&Nos, QuantosAntes, Morte](const FVector2D& Atrator)
				{
					for (int32 No = QuantosAntes; No < Nos.Num(); ++No)
					{
						if (FVector2D::DistSquared(Atrator, Nos[No]) < Morte * Morte)
						{
							return true;
						}
					}

					return false;
				});
			}

			// Quantos filhos cada nó tem — é o que diz onde estão as
			// bifurcações, e portanto onde um curso acaba e outro começa.
			TArray<int32> Filhos;
			Filhos.Init(0, Nos.Num());
			for (int32 No = 0; No < Nos.Num(); ++No)
			{
				if (Pai[No] != INDEX_NONE)
				{
					++Filhos[Pai[No]];
				}
			}

			// A ORDEM DE STRAHLER, de folha para raiz: folha é 1, e um nó com
			// dois filhos de mesma ordem sobe um. É a largura da calha.
			TArray<int32> Ordem;
			Ordem.Init(1, Nos.Num());
			for (int32 No = Nos.Num() - 1; No >= 0; --No)
			{
				if (Pai[No] == INDEX_NONE)
				{
					continue;
				}

				const int32 NoPai = Pai[No];
				if (Ordem[No] > Ordem[NoPai])
				{
					Ordem[NoPai] = Ordem[No];
				}
				else if (Ordem[No] == Ordem[NoPai] && Filhos[NoPai] > 1)
				{
					Ordem[NoPai] = Ordem[No] + 1;
				}
			}

			// Cada CURSO é o pedaço entre duas bifurcações.
			for (int32 Ponta = 0; Ponta < Nos.Num(); ++Ponta)
			{
				// Um curso começa numa FOLHA, numa BIFURCAÇÃO, ou logo abaixo de
				// uma bifurcação.
				//
				// Faltava a bifurcação, e o defeito era grande: cada cadeia
				// sobe de uma folha e para no primeiro nó que se parte — então
				// o pedaço entre esse nó e a FOZ não pertencia a curso nenhum.
				// A ilha ficou com 390 cursos e nenhum chegando ao mar, e junto
				// foram-se as cachoeiras, as grutas e as passagens.
				const bool bComecaAqui = (Filhos[Ponta] != 1)
					|| (Pai[Ponta] != INDEX_NONE && Filhos[Pai[Ponta]] > 1);

				if (!bComecaAqui)
				{
					continue;
				}

				FRiverCourse Curso;
				Curso.Order = Ordem[Ponta];

				int32 Onde = Ponta;
				Curso.PointsUnits.Add(Nos[Onde]);

				while (Pai[Onde] != INDEX_NONE)
				{
					Onde = Pai[Onde];
					Curso.PointsUnits.Add(Nos[Onde]);

					// Para na bifurcação seguinte: dali para baixo já é outro
					// curso, mais largo, e emendá-los faria a calha do fio de
					// cabeceira chegar ao mar.
					if (Filhos[Onde] > 1 && Onde != Ponta)
					{
						break;
					}
				}

				if (Curso.PointsUnits.Num() < 2)
				{
					continue;
				}

				// APARA a ponta que sobe.
				//
				// O empurrão morro acima corrige o RUMO, mas a última perna do
				// galho ainda pode cair num vale — e aí o curso "nasceria" mais
				// baixo que o ponto seguinte, ou seja, correria morro acima.
				//
				// Aparar é honesto e mínimo: a nascente recua até o primeiro
				// ponto que de fato domina o seguinte. Rio não nasce num
				// buraco.
				// Duas condições, e a segunda é a que importa: a nascente tem de
				// DOMINAR A FOZ. Aparar só o primeiro passo consertava o começo
				// e deixava o curso inteiro subindo.
				while (Curso.PointsUnits.Num() > 2
					&& (IslandGeography::BedrockHeightAt(Curso.PointsUnits[0])
							< IslandGeography::BedrockHeightAt(Curso.PointsUnits[1])
						|| IslandGeography::BedrockHeightAt(Curso.PointsUnits[0])
							< IslandGeography::BedrockHeightAt(Curso.PointsUnits.Last())))
				{
					Curso.PointsUnits.RemoveAt(0);
				}

				Curso.SourceRadiusUnits = static_cast<float>(Curso.PointsUnits[0].Size());
				Curso.MouthRadiusUnits = static_cast<float>(Curso.PointsUnits.Last().Size());
				Curso.BearingRadians = FMath::Atan2(
					Curso.PointsUnits.Last().Y, Curso.PointsUnits.Last().X);

				const bool bChegaNoMar =
					Curso.PointsUnits.Last().Size() >= Raio - Passo * 1.5f;

				if (!bChegaNoMar)
				{
					Curso.JoinRadiusUnits = Curso.MouthRadiusUnits;
					Curso.JoinBearingRadians = Curso.BearingRadians;
					// Fora da faixa 0..1 para dizer "não tem". Quem perguntar tem
					// de checar `FlowsToTheSea` antes — foi ler este valor como
					// posição que pôs o mercado fora da ilha.
					Curso.LakeAtProgress = -1.0f;
					Curso.FallAtProgress = -1.0f;
				}
				else
				{
					const uint32 Semente = BattleSpread::SeedFromText(
						FString::Printf(TEXT("tronco-da-bacia-%d"), Cursos.Num()));

					const float Comprimento = FMath::Max(1.0f, CourseLengthUnits(Curso));

					Curso.LakeAtProgress = BattleSpread::Between(
						PrimeiroLago, UltimoLago, BattleSpread::Fraction(Semente, 2));

					// A queda SEMPRE depois do lago, por SOMA: sorteados à
					// parte, um dia sairia cachoeira dentro do lago.
					Curso.FallAtProgress = FMath::Min(
						Curso.LakeAtProgress
							+ (AlcanceDoLago() + BattleSpread::Between(MenorDegrau(),
								MaiorDegrau(), BattleSpread::Fraction(Semente, 3))) / Comprimento,
						1.0f - MeiaQueda() * 3.0f / Comprimento);

					// A QUEDA PRECISA CAIR EM TERRA ALCANÇÁVEL.
					//
					// "Tronco" passou a ser o trecho entre a última bifurcação e
					// o mar — que numa bacia densa é curto e costeiro. O lago e
					// a cachoeira caíam nele, e o resultado foram sete trilhas
					// mirando a areia, uma delas um ponto NO MAR.
					//
					// Nem todo rio tem cachoeira, e forçar uma em cada foz é
					// inventar acidente onde o terreno não tem. Sem lugar, o
					// tronco simplesmente não tem queda — e aí não tem gruta,
					// não tem poço e não tem trilha prometendo uma.
					const float Limite = IslandGeography::LandRadiusUnits()
						- IslandGeography::BeachWidthUnits() * 1.6f;

					if (PointAtProgress(Curso, Curso.FallAtProgress).Size() > Limite)
					{
						Curso.LakeAtProgress = -1.0f;
						Curso.FallAtProgress = -1.0f;
					}
				}

				Cursos.Add(Curso);
			}

			// AS CACHOEIRAS, escolhidas DEPOIS e pelo LEITO.
			//
			// Elas estavam presas ao lago, e o lago mora no tronco — que numa
			// bacia densa é o trecho curto e costeiro entre a última bifurcação
			// e o mar. As quedas caíam na areia, e sete trilhas apontaram para
			// lá; prender a queda a terra alcançável tirou as retas e tirou
			// TODAS as cachoeiras junto.
			//
			// O erro era de modelo: cachoeira não acontece na foz. Ela acontece
			// onde o leito DESPENCA, que é rio acima, perto do monte. A conta
			// já existia — `BedGradientAtProgress` — e não estava sendo usada
			// para decidir nada.
			ColocarOsLagos(Cursos);
			ColocarAsQuedas(Cursos);

			return Cursos;
		}

	TArray<FRiverCourse> Plan()
	{
		// Guardado pelo mesmo motivo da região: o mapa é fixo, e este plano era
		// remontado dentro do laço do traçado de trilha.
		static const TArray<FRiverCourse> Guardado = []()
		{
		const FPlanReentryGuard Guarda(TEXT("FreshWater::Plan"));

		TArray<FRiverCourse> Cursos;
		const float Foz = IslandGeography::LandRadiusUnits();

		TArray<FVector2D> Bocas;
		int32 Indice = 0;
		for (const IslandFeatureLayout::FFeaturePlacement& Peca : IslandFeatureLayout::Plan())
		{
			if (Peca.Feature != IslandFeatureLayout::EIslandFeature::WalkableMountain)
			{
				continue;
			}

			// Este monte contribui com UMA FOZ. A bacia inteira é gerada
			// depois, de uma vez, porque o algoritmo precisa ver todos os
			// atratores para repartir o espaço entre as bacias.
			Bocas.Add(FVector2D(
				FMath::Cos(FMath::DegreesToRadians(Peca.AngleDegrees)),
				FMath::Sin(FMath::DegreesToRadians(Peca.AngleDegrees))) * Foz);
			++Indice;
		}

		// PROCURA o número de atratores que acerta a cobertura pedida.
		//
		// Calibrar por fórmula falhou duas vezes seguidas, e a segunda ensinou
		// por quê: não é lei de potência. Três pontos medidos —
		// 72 -> 1,8%, 149 -> 16,2%, 420 -> 35,1% — descrevem uma curva que
		// dispara e depois satura, porque mais atratores criam CONFLUÊNCIA, e
		// confluência sobe a ordem de Strahler, que alarga a calha.
		//
		// O mapa é fixo e o gerador é rápido: então não se estima, procura-se.
		// A busca binária custa algumas montagens uma única vez, e em troca a
		// porcentagem vira um botão de verdade em vez de um desejo.
		{
			const float Pedida =
				FreshWater::WaterCoverageForBiome(IslandGeography::IslandBiome());

			// A busca varia a DISTÂNCIA DE MORTE, não o número de atratores.
			//
			// Foi o erro de botão mais caro desta sequência: atrator que nasce
			// mais perto de um nó do que essa distância morre na hora, então a
			// rede satura e jogar mais atratores não molha mais nada. Eu subi o
			// teto de 700 para 2600 e a cobertura não se mexeu um décimo.
			//
			// Quem controla densidade de rede é o quanto ela pode se aproximar
			// de si mesma. Os atratores são abundantes de propósito: eles só
			// precisam existir onde a rede ainda pode crescer.
			float Menos = MorreMaisPerto;
			float Mais = MorreMaisLonge;

			// Guarda o MELHOR, não o último.
			//
			// Devolver o último candidato entrega o que a busca estava
			// experimentando quando acabou o orçamento de tentativas, e não o
			// que ela achou. Deu 9,3% para 6% pedidos — a busca tinha o número
			// certo e jogou fora.
			TArray<FRiverCourse> Melhor;
			float MenorErro = TNumericLimits<float>::Max();

			for (int32 Tentativa = 0; Tentativa < TentativasDeCalibragem; ++Tentativa)
			{
				const float Meio = (Menos + Mais) * 0.5f;
				TArray<FRiverCourse> Tentada = ColonizarBacia(Bocas, AtratoresFartos, Meio);

				const float Deu = CoberturaDosCursos(Tentada);
				const float Erro = FMath::Abs(Deu - Pedida);

				if (Erro < MenorErro)
				{
					MenorErro = Erro;
					Melhor = Tentada;
				}

				// Distância MENOR dá rede mais densa: a comparação inverte.
				if (Deu < Pedida)
				{
					Mais = Meio;
				}
				else
				{
					Menos = Meio;
				}
			}

			Cursos = MoveTemp(Melhor);
		}

		return Cursos;
		}();

		return Guardado;
	}

	FVector2D PointAtProgress(const FRiverCourse& Course, float Progress)
	{
		const TArray<FVector2D>& Pontos = Course.PointsUnits;
		if (Pontos.Num() == 0)
		{
			return FVector2D::ZeroVector;
		}
		if (Pontos.Num() == 1)
		{
			return Pontos[0];
		}

		// Progresso é medido em COMPRIMENTO andado, não em índice de ponto: os
		// passos do gerador não têm todos o mesmo tamanho, e contar índices
		// faria o meio do curso cair no lugar errado.
		TArray<float> Acumulado;
		float Total = 0.0f;
		MedirCurso(Course, Acumulado, Total);

		if (Total <= KINDA_SMALL_NUMBER)
		{
			return Pontos[0];
		}

		const float Alvo = FMath::Clamp(Progress, 0.0f, 1.0f) * Total;

		for (int32 Ponto = 1; Ponto < Pontos.Num(); ++Ponto)
		{
			if (Acumulado[Ponto] < Alvo)
			{
				continue;
			}

			const float Vao = Acumulado[Ponto] - Acumulado[Ponto - 1];
			const float Onde = (Vao > KINDA_SMALL_NUMBER)
				? (Alvo - Acumulado[Ponto - 1]) / Vao
				: 0.0f;

			return FMath::Lerp(Pontos[Ponto - 1], Pontos[Ponto], Onde);
		}

		return Pontos.Last();
	}

	float NearestOn(const FRiverCourse& Course, const FVector2D& PositionUnits, float& OutProgress)
	{
		OutProgress = 0.0f;

		const TArray<FVector2D>& Pontos = Course.PointsUnits;
		if (Pontos.Num() == 0)
		{
			return TNumericLimits<float>::Max();
		}
		if (Pontos.Num() == 1)
		{
			return static_cast<float>(FVector2D::Distance(PositionUnits, Pontos[0]));
		}

		TArray<float> Acumulado;
		float Total = 0.0f;
		MedirCurso(Course, Acumulado, Total);

		float Menor = TNumericLimits<float>::Max();

		for (int32 Ponto = 1; Ponto < Pontos.Num(); ++Ponto)
		{
			const FVector2D Trecho = Pontos[Ponto] - Pontos[Ponto - 1];
			const float Comprimento = static_cast<float>(Trecho.SizeSquared());
			if (Comprimento <= KINDA_SMALL_NUMBER)
			{
				continue;
			}

			const float Onde = FMath::Clamp(static_cast<float>(
				FVector2D::DotProduct(PositionUnits - Pontos[Ponto - 1], Trecho)) / Comprimento,
				0.0f, 1.0f);

			const FVector2D MaisPerto = Pontos[Ponto - 1] + Trecho * Onde;
			const float Ate = static_cast<float>(FVector2D::Distance(PositionUnits, MaisPerto));

			if (Ate < Menor)
			{
				Menor = Ate;
				OutProgress = (Total > KINDA_SMALL_NUMBER)
					? (Acumulado[Ponto - 1] + FMath::Sqrt(Comprimento) * Onde) / Total
					: 0.0f;
			}
		}

		return Menor;
	}

	float HalfWidthAtProgress(const FRiverCourse& Course, float Progress)
	{
		const float Onde = FMath::Clamp(Progress, 0.0f, 1.0f);

		// A calha sai da ORDEM do curso e de quanto ele já andou. Fio de
		// cabeceira é fio; tronco é rio; e todo curso engrossa rio abaixo,
		// porque a chuva da bacia entra pela margem o percurso inteiro — não só
		// nas junções.
		const float PorOrdem =
			FMath::Pow(EngrossaPorOrdem, static_cast<float>(Course.Order - 1));
		const float DaCalha = MeiaCalhaDoRio() * PorOrdem
			* FMath::Lerp(1.0f / EngrossaAteAFoz, 1.0f, Onde);

		// O lago é uma barriga no meio do curso, medida em PROGRESSO. Antes ela
		// era medida em raio, e num galho que corre de lado o mesmo raio
		// aparecia duas vezes — o lago nascia partido em dois.
		const float ateOLago = FMath::Abs(Onde - Course.LakeAtProgress);
		const float AlcanceEmProgresso = AlcanceDoLago() / FMath::Max(1.0f, CourseLengthUnits(Course));

		if (ateOLago >= AlcanceEmProgresso)
		{
			return DaCalha;
		}

		const float Subida = 1.0f - ateOLago / AlcanceEmProgresso;
		const float Suave = Subida * Subida * (3.0f - 2.0f * Subida);
		return DaCalha + (MeiaCalhaDoLago() - DaCalha) * Suave;
	}

	bool IsFallAtProgress(const FRiverCourse& Course, float Progress)
	{
		const float EmProgresso = MeiaQueda() / FMath::Max(1.0f, CourseLengthUnits(Course));
		return FMath::Abs(Progress - Course.FallAtProgress) <= EmProgresso;
	}

	namespace
	{
		/** A menor distância entre um ponto e a água doce de toda a ilha. */
		float MargemDaAgua(const TArray<FRiverCourse>& Cursos, const FVector2D& Ponto)
		{
			float Menor = TNumericLimits<float>::Max();
			for (const FRiverCourse& Curso : Cursos)
			{
				// A pergunta é "a que distância da margem", e agora ela tem
				// função própria: `NearestOn`. Antes era uma varredura por raio,
				// que numa árvore de verdade pula os trechos que correm de lado.
				float Onde = 0.0f;
				const float Ate = NearestOn(Curso, Ponto, Onde);
				Menor = FMath::Min(Menor, Ate - HalfWidthAtProgress(Curso, Onde));
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
			// A gruta é da CACHOEIRA, e a cachoeira mora no tronco. Galho de
			// cabeceira não tem queda: a foz dele é a junção, e uma gruta ali
			// prometeria uma cachoeira que não existe.
			if (!Cursos[Indice].HasFall())
			{
				continue;
			}

			const FVector2D NaQueda = PointAtProgress(Cursos[Indice], Cursos[Indice].FallAtProgress);

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
	// A REDE SUBTERRÂNEA, pelo mesmo algoritmo da bacia — e é aqui que o
	// desenho de raiz pertence de verdade.
	//
	// Água que corre em calcário dissolve a pedra seguindo fratura, e o que sai
	// é uma rede dendrítica: galho fino entrando em galho grosso, sem nenhuma
	// direção privilegiada. Era um risco reto entre duas grutas, e era a coisa
	// mais fora de lugar no mapa inteiro.
	TArray<FUnderwaterLink> Passagens;

	const TArray<IslandFeatureLayout::FFeaturePlacement> Grutas = PlanGrottoes();
	if (Grutas.Num() == 0)
	{
		return Passagens;
	}

	const float Raio = IslandGeography::LandRadiusUnits();
	const float Alcance = Raio * AlcanceDoAtratorNoSubsolo;
	const float Morte = Raio * MorreAPertoNoSubsolo;
	const float Passo = Raio * PassoDaGaleria;

	// Os atratores do subsolo ficam ENTRE as grutas, não espalhados pela ilha:
	// a rede existe para LIGAR bocas, e atrator solto faz galeria que não vai
	// a lugar nenhum.
	// Em GRADE SACUDIDA sobre a ilha inteira, e não na linha entre as grutas.
	//
	// Pô-los na linha entre duas bocas foi o erro mais direto que cometi aqui:
	// o algoritmo devolve exatamente a reta que ele veio substituir, só que
	// arqueada. Espalhados por área, a galeria vagueia e as bocas se encontram
	// porque a rede cresceu, não porque eu apontei uma para a outra.
	TArray<FVector2D> Atratores = AtratoresEmGrade(
		// O subsolo acompanha o tamanho da bacia que a busca achou, e não um
		// número próprio: dois números soltos divergem na primeira edição.
FMath::RoundToInt(Plan().Num() * SubsoloSobreSuperficie),
		0.0f, Raio * 0.94f, TEXT("subsolo"));

	TArray<FVector2D> Nos;
	TArray<int32> Pai;
	for (const IslandFeatureLayout::FFeaturePlacement& Gruta : Grutas)
	{
		Nos.Add(Gruta.CenterUnits());
		Pai.Add(INDEX_NONE);
	}

	for (int32 Volta = 0; Volta < VoltasNoSubsolo && Atratores.Num() > 0; ++Volta)
	{
		TMap<int32, FVector2D> Puxao;

		for (const FVector2D& Atrator : Atratores)
		{
			int32 MaisPerto = INDEX_NONE;
			float Menor = Alcance * Alcance;

			for (int32 No = 0; No < Nos.Num(); ++No)
			{
				const float Ate = FVector2D::DistSquared(Atrator, Nos[No]);
				if (Ate < Menor)
				{
					Menor = Ate;
					MaisPerto = No;
				}
			}

			if (MaisPerto != INDEX_NONE)
			{
				Puxao.FindOrAdd(MaisPerto) += (Atrator - Nos[MaisPerto]).GetSafeNormal();
			}
		}

		if (Puxao.Num() == 0)
		{
			break;
		}

		const int32 QuantosAntes = Nos.Num();
		for (const TPair<int32, FVector2D>& Quem : Puxao)
		{
			const FVector2D Rumo = (Quem.Value.GetSafeNormal()
				+ EmpurraoOrganico(Nos[Quem.Key], TorceAGaleria)).GetSafeNormal();
			if (Rumo.IsNearlyZero())
			{
				continue;
			}

			const FVector2D Novo = Nos[Quem.Key] + Rumo * Passo;
			if (Novo.Size() > Raio)
			{
				continue;
			}

			Nos.Add(Novo);
			Pai.Add(Quem.Key);
		}

		if (Nos.Num() == QuantosAntes)
		{
			break;
		}

		Atratores.RemoveAll([&Nos, QuantosAntes, Morte](const FVector2D& Atrator)
		{
			for (int32 No = QuantosAntes; No < Nos.Num(); ++No)
			{
				if (FVector2D::DistSquared(Atrator, Nos[No]) < Morte * Morte)
				{
					return true;
				}
			}

			return false;
		});
	}

	TArray<int32> Filhos;
	Filhos.Init(0, Nos.Num());
	for (int32 No = 0; No < Nos.Num(); ++No)
	{
		if (Pai[No] != INDEX_NONE)
		{
			++Filhos[Pai[No]];
		}
	}

	// DE QUE BOCA cada nó veio. Sem isto não dá para saber se duas galerias que
	// se encontram no desenho pertencem a cavernas diferentes.
	TArray<int32> DeQueBoca;
	DeQueBoca.Init(INDEX_NONE, Nos.Num());
	for (int32 No = 0; No < Nos.Num(); ++No)
	{
		DeQueBoca[No] = (Pai[No] == INDEX_NONE) ? No : DeQueBoca[Pai[No]];
	}

	// AS EMENDAS ENTRE CAVERNAS.
	//
	// Este é o ponto que engana: no space colonization cada nó tem UM pai, e
	// duas árvores nunca se fundem. As galerias de duas cavernas chegam a se
	// encostar no desenho e continuam sendo dois sistemas separados — quem
	// entrasse numa não sairia na outra.
	//
	// A emenda é explícita, e é o que transforma "parece ligado" em "está
	// ligado". Uma por par de cavernas, na aproximação mais curta entre elas.
	TMap<uint64, TPair<int32, int32>> MaisPerto;
	TMap<uint64, float> Menor;

	for (int32 Um = 0; Um < Nos.Num(); ++Um)
	{
		for (int32 Outro = Um + 1; Outro < Nos.Num(); ++Outro)
		{
			if (DeQueBoca[Um] == DeQueBoca[Outro])
			{
				continue;
			}

			const float Ate = FVector2D::DistSquared(Nos[Um], Nos[Outro]);
			if (Ate > FMath::Square(Raio * EmendaNoMaximo))
			{
				continue;
			}

			const int32 Menorzinha = FMath::Min(DeQueBoca[Um], DeQueBoca[Outro]);
			const int32 Maiorzinha = FMath::Max(DeQueBoca[Um], DeQueBoca[Outro]);
			const uint64 Chave = (static_cast<uint64>(Menorzinha) << 32) | Maiorzinha;

			float& Guardado = Menor.FindOrAdd(Chave, TNumericLimits<float>::Max());
			if (Ate < Guardado)
			{
				Guardado = Ate;
				MaisPerto.Add(Chave, TPair<int32, int32>(Um, Outro));
			}
		}
	}

	// E AS EMENDAS QUE FALTAM PARA A REDE SER CONEXA.
	//
	// As galerias ligam gruta a gruta, e isso deixa de fora a bacia que não tem
	// gruta nenhuma — e uma bacia isolada é água que não se alcança de barco
	// por caminho nenhum.
	//
	// Aqui a rede subterrânea assume o papel que ela já tinha na intenção: ela
	// é o que liga BACIA a BACIA. Cada componente ainda solto ganha uma
	// passagem até o mais próximo.
	{
		const TArray<FRiverCourse> Rios = Plan();

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

		// Junta o que já está junto: confluência e galeria existente.
		for (int32 Qual = 0; Qual < Rios.Num(); ++Qual)
		{
			if (Rios[Qual].PointsUnits.Num() < 2)
			{
				continue;
			}

			for (int32 Outro = 0; Outro < Rios.Num(); ++Outro)
			{
				if (Outro == Qual)
				{
					continue;
				}

				float Aonde = 0.0f;
				if (NearestOn(Rios[Outro], Rios[Qual].PointsUnits.Last(), Aonde)
					<= Raio * PassoDoGalho * 1.5f)
				{
					Dono[Raiz(Qual)] = Raiz(Outro);
				}
			}
		}

		// E liga o que sobrou, um componente de cada vez.
		for (int32 Qual = 1; Qual < Rios.Num(); ++Qual)
		{
			if (Raiz(Qual) == Raiz(0) || Rios[Qual].PointsUnits.Num() < 2)
			{
				continue;
			}

			int32 Vizinho = INDEX_NONE;
			float MaisCurta = TNumericLimits<float>::Max();

			for (int32 Outro = 0; Outro < Rios.Num(); ++Outro)
			{
				if (Raiz(Outro) == Raiz(Qual) || Rios[Outro].PointsUnits.Num() < 2)
				{
					continue;
				}

				float Aonde = 0.0f;
				const float Ate = NearestOn(Rios[Outro], Rios[Qual].PointsUnits[0], Aonde);
				if (Ate < MaisCurta)
				{
					MaisCurta = Ate;
					Vizinho = Outro;
				}
			}

			if (Vizinho == INDEX_NONE)
			{
				continue;
			}

			float Aonde = 0.0f;
			NearestOn(Rios[Vizinho], Rios[Qual].PointsUnits[0], Aonde);

			FUnderwaterLink Passagem;
			Passagem.Navigability = ENavigability::BarcoPequeno;
			Passagem.PointsUnits.Add(Rios[Qual].PointsUnits[0]);
			Passagem.PointsUnits.Add(PointAtProgress(Rios[Vizinho], Aonde));
			Passagens.Add(MoveTemp(Passagem));

			Dono[Raiz(Qual)] = Raiz(Vizinho);
		}
	}

	for (const TPair<uint64, TPair<int32, int32>>& Emenda : MaisPerto)
	{
		// A emenda CAVA, não risca.
		//
		// Ligar dois pontos com um segmento é exatamente o desenho reto que
		// esta rede veio substituir — eu consertei o gerador e recriei o
		// defeito na costura. Galeria de calcário não vai em linha: ela segue
		// fratura, e o que sai é caminho torto.
		//
		// É a caminhada aleatória com ruído COERENTE: cada passo mira o destino
		// mas é empurrado pelo ruído da posição. Sorteio independente daria
		// tremor; ruído da posição dá curva, e a mesma pedra dá sempre a mesma
		// curva.
		const FVector2D DaBoca = Nos[Emenda.Value.Key];
		const FVector2D ParaABoca = Nos[Emenda.Value.Value];

		const float Distancia = static_cast<float>(FVector2D::Distance(DaBoca, ParaABoca));
		const int32 Passos = FMath::Max(3,
			FMath::CeilToInt(Distancia / (Raio * PassoDaGaleria)));

		FUnderwaterLink Passagem;
		Passagem.Navigability = ENavigability::BarcoPequeno;
		Passagem.PointsUnits.Add(DaBoca);

		FVector2D NaGaleria = DaBoca;
		for (int32 Avanco = 1; Avanco < Passos; ++Avanco)
		{
			const FVector2D ParaOAlvo = (ParaABoca - NaGaleria).GetSafeNormal();

			// A mira APERTA no fim: perto do destino o ruído perde peso, senão
			// a galeria passa do ponto e volta.
			const float Quanto = static_cast<float>(Avanco) / static_cast<float>(Passos);
			const FVector2D Rumo = (ParaOAlvo
				+ EmpurraoOrganico(NaGaleria, TorceAGaleria * (1.0f - Quanto))).GetSafeNormal();

			NaGaleria += Rumo * (Distancia / static_cast<float>(Passos));
			Passagem.PointsUnits.Add(NaGaleria);
		}

		Passagem.PointsUnits.Add(ParaABoca);
		Passagens.Add(MoveTemp(Passagem));
	}

	for (int32 Ponta = 0; Ponta < Nos.Num(); ++Ponta)
	{
		const bool bComecaAqui = (Filhos[Ponta] != 1)
			|| (Pai[Ponta] != INDEX_NONE && Filhos[Pai[Ponta]] > 1);

		if (!bComecaAqui)
		{
			continue;
		}

		FUnderwaterLink Passagem;
		Passagem.PointsUnits.Add(Nos[Ponta]);

		int32 Onde = Ponta;
		while (Pai[Onde] != INDEX_NONE)
		{
			Onde = Pai[Onde];
			Passagem.PointsUnits.Add(Nos[Onde]);

			if (Filhos[Onde] > 1 && Onde != Ponta)
			{
				break;
			}
		}

		if (Passagem.PointsUnits.Num() < 2)
		{
			continue;
		}

		// Passagem de pedra é apertada, sempre: se coubesse barco grande, o
		// atalho de baixo tornaria o rio de cima decorativo.
		Passagem.Navigability = ENavigability::BarcoPequeno;
		Passagens.Add(MoveTemp(Passagem));
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
			float Aonde = 0.0f;
			const float Ate = NearestOn(Rios[Indice], Onde, Aonde);
			if (Ate < Menor)
			{
				Menor = Ate;
				Melhor = Indice;
			}
		}

		return Melhor;
	};

	// PRIMEIRO a bacia consigo mesma.
	//
	// Faltava isto, e era o buraco mais óbvio: um galho DESEMBOCA no tronco —
	// eles se tocam por construção — e a conta de conectividade só considerava
	// córrego e galeria. A bacia inteira aparecia como dezenas de pedaços
	// soltos, e a resposta "dá para ir de barco a todo lugar" era não por um
	// motivo que não existia.
	//
	// Dois cursos estão ligados quando a FOZ de um encosta no outro: é o que
	// uma confluência é.
	for (int32 Qual = 0; Qual < Rios.Num(); ++Qual)
	{
		if (Rios[Qual].PointsUnits.Num() < 2)
		{
			continue;
		}

		const FVector2D NaFoz = Rios[Qual].PointsUnits.Last();

		for (int32 Outro = 0; Outro < Rios.Num(); ++Outro)
		{
			if (Outro == Qual)
			{
				continue;
			}

			float Aonde = 0.0f;
			const float Ate = NearestOn(Rios[Outro], NaFoz, Aonde);

			// A tolerância é o PASSO do gerador, e não a calha: a foz de um
			// curso é o nó onde ele nasceu no outro, e os dois foram amostrados
			// em passos discretos — dois nós vizinhos ficam um passo apart.
			// Medir pela calha reprovava confluências verdadeiras num fio fino.
			if (Ate <= IslandGeography::LandRadiusUnits() * PassoDoGalho * 1.5f)
			{
				Juntar(Qual, Outro);
			}
		}
	}

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
		// A galeria liga o rio mais perto de UMA ponta ao mais perto da outra.
		// Ela é uma linha agora, não um segmento — e as duas pontas dela são o
		// que importa para a conta de conectividade.
		Juntar(RioDoPonto(Passagem.PointsUnits[0]),
			RioDoPonto(Passagem.PointsUnits.Last()));
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

float FreshWater::BedGradientAtProgress(const FRiverCourse& Course, float Progress)
{
	// Medido ao longo do CURSO, não em linha reta: o rio serpenteia, e a mesma
	// queda de altura repartida por um percurso maior é um leito mais manso.
	// Foi por serpentear que ele ficou manso.
	const float Passo = MeiaCalhaDoRio() * 2.0f;

	const float Meio = Passo / FMath::Max(1.0f, CourseLengthUnits(Course)) * 0.5f;
	const FVector2D Antes = PointAtProgress(Course, Progress - Meio);
	const FVector2D Depois = PointAtProgress(Course, Progress + Meio);

	const float Andado = FVector2D::Distance(Antes, Depois);
	if (Andado <= KINDA_SMALL_NUMBER)
	{
		return 0.0f;
	}

	// A ROCHA, e não a altura "natural": a mesa da Cidade Alta também pergunta
	// onde é a cidade, e o ciclo continuava por ali. Eu tinha tirado só o
	// achatamento dos lotes e dado o problema por resolvido — o rastro de pilha
	// do processo abortando mostrou o caminho inteiro.
	//
	// E a camada certa é essa mesmo: o leito do rio é anterior a qualquer
	// decisão sobre onde as pessoas moram.
	const float Desceu = IslandGeography::BedrockHeightAt(Antes)
		- IslandGeography::BedrockHeightAt(Depois);

	// Só a DESCIDA conta. Um trecho em que o terreno sobe não é corredeira: é
	// erro de amostragem do relevo, e tratá-lo como declive poria espuma onde
	// a água estaria empoçando.
	return FMath::Max(0.0f, Desceu) / Andado;
}

float FreshWater::RapidsGradient()
{
	// Quatro por cento. É o dobro do declive sustentável de uma trilha, e a
	// água quebra bem antes de uma pessoa achar a subida difícil.
	return 0.04f;
}

bool FreshWater::IsRapidsAtProgress(const FRiverCourse& Course, float Progress)
{
	if (Progress < 0.0f || Progress > 1.0f)
	{
		return false;
	}

	// Dentro do lago não há corredeira, por mais inclinado que esteja o chão em
	// volta: lago é água PARADA, e a inclinação ali é a da encosta que o cerca.
	//
	// A comparação é com a calha DESTE curso, não com a do rio base. Comparando
	// com a base, todo tronco parecia lago — ele é 2,9 vezes mais largo que a
	// calha base só por ser de ordem 3, e nenhuma corredeira existia na ilha.
	// É o mesmo erro de escalar pela coisa errada que já custou a gruta.
	const float DaCalha = HalfWidthAtProgress(Course, 0.0f);
	if (HalfWidthAtProgress(Course, Progress) > DaCalha * 1.6f)
	{
		return false;
	}

	return BedGradientAtProgress(Course, Progress) >= RapidsGradient();
}

float FreshWater::PlungePoolDepthUnits(const FRiverCourse& Course)
{
	if (!Course.HasFall())
	{
		return 0.0f;
	}

	// A profundidade sai da ALTURA da queda, que é o que a morfologia mede.
	const float EmProgresso = MeiaQueda() / FMath::Max(1.0f, CourseLengthUnits(Course));
	const FVector2D NoAlto = PointAtProgress(Course, Course.FallAtProgress - EmProgresso);
	const FVector2D LaEmbaixo = PointAtProgress(Course, Course.FallAtProgress + EmProgresso);

	const float Caiu = FMath::Max(0.0f, IslandGeography::BedrockHeightAt(NoAlto)
		- IslandGeography::BedrockHeightAt(LaEmbaixo));

	return Caiu * ProfundidadePorAltura;
}

float FreshWater::PlungePoolHalfWidthUnits(const FRiverCourse& Course)
{
	// O poço aprofunda cerca de DEZ vezes mais rápido do que alarga. Poço de
	// cachoeira é furo, não bacia — e é por isso que a largura sai da
	// profundidade dividida, e não de um raio escolhido.
	const float Fundo = PlungePoolDepthUnits(Course);
	if (Fundo <= 0.0f)
	{
		return 0.0f;
	}

	// A largura sai da profundidade, e é MAIOR que ela: o poço se abre em volta
	// do jato. O piso é a calha do rio — poço mais estreito que o rio que o
	// alimenta seria um estrangulamento, não um poço.
	return FMath::Max(MeiaCalhaDoRio() * 1.15f, Fundo * AlargaSobreAprofunda);
}

float FreshWater::DistanceToFreshWater(const FVector2D& PositionUnits)
{
	float Menor = TNumericLimits<float>::Max();

	for (const FRiverCourse& Curso : Plan())
	{
		float Onde = 0.0f;
		const float Ate = NearestOn(Curso, PositionUnits, Onde);
		Menor = FMath::Min(Menor, Ate - HalfWidthAtProgress(Curso, Onde));
	}

	// Os CÓRREGOS contam: eles são água, e uma trilha que os ignore atravessa
	// molhada sem dizer nada. Fonte também — ela é uma poça de verdade.
	for (const FBrook& Corrego : PlanBrooks())
	{
		for (int32 Ponto = 1; Ponto < Corrego.PointsUnits.Num(); ++Ponto)
		{
			const FVector2D Trecho = Corrego.PointsUnits[Ponto] - Corrego.PointsUnits[Ponto - 1];
			const float Comprimento = static_cast<float>(Trecho.SizeSquared());
			if (Comprimento <= KINDA_SMALL_NUMBER)
			{
				continue;
			}

			const float Onde = FMath::Clamp(static_cast<float>(FVector2D::DotProduct(
				PositionUnits - Corrego.PointsUnits[Ponto - 1], Trecho)) / Comprimento, 0.0f, 1.0f);

			const FVector2D MaisPerto = Corrego.PointsUnits[Ponto - 1] + Trecho * Onde;
			Menor = FMath::Min(Menor, static_cast<float>(
				FVector2D::Distance(PositionUnits, MaisPerto)) - Corrego.HalfWidthUnits);
		}
	}

	for (const FSpring& Fonte : PlanSprings())
	{
		Menor = FMath::Min(Menor, static_cast<float>(
			FVector2D::Distance(PositionUnits, Fonte.CenterUnits)) - Fonte.PoolHalfWidthUnits);
	}

	return Menor;
}

namespace MascaraDaAgua
{
	/** A mesma resolução da sonda de calibragem. Uma só, e é o ponto. */
	const int32 Lado = FreshWater::LadoDaGradeDeAgua;

	const TArray<bool>& Grade()
	{
		// Desenhada pela MESMA função que a sonda usa. Quando eram duas, a
		// máscara lia 11,6% onde a sonda lia 6% — a busca acertava um alvo e o
		// mundo entregava outro.
		static TArray<bool> Molhado = FreshWater::RasterizarAgua(
			FreshWater::Plan(), FreshWater::PlanBrooks(), FreshWater::PlanSprings(), Lado);

		return Molhado;
	}
}

bool FreshWater::IsFreshWaterAt(const FVector2D& PositionUnits)
{
	const float Raio = IslandGeography::LandRadiusUnits();
	const float Casa = (Raio * 2.0f) / static_cast<float>(MascaraDaAgua::Lado - 1);

	const int32 Coluna = FMath::RoundToInt((PositionUnits.X + Raio) / Casa);
	const int32 Linha = FMath::RoundToInt((PositionUnits.Y + Raio) / Casa);

	if (Coluna < 0 || Linha < 0
		|| Coluna >= MascaraDaAgua::Lado || Linha >= MascaraDaAgua::Lado)
	{
		return false;
	}

	return MascaraDaAgua::Grade()[Linha * MascaraDaAgua::Lado + Coluna];
}

float FreshWater::WaterCoverageForBiome(EIslandBiome Biome)
{
	// A tabela mora em `WorldBudget`, com os outros números do bioma. Ela
	// estava aqui, e aqui era o lugar errado: fazer um deserto significaria
	// caçar constante por constante em seis arquivos.
	return WorldBudget::WaterCoverage(Biome);
}

float FreshWater::MeasuredWaterCoverage()
{
	// Medido na MÁSCARA, que é a mesma coisa que o jogo consulta para saber se
	// um ponto está molhado. Medir no plano dos cursos daria a intenção; medir
	// na máscara dá o que existe.
	const float Raio = IslandGeography::LandRadiusUnits();
	const float Casa = (Raio * 2.0f) / static_cast<float>(MascaraDaAgua::Lado - 1);

	int32 EmTerra = 0;
	int32 Molhadas = 0;

	for (int32 Linha = 0; Linha < MascaraDaAgua::Lado; ++Linha)
	{
		for (int32 Coluna = 0; Coluna < MascaraDaAgua::Lado; ++Coluna)
		{
			const FVector2D Onde(-Raio + Coluna * Casa, -Raio + Linha * Casa);
			if (Onde.Size() > Raio)
			{
				continue;
			}

			++EmTerra;
			Molhadas += MascaraDaAgua::Grade()[Linha * MascaraDaAgua::Lado + Coluna] ? 1 : 0;
		}
	}

	return (EmTerra > 0) ? static_cast<float>(Molhadas) / static_cast<float>(EmTerra) : 0.0f;
}

namespace CachoeiraEmDegraus
{
	/**
	 * Quantos patamares, e a altura mínima de um.
	 *
	 * A conta é a altura da queda dividida pelo degrau que uma pessoa vence —
	 * e não um número de degraus escolhido. Queda baixa dá um tombo só; queda
	 * alta vira escada, que é o que a rocha faz de verdade.
	 */
	constexpr float AlturaDeUmDegrau = 420.0f;
	constexpr int32 MaisDegraus = 6;

	/** Quantas pedras por patamar, e o tamanho delas em calhas de rio. */
	constexpr int32 PedrasPorDegrau = 5;
	constexpr float MenorPedra = 0.22f;
	constexpr float MaiorPedra = 0.55f;

	/** Quanto a subida se afasta da lâmina: a margem, não a água. */
	constexpr float AfastaDaLamina = 1.9f;
}

TArray<FreshWater::FFallStep> FreshWater::PlanFallSteps(const FRiverCourse& Course)
{
	TArray<FFallStep> Degraus;
	if (!Course.HasFall())
	{
		return Degraus;
	}

	const float Caiu = PlungePoolDepthUnits(Course) / 0.55f;
	if (Caiu <= 0.0f)
	{
		return Degraus;
	}

	const int32 Quantos = FMath::Clamp(
		FMath::RoundToInt(Caiu / CachoeiraEmDegraus::AlturaDeUmDegrau),
		1, CachoeiraEmDegraus::MaisDegraus);

	const float EmProgresso = MeiaQueda() / FMath::Max(1.0f, CourseLengthUnits(Course));

	for (int32 Qual = 0; Qual < Quantos; ++Qual)
	{
		// Do PÉ para o topo: o primeiro patamar é o que recebe a água, e é
		// dele que a subida parte.
		const float Quanto = static_cast<float>(Qual) / static_cast<float>(Quantos);

		FFallStep Degrau;
		Degrau.CenterUnits = PointAtProgress(Course,
			Course.FallAtProgress + EmProgresso * (1.0f - 2.0f * Quanto));
		Degrau.HeightUnits = Caiu * (static_cast<float>(Qual + 1) / Quantos);
		Degrau.HalfWidthUnits = HalfWidthAtProgress(Course, Course.FallAtProgress);

		Degraus.Add(Degrau);
	}

	return Degraus;
}

TArray<FreshWater::FFallStone> FreshWater::PlanFallStones(const FRiverCourse& Course)
{
	TArray<FFallStone> Pedras;

	const TArray<FFallStep> Degraus = PlanFallSteps(Course);
	const TArray<FVector2D> Subida = PlanFallClimb(Course);
	if (Degraus.Num() == 0)
	{
		return Pedras;
	}

	const float DaCalha = HalfWidthAtProgress(Course, Course.FallAtProgress);

	for (int32 Qual = 0; Qual < Degraus.Num(); ++Qual)
	{
		for (int32 Pedra = 0; Pedra < CachoeiraEmDegraus::PedrasPorDegrau; ++Pedra)
		{
			const uint32 Semente = BattleSpread::SeedFromText(
				FString::Printf(TEXT("pedra-da-queda-%d-%d-%d"),
					FMath::RoundToInt(Course.FallAtProgress * 1000.0f), Qual, Pedra));

			const float Rumo = BattleSpread::Between(0.0f, 2.0f * PI,
				BattleSpread::Fraction(Semente, 0));
			const float Longe = DaCalha * BattleSpread::Between(0.4f, 2.2f,
				BattleSpread::Fraction(Semente, 1));

			FFallStone Bloco;
			Bloco.CenterUnits = Degraus[Qual].CenterUnits
				+ FVector2D(FMath::Cos(Rumo), FMath::Sin(Rumo)) * Longe;
			Bloco.RadiusUnits = DaCalha * BattleSpread::Between(
				CachoeiraEmDegraus::MenorPedra, CachoeiraEmDegraus::MaiorPedra,
				BattleSpread::Fraction(Semente, 2));
			Bloco.TopHeightUnits = Degraus[Qual].HeightUnits + Bloco.RadiusUnits;

			Pedras.Add(Bloco);
		}
	}

	// E AS PEDRAS DA ESCADA, POSTAS no caminho — não sorteadas na esperança de
	// que alguma caia perto.
	//
	// A primeira versão espalhava tudo em volta do patamar e marcava como
	// degrau o que por acaso encostasse na subida. Nenhuma encostava, e o teste
	// pegou. Escada improvisada é pedra POSTA ali: quem sobe empilhou o que a
	// água derrubou, e é essa intenção que faz dela uma escada.
	for (int32 Onde = 0; Onde + 1 < Subida.Num(); ++Onde)
	{
		const uint32 Semente = BattleSpread::SeedFromText(
			FString::Printf(TEXT("degrau-de-pedra-%d-%d"),
				FMath::RoundToInt(Course.FallAtProgress * 1000.0f), Onde));

		// No meio de cada lance, que é onde o pé precisa de apoio: nas pontas
		// ele já está num patamar.
		FFallStone Degrau;
		Degrau.CenterUnits = FMath::Lerp(Subida[Onde], Subida[Onde + 1], 0.5f);
		Degrau.RadiusUnits = DaCalha * BattleSpread::Between(0.30f, 0.48f,
			BattleSpread::Fraction(Semente, 0));

		const int32 QualPatamar = FMath::Min(Onde / 2, Degraus.Num() - 1);
		Degrau.TopHeightUnits = Degraus[QualPatamar].HeightUnits * 0.5f + Degrau.RadiusUnits;
		Degrau.bIsStep = true;

		Pedras.Add(Degrau);
	}

	return Pedras;
}

TArray<FVector2D> FreshWater::PlanFallClimb(const FRiverCourse& Course)
{
	TArray<FVector2D> Subida;

	const TArray<FFallStep> Degraus = PlanFallSteps(Course);
	if (Degraus.Num() == 0)
	{
		return Subida;
	}

	// A MAIOR lâmina da vizinhança da queda, não a do ponto exato dela.
	//
	// A calha muda ao longo do curso, e a subida percorre uma faixa: afastar-se
	// pela largura de um ponto só põe metade do caminho dentro da água quando o
	// trecho ao lado é mais largo.
	float DaCalha = HalfWidthAtProgress(Course, Course.FallAtProgress);
	for (int32 Passo = -4; Passo <= 4; ++Passo)
	{
		DaCalha = FMath::Max(DaCalha,
			HalfWidthAtProgress(Course, Course.FallAtProgress + Passo * 0.03f));
	}

	// E a folga é generosa: a subida percorre uma faixa, e encostar na lâmina
	// em um ponto já é caminho dentro da água.
	const float Afasta = DaCalha * CachoeiraEmDegraus::AfastaDaLamina
		+ MeiaCalhaDoRio() * 2.0f;

	// O rumo da água, para saber o que é "de lado".
	const float EmProgresso = MeiaQueda() / FMath::Max(1.0f, CourseLengthUnits(Course));
	const FVector2D AoLongo = (PointAtProgress(Course, Course.FallAtProgress + EmProgresso)
		- PointAtProgress(Course, Course.FallAtProgress - EmProgresso)).GetSafeNormal();
	const FVector2D DeLado(-AoLongo.Y, AoLongo.X);

	// ZIGUEZAGUE pela margem, alternando o lado a cada patamar. É a mesma regra
	// da trilha de serra, apertada: a margem de uma queda é estreita, e é por
	// isso que as pedras precisam virar degrau.
	// SEMPRE pé e topo, mesmo com um patamar só.
	//
	// A primeira versão punha um ponto por patamar, e a queda de um degrau
	// ficava com um caminho de UM ponto — que não é caminho, e por isso não
	// tinha lance nenhum onde pôr pedra. Um degrau ainda tem base e topo.
	const FVector2D NoPe = PointAtProgress(Course, Course.FallAtProgress + EmProgresso);
	const FVector2D NoTopo = PointAtProgress(Course, Course.FallAtProgress - EmProgresso);

	float ParaQualLado = 1.0f;
	Subida.Add(NoPe + DeLado * (ParaQualLado * Afasta));

	for (int32 Qual = 0; Qual < Degraus.Num(); ++Qual)
	{
		ParaQualLado = -ParaQualLado;
		Subida.Add(Degraus[Qual].CenterUnits + DeLado * (ParaQualLado * Afasta));

		// O ponto do meio do lance: é onde o pé precisa de apoio, e é onde a
		// pedra de degrau vai.
		if (Qual + 1 < Degraus.Num())
		{
			const FVector2D EntreOsDois = FMath::Lerp(
				Degraus[Qual].CenterUnits, Degraus[Qual + 1].CenterUnits, 0.5f);
			Subida.Add(EntreOsDois + DeLado * (-ParaQualLado * Afasta * 1.35f));
		}
	}

	Subida.Add(NoTopo + DeLado * (-ParaQualLado * Afasta));

	return Subida;
}

FVector2D FreshWater::FlowDirectionAtProgress(const FRiverCourse& Course, float Progress)
{
	// Da nascente para a foz: progresso crescente é o sentido da água. Medido
	// no próprio traçado, com um passo curto para a direção acompanhar a curva
	// em vez de apontar para a foz em linha reta.
	const float Passo = 0.02f;

	const FVector2D Aqui = PointAtProgress(Course, FMath::Clamp(Progress, 0.0f, 1.0f - Passo));
	const FVector2D Adiante = PointAtProgress(Course, FMath::Clamp(Progress + Passo, 0.0f, 1.0f));

	return (Adiante - Aqui).GetSafeNormal();
}

bool FreshWater::IsThermalAt(const FVector2D& PositionUnits)
{
	// O cruzamento de duas coisas que já existem: o alcance do calor do vulcão
	// e a máscara de água. Nada aqui é escolhido — é consequência.
	if (!IsFreshWaterAt(PositionUnits))
	{
		return false;
	}

	return FVector2D::Distance(PositionUnits, IslandGeography::VolcanoCenterUnits())
		<= IslandGeography::VolcanoHeatRadiusUnits();
}

// Copyright 2026 Anderson. All Rights Reserved.

#include "Environment/ForestBackdrop.h"

#include "Battle/DeterministicSpread.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "World/WorldObstacleBreaking.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Environment/BiomeFlora.h"
#include "Environment/FreshWater.h"
#include "Environment/IslandGeography.h"
#include "Environment/ScenaryPalette.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace MataDoCenario
{
	/** Onde o pacote de mata (Kenney Nature Kit, CC0) foi importado. */
	const TCHAR* PastaDaMata = TEXT("/Game/Environment/Nature/");

	/** Primitiva do disco de chão — conteúdo da engine, não vendorizado. */
	const TCHAR* CilindroDaEngine = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");

	/**
	 * Primitiva do LADRILHO de chão.
	 *
	 * Quadrado porque ele encosta nos vizinhos pelos quatro lados. Disco
	 * ladrilhado deixa buraco em cada canto, e o jogador que anda em diagonal
	 * cai justamente por lá.
	 */
	const TCHAR* CuboDaEngine = TEXT("/Engine/BasicShapes/Cube.Cube");

	/**
	 * Primitiva do MONTE de relevo.
	 *
	 * Esfera, e não cone: duna é curva. O cone foi o que deu ao jogador as
	 * "montanhas pontudas extremamente esquisitas", e repeti-lo no chão
	 * espalharia o mesmo defeito por bioma inteiro.
	 */
	const TCHAR* EsferaDaEngine = TEXT("/Engine/BasicShapes/Sphere.Sphere");

	/**
	 * Onde começam os fluxos de sorteio dos montes.
	 *
	 * Longe dos das plantas de propósito: dois sorteios no mesmo índice saem
	 * IGUAIS, e a duna nasceria debaixo do mesmo arbusto em todo pedaço.
	 */
	constexpr int32 PrimeiroFluxoDoMonte = 900000;

	/** Grandezas sorteadas por monte: dois eixos, um tamanho, um giro. */
	constexpr int32 SorteiosPorMonte = 4;

	/** Onde começam os fluxos de sorteio das árvores de beira. */
	constexpr int32 PrimeiroFluxoDaBeira = 1900000;

	/** Grandezas sorteadas por árvore de beira: recuo, porte e tombo. */
	constexpr int32 SorteiosPorArvoreDeBeira = 3;

	/** Comprimento de cada laje da orla, medido AO LONGO do arco. */
	constexpr float ComprimentoDaLajeDaOrla = 300.0f;

	/** Largura RADIAL da faixa molhada — o que a onda alcança. */
	constexpr float LarguraDaFaixaMolhada = 420.0f;

	/** Largura RADIAL da espuma. Estreita: espuma larga vira nevoeiro no chão. */
	constexpr float LarguraDaEspuma = 90.0f;

	/** Espessura das lajes da orla: meia laje enterrada, meia à vista. */
	constexpr float EspessuraDaLajeDaOrla = 8.0f;

	/** Uma árvore de beira a cada tantos passos do arco. */
	constexpr int32 PassosPorArvoreDeBeira = 9;

	/** Quanto a árvore de beira pode recuar (ou avançar) sobre a areia. */
	constexpr float RecuoDaArvoreDeBeira = 260.0f;

	/** Altura desejada da árvore de beira, em unidades de mundo. */
	constexpr float AlturaDaArvoreDeBeira = 640.0f;

	/** Lado da primitiva da engine, em unidades de mundo. */
	constexpr float CilindroDaEngineUnidades = 100.0f;

	/** Espessura do disco: fino, mas acima do plano do template. */
	/**
	 * O que se VÊ do chão: uma borda fina, porque ele é uma clareira e não um
	 * platô.
	 */
	constexpr float EspessuraDoChao = 4.0f;

	/**
	 * O que o chão TEM de corpo, para baixo.
	 *
	 * Quatro unidades de espessura seguram um pet parado e NÃO seguram um
	 * jogador caindo: a cápsula atravessa uma laje fina entre dois quadros, e
	 * o mundo parece não ter piso. Duzentas unidades de pedra invisível abaixo
	 * da superfície custam nada e fecham esse buraco.
	 *
	 * O TOPO não muda — só cresce para baixo. A clareira continua rente.
	 */
	constexpr float ProfundidadeDoChao = 200.0f;

	/**
	 * Quanto o chão de um pedaço passa das bordas dele.
	 *
	 * Quatro por cento de 6400 é 256 unidades de cada lado — bem mais que o
	 * passo de um personagem, que é a medida que importa: a fresta só engole
	 * quem cabe nela.
	 */
	constexpr float TransbordoDoChao = 1.04f;

	/** Quantas vezes tentar reposicionar uma planta antes de desistir dela. */
	constexpr int32 TentativasPorPlanta = 8;

	/**
	 * Faixa em que a espécie nasce, em casas do tabuleiro.
	 *
	 * O enquadramento é um diorama: o chão só é visível numa coroa estreita
	 * atrás da grade, e o que estiver mais longe sobe para fora do topo da
	 * tela. Por isso a rasteira fica perto (ela precisa ser vista no chão) e
	 * o dossel fica na faixa em que o TRONCO cruza o topo do quadro — é ele
	 * que fecha o fundo.
	 */
	struct FFaixa
	{
		float RaioMinimoEmCasas;
		float RaioMaximoEmCasas;
	};

	constexpr FFaixa Rasteira{ 2.3f, 6.0f };
	constexpr FFaixa Borda{ 2.6f, 7.5f };
	constexpr FFaixa Mata{ 3.4f, 9.0f };
	constexpr FFaixa Dossel{ 5.0f, 16.0f };

	/**
	 * Uma espécie da mata.
	 *
	 * `AlturaEmCasas` é a altura DESEJADA; a escala sai dela dividida pela
	 * caixa medida da malha. Escala fixa por espécie seria um número mágico
	 * contra uma malha que pode ser trocada — e trocar a malha calaria o
	 * defeito em vez de corrigi-lo.
	 */
	struct FEspecie
	{
		const TCHAR* Nome;
		FFaixa Faixa;
		float AlturaEmCasas;
		int32 Quantidade;
		/**
		 * O que esta espécie precisa DIZER na tela.
		 *
		 * Não sai da malha nem do material: `plant_bush` e `grass_large`
		 * chegam do pacote com o mesmo slot `grass`, e enquanto a cor vinha
		 * de lá o arbusto e o capim eram o mesmo verde. É esta coluna que os
		 * separa.
		 */
		EScenaryRole Papel;
	};

	const FEspecie Especies[] = {
		// Rasteira: o que dá textura ao chão logo atrás da grade.
		// Não existe malha "grass": o kit traz um MATERIAL com esse nome, e
		// ele ficou com o slot do asset na importação. As duas abaixo cobrem
		// o mesmo papel de capim rasteiro.
		{ TEXT("grass_large"),             Rasteira, 0.42f, 56, EScenaryRole::GroundCover },
		{ TEXT("grass_leafs"),             Rasteira, 0.26f, 54, EScenaryRole::GroundCover },
		{ TEXT("flower_redA"),             Rasteira, 0.24f, 14, EScenaryRole::Accent },
		{ TEXT("flower_yellowA"),          Rasteira, 0.20f, 14, EScenaryRole::Accent },
		{ TEXT("mushroom_red"),            Rasteira, 0.18f, 10, EScenaryRole::Accent },

		// Borda: o volume baixo que separa o tabuleiro da mata.
		{ TEXT("plant_bushSmall"),         Borda,    0.40f, 20, EScenaryRole::Undergrowth },
		{ TEXT("plant_bush"),              Borda,    0.55f, 18, EScenaryRole::Undergrowth },
		{ TEXT("plant_bushLarge"),         Borda,    0.62f, 16, EScenaryRole::Undergrowth },
		{ TEXT("stump_round"),             Borda,    0.38f,  8, EScenaryRole::DeadWood },
		{ TEXT("log"),                     Borda,    0.34f,  8, EScenaryRole::DeadWood },
		{ TEXT("rock_smallA"),             Borda,    0.32f, 12, EScenaryRole::Rock },
		{ TEXT("rock_smallD"),             Borda,    0.38f, 12, EScenaryRole::Rock },

		// Mata: primeiro plano de árvore, ainda inteira no quadro.
		{ TEXT("rock_largeA"),             Mata,     0.58f,  8, EScenaryRole::Rock },
		{ TEXT("rock_largeC"),             Mata,     0.70f,  6, EScenaryRole::Rock },
		{ TEXT("rock_tallD"),              Mata,     1.30f,  6, EScenaryRole::Rock },
		{ TEXT("rock_tallA"),              Mata,     1.80f,  4, EScenaryRole::Rock },
		{ TEXT("tree_pineSmallA"),         Mata,     1.70f, 12, EScenaryRole::ForestTree },
		{ TEXT("tree_pineSmallC"),         Mata,     1.90f, 12, EScenaryRole::ForestTree },
		{ TEXT("tree_oak"),                Mata,     2.30f, 10, EScenaryRole::ForestTree },
		{ TEXT("tree_blocks"),             Mata,     2.10f,  8, EScenaryRole::ForestTree },

		// Dossel: o paredão que fecha o fundo.
		{ TEXT("tree_pineRoundC"),         Dossel,   2.50f, 12, EScenaryRole::CanopyTree },
		{ TEXT("tree_pineRoundA"),         Dossel,   2.70f, 12, EScenaryRole::CanopyTree },
		{ TEXT("tree_default"),            Dossel,   2.90f, 12, EScenaryRole::CanopyTree },
		{ TEXT("tree_thin"),               Dossel,   3.20f, 10, EScenaryRole::CanopyTree },
		{ TEXT("tree_tall"),               Dossel,   3.40f, 10, EScenaryRole::CanopyTree },
		{ TEXT("tree_pineTallA_detailed"), Dossel,   3.50f, 12, EScenaryRole::CanopyTree },
		{ TEXT("tree_pineTallB_detailed"), Dossel,   3.90f, 12, EScenaryRole::CanopyTree },
		{ TEXT("tree_pineTallC_detailed"), Dossel,   3.70f, 12, EScenaryRole::CanopyTree },
	};

	constexpr int32 TotalDeEspecies = UE_ARRAY_COUNT(Especies);

	/** Fluxos de sorteio de uma planta — um índice por grandeza sorteada. */
	enum class ESorteio : int32
	{
		Angulo = 0,
		Raio = 1,
		Giro = 2,
		Tamanho = 3,
		Inclinacao = 4,
		PorPlanta = 5
	};

	int32 FluxoDaPlanta(int32 Planta, int32 Tentativa, ESorteio Qual)
	{
		const int32 PorTentativa = static_cast<int32>(ESorteio::PorPlanta);
		return (Planta * TentativasPorPlanta + Tentativa) * PorTentativa + static_cast<int32>(Qual);
	}

	/**
	 * O que um bioma deixa nascer, em porcentagem do que a mata tem.
	 *
	 * A tabela de espécies continua sendo UMA. O bioma a FILTRA — nunca a
	 * duplica: cinco tabelas paralelas concordariam entre si até a primeira
	 * edição, que é como L-032 e L-033 nasceram.
	 *
	 * `PapelDaPedra` existe porque pedra é o que sobra em quase todo bioma, e
	 * pedra da cor do chão é pedra invisível. No deserto ela é a pedra seca;
	 * na geleira e no vulcão ela é o cinza da serra, que se lê tanto contra o
	 * gelo branco quanto contra o basalto preto.
	 */
	struct FPresencaDoBioma
	{
		EScenaryRole PapelDoChao;
		EScenaryRole PapelDaPedra;
		int32 PercentualRasteiro;
		int32 PercentualDeEnfeite;
		int32 PercentualDeArbusto;
		int32 PercentualDeTronco;
		int32 PercentualDePedra;
		int32 PercentualDeArvore;

		/**
		 * Quantos montes de relevo o pedaço ganha, e de que tamanho.
		 *
		 * Zero é a resposta certa para a mata e para a praia: mata tem copa
		 * para dar escala, e praia é a faixa plana por onde se anda até o mar.
		 * Quem precisa de relevo é o bioma que, sem ele, vira um plano de uma
		 * cor só.
		 */
		int32 MontesPorPedaco;
		float LarguraDoMonteEmCasas;
		float AlturaDoMonteEmCasas;
	};

	FPresencaDoBioma PresencaDe(EIslandBiome Bioma)
	{
		switch (Bioma)
		{
		case EIslandBiome::Desert:
			// Nem capim nem flor: a duna é pedra e areia, e um único tufo
			// verde no meio dela desfaz o deserto inteiro.
			return { EScenaryRole::DesertSand, EScenaryRole::DesertRock, 0, 0, 6, 25, 70, 0,
				/*Montes=*/14, /*Largura=*/26.0f, /*Altura=*/3.0f };

		case EIslandBiome::Glacier:
			// A conífera resiste ao frio, e é ela que dá altura à geleira —
			// sem nada em pé, o gelo vira um plano branco sem escala.
			return { EScenaryRole::GlacierIce, EScenaryRole::MountainRock, 0, 0, 0, 10, 55, 18,
				/*Montes=*/10, /*Largura=*/15.0f, /*Altura=*/3.6f };

		case EIslandBiome::Volcano:
			// Pedra sobre pedra. O tronco morto que sobra é o que diz que
			// aqui já houve mata.
			return { EScenaryRole::VolcanicRock, EScenaryRole::MountainRock, 0, 0, 0, 15, 85, 0,
				/*Montes=*/8, /*Largura=*/12.0f, /*Altura=*/2.4f };

		case EIslandBiome::Beach:
			// Rala de propósito: a praia é a faixa por onde se ANDA até o
			// mar, e enchê-la de arbusto fecharia justamente a passagem.
			return { EScenaryRole::BeachSand, EScenaryRole::Rock, 12, 0, 8, 20, 35, 0,
				/*Montes=*/0, /*Largura=*/0.0f, /*Altura=*/0.0f };

		case EIslandBiome::Swamp:
			// O brejo é a mata que NÃO drenou, e a diferença entre os dois é
			// o que estava faltando na tela: aqui o chão é lama, o tronco
			// caído é o que mais aparece, e a copa alta some. Mata fechada
			// com chão escuro seria só uma mata à noite.
			//
			// As poucas pedras viram POÇA — mesma malha, pintada com a água
			// parada. Pântano sem água visível é floresta parda, e este era
			// justamente o defeito: a geografia sabia que ali era brejo, o
			// clima sabia, o mapa sabia, e só a tela não.
			return { EScenaryRole::SwampMud, EScenaryRole::SwampWater,
				90, 60, 70, 100, 18, 25,
				/*Montes=*/0, /*Largura=*/0.0f, /*Altura=*/0.0f };

		case EIslandBiome::Forest:
			break;
		}

		// A mata é a tabela inteira, sem filtro — e o `Count` no chão diz
		// "use a cor de chão de sempre", que é a mesma da arena.
		return { EScenaryRole::Count, EScenaryRole::Rock, 100, 100, 100, 100, 100, 100,
			/*Montes=*/0, /*Largura=*/0.0f, /*Altura=*/0.0f };
	}

	/**
	 * Como uma planta POUSA: giro, tamanho e inclinação, do mesmo sorteio.
	 *
	 * Uma função só porque a mata da arena e o ladrilho do mundo plantam a
	 * mesma espécie — e duas cópias deste jitter divergiriam na primeira
	 * edição, deixando o mesmo pinheiro com outro porte de um lado do mapa
	 * para o outro.
	 */
	FTransform PousoDaPlanta(
		uint32 SementeDaEspecie, int32 Planta, int32 Tentativa,
		const FVector2D& Posicao, float EscalaBase)
	{
		const float Giro = BattleSpread::Fraction(
			SementeDaEspecie, FluxoDaPlanta(Planta, Tentativa, ESorteio::Giro)) * 360.0f;
		const float Tamanho = EscalaBase * BattleSpread::Between(0.78f, 1.28f,
			BattleSpread::Fraction(SementeDaEspecie, FluxoDaPlanta(Planta, Tentativa, ESorteio::Tamanho)));
		const float Inclinacao = BattleSpread::Between(-4.0f, 4.0f,
			BattleSpread::Fraction(SementeDaEspecie, FluxoDaPlanta(Planta, Tentativa, ESorteio::Inclinacao)));

		return FTransform(
			FRotator(Inclinacao, Giro, 0.0f),
			FVector(Posicao.X, Posicao.Y, EspessuraDoChao * 0.5f),
			FVector(Tamanho));
	}

	int32 PercentualDoPapel(const FPresencaDoBioma& Presenca, EScenaryRole Papel)
	{
		switch (Papel)
		{
		case EScenaryRole::GroundCover: return Presenca.PercentualRasteiro;
		case EScenaryRole::Accent:      return Presenca.PercentualDeEnfeite;
		case EScenaryRole::Undergrowth: return Presenca.PercentualDeArbusto;
		case EScenaryRole::DeadWood:    return Presenca.PercentualDeTronco;
		case EScenaryRole::Rock:        return Presenca.PercentualDePedra;
		case EScenaryRole::ForestTree:  return Presenca.PercentualDeArvore;
		case EScenaryRole::CanopyTree:  return Presenca.PercentualDeArvore;
		default:                        return 0;
		}
	}

	/**
	 * Dá FORMA ao chão do pedaço: meias esferas meio enterradas.
	 *
	 * O centro fica no topo do chão, então metade da esfera aparece e metade
	 * some — é isso que faz a duna encontrar o plano numa curva em vez de num
	 * degrau. Uma esfera pousada EM CIMA do chão seria uma bola no deserto.
	 */
	void PlantarMontes(UHierarchicalInstancedStaticMeshComponent* Montes,
		const FPresencaDoBioma& Presenca, float CasaEmUnidades, uint32 Semente, float LadoDoPedaco)
	{
		if (!Montes)
		{
			return;
		}

		Montes->ClearInstances();
		if (!Montes->GetStaticMesh() || Presenca.MontesPorPedaco <= 0)
		{
			return;
		}

		const float Meio = LadoDoPedaco * 0.5f;
		const float Largura = Presenca.LarguraDoMonteEmCasas * CasaEmUnidades;
		const float Altura = Presenca.AlturaDoMonteEmCasas * CasaEmUnidades;
		const float TopoDoChao = AForestBackdrop::GroundTopLocalZ();

		for (int32 Monte = 0; Monte < Presenca.MontesPorPedaco; ++Monte)
		{
			const int32 Fluxo = PrimeiroFluxoDoMonte + Monte * SorteiosPorMonte;
			const float X = BattleSpread::Between(-Meio, Meio, BattleSpread::Fraction(Semente, Fluxo));
			const float Y = BattleSpread::Between(-Meio, Meio, BattleSpread::Fraction(Semente, Fluxo + 1));
			const float Porte = BattleSpread::Between(0.6f, 1.4f, BattleSpread::Fraction(Semente, Fluxo + 2));
			const float Giro = BattleSpread::Between(0.0f, 360.0f, BattleSpread::Fraction(Semente, Fluxo + 3));

			// Giro em torno de DOIS eixos: a esfera girada só no eixo vertical
			// continua igual a si mesma, e catorze dunas idênticas leem como
			// repetição de asset, não como areia.
			FTransform Pouso;
			Pouso.SetLocation(FVector(X, Y, TopoDoChao));
			Pouso.SetRotation(FQuat(FRotator(Giro * 0.05f, Giro, 0.0f)));
			Pouso.SetScale3D(FVector(
				Largura * Porte * 2.0f / CilindroDaEngineUnidades,
				Largura * Porte * 2.0f / CilindroDaEngineUnidades,
				Altura * Porte * 2.0f / CilindroDaEngineUnidades));
			Montes->AddInstance(Pouso);
		}
	}

	/**
	 * Onde, em espaço local do pedaço, cai um ponto do arco da costa — e se
	 * ele cai DENTRO deste pedaço.
	 *
	 * O ladrilho tem 6400 de lado e a praia tem 1600 de largura: nenhum pedaço
	 * de praia é só praia. Recortar pelo quadrado é o que impede a faixa de
	 * atravessar mata adentro num pedaço e de sumir no mar noutro.
	 */
	bool PontoDoArcoNoPedaco(const FVector2D& CentroDoPedaco, float Meio,
		float Raio, const FVector2D& Direcao, FVector2D& OutLocal)
	{
		OutLocal = Direcao * Raio - CentroDoPedaco;
		return FMath::Abs(OutLocal.X) <= Meio && FMath::Abs(OutLocal.Y) <= Meio;
	}

	/** Deita uma laje da orla, virada de frente para o mar. */
	void DeitarLajeDaOrla(UHierarchicalInstancedStaticMeshComponent* Laje,
		const FVector2D& Local, float Graus, float LarguraRadial, float AlturaLocal)
	{
		FTransform Pouso;
		Pouso.SetLocation(FVector(Local.X, Local.Y, AlturaLocal));
		Pouso.SetRotation(FQuat(FRotator(0.0f, Graus, 0.0f)));
		// X é RADIAL depois do giro, Y é o arco. Trocar os dois deixaria a
		// faixa com 300 de largura e 420 de comprimento: uma fileira de
		// tijolos soltos em vez de uma beira contínua.
		Pouso.SetScale3D(FVector(
			LarguraRadial / CilindroDaEngineUnidades,
			ComprimentoDaLajeDaOrla / CilindroDaEngineUnidades,
			EspessuraDaLajeDaOrla / CilindroDaEngineUnidades));
		Laje->AddInstance(Pouso);
	}

	/** Comprimento de cada laje do rio, medido AO LONGO da correnteza. */
	constexpr float PassoDoRio = 240.0f;

	/**
	 * Quanto cada laje do rio invade a seguinte.
	 *
	 * Sem sobra, duas lajes vizinhas em rumos diferentes deixam uma fresta em
	 * cada curva — e um rio pontilhado é pior que rio nenhum, porque parece
	 * defeito em vez de escolha.
	 */
	constexpr float SobreposicaoDoRio = 1.15f;

	/**
	 * A que altura a lâmina d'água corre sobre o chão.
	 *
	 * Acima da orla inteira de propósito: a areia molhada ocupa até quatro
	 * unidades acima do chão e a espuma até doze, e lâmina disputando altura
	 * com elas seria o rio piscando na foz. Dezesseis unidades num mundo em
	 * que um pet tem centenas é degrau que ninguém enxerga.
	 */
	constexpr float AlturaDaLaminaDoRio = 16.0f;
	constexpr float EspessuraDaLamina = 8.0f;

	/** A espuma da queda vai por cima da lâmina, e é mais grossa. */
	constexpr float AlturaDaEspumaDaQueda = 26.0f;
	constexpr float EspessuraDaEspumaDaQueda = 10.0f;

	/** A espuma transborda um pouco da calha: água caindo espirra. */
	constexpr float AlargamentoDaEspuma = 1.15f;

	/** A trilha é chão pisado, e por isso quase rente ao chão. */
	constexpr float AlturaDaTrilhaDoRio = 2.0f;
	constexpr float EspessuraDaTrilhaDoRio = 6.0f;

	/** Se este ponto do mundo cai dentro do ladrilho, e onde ele cai. */
	bool PontoNoPedaco(const FVector2D& Mundo, const FVector2D& CentroDoPedaco,
		float Meio, FVector2D& OutLocal)
	{
		OutLocal = Mundo - CentroDoPedaco;
		return FMath::Abs(OutLocal.X) <= Meio && FMath::Abs(OutLocal.Y) <= Meio;
	}

	/** Deita uma laje de curso d'água, virada na direção da correnteza. */
	void DeitarLajeDoRio(UHierarchicalInstancedStaticMeshComponent* Laje,
		const FVector2D& Local, float Graus, float Largura, float AlturaLocal,
		float Espessura)
	{
		FTransform Pouso;
		Pouso.SetLocation(FVector(Local.X, Local.Y, AlturaLocal));
		Pouso.SetRotation(FQuat(FRotator(0.0f, Graus, 0.0f)));
		// Depois do giro o X aponta rio abaixo e o Y atravessa a calha.
		Pouso.SetScale3D(FVector(
			PassoDoRio * SobreposicaoDoRio / CilindroDaEngineUnidades,
			Largura / CilindroDaEngineUnidades,
			Espessura / CilindroDaEngineUnidades));
		Laje->AddInstance(Pouso);
	}

	/**
	 * Monta os cursos d'água que passam por este pedaço.
	 *
	 * Mesmo desenho da orla, e pelo mesmo motivo: percorre o rio INTEIRO,
	 * da nascente à foz, e guarda só o que cai no ladrilho. Assim nenhum
	 * pedaço precisa saber que o vizinho existe, e o trecho de um encosta no
	 * do outro por construção.
	 *
	 * A laje é de quem tem o CENTRO dela dentro do quadrado. Aceitar também as
	 * que só encostam faria dois pedaços desenharem a mesma laje, e duas lajes
	 * no mesmo lugar brigam por pixel.
	 */
	void PlantarAguaDoce(UHierarchicalInstancedStaticMeshComponent* Lamina,
		UHierarchicalInstancedStaticMeshComponent* Queda,
		UHierarchicalInstancedStaticMeshComponent* Trilha,
		const FVector2D& CentroDoPedaco, float LadoDoPedaco)
	{
		if (LadoDoPedaco <= KINDA_SMALL_NUMBER)
		{
			return;
		}

		const float Meio = LadoDoPedaco * 0.5f;
		const float TopoDoChao = AForestBackdrop::GroundTopLocalZ();

		// A trilha morre onde a areia começa: caminho de terra sobre praia não
		// é caminho, é mancha — na areia se anda em qualquer direção.
		const float FimDaTrilha = IslandGeography::LandRadiusUnits()
			- IslandGeography::BeachWidthUnits();

		for (const FreshWater::FRiverCourse& Curso : FreshWater::Plan())
		{
			for (float Raio = Curso.SourceRadiusUnits; Raio <= Curso.MouthRadiusUnits;
				Raio += PassoDoRio)
			{
				const FVector2D Aqui = FreshWater::PointAt(Curso, Raio);
				const FVector2D Adiante = FreshWater::PointAt(Curso,
					FMath::Min(Raio + PassoDoRio, Curso.MouthRadiusUnits));

				const FVector2D Correnteza = (Adiante - Aqui).GetSafeNormal();
				if (Correnteza.IsNearlyZero())
				{
					continue;
				}

				const float Graus = FMath::RadiansToDegrees(
					FMath::Atan2(Correnteza.Y, Correnteza.X));
				const float MeiaLargura = FreshWater::HalfWidthAt(Curso, Raio);

				FVector2D Local;
				if (PontoNoPedaco(Aqui, CentroDoPedaco, Meio, Local))
				{
					DeitarLajeDoRio(Lamina, Local, Graus, MeiaLargura * 2.0f,
						TopoDoChao + AlturaDaLaminaDoRio, EspessuraDaLamina);

					if (FreshWater::IsFallAt(Curso, Raio))
					{
						DeitarLajeDoRio(Queda, Local, Graus,
							MeiaLargura * 2.0f * AlargamentoDaEspuma,
							TopoDoChao + AlturaDaEspumaDaQueda, EspessuraDaEspumaDaQueda);
					}
				}

				if (Raio >= FimDaTrilha)
				{
					continue;
				}

				// Sempre a MESMA margem. Trocando de lado no meio do caminho, a
				// trilha atravessaria o rio sem ponte — e quem a segue chegaria
				// à água esperando continuar andando.
				const FVector2D Margem(-Correnteza.Y, Correnteza.X);
				const FVector2D NaTrilha = Aqui + Margem
					* (MeiaLargura + FreshWater::TrailOffsetUnits());

				FVector2D LocalDaTrilha;
				if (PontoNoPedaco(NaTrilha, CentroDoPedaco, Meio, LocalDaTrilha))
				{
					DeitarLajeDoRio(Trilha, LocalDaTrilha, Graus,
						FreshWater::TrailHalfWidthUnits() * 2.0f,
						TopoDoChao + AlturaDaTrilhaDoRio, EspessuraDaTrilhaDoRio);
				}
			}
		}
	}

	/**
	 * Monta a ORLA deste pedaço: areia molhada, espuma e árvores de beira.
	 *
	 * Percorre o arco da ilha inteira e guarda só o que cai dentro do
	 * ladrilho. Varrer 20 mil unidades de raio para aproveitar um punhado de
	 * lajes parece desperdício, e é — quatrocentos cossenos por pedaço de
	 * praia. Em troca, não existe caso de borda: nenhum pedaço precisa saber
	 * em que quadrante está, nem por onde a costa entra e sai dele, e a laje
	 * do vizinho encosta na desta por construção.
	 */
	void PlantarOrla(UHierarchicalInstancedStaticMeshComponent* Molhada,
		UHierarchicalInstancedStaticMeshComponent* Espuma,
		UHierarchicalInstancedStaticMeshComponent* Beira,
		const FVector2D& CentroDoPedaco, float LadoDoPedaco, uint32 Semente)
	{
		const float RaioDaCosta = IslandGeography::LandRadiusUnits();
		if (RaioDaCosta <= KINDA_SMALL_NUMBER || LadoDoPedaco <= KINDA_SMALL_NUMBER)
		{
			return;
		}

		const float Meio = LadoDoPedaco * 0.5f;
		const float TopoDoChao = AForestBackdrop::GroundTopLocalZ();

		const float RaioDaFaixa = RaioDaCosta - LarguraDaFaixaMolhada * 0.5f;
		// A espuma fica do lado de FORA da areia: é ali que a onda quebra, e
		// espuma por dentro da faixa molhada seria espuma em terra seca.
		const float RaioDaEspuma = RaioDaCosta + LarguraDaEspuma * 0.5f;
		const float RaioDaBeira = RaioDaCosta - IslandGeography::BeachWidthUnits() * 0.55f;

		const float PassoEmRadianos = ComprimentoDaLajeDaOrla / RaioDaCosta;
		const int32 Passos = FMath::CeilToInt(2.0f * PI / PassoEmRadianos);

		UStaticMesh* MalhaDaBeira = Beira->GetStaticMesh();
		const float AlturaDaMalhaDaBeira = MalhaDaBeira
			? MalhaDaBeira->GetBoundingBox().GetSize().Z
			: 0.0f;
		const float EscalaDaBeira = (AlturaDaMalhaDaBeira > KINDA_SMALL_NUMBER)
			? AlturaDaArvoreDeBeira / AlturaDaMalhaDaBeira
			: 0.0f;

		for (int32 Passo = 0; Passo < Passos; ++Passo)
		{
			const float Angulo = static_cast<float>(Passo) * PassoEmRadianos;
			const FVector2D Direcao(FMath::Cos(Angulo), FMath::Sin(Angulo));
			const float Graus = FMath::RadiansToDegrees(Angulo);

			FVector2D Local;
			if (PontoDoArcoNoPedaco(CentroDoPedaco, Meio, RaioDaFaixa, Direcao, Local))
			{
				DeitarLajeDaOrla(Molhada, Local, Graus, LarguraDaFaixaMolhada, TopoDoChao);
			}
			if (PontoDoArcoNoPedaco(CentroDoPedaco, Meio, RaioDaEspuma, Direcao, Local))
			{
				// Um dedo acima da areia: espuma no MESMO plano da faixa
				// pisca contra ela, e o cintilar lê como defeito de vídeo.
				DeitarLajeDaOrla(Espuma, Local, Graus, LarguraDaEspuma,
					TopoDoChao + EspessuraDaLajeDaOrla);
			}

			if (Passo % PassosPorArvoreDeBeira != 0 || EscalaDaBeira <= 0.0f)
			{
				continue;
			}

			const int32 Fluxo = PrimeiroFluxoDaBeira + Passo * SorteiosPorArvoreDeBeira;
			const float Recuo = BattleSpread::Between(-RecuoDaArvoreDeBeira, RecuoDaArvoreDeBeira,
				BattleSpread::Fraction(Semente, Fluxo));
			const float Porte = BattleSpread::Between(0.78f, 1.24f,
				BattleSpread::Fraction(Semente, Fluxo + 1));
			const float Tombo = BattleSpread::Between(7.0f, 18.0f,
				BattleSpread::Fraction(Semente, Fluxo + 2));

			FVector2D LocalDaBeira;
			if (!PontoDoArcoNoPedaco(CentroDoPedaco, Meio, RaioDaBeira + Recuo, Direcao, LocalDaBeira))
			{
				continue;
			}

			// Tombo NEGATIVO no arfar: com o giro apontando o X para o mar, é
			// o sinal negativo que joga o topo do tronco para fora. Positivo
			// deitaria a árvore para dentro da mata, contra o vento que a
			// entortou.
			FTransform Pouso;
			Pouso.SetLocation(FVector(LocalDaBeira.X, LocalDaBeira.Y, TopoDoChao));
			Pouso.SetRotation(FQuat(FRotator(-Tombo, Graus, 0.0f)));
			Pouso.SetScale3D(FVector(EscalaDaBeira * Porte));
			Beira->AddInstance(Pouso);
		}
	}
}

AForestBackdrop::AForestBackdrop()
{
	using namespace MataDoCenario;

	PrimaryActorTick.bCanEverTick = false;

	ForestRoot = CreateDefaultSubobject<USceneComponent>(TEXT("ForestRoot"));
	SetRootComponent(ForestRoot);

	GroundMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ForestGround"));
	GroundMesh->SetupAttachment(ForestRoot);
	// Cenário não empurra ninguém: quem decide onde o pet está é o núcleo.
	// O CHÃO COLIDE, e isto não é detalhe: ele é o piso do mundo.
	//
	// Nasceu sem colisão porque a mata começou como cenário de FUNDO da arena,
	// onde ninguém anda. No mundo aberto ele é a ilha — e enquanto os 225
	// cubos do teste de streaming estiveram lá, eles seguravam o jogador e
	// esconderam isto. Ao removê-los, o jogador passou a cair pelo chão.
	//
	// A lição, e ela é geral: o que segurava o jogador não era o que parecia
	// segurá-lo. Andaime que sustenta produção não é andaime — é estrutura
	// que ninguém declarou.
	GroundMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GroundMesh->SetCollisionResponseToAllChannels(ECR_Block);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CilindroDoChao(CilindroDaEngine);
	if (CilindroDoChao.Succeeded())
	{
		GroundMesh->SetStaticMesh(CilindroDoChao.Object);
	}

	// O quadrado fica GUARDADO, não montado: quem é ladrilho troca a forma na
	// hora de montar. `ConstructorHelpers` só funciona aqui, então as duas
	// primitivas precisam ser achadas agora, mesmo que uma delas nunca sirva.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CuboDoChao(CuboDaEngine);
	if (CuboDoChao.Succeeded())
	{
		SquareGroundAsset = CuboDoChao.Object;
	}

	ReliefMounds = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(
		TEXT("ReliefMounds"));
	ReliefMounds->SetupAttachment(ForestRoot);
	// Sem colisão: o monte DESENHA o chão, não o fecha. O piso que segura o
	// jogador continua sendo um só — o ladrilho — e ter dois pisos com alturas
	// diferentes é como se ganha um jogador preso dentro de uma duna.
	ReliefMounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ReliefMounds->SetCastShadow(true);

	// A malha vai AQUI. Componente criado sem asset passa em todo teste de
	// lógica e não existe na tela — três vezes neste projeto.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> EsferaDoMonte(EsferaDaEngine);
	if (EsferaDoMonte.Succeeded())
	{
		ReliefMounds->SetStaticMesh(EsferaDoMonte.Object);
	}

	// As três peças da ORLA. Elas ficam vazias em todo bioma que não é praia,
	// e é `PlantarOrla` quem decide — o construtor só garante que existe malha
	// para o dia em que houver instância.
	ShoreWetSand = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(
		TEXT("ShoreWetSand"));
	ShoreWetSand->SetupAttachment(ForestRoot);
	// Sem colisão: a faixa é a MARCA da onda no chão, não um degrau. Fechá-la
	// faria o jogador esbarrar num muro invisível de oito unidades ao caminhar
	// para o mar.
	ShoreWetSand->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ShoreWetSand->SetCastShadow(false);

	ShoreFoam = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(
		TEXT("ShoreFoam"));
	ShoreFoam->SetupAttachment(ForestRoot);
	ShoreFoam->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ShoreFoam->SetCastShadow(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CuboDaOrla(CuboDaEngine);
	if (CuboDaOrla.Succeeded())
	{
		ShoreWetSand->SetStaticMesh(CuboDaOrla.Object);
		ShoreFoam->SetStaticMesh(CuboDaOrla.Object);
	}

	ShoreTrees = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(
		TEXT("ShoreTrees"));
	ShoreTrees->SetupAttachment(ForestRoot);
	// A árvore de beira FECHA, como qualquer tronco: ela é o que dá escala à
	// praia, e atravessá-la desmentiria o tamanho que ela acabou de sugerir.
	ShoreTrees->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	ShoreTrees->SetCastShadow(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> TroncoDaBeira(
		*FString::Printf(TEXT("%s%s.%s"), PastaDaMata, TEXT("tree_thin"), TEXT("tree_thin")));
	if (TroncoDaBeira.Succeeded())
	{
		ShoreTrees->SetStaticMesh(TroncoDaBeira.Object);
	}

	// As tres pecas da AGUA DOCE: a lamina do rio, a espuma da queda e a
	// trilha que acompanha a margem. Elas vao juntas porque sao o mesmo curso
	// visto de tres jeitos, e separa-las em atores diferentes seria abrir a
	// porta para a trilha ficar num pedaco em que o rio ja nao passa.
	RiverSurface = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(
		TEXT("RiverSurface"));
	RiverSurface->SetupAttachment(ForestRoot);
	// Sem colisão: a lâmina é a SUPERFÍCIE da água, e água não é degrau. Foi
	// exatamente o contrário disso que produziu "parte da agua ele afunda" —
	// ali o pet caía; aqui ele anda por cima, porque a lâmina fica acima do
	// chão e não tenta empurrá-lo.
	RiverSurface->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RiverSurface->SetCastShadow(false);

	RiverFallFoam = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(
		TEXT("RiverFallFoam"));
	RiverFallFoam->SetupAttachment(ForestRoot);
	RiverFallFoam->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RiverFallFoam->SetCastShadow(false);

	RiverTrail = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(
		TEXT("RiverTrail"));
	RiverTrail->SetupAttachment(ForestRoot);
	RiverTrail->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RiverTrail->SetCastShadow(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CuboDoRio(CuboDaEngine);
	if (CuboDoRio.Succeeded())
	{
		RiverSurface->SetStaticMesh(CuboDoRio.Object);
		RiverFallFoam->SetStaticMesh(CuboDoRio.Object);
		RiverTrail->SetStaticMesh(CuboDoRio.Object);
	}

	SpeciesClusters.Reset();
	SpeciesRoles.Reset();
	for (int32 Indice = 0; Indice < TotalDeEspecies; ++Indice)
	{
		const FEspecie& Especie = Especies[Indice];

		UHierarchicalInstancedStaticMeshComponent* Grupo =
			CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(
				FName(*FString::Printf(TEXT("Species_%s"), Especie.Nome)));
		Grupo->SetupAttachment(ForestRoot);
		// O QUE BLOQUEIA É O QUE SE DERRUBA. Uma regra só, e ela vem do mesmo
		// lugar: FWorldObstacleBreaking::StartingHealthFor. Árvore, pedra e
		// tronco têm corpo; capim, flor e arbusto não.
		//
		// Duas listas — uma de "o que colide" e outra de "o que cai" —
		// discordariam na primeira edição, e a discordância seria cruel de
		// qualquer lado: uma árvore que barra e não cai é uma parede sem
		// saída; uma que cai e não barra é um golpe sem motivo.
		const bool bTemCorpo = FWorldObstacleBreaking::StartingHealthFor(Especie.Papel) > 0;
		Grupo->SetCollisionEnabled(bTemCorpo
			? ECollisionEnabled::QueryAndPhysics
			: ECollisionEnabled::NoCollision);
		if (bTemCorpo)
		{
			Grupo->SetCollisionResponseToAllChannels(ECR_Block);
		}
		Grupo->SetCastShadow(true);

		// A malha é atribuída AQUI, no construtor. Componente criado sem
		// asset passa em todo teste de lógica e não existe na tela — foi
		// assim três vezes neste projeto (pets, inimigos do mundo, o
		// próprio jogador).
		const FString Caminho = FString::Printf(TEXT("%s%s.%s"), PastaDaMata, Especie.Nome, Especie.Nome);
		ConstructorHelpers::FObjectFinder<UStaticMesh> Malha(*Caminho);
		if (Malha.Succeeded())
		{
			Grupo->SetStaticMesh(Malha.Object);
		}

		SpeciesClusters.Add(Grupo);

		// O PAPEL viaja junto do agrupamento. Sem isto, saber se uma instância
		// é árvore ou capim exigiria reconsultar a tabela de espécies por
		// índice — uma segunda leitura da mesma lista, que discorda dela na
		// primeira reordenação.
		SpeciesRoles.Add(Especie.Papel);
	}
}

TArray<FString> AForestBackdrop::SpeciesNames()
{
	using namespace MataDoCenario;

	TArray<FString> Nomes;
	Nomes.Reserve(TotalDeEspecies);
	for (int32 Indice = 0; Indice < TotalDeEspecies; ++Indice)
	{
		Nomes.Add(Especies[Indice].Nome);
	}

	return Nomes;
}

void AForestBackdrop::BuildForest(float CellSize, uint32 Seed, const FVector2D& CameraGroundOffset,
	EIslandBiome Biome)
{
	using namespace MataDoCenario;

	if (CellSize <= 0.0f)
	{
		return;
	}

	// A MESMA tabela que filtra o pedaço do mundo. Uma segunda, escrita para
	// a arena, concordaria com esta até a primeira edição — e o sintoma seria
	// uma geleira com capim numa tela e sem capim na outra (L-032/L-033).
	const FPresencaDoBioma Presenca = PresencaDe(Biome);

	// O chão do disco pela cor do bioma, pelo caminho que `ApplyGroundMaterial`
	// já tinha: `Count` continua querendo dizer "a cor de chão de sempre", e é
	// o que a mata devolve. Material emprestado do mundo ainda vence os dois.
	RegionGroundRole = Presenca.PapelDoChao;

	const float RaioDoChao = GroundRadiusInCells * CellSize;
	GroundMesh->SetRelativeScale3D(FVector(
		RaioDoChao * 2.0f / CilindroDaEngineUnidades,
		RaioDoChao * 2.0f / CilindroDaEngineUnidades,
		ProfundidadeDoChao / CilindroDaEngineUnidades));

	// Descido pela metade do que cresceu, para o TOPO ficar onde estava:
	// GroundTopLocalZ() continua valendo, e quem posiciona a mata contra uma
	// superfície não precisa saber que ela engrossou.
	GroundMesh->SetRelativeLocation(FVector(0.0f, 0.0f,
		EspessuraDoChao * 0.5f - ProfundidadeDoChao * 0.5f));

	// A mata não tem duna. Limpar é obrigatório e não é zelo: o mesmo ator
	// vira pedaço e volta a ser mata, e o monte que sobrasse ficaria de pé no
	// meio da clareira sem nada no mundo explicando de onde veio.
	if (ReliefMounds)
	{
		ReliefMounds->ClearInstances();
	}

	// Pelo mesmo motivo, a mata não tem beira de mar. Limpar aqui é o que
	// impede uma laje de espuma de continuar de pé no meio de uma clareira.
	LimparOrla();

	// Nem rio: a arena é um recorte de mata, não um trecho da ilha, e não tem
	// raio nenhum em relação ao centro do mundo para um curso d'água seguir.
	LimparAguaDoce();

	ApplyGroundMaterial();

	const float FolgaDoTabuleiro = BoardClearanceInCells * CellSize;
	const float FolgaDaCamera = CameraClearanceInCells * CellSize;

	for (int32 Indice = 0; Indice < TotalDeEspecies && Indice < SpeciesClusters.Num(); ++Indice)
	{
		const FEspecie& Especie = Especies[Indice];
		UHierarchicalInstancedStaticMeshComponent* Grupo = SpeciesClusters[Indice];

		// Limpar ANTES de decidir se a espécie entra: a mesma arena pode
		// vestir outro bioma na batalha seguinte, e o pinheiro que sobrasse
		// da mata ficaria de pé no meio da duna.
		Grupo->ClearInstances();

		UStaticMesh* Malha = Grupo->GetStaticMesh();
		if (!Malha)
		{
			continue;
		}

		// O ELENCO antes da proporção: espécie que não é deste bioma não
		// nasce, qualquer que seja o percentual do papel dela. Sem isto,
		// geleira e floresta plantavam a MESMA árvore — só em quantidades
		// diferentes —, e proporção diferente sobre elenco igual não faz
		// lugar diferente.
		if (!BiomeFlora::FitsBiome(Especie.Nome, Biome))
		{
			continue;
		}

		const int32 Percentual = PercentualDoPapel(Presenca, Especie.Papel);
		if (Percentual <= 0)
		{
			continue;
		}

		// Sem fator de área, ao contrário do pedaço do mundo: as quantidades
		// da tabela foram escolhidas para ESTE disco, e é ele que está aqui.
		const int32 Quantidade = FMath::RoundToInt(
			static_cast<float>(Especie.Quantidade) * (static_cast<float>(Percentual) / 100.0f));
		if (Quantidade <= 0)
		{
			continue;
		}

		// A escala sai da caixa MEDIDA: pedir "3 casas de altura" e dividir
		// pela altura real da malha é o que mantém o conjunto coerente
		// quando as espécies vêm em tamanhos diferentes.
		const float AlturaDaMalha = Malha->GetBoundingBox().GetSize().Z;
		if (AlturaDaMalha <= KINDA_SMALL_NUMBER)
		{
			continue;
		}
		const float EscalaBase = (Especie.AlturaEmCasas * CellSize) / AlturaDaMalha;

		// A cor vem da NOSSA paleta, não do pacote. Os dez materiais do kit
		// são variações de uma faixa estreita: trocar um pelo outro moveria o
		// problema em vez de resolvê-lo. Aqui capim, arbusto, árvore, dossel
		// e pedra recebem degraus de BRILHO distintos, que é o que se lê de
		// longe.
		//
		// A pedra sai da cor do BIOMA, e não da sua: pedra da cor do chão é
		// pedra invisível, e pedra é justamente o que sobra onde não há mata.
		const EScenaryRole PapelPintado = (Especie.Papel == EScenaryRole::Rock)
			? Presenca.PapelDaPedra
			: Especie.Papel;
		ScenaryPalette::PaintComponent(Grupo, PapelPintado);

		const uint32 SementeDaEspecie = BattleSpread::Scatter(Seed ^ BattleSpread::SeedFromText(Especie.Nome));

		for (int32 Planta = 0; Planta < Quantidade; ++Planta)
		{
			for (int32 Tentativa = 0; Tentativa < TentativasPorPlanta; ++Tentativa)
			{
				const float Angulo = BattleSpread::Fraction(
					SementeDaEspecie, FluxoDaPlanta(Planta, Tentativa, ESorteio::Angulo)) * 2.0f * PI;

				// Raio pela raiz: sem ela a densidade se acumula no centro da
				// coroa e a mata fica com um anel visível.
				const float Sorteio = BattleSpread::Fraction(
					SementeDaEspecie, FluxoDaPlanta(Planta, Tentativa, ESorteio::Raio));
				const float RaioMinimo = Especie.Faixa.RaioMinimoEmCasas * CellSize;
				const float RaioMaximo = Especie.Faixa.RaioMaximoEmCasas * CellSize;
				const float Raio = FMath::Sqrt(BattleSpread::Between(
					RaioMinimo * RaioMinimo, RaioMaximo * RaioMaximo, Sorteio));

				const FVector2D Posicao(Raio * FMath::Cos(Angulo), Raio * FMath::Sin(Angulo));

				if (Posicao.Size() < FolgaDoTabuleiro)
				{
					continue;
				}
				if (FVector2D::Distance(Posicao, CameraGroundOffset) < FolgaDaCamera)
				{
					continue;
				}

				Grupo->AddInstance(PousoDaPlanta(SementeDaEspecie, Planta, Tentativa, Posicao, EscalaBase));
				break;
			}
		}
	}
}

void AForestBackdrop::BuildRegion(float CellSize, uint32 Seed, EIslandBiome Biome, float SideUnits)
{
	using namespace MataDoCenario;

	if (CellSize <= 0.0f || SideUnits <= 0.0f)
	{
		return;
	}

	const FPresencaDoBioma Presenca = PresencaDe(Biome);
	RegionGroundRole = Presenca.PapelDoChao;

	if (SquareGroundAsset)
	{
		GroundMesh->SetStaticMesh(SquareGroundAsset);
	}

	const float LadoDoChao = RegionGroundSideUnits(SideUnits);
	GroundMesh->SetRelativeScale3D(FVector(
		LadoDoChao / CilindroDaEngineUnidades,
		LadoDoChao / CilindroDaEngineUnidades,
		ProfundidadeDoChao / CilindroDaEngineUnidades));

	// O mesmo abaixamento do disco: o TOPO fica onde `GroundTopLocalZ()` diz,
	// e quem encosta coisa no chão não precisa saber de que forma ele é.
	GroundMesh->SetRelativeLocation(FVector(0.0f, 0.0f,
		EspessuraDoChao * 0.5f - ProfundidadeDoChao * 0.5f));

	ApplyGroundMaterial();

	// O relevo é do CHÃO, e por isso ganha a cor do chão. Monte de outra cor
	// não seria duna: seria pedra grande, que este bioma já tem de sobra.
	PlantarMontes(ReliefMounds, Presenca, CellSize, Seed, SideUnits);
	if (Presenca.PapelDoChao != EScenaryRole::Count)
	{
		ScenaryPalette::PaintComponent(ReliefMounds, Presenca.PapelDoChao);
	}

	// A ORLA é a única parte deste ator que depende de ONDE ele está, e não só
	// do bioma que lhe mandaram: a linha d'água é um arco de raio conhecido em
	// volta da ilha. Sem a posição, a faixa cairia no meio da areia num pedaço
	// e dentro do mar no vizinho.
	LimparOrla();
	if (Biome == EIslandBiome::Beach)
	{
		const FVector Aqui = GetActorLocation();
		PlantarOrla(ShoreWetSand, ShoreFoam, ShoreTrees,
			FVector2D(Aqui.X, Aqui.Y), SideUnits, Seed);

		ScenaryPalette::PaintComponent(ShoreWetSand, EScenaryRole::WetSand);
		ScenaryPalette::PaintComponent(ShoreFoam, EScenaryRole::WaterFoam);
		// Árvore de praia continua sendo árvore: pintá-la de outra cor abriria
		// um papel novo para dizer exatamente o que `CanopyTree` já diz.
		ScenaryPalette::PaintComponent(ShoreTrees, EScenaryRole::CanopyTree);
	}

	// A ÁGUA DOCE não pergunta o bioma, e é essa a diferença dela para a orla:
	// a orla só existe onde a areia está, mas o rio ATRAVESSA — nasce na saia
	// de um monte, corta mata, campo e areia, e só acaba no mar. Condicioná-lo
	// ao bioma o cortaria em pedaços na primeira fronteira que ele cruzasse.
	LimparAguaDoce();
	{
		const FVector Aqui = GetActorLocation();
		PlantarAguaDoce(RiverSurface, RiverFallFoam, RiverTrail,
			FVector2D(Aqui.X, Aqui.Y), SideUnits);

		ScenaryPalette::PaintComponent(RiverSurface, EScenaryRole::FreshWater);
		ScenaryPalette::PaintComponent(RiverFallFoam, EScenaryRole::WaterFoam);
		// A trilha de beira é a MESMA coisa que a trilha da montanha: chão
		// pisado. Cor própria diria ao jogador que são dois tipos de caminho.
		ScenaryPalette::PaintComponent(RiverTrail, EScenaryRole::MountainTrail);
	}

	// A vida dos obstáculos é POSICIONAL: a chave aponta para um agrupamento e
	// uma instância, não para uma árvore. Herdar o mapa do pedaço anterior
	// faria uma árvore nascer meio quebrada por causa de outra, longe dali,
	// que alguém derrubou antes.
	ObstacleHealthByHandle.Reset();

	// A densidade acompanha a ÁREA. As quantidades da tabela foram escolhidas
	// para o disco da mata; repetir a contagem num ladrilho de outro tamanho
	// deixaria um pedaço entupido e o vizinho pelado, sem que nada no mundo
	// explicasse a diferença.
	const float RaioDaTabela = GroundRadiusInCells * CellSize;
	const float AreaDaTabela = PI * RaioDaTabela * RaioDaTabela;
	const float FatorDeArea = (AreaDaTabela > KINDA_SMALL_NUMBER)
		? (SideUnits * SideUnits) / AreaDaTabela
		: 1.0f;

	const float Meio = SideUnits * 0.5f;

	for (int32 Indice = 0; Indice < TotalDeEspecies && Indice < SpeciesClusters.Num(); ++Indice)
	{
		const FEspecie& Especie = Especies[Indice];
		UHierarchicalInstancedStaticMeshComponent* Grupo = SpeciesClusters[Indice];

		// Limpar ANTES de decidir se a espécie entra: o mesmo ator é
		// reaproveitado ao mudar de bioma, e o pinheiro que sobrasse da mata
		// ficaria de pé no meio da duna.
		Grupo->ClearInstances();

		UStaticMesh* Malha = Grupo->GetStaticMesh();
		if (!Malha)
		{
			continue;
		}

		// Mesmo elenco, mesma ordem: o pedaço do mundo e a mata da arena
		// plantam pela MESMA lista, senão o lugar da briga não parece o lugar
		// de onde ela veio.
		if (!BiomeFlora::FitsBiome(Especie.Nome, Biome))
		{
			continue;
		}

		const int32 Percentual = PercentualDoPapel(Presenca, Especie.Papel);
		if (Percentual <= 0)
		{
			continue;
		}

		const int32 Quantidade = FMath::RoundToInt(
			static_cast<float>(Especie.Quantidade) * (static_cast<float>(Percentual) / 100.0f) * FatorDeArea);
		if (Quantidade <= 0)
		{
			continue;
		}

		const float AlturaDaMalha = Malha->GetBoundingBox().GetSize().Z;
		if (AlturaDaMalha <= KINDA_SMALL_NUMBER)
		{
			continue;
		}
		const float EscalaBase = (Especie.AlturaEmCasas * CellSize) / AlturaDaMalha;

		// Pedra da COR DO BIOMA. Pedra da cor do chão é pedra invisível — e
		// pedra é justamente o que sobra onde não há mata.
		const EScenaryRole PapelPintado = (Especie.Papel == EScenaryRole::Rock)
			? Presenca.PapelDaPedra
			: Especie.Papel;
		ScenaryPalette::PaintComponent(Grupo, PapelPintado);

		const uint32 SementeDaEspecie = BattleSpread::Scatter(Seed ^ BattleSpread::SeedFromText(Especie.Nome));

		for (int32 Planta = 0; Planta < Quantidade; ++Planta)
		{
			// Uniforme no QUADRADO, e sem tentativa nenhuma: aqui não há
			// tabuleiro nem lente a evitar, e a raiz que espalha o disco
			// deixaria o ladrilho vazio nas quinas.
			const FVector2D Posicao(
				BattleSpread::Between(-Meio, Meio, BattleSpread::Fraction(
					SementeDaEspecie, FluxoDaPlanta(Planta, 0, ESorteio::Angulo))),
				BattleSpread::Between(-Meio, Meio, BattleSpread::Fraction(
					SementeDaEspecie, FluxoDaPlanta(Planta, 0, ESorteio::Raio))));

			Grupo->AddInstance(PousoDaPlanta(SementeDaEspecie, Planta, 0, Posicao, EscalaBase));
		}
	}
}

int32 AForestBackdrop::GetPlantedCount() const
{
	int32 Total = 0;
	for (const TObjectPtr<UHierarchicalInstancedStaticMeshComponent>& Grupo : SpeciesClusters)
	{
		if (Grupo)
		{
			Total += Grupo->GetInstanceCount();
		}
	}
	return Total;
}

TArray<FVector> AForestBackdrop::GetPlantedLocations() const
{
	TArray<FVector> Posicoes;
	for (const TObjectPtr<UHierarchicalInstancedStaticMeshComponent>& Grupo : SpeciesClusters)
	{
		if (!Grupo)
		{
			continue;
		}
		const int32 Quantas = Grupo->GetInstanceCount();
		for (int32 Instancia = 0; Instancia < Quantas; ++Instancia)
		{
			FTransform Onde;
			if (Grupo->GetInstanceTransform(Instancia, Onde, /*bWorldSpace=*/false))
			{
				Posicoes.Add(Onde.GetLocation());
			}
		}
	}
	return Posicoes;
}

float AForestBackdrop::RegionGroundSideUnits(float SideUnits)
{
	return SideUnits * MataDoCenario::TransbordoDoChao;
}

UHierarchicalInstancedStaticMeshComponent* AForestBackdrop::GetReliefMounds() const
{
	return ReliefMounds;
}

UHierarchicalInstancedStaticMeshComponent* AForestBackdrop::GetShoreWetSand() const
{
	return ShoreWetSand;
}

UHierarchicalInstancedStaticMeshComponent* AForestBackdrop::GetShoreFoam() const
{
	return ShoreFoam;
}

UHierarchicalInstancedStaticMeshComponent* AForestBackdrop::GetShoreTrees() const
{
	return ShoreTrees;
}

UHierarchicalInstancedStaticMeshComponent* AForestBackdrop::GetRiverSurface() const
{
	return RiverSurface;
}

UHierarchicalInstancedStaticMeshComponent* AForestBackdrop::GetRiverFallFoam() const
{
	return RiverFallFoam;
}

UHierarchicalInstancedStaticMeshComponent* AForestBackdrop::GetRiverTrail() const
{
	return RiverTrail;
}

void AForestBackdrop::LimparAguaDoce()
{
	if (RiverSurface)  { RiverSurface->ClearInstances(); }
	if (RiverFallFoam) { RiverFallFoam->ClearInstances(); }
	if (RiverTrail)    { RiverTrail->ClearInstances(); }
}

void AForestBackdrop::LimparOrla()
{
	// As três juntas, sempre. Limpar duas e esquecer a terceira deixaria a
	// espuma boiando sozinha num deserto, que é pior que não ter praia.
	if (ShoreWetSand) { ShoreWetSand->ClearInstances(); }
	if (ShoreFoam)    { ShoreFoam->ClearInstances(); }
	if (ShoreTrees)   { ShoreTrees->ClearInstances(); }
}

float AForestBackdrop::GroundTopLocalZ()
{
	return MataDoCenario::EspessuraDoChao * 0.5f;
}

void AForestBackdrop::SetGroundMaterialOverride(UMaterialInterface* Material)
{
	AdoptedGroundMaterial = Material;

	// Aplicar JÁ, e não só na próxima construção: a arena empresta o chão
	// depois de sondar o mundo, que é depois de a mata estar plantada.
	ApplyGroundMaterial();
}

void AForestBackdrop::ApplyGroundMaterial()
{
	if (!GroundMesh)
	{
		return;
	}

	// O que o mundo emprestou vem primeiro: a batalha que nasce de um
	// encontro deve acontecer no chão daquele lugar, não numa paleta à parte.
	if (UMaterialInterface* Emprestado = AdoptedGroundMaterial.Get())
	{
		GroundMesh->SetMaterial(0, Emprestado);
		return;
	}

	// Sem empréstimo, o chão sai da NOSSA paleta.
	//
	// Ele já foi vestido com `leafsGreen` — o mesmo material das folhas das
	// árvores. Chão e copa eram literalmente uma cor só, e o quadro inteiro
	// lia como uma mancha verde-água sem relevo. O chão agora é o degrau
	// mais escuro da escala, justamente para o resto se destacar contra ele.
	if (UMaterialInterface* Base = ScenaryPalette::ColorableBaseMaterial())
	{
		if (UMaterialInstanceDynamic* Tinta = GroundMesh->CreateDynamicMaterialInstance(0, Base))
		{
			// Pedaço do mundo tem a cor do SEU bioma; a mata da arena tem a
			// cor de chão de sempre. `Count` é a ausência de bioma, não um.
			const FLinearColor Cor = (RegionGroundRole == EScenaryRole::Count)
				? ScenaryPalette::GroundColor()
				: ScenaryPalette::ColorFor(RegionGroundRole, NAME_None);
			Tinta->SetVectorParameterValue(TEXT("Color"), Cor);
		}
	}
}

namespace
{
	/**
	 * A chave opaca junta AGRUPAMENTO e INSTÂNCIA num inteiro.
	 *
	 * Mil instâncias por agrupamento é folga larga sobre as poucas dezenas que
	 * existem, e um par empacotado evita expor a estrutura da mata a quem só
	 * quer bater numa árvore.
	 */
	constexpr int32 InstanciasPorAgrupamento = 1000;

	int32 EmpacotarChave(int32 Agrupamento, int32 Instancia)
	{
		return Agrupamento * InstanciasPorAgrupamento + Instancia;
	}

	void DesempacotarChave(int32 Chave, int32& Agrupamento, int32& Instancia)
	{
		Agrupamento = Chave / InstanciasPorAgrupamento;
		Instancia = Chave % InstanciasPorAgrupamento;
	}
}

TArray<FWorldObstacleCandidate> AForestBackdrop::CollectObstaclesNear(
	const FVector& WorldLocation, float RadiusUnits, TArray<int32>& OutHandles) const
{
	TArray<FWorldObstacleCandidate> Candidatos;
	OutHandles.Reset();

	const float RaioAoQuadrado = RadiusUnits * RadiusUnits;

	for (int32 Agrupamento = 0; Agrupamento < SpeciesClusters.Num(); ++Agrupamento)
	{
		const UHierarchicalInstancedStaticMeshComponent* Grupo = SpeciesClusters[Agrupamento];
		if (!Grupo || !SpeciesRoles.IsValidIndex(Agrupamento))
		{
			continue;
		}

		const EScenaryRole Papel = SpeciesRoles[Agrupamento];
		const int32 VidaCheia = FWorldObstacleBreaking::StartingHealthFor(Papel);
		if (VidaCheia <= 0)
		{
			// Capim e flor não são obstáculo. Devolvê-los faria o golpe
			// escolher grama por ela estar mais perto que a árvore.
			continue;
		}

		for (int32 Instancia = 0; Instancia < Grupo->GetInstanceCount(); ++Instancia)
		{
			FTransform Onde;
			if (!Grupo->GetInstanceTransform(Instancia, Onde, /*bWorldSpace=*/true))
			{
				continue;
			}

			if (FVector::DistSquared2D(Onde.GetLocation(), WorldLocation) > RaioAoQuadrado)
			{
				continue;
			}

			const int32 Chave = EmpacotarChave(Agrupamento, Instancia);

			FWorldObstacleCandidate Candidato;
			Candidato.Location = Onde.GetLocation();
			Candidato.Role = Papel;
			// Ausente do mapa significa INTEIRO: só o que apanhou é guardado.
			Candidato.RemainingHealth = ObstacleHealthByHandle.Contains(Chave)
				? ObstacleHealthByHandle[Chave]
				: VidaCheia;

			Candidatos.Add(Candidato);
			OutHandles.Add(Chave);
		}
	}

	return Candidatos;
}

bool AForestBackdrop::DamageObstacle(int32 Handle, int32 Damage)
{
	int32 Agrupamento = 0;
	int32 Instancia = 0;
	DesempacotarChave(Handle, Agrupamento, Instancia);

	if (!SpeciesClusters.IsValidIndex(Agrupamento) || !SpeciesRoles.IsValidIndex(Agrupamento))
	{
		return false;
	}

	UHierarchicalInstancedStaticMeshComponent* Grupo = SpeciesClusters[Agrupamento];
	if (!Grupo || Instancia >= Grupo->GetInstanceCount())
	{
		return false;
	}

	const int32 VidaCheia = FWorldObstacleBreaking::StartingHealthFor(SpeciesRoles[Agrupamento]);
	const int32 Antes = ObstacleHealthByHandle.Contains(Handle)
		? ObstacleHealthByHandle[Handle]
		: VidaCheia;

	const int32 Depois = Antes - FMath::Max(0, Damage);
	ObstacleHealthByHandle.Add(Handle, Depois);

	if (Depois > 0)
	{
		return false;
	}

	// ENCOLHE em vez de remover: RemoveInstance desloca o índice de todas as
	// instâncias seguintes, e as chaves já entregues passariam a apontar para
	// outra árvore. Um obstáculo derrubado que vira o vizinho é pior que um
	// que não cai.
	FTransform Onde;
	if (Grupo->GetInstanceTransform(Instancia, Onde, /*bWorldSpace=*/false))
	{
		Onde.SetScale3D(FVector::ZeroVector);
		Grupo->UpdateInstanceTransform(Instancia, Onde, /*bWorldSpace=*/false,
			/*bMarkRenderStateDirty=*/true, /*bTeleport=*/true);
	}

	return true;
}

bool AForestBackdrop::IsSolidSpecies(int32 SpeciesIndex) const
{
	return SpeciesRoles.IsValidIndex(SpeciesIndex)
		&& FWorldObstacleBreaking::StartingHealthFor(SpeciesRoles[SpeciesIndex]) > 0;
}

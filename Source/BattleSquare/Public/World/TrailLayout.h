// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "World/RegionLayout.h"

/**
 * Onde uma trilha termina.
 *
 * Nem toda trilha vai de vila a vila. Quem anda vai ver a cachoeira e subir o
 * monte — e uma cachoeira sem caminho é a cachoeira que o relato de jogo disse
 * nunca ter visto.
 *
 * E é isto que faz a PONTE existir: os rios correm do monte para o mar, e
 * enquanto todo destino ficava ENTRE eles, nenhuma trilha precisava atravessar
 * um. Um caminho até a queda tem de chegar na margem.
 */
enum class ETrailDestination : uint8
{
	Assentamento,
	Cachoeira,
	Monte
};

/** Uma trilha ligando dois lugares. */
struct BATTLESQUARE_API FTrailRoute
{
	ESettlementKind From = ESettlementKind::VilaInicial;
	ESettlementKind To = ESettlementKind::VilaInicial;

	ETrailDestination Destination = ETrailDestination::Assentamento;

	/** O caminho, ponto a ponto. Nunca reto: o terreno é que o entorta. */
	TArray<FVector2D> PointsUnits;

	/**
	 * O traçado FALHOU e isto é uma linha reta de emergência.
	 *
	 * Existe porque o silêncio custou caro: seis trilhas saíram com sinuosidade
	 * exatamente 1,000 e eu só descobri medindo. Um caminho reto e um caminho
	 * que não existe pareciam a mesma coisa no mapa.
	 *
	 * Quando isto é verdade, alguma coisa está fora do alcance do traçado — e
	 * o certo é consertar o alcance ou o destino, nunca aceitar a reta.
	 */
	bool bFellBackToStraightLine = false;
};

/**
 * As trilhas — CALCULADAS, não desenhadas.
 *
 * A regra vem da spec da montaria: **a trilha vai pelo caminho barato**. Ela
 * não é uma curva bonita que alguém traçou; ela é o caminho de menor custo
 * sobre o relevo, e curva porque o TERRENO a curva. Contornar o morro é o
 * resultado, não a decisão.
 *
 * Por isso ela usa `IslandGeography::TravelCostBetween`, a mesma conta que
 * cobra o cansaço de quem anda. Se o traçador usasse outra, a trilha passaria
 * pelo caminho que ela mesma diz ser caro — e ninguém entenderia por quê.
 *
 * Trilha, e não estrada: uma faixa sem árvore com o chão de outra cor. Asfalto
 * e meio-fio implicariam uma civilização que este mundo não tem.
 */
namespace TrailLayout
{
	/** Todas as trilhas da região. */
	BATTLESQUARE_API const TArray<FTrailRoute>& Plan();

	/** Meia largura da faixa limpa. Fração do lote, nunca metros à mão. */
	BATTLESQUARE_API float HalfWidthUnits();

	/** O ponto está em cima de alguma trilha. */
	BATTLESQUARE_API bool IsOnTrail(const FVector2D& PositionUnits);

	/**
	 * Onde uma trilha cruza um rio — é ali que a PONTE fica.
	 *
	 * A ponte não é enfeite: sem ela a trilha entra na água, e um caminho que
	 * afunda é pior que caminho nenhum, porque promete passagem.
	 */
	/**
	 * O que a trilha faz onde ela encontra água.
	 *
	 * Não é sempre ponte, e essa era a distorção: com água em toda parte, a
	 * região ganhou 483 pontes. Quem cruza um córrego de três metros molha o
	 * pé; ponte ali é obra sem motivo, e denuncia que ninguém pensou na
	 * travessia.
	 */
	/**
	 * DE QUE a ponte é feita, e se ela ainda serve.
	 *
	 * É propriedade da travessia, e NÃO um tipo novo de `ECrossingKind`. Um
	 * `PonteDeMadeira` ao lado de `Ponte` obrigaria todo `switch` do mundo a
	 * tratar os dois — e o terceiro que alguém acrescentasse cairia no
	 * `default` de metade deles, que é o defeito que a cor dos prédios já
	 * custou uma vez.
	 *
	 * A ponte continua sendo `ECrossingKind::Ponte`; isto diz COMO ela é.
	 */
	enum class EBridgeMaterial : uint8
	{
		/** Não é ponte. É o que toda travessia que não é ponte carrega. */
		Nenhum,

		/**
		 * BLOCO: pedra encaixada, o vão curto que a rocha permite.
		 *
		 * É a que dura. Onde há pedra à mão e o vão é curto, ninguém corta
		 * árvore.
		 */
		Bloco,

		/**
		 * MADEIRA: o vão longo, onde carregar pedra não compensa.
		 *
		 * Mais barata e mais frágil — e é ela que a água leva.
		 */
		Madeira,

		/**
		 * DESTRUÍDA: a ponte que EXISTE E NÃO SERVE.
		 *
		 * Não é ausência de ponte, e a diferença é o conteúdo: ausência é um
		 * rio que ninguém atravessou; ruína é a promessa de um caminho que
		 * alguém já teve. Quem chega vê que houve passagem ali, e que hoje não
		 * há — e isso conta uma história que o vazio não conta.
		 *
		 * ⚠️ Ela NÃO deixa passar. Ponte destruída que se atravessa é
		 * decoração, e decoração não muda mapa.
		 */
		Destruida
	};

	/**
	 * O nome do material, para o painel e para o despejo.
	 *
	 * Mora aqui, ao lado do enum, e não em quem imprime: a tabela vivia dentro
	 * do teste de despejo, e uma segunda cópia dela no painel concordaria com a
	 * primeira até alguém acrescentar um quarto material.
	 */
	BATTLESQUARE_API const TCHAR* MaterialDebugName(EBridgeMaterial Material);

	enum class ECrossingKind : uint8
	{
		/** VAU: raso o bastante para passar andando. Não constrói nada. */
		Vau,

		/** PONTE: fundo demais para passar, e a margem permite apoiar. */
		Ponte,

		/**
		 * BARRANCO: a margem é alta, e a travessia é um corte na terra.
		 *
		 * Ele existe porque ponte precisa de duas margens no mesmo nível. Onde
		 * uma delas é um degrau, o que se faz é cavar a descida — e isso é uma
		 * coisa diferente de construir, com outra silhueta.
		 */
		Barranco,

		/**
		 * BALSA: a água é larga demais para ponte, e funda o bastante para
		 * barco grande.
		 *
		 * Ela fecha a regra pelo alto. Ponte tem um vão máximo — acima dele a
		 * obra vira outra coisa, e o que se faz num rio largo é atravessar
		 * flutuando. E isso dá função ao rio como CAMINHO, não só obstáculo:
		 * onde passa balsa, passa barco.
		 */
		Balsa
	};

	struct BATTLESQUARE_API FCrossing
	{
		FVector2D CenterUnits = FVector2D::ZeroVector;
		ECrossingKind Kind = ECrossingKind::Vau;

		/** A fundura estimada da água ali, que é o que decide. */
		float DepthUnits = 0.0f;

		/**
		 * De que a ponte é feita. `Nenhum` em tudo que não é ponte.
		 *
		 * Fica ao lado do tipo, e não dentro dele: ver a nota em
		 * `EBridgeMaterial`.
		 */
		EBridgeMaterial Material = EBridgeMaterial::Nenhum;

		/** Dá para passar por aqui? Falso só na ponte destruída. */
		bool CanBeCrossed() const
		{
			return Material != EBridgeMaterial::Destruida;
		}
	};

	/** Onde cada trilha encontra água, e o que se faz ali. */
	BATTLESQUARE_API TArray<FCrossing> Crossings();

	/** Só as travessias que viram ponte — o que o mundo constrói. */
	BATTLESQUARE_API TArray<FVector2D> BridgePoints();

	/** A fundura a partir da qual não se passa a pé. */
	BATTLESQUARE_API float WadableDepthUnits();

	/**
	 * A sinuosidade mínima de uma trilha, abaixo da qual ela é reta demais.
	 *
	 * Caminho pisado por gente não sai reto nem em campo aberto: desvia-se de
	 * uma pedra, de uma poça, do sol. Trilha reta num mapa é sempre obra — e
	 * este mundo não tem obra.
	 */
	BATTLESQUARE_API float MinimumSinuosity();

	/**
	 * E o TETO, que o ziguezague precisa tanto quanto o piso.
	 *
	 * Trilha de serra real anda duas a quatro vezes a linha reta. Oito é
	 * caricatura: vira escada de caracol, e quem sobe desiste antes.
	 */
	BATTLESQUARE_API float MaximumSinuosity();

	/**
	 * Se linha reta é aceitável. Configurável em `DefaultGame.ini`, chave
	 * `WorldAllowStraightTrails`.
	 *
	 * Existe porque às vezes a reta é o certo — uma passarela, um trecho curto
	 * entre duas praças, uma estrada de verdade quando o mundo tiver uma. O
	 * padrão é NÃO, porque o padrão deste mundo é caminho pisado.
	 */
	BATTLESQUARE_API bool AllowsStraightTrails();

	/** O tamanho do passo do traçado — também a distância entre pontos. */
	BATTLESQUARE_API float StepUnits();
}

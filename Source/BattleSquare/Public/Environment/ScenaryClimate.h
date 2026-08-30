// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * O clima do lugar onde a cena se passa.
 *
 * Existe por causa do gelo da serra: "montanha alta tem neve" é meia regra, e
 * a metade que falta é o clima do pé dela. A mesma altitude que congela numa
 * mata temperada é rocha nua no deserto — a temperatura cai com a altura, mas
 * ela cai A PARTIR da temperatura de baixo, e é essa que muda de bioma para
 * bioma.
 */
enum class EScenaryClimate : uint8
{
	/** A mata: o clima que a arena e o mundo aberto usam hoje. */
	Temperate,
	/** Serra fria: congela quase desde o pé. */
	Cold,
	/** Clima bom, ameno o ano inteiro — a serra fica verde até o cume. */
	Mild,
	/** Deserto: nenhuma montanha desta escala alcança o congelamento. */
	Desert
};

/**
 * Onde a neve começa, e por quê.
 *
 * Regra ÚNICA do gelo do cenário. Ela é pura e mora fora do ator de propósito:
 * o que decide se um pico é branco é aritmética, e aritmética se verifica sem
 * levantar mundo, sem carregar malha e sem abrir o editor.
 *
 * O modelo é o real, e não uma tabela de gosto: a atmosfera esfria com a
 * altitude a uma taxa conhecida, e a linha da neve é a altura em que essa
 * queda chega a zero grau. Isso dá "depende do tamanho" de graça — a mesma
 * fórmula que põe gelo num pico de 3 km deixa um de 1 km limpo, sem ninguém
 * precisar decidir caso a caso.
 */
namespace ScenaryClimate
{
	/**
	 * Queda de temperatura por quilômetro de altitude.
	 *
	 * 6,5 °C/km é a taxa média da troposfera. Não é número escolhido para o
	 * jogo ficar bonito; é o que faz a serra parecer uma serra.
	 */
	constexpr float LapseRateCelsiusPerKilometer = 6.5f;

	/** Onde a água vira gelo. */
	constexpr float FreezingCelsius = 0.0f;

	/** Temperatura ao PÉ da serra, no clima dado. */
	BATTLESQUARE_API float BaseTemperatureCelsius(EScenaryClimate Climate);

	/** Temperatura àquela altitude, no clima dado. */
	BATTLESQUARE_API float TemperatureAtMeters(EScenaryClimate Climate, float AltitudeMeters);

	/**
	 * Altitude, em metros, em que a neve começa neste clima.
	 *
	 * Acima dela há gelo; abaixo, rocha. Um clima quente devolve um número
	 * grande — e é assim que deserto e clima ameno ficam sem neve, sem
	 * precisar de um `if` a mais em quem desenha.
	 */
	BATTLESQUARE_API float SnowLineMeters(EScenaryClimate Climate);

	/**
	 * Que fatia do pico, de 0 a 1, fica coberta de gelo.
	 *
	 * Zero quer dizer serra limpa. É a forma que quem desenha consome: ela já
	 * traz a decisão E a proporção, e por isso não existe um segundo lugar
	 * comparando altura com linha da neve (L-032/L-033).
	 */
	BATTLESQUARE_API float SnowCapFraction(EScenaryClimate Climate, float PeakMeters);

	/**
	 * Umidade do lugar, 0 a 100 — o que decide se poça vira LAMA ou seca.
	 *
	 * Inteiro, e não float como o resto deste arquivo, porque este número
	 * atravessa para dentro do BattleSim: lá não entra float (AD-004), e um
	 * arredondamento na fronteira seria determinismo dependendo de quem
	 * converteu.
	 *
	 * Mora AQUI, ao lado da temperatura, porque é a mesma pergunta sobre o
	 * mesmo lugar. Numa segunda casa, o deserto acabaria úmido em alguma
	 * tela e seco em outra.
	 */
	BATTLESQUARE_API int32 HumidityPercent(EScenaryClimate Climate);

	/**
	 * O clima pelo nome escrito — `Temperate`, `Cold`, `Mild`, `Desert`.
	 *
	 * Nome desconhecido cai em `Temperate`, que é o bioma que a mata já usa:
	 * um erro de digitação no `.ini` deixa o cenário como está, não sem serra.
	 */
	BATTLESQUARE_API EScenaryClimate ClimateFromName(FName Name);

	/**
	 * O clima escolhido em `DefaultGame.ini`, seção
	 * `[/Script/BattleSquare.MountainRange]`, chave `Climate`.
	 *
	 * Existe para o deserto e o clima ameno serem ALCANÇÁVEIS sem recompilar.
	 * A exceção que o gelo tem — que em deserto e em clima bom não há neve —
	 * não vale nada se ninguém puder ver os três casos na tela.
	 */
	BATTLESQUARE_API EScenaryClimate ConfiguredClimate();
}

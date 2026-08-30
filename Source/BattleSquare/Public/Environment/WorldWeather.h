// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Environment/ScenaryClimate.h"

/**
 * Que tempo faz agora.
 *
 * São quatro estados e não os quatro que se pede em voz alta ("chuva, limpo,
 * nublado, ensolarado"), porque *limpo* e *ensolarado* são o MESMO céu — a
 * diferença entre eles é a altura do sol, que o relógio já sabe. Inventar um
 * valor de enum para "limpo com sol alto" criaria um segundo lugar decidindo
 * se é dia, e ele discordaria do relógio no primeiro pôr do sol.
 *
 * Então: o céu tem quatro graus de nuvem, e `IsSunny` responde a pergunta
 * combinando céu limpo com sol no alto.
 */
enum class EWeather : uint8
{
	/** Céu limpo. De dia é ensolarado; de noite é onde se veem as estrelas. */
	Clear,
	/** Nuvem esparsa: o sol aparece e some. */
	Cloudy,
	/** Encoberto: fechado de ponta a ponta, sem sol, e ainda sem chuva. */
	Overcast,
	/** Chuva. */
	Rain
};

/**
 * O tempo, como função pura de semente, clima e hora.
 *
 * Puro de propósito, como o relógio: dá para perguntar "que tempo fará na
 * hora 37 deste mundo" sem levantar mundo nenhum, e por isso dá para TESTAR
 * que no deserto quase nunca chove — afirmação que, medida jogando, custaria
 * horas de espera.
 *
 * O tempo não é sorteado por quadro nem por hora: ele dura um BLOCO
 * (`HoursPerSpell`). Sorteio a cada instante daria um céu piscando entre sol e
 * chuva, que não é tempo — é ruído.
 */
namespace WorldWeather
{
	/**
	 * Quanto dura um tempo, em horas de jogo.
	 *
	 * Três horas: com o dia de 20 minutos de relógio de parede, isso dá dois
	 * minutos e meio por tempo — longo o bastante para a chuva ser um evento,
	 * curto o bastante para quem jogar meia hora ver o céu mudar.
	 */
	constexpr float HoursPerSpell = 3.0f;

	/** Chance de chover, de 0 a 100, neste clima. */
	BATTLESQUARE_API int32 RainChancePercent(EScenaryClimate Climate);

	/**
	 * O tempo naquele bloco de horas, neste mundo e neste clima.
	 *
	 * Mesma semente e mesma hora devolvem sempre o mesmo céu — dois jogadores
	 * no mesmo mundo veem a mesma chuva, e uma investigação pode voltar à hora
	 * exata em que algo aconteceu.
	 */
	BATTLESQUARE_API EWeather WeatherAt(uint32 Seed, EScenaryClimate Climate, float ElapsedHours);

	/** Quanto do céu está tomado por nuvem, de 0 a 1. */
	BATTLESQUARE_API float CloudCover(EWeather Weather);

	/**
	 * Quanto deste tempo chega a passar do sol, de 0 a 1.
	 *
	 * É o que faz o tempo APARECER sem uma única partícula: nuvem escurece, e
	 * a tela escurecendo é a chuva sendo vista. Multiplica o brilho do
	 * relógio; não o substitui, porque a hora continua mandando na cor.
	 */
	BATTLESQUARE_API float SunDimming(EWeather Weather);

	/**
	 * A umidade do lugar somando o tempo que faz — 0 a 100.
	 *
	 * Aqui é onde o tempo vira REGRA em vez de enfeite: este número atravessa
	 * para o `BattleSim` e é ele que decide se a poça da arena é LAMA ou chão
	 * seco. Chover de verdade enlameia o campo da próxima batalha, e nenhuma
	 * linha de combate precisou saber que existe chuva.
	 *
	 * Inteiro pelo mesmo motivo de `ScenaryClimate::HumidityPercent`: float
	 * não atravessa para o núcleo determinístico (AD-004). E soma A PARTIR
	 * dela, em vez de trazer a própria tabela, para o deserto não acabar úmido
	 * numa casa e seco na outra (L-032).
	 */
	BATTLESQUARE_API int32 HumidityPercent(EScenaryClimate Climate, EWeather Weather);

	/** Está chovendo. */
	BATTLESQUARE_API bool IsRaining(EWeather Weather);

	/**
	 * Está ensolarado: céu limpo E sol no alto.
	 *
	 * A hora vem de fora porque quem sabe que horas são é o relógio, e ele é
	 * um só.
	 */
	BATTLESQUARE_API bool IsSunny(EWeather Weather, float Hour);

	/** Nome curto para o painel. Ferramenta de desenvolvimento, não texto de jogo. */
	BATTLESQUARE_API const TCHAR* WeatherDebugName(EWeather Weather);
}

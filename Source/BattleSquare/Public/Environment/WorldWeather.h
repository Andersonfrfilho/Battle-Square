// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Environment/ScenaryClimate.h"

/**
 * Que tempo faz agora.
 *
 * *Limpo* e *ensolarado* NÃO são dois valores, porque são o mesmo céu — a
 * diferença entre eles é a altura do sol, que o relógio já sabe. Inventar um
 * valor de enum para "limpo com sol alto" criaria um segundo lugar decidindo
 * se é dia, e ele discordaria do relógio no primeiro pôr do sol. Quem responde
 * é `IsSunny`, combinando céu limpo com sol no alto.
 *
 * A chuva, ao contrário, tem GRAU — e o grau é valor de enum e não um número
 * ao lado, porque um número separado permitiria "céu limpo com força 3", que
 * é um estado sem significado que alguém teria de lembrar de nunca escrever.
 *
 * A ORDEM É A SEVERIDADE, e disso dependem `IsRaining` e todo teste que
 * compara dois céus. Enfiar um valor no meio muda o significado de todos eles
 * de uma vez; valor novo entra no FIM da faixa a que ele pertence.
 */
enum class EWeather : uint8
{
	/** Céu limpo. De dia é ensolarado; de noite é onde se veem as estrelas. */
	Clear,
	/** Nuvem esparsa: o sol aparece e some. */
	Cloudy,
	/** Encoberto: fechado de ponta a ponta, sem sol, e ainda sem chuva. */
	Overcast,
	/** Garoa: molha o chão e não faz mais nada. */
	Drizzle,
	/** Chuva. */
	Rain,
	/** Chuva forte: escurece de dia e encharca o campo. */
	Downpour,
	/** Tempestade: chuva forte com ALAGAMENTO — a água sai do leito. */
	Storm
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

	/** Chance de chover de qualquer jeito, de 0 a 100, neste clima. */
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

	/** Está chovendo, de garoa a tempestade. */
	BATTLESQUARE_API bool IsRaining(EWeather Weather);

	/**
	 * A água SAIU DO LEITO: o campo da próxima batalha vem alagado.
	 *
	 * É a única pergunta sobre o tempo que muda o TABULEIRO e não só um
	 * número. Umidade encharca o chão que já era beira de água; alagamento
	 * cria água onde não havia — a margem sobe, e casa seca encostada na
	 * água vira poça.
	 *
	 * Só a tempestade alaga. Chuva forte encharca sem transbordar, e é a
	 * diferença que faz a tempestade valer a espera.
	 */
	BATTLESQUARE_API bool IsFlooding(EWeather Weather);

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

// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Um evento de terra: furacão, terremoto, tsunami.
 *
 * Eles são IRMÃOS do clima e não uma variedade dele. A chuva não tem lugar —
 * ela cai no clima em que você está, e a pergunta "está chovendo?" se responde
 * com hora e clima. O terremoto tem EPICENTRO, o furacão tem OLHO e o tsunami
 * tem uma costa para bater. Enfiá-los em `EWeather` obrigaria todo lugar que
 * pergunta o tempo a saber onde ele está, e daria "terremoto" como um céu.
 *
 * A ORDEM É A SEVERIDADE, e é dela que sai a regra de quando dois se
 * sobrepõem: vence o mais forte. Um furacão no mar e um terremoto perto do
 * vulcão não se encontram — são lugares diferentes —, mas o tsunami bate
 * justamente na costa por onde o furacão passa, e aí é o tsunami que se vê.
 * Valor novo entra pela severidade, não no fim por comodidade.
 */
enum class EWorldEvent : uint8
{
	/** Nada acontecendo aqui e agora. */
	None,
	/** A terra tremendo, mais forte perto do vulcão. */
	Earthquake,
	/** Vento e chuva girando sobre o mar e a praia. */
	Hurricane,
	/** A onda que o terremoto forte levantou, chegando à costa. */
	Tsunami
};

/**
 * Os eventos de terra, como função pura de semente, lugar e hora.
 *
 * Mesma forma do clima e do céu, e pelo mesmo motivo: dá para perguntar "que
 * evento havia na hora 300 deste mundo, naquele ponto" sem levantar mundo
 * nenhum, e por isso dá para TESTAR que o tsunami vem DEPOIS do terremoto —
 * afirmação que, medida jogando, custaria horas de espera na beira da praia.
 *
 * Não há máquina de estados e não há nada salvo. Um evento com estado teria de
 * ser gravado, sincronizado entre jogadores e restaurado no `load`; sendo
 * função pura, dois jogadores no mesmo mundo veem o mesmo terremoto por
 * construção, e uma investigação volta à hora exata.
 */
namespace WorldEvents
{
	/**
	 * De quanto em quanto tempo a terra treme, em horas de jogo.
	 *
	 * Um por período, como a passagem do cometa: o INSTANTE dentro do período é
	 * sorteado, então o terremoto é imprevisível sem nunca deixar de vir. Um
	 * sorteio de "acontece ou não" a cada hora daria mundos inteiros sem um
	 * único tremor, e mundos com três numa tarde.
	 */
	constexpr float EarthquakePeriodHours = 96.0f;

	/** Quanto dura o tremor. Curto de propósito — terremoto não é um clima. */
	constexpr float EarthquakeSpanHours = 0.5f;

	/** De quanto em quanto tempo um furacão se forma. */
	constexpr float HurricanePeriodHours = 72.0f;

	/** Quanto dura a passagem do furacão. */
	constexpr float HurricaneSpanHours = 8.0f;

	/**
	 * Quanto o tsunami demora a chegar depois do tremor.
	 *
	 * A espera é o evento. Sem ela o tsunami seria um segundo nome para
	 * terremoto, e quem estivesse na praia não teria o que fazer com o aviso.
	 */
	constexpr float TsunamiDelayHours = 0.75f;

	/** Quanto tempo a onda fica batendo. */
	constexpr float TsunamiSpanHours = 1.5f;

	/**
	 * Abaixo desta magnitude o tremor não levanta onda.
	 *
	 * É o que faz o tsunami ser mais raro que o terremoto sem ter período
	 * próprio: ele não é sorteado, é CONSEQUÊNCIA. Sortear os dois em separado
	 * permitiria uma onda sem tremor nenhum antes dela.
	 */
	constexpr float TsunamiMinMagnitude = 0.6f;

	/** A magnitude mais fraca e a mais forte que um tremor pode ter. */
	constexpr float MinMagnitude = 0.2f;
	constexpr float MaxMagnitude = 1.0f;

	/** Até onde o tremor é sentido, a partir do epicentro. */
	constexpr float EarthquakeReachUnits = 9000.0f;

	/** Quão longe do vulcão o epicentro pode cair. */
	constexpr float FaultSpreadUnits = 3000.0f;

	/** Meia-largura do setor por onde o furacão passa, em graus. */
	constexpr float HurricaneHalfWidthDegrees = 40.0f;

	/**
	 * Quanto o tsunami levanta a água do mundo, no pico da onda mais forte.
	 *
	 * É o que faz o tsunami APARECER sem uma única partícula nova: a água que
	 * já cerca a ilha sobe, e a praia desaparece por baixo dela.
	 */
	constexpr float TsunamiMaxRiseUnits = 260.0f;

	/**
	 * Onde a falha mora: o centro em volta do qual o epicentro é sorteado.
	 *
	 * Sai do vulcão que o `IslandFeatureLayout` já colocou, e não de uma
	 * coordenada escrita aqui. Duas cópias do lugar do vulcão concordariam até
	 * a primeira vez que alguém movesse o vulcão (L-032).
	 */
	BATTLESQUARE_API FVector2D FaultCenterUnits();

	/**
	 * A magnitude do tremor deste período, de 0 a 1 — 0 quando não há tremor.
	 *
	 * Ela é do TREMOR e não do lugar: quem está longe sente menos, e é
	 * `EarthquakeShaking` que faz essa conta.
	 */
	BATTLESQUARE_API float EarthquakeMagnitude(uint32 Seed, float ElapsedHours);

	/** Onde o tremor deste período nasceu. */
	BATTLESQUARE_API FVector2D EarthquakeEpicenterUnits(uint32 Seed, float ElapsedHours);

	/** Quanto se sente o tremor NESTE ponto, de 0 a 1. */
	BATTLESQUARE_API float EarthquakeShaking(uint32 Seed, const FVector2D& PositionUnits, float ElapsedHours);

	/** Em que ângulo o olho do furacão deste período entra na ilha. */
	BATTLESQUARE_API float HurricaneEyeAngleDegrees(uint32 Seed, float ElapsedHours);

	/**
	 * Quanto do furacão bate NESTE ponto, de 0 a 1.
	 *
	 * Zero terra adentro, por definição: furacão morre em cima de terra seca, e
	 * é isso que responde ao "em certas áreas que faz sentido".
	 */
	BATTLESQUARE_API float HurricaneStrength(uint32 Seed, const FVector2D& PositionUnits, float ElapsedHours);

	/** O tsunami deste período existe: houve tremor, e ele foi forte. */
	BATTLESQUARE_API bool TsunamiRises(uint32 Seed, float ElapsedHours);

	/**
	 * Quanto a onda levantou a água agora, em unidades — 0 fora da onda.
	 *
	 * Vale em toda a volta da ilha: a onda parte do epicentro e o mar é um só,
	 * então não há costa poupada. Quem decide se ESTE ponto se molha é a altura
	 * do chão dele, não esta conta.
	 */
	BATTLESQUARE_API float TsunamiRiseUnits(uint32 Seed, float ElapsedHours);

	/** O tsunami alcança este ponto: mar, praia, ou o que a onda subiu. */
	BATTLESQUARE_API bool TsunamiReaches(const FVector2D& PositionUnits);

	/**
	 * O evento que se vê deste ponto, nesta hora — o mais forte, se houver dois.
	 *
	 * Um valor só, e não uma lista: o que a tela precisa dizer é o que está
	 * acontecendo, e uma linha com "furacão e tsunami" faria quem lê procurar
	 * qual dos dois é o que importa.
	 */
	BATTLESQUARE_API EWorldEvent EventAt(uint32 Seed, const FVector2D& PositionUnits, float ElapsedHours);

	/**
	 * A força do evento que `EventAt` devolveu, de 0 a 1.
	 *
	 * Vem junto porque sem ela a tela mostraria "TERREMOTO" com a mesma cara
	 * para um tremor que quase não se nota e para o que derruba pedra.
	 */
	BATTLESQUARE_API float EventStrength(uint32 Seed, const FVector2D& PositionUnits, float ElapsedHours);

	/** Nome curto para o painel. Ferramenta de desenvolvimento, não texto de jogo. */
	BATTLESQUARE_API const TCHAR* EventDebugName(EWorldEvent Event);
}

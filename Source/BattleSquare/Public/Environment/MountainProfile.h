// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * O PERFIL da montanha — o raio em cada altura, e nada mais.
 *
 * Existe separado porque duas coisas leem a mesma silhueta e precisam
 * concordar: o CORPO, que é o que se vê, e a TRILHA, que precisa encostar na
 * encosta. Enquanto o perfil era um cone, as duas concordavam por acidente —
 * a conta era simples o bastante para ser reescrita em dois lugares. Um perfil
 * curvo escrito duas vezes seria uma trilha flutuando no ar na primeira
 * edição de um dos lados.
 *
 * O cone foi o defeito que o jogador enxergou primeiro: "montanhas pontudas
 * extremamente esquisitas". Cone é forma de montanha para quem só verifica
 * número — a lógica passa em todo teste e a tela mostra um chapéu de festa.
 * Montanha de verdade tem OMBRO: sai larga da base, engorda no meio e
 * arredonda no cume.
 *
 * A curva é `cos(t·π/2)^ShoulderExponent`, com `t` indo de 0 na base a 1 no
 * topo. O expoente abaixo de 1 é o que empurra o volume para fora: em t=0,5 o
 * cone teria metade do raio, e este perfil tem 78% dele. E como a derivada
 * tende a menos infinito no topo, o cume termina em CALOTA, não em agulha —
 * o raio cai de vez nos últimos por cento em vez de afinar até virar ponta.
 *
 * Nada aqui é `BATTLESQUARE_API`: são funções `inline` num cabeçalho, como
 * `BattleSpread`. Aparência não é regra de batalha e não mora no `BattleSim`.
 */
namespace MountainProfile
{
	/**
	 * Em quantas fatias horizontais o corpo é montado.
	 *
	 * A engine não tem malha de tronco de cone, e um cone escalado continua
	 * sendo um cone. Curva se obtém EMPILHANDO cilindros de raios diferentes.
	 * Catorze fatias dão degraus de estrato de rocha, que o costado de pedras
	 * quebra; menos que isso vira escada, mais que isso é instância à toa.
	 */
	/**
	 * Quantas fatias.
	 *
	 * Vinte, e o número saiu de um teste que já existia: a trilha exige
	 * encostar na encosta com folga de 130 unidades, e a fatia aproxima a
	 * curva por um degrau. Perto do cume, onde a curva desce mais rápido,
	 * 20 fatias erram ~113 unidades num raio de 2400 e 14 errariam ~360.
	 * De quebra, 2100 de altura em 20 fatias dá 105 por fatia, que na tela
	 * lê como camada de rocha em vez de degrau de escada.
	 */
	constexpr int32 SliceCount = 20;

	/** Abaixo de 1 a encosta engorda; em 1 exato ela voltaria a ser um cone. */
	constexpr float ShoulderExponent = 0.72f;

	/** Fração do raio da base na altura relativa dada. Zero do cume para cima. */
	inline float RadiusFractionAt(float HeightFraction)
	{
		if (HeightFraction <= 0.0f)
		{
			return 1.0f;
		}

		if (HeightFraction >= 1.0f)
		{
			return 0.0f;
		}

		const float Cosseno = FMath::Cos(HeightFraction * PI * 0.5f);
		return FMath::Pow(Cosseno, ShoulderExponent);
	}

	/** Qual fatia contém a altura relativa dada. */
	inline int32 SliceIndexAt(float HeightFraction)
	{
		return FMath::Clamp(
			FMath::FloorToInt(HeightFraction * static_cast<float>(SliceCount)),
			0,
			SliceCount - 1);
	}

	/**
	 * Raio da fatia — medido no MEIO dela, não na borda.
	 *
	 * É este o número que a trilha usa, e não a curva contínua: a encosta
	 * construída é a parede do cilindro, e um patamar apoiado no raio teórico
	 * ficaria enterrado na metade de baixo da fatia e no ar na de cima.
	 */
	inline float SliceRadiusFraction(int32 SliceIndex)
	{
		const float Meio = (static_cast<float>(SliceIndex) + 0.5f)
			/ static_cast<float>(SliceCount);
		return RadiusFractionAt(Meio);
	}

	/** Fração da altura ocupada por uma fatia. */
	inline float SliceHeightFraction()
	{
		return 1.0f / static_cast<float>(SliceCount);
	}
}

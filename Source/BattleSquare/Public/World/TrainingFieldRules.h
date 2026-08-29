// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Meta/PetCollectionSaveGame.h"

/**
 * Quanto um campo de treino rende, e em quê.
 *
 * Pura sobre números e um nome de atributo: não lê save, não conhece ator, não
 * olha relógio. O TEMPO chega como parâmetro — quem o mede é quem chama, e é
 * lá que mora a decisão de produto ainda aberta (relógio de servidor contra
 * jogo aberto). Manter a regra ignorante disso é o que permite trocar a fonte
 * do tempo sem reescrever o rendimento.
 */
class BATTLESQUARE_API FTrainingFieldRules
{
public:
	/**
	 * Segundos de permanência que valem UM ponto do atributo.
	 *
	 * Curto de propósito nesta fatia: treino que leva minutos não é testável
	 * numa sessão, e o que precisa ser respondido primeiro é se ir até o campo
	 * é interessante — não se a espera é longa o bastante.
	 */
	static constexpr float SecondsPerPoint = 6.0f;

	/**
	 * Multiplicador do estudo do dono, em PORCENTAGEM (100 = sem estudo).
	 *
	 * DP-atr-09: o estudo MULTIPLICA, nunca substitui. O treinador que estudou
	 * faz o mesmo treino render mais; ele nunca treina no lugar do pet — senão
	 * o pet vira detalhe e um jogador novo trava atrás de uma barra que nem
	 * sabia que existia.
	 */
	static constexpr int32 SpecialistBonusPercent = 150;

	/**
	 * Pontos ganhos por um tempo de permanência, e o resto que fica.
	 *
	 * O RESTO importa: sem devolvê-lo, cada visita curta ao campo perderia a
	 * fração acumulada, e visitar dez vezes por cinco segundos renderia zero
	 * enquanto uma visita de cinquenta renderia oito. Duas formas de fazer a
	 * mesma coisa não podem ter resultados diferentes.
	 */
	static int32 PointsForTime(float SecondsInField, bool bOwnerIsSpecialist, float& InOutCarrySeconds);

	/** Aplica os pontos ao atributo nomeado. Nome desconhecido não muda nada. */
	static void ApplyPoints(const FString& Attribute, int32 Points, FOwnedPetInstance& Instance);
};

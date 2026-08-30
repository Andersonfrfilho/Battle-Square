// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Environment/ScenaryPalette.h"

/** Um obstáculo do mundo, como a regra o enxerga. */
struct BATTLESQUARE_API FWorldObstacleCandidate
{
	FVector Location = FVector::ZeroVector;
	EScenaryRole Role = EScenaryRole::ForestTree;

	/** O que ainda falta para ele cair. */
	int32 RemainingHealth = 0;
};

/**
 * Quem é derrubado, com quanta força, e quando cai.
 *
 * Pura: recebe posição, olhar e uma lista, devolve um índice e um número. Não
 * conhece ator, componente nem malha — por isso o alcance, o arco e a fórmula
 * têm teste sem abrir o editor.
 *
 * O VERBO é do mundo, mas a REGRA é a mesma da arena: força derruba. Um pet
 * que derruba a pedra do tabuleiro e não derruba a árvore do mapa ensinaria
 * duas coisas contraditórias sobre o mesmo bicho.
 */
class BATTLESQUARE_API FWorldObstacleBreaking
{
public:
	/** Até onde o golpe chega. */
	static constexpr float ReachUnits = 260.0f;

	/**
	 * Meio ângulo do arco à frente, em graus.
	 *
	 * Generoso de propósito: um arco estreito faria o jogador errar a árvore
	 * que está vendo, e "eu bati e não aconteceu nada" é indistinguível de
	 * defeito.
	 */
	static constexpr float HalfArcDegrees = 55.0f;

	/**
	 * Vida de um obstáculo, POR PAPEL.
	 *
	 * Pedra aguenta mais que tronco caído, e capim não é obstáculo nenhum —
	 * derrubar grama com um golpe seria trabalho sem consequência, e faria o
	 * jogador achar que o golpe é fraco quando ele acertou o alvo errado.
	 * Zero significa INDESTRUTÍVEL, e é o padrão.
	 */
	static int32 StartingHealthFor(EScenaryRole Role);

	/**
	 * Quanto um pet tira, pela musculatura dele.
	 *
	 * Mínimo de 1: um pet fraco derruba a árvore devagar, e nunca fica preso
	 * batendo em algo que não muda. Progresso lento ensina; progresso zero
	 * parece defeito.
	 */
	static int32 DamageFromMusculature(int32 Musculature);

	/**
	 * O alvo À FRENTE e ao alcance, ou INDEX_NONE.
	 *
	 * Escolhe o MAIS PRÓXIMO entre os que estão no arco — e não o mais
	 * alinhado: o jogador mira com o corpo, e a árvore encostada nele é a que
	 * ele espera acertar.
	 */
	static int32 FindTarget(const FVector& PlayerLocation, const FVector& PlayerForward,
		const TArray<FWorldObstacleCandidate>& Candidates);
};

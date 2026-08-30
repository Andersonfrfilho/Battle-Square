// Copyright 2026 Anderson. All Rights Reserved.

#include "World/WorldObstacleBreaking.h"

int32 FWorldObstacleBreaking::StartingHealthFor(EScenaryRole Role)
{
	switch (Role)
	{
		case EScenaryRole::Rock:       return 60;
		case EScenaryRole::CanopyTree: return 50;
		case EScenaryRole::ForestTree: return 35;
		case EScenaryRole::DeadWood:   return 20;

		// Capim, arbusto e flor NÃO são obstáculo: passar por eles já
		// funciona, e derrubá-los seria esforço sem consequência.
		default: return 0;
	}
}

int32 FWorldObstacleBreaking::DamageFromMusculature(int32 Musculature)
{
	// Base + musculatura: o pet sem treino nenhum ainda derruba um tronco em
	// quatro golpes, e o treinado sente a diferença sem que ela vire tudo.
	return FMath::Max(1, 5 + FMath::Max(0, Musculature) / 2);
}

int32 FWorldObstacleBreaking::FindTarget(const FVector& PlayerLocation,
	const FVector& PlayerForward, const TArray<FWorldObstacleCandidate>& Candidates)
{
	FVector Frente = PlayerForward;
	Frente.Z = 0.0f;
	if (!Frente.Normalize())
	{
		return INDEX_NONE;
	}

	const float CossenoDoArco = FMath::Cos(FMath::DegreesToRadians(HalfArcDegrees));

	int32 Melhor = INDEX_NONE;
	float MenorDistancia = TNumericLimits<float>::Max();

	for (int32 Indice = 0; Indice < Candidates.Num(); ++Indice)
	{
		const FWorldObstacleCandidate& Candidato = Candidates[Indice];

		// Já caído não é alvo. Sem isto, continuar batendo no toco daria
		// resposta de acerto e nenhuma mudança — e o jogador conclui que o
		// golpe parou de funcionar.
		if (Candidato.RemainingHealth <= 0)
		{
			continue;
		}

		FVector Ate = Candidato.Location - PlayerLocation;

		// A ALTURA não conta: a copa da árvore está muito acima do jogador, e
		// medir em três dimensões faria a árvore alta sair do alcance
		// justamente por ser alta.
		Ate.Z = 0.0f;

		const float Distancia = Ate.Size();
		if (Distancia > ReachUnits || Distancia <= KINDA_SMALL_NUMBER)
		{
			continue;
		}

		if (FVector::DotProduct(Ate / Distancia, Frente) < CossenoDoArco)
		{
			continue;
		}

		if (Distancia < MenorDistancia)
		{
			MenorDistancia = Distancia;
			Melhor = Indice;
		}
	}

	return Melhor;
}

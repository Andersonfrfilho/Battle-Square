// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * O que ocupa o chão entre as trilhas.
 *
 * A ilha tinha assentamento, água e marco — e entre eles, mata igual em toda
 * parte. Mata uniforme não é floresta: é textura. Quem anda por ela não sabe
 * se andou cem metros ou mil.
 */
UENUM()
enum class EGroundUse : uint8
{
	/** Nada de especial: a mata comum. */
	Nenhum,

	/**
	 * BOSQUE: mata fechada, mais densa que o resto.
	 *
	 * É o que dá referência ao caminhar — sair do aberto e entrar no escuro é
	 * a menor unidade de "cheguei em outro lugar" que existe sem placa.
	 */
	Bosque,

	/**
	 * CLAREIRA FECHADA: o vazio cercado de mata por todos os lados.
	 *
	 * O contrário do bosque, e vale pelo mesmo motivo. Ela é FECHADA de
	 * propósito — nenhuma trilha entra. Achá-la é acidente, e é isso que faz
	 * dela um lugar em vez de um pátio.
	 */
	ClareiraFechada,

	/**
	 * FAZENDA: o chão trabalhado, perto de uma vila e perto da água.
	 *
	 * Ela é a razão pela qual a vila come, e é o primeiro sinal de que alguém
	 * mora ali — quem chega vê roçado antes de telhado.
	 */
	Fazenda
};

struct BATTLESQUARE_API FGroundUsePatch
{
	EGroundUse Use = EGroundUse::Nenhum;
	FVector2D CenterUnits = FVector2D::ZeroVector;
	float HalfExtentUnits = 0.0f;
};

/**
 * O uso do solo — PURO, como todo o resto do traçado deste mundo.
 *
 * As posições saem do que JÁ EXISTE: fazenda nasce ao lado de uma vila e
 * virada para a água; clareira nasce longe de tudo; bosque nasce entre os
 * marcos. Nenhuma delas é uma coordenada escrita à mão, e é isso que faz o
 * traçado continuar certo quando a ilha mudar de tamanho.
 */
namespace LandUseLayout
{
	BATTLESQUARE_API TArray<FGroundUsePatch> Plan();

	/** O que este ponto é. */
	BATTLESQUARE_API EGroundUse UseAt(const FVector2D& PositionUnits);

	/**
	 * Aqui não se planta árvore.
	 *
	 * Vale para fazenda e para clareira fechada — as duas são vazios, por
	 * motivos opostos. O bosque é o contrário: ali se planta MAIS.
	 */
	BATTLESQUARE_API bool BlocksPlanting(const FVector2D& PositionUnits);

	/**
	 * Quanto a densidade da mata muda aqui, como multiplicador.
	 *
	 * Um no chão comum, mais no bosque, zero onde não se planta. É um número
	 * só para o gerador da mata multiplicar, em vez de ele aprender as regras
	 * de uso do solo — que não são dele.
	 */
	BATTLESQUARE_API float PlantingDensityAt(const FVector2D& PositionUnits);
}

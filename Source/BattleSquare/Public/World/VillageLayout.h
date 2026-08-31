// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * O que cada prédio da vila É.
 *
 * A lista é CURTA e cada entrada serve a um laço que o jogo já tem — a regra
 * que ordena a spec das cidades. Prédio que não serve a nada é cenário com
 * porta, e este projeto já pagou caro por recursos completos que ninguém
 * alcançava.
 */
UENUM()
enum class EVillageBuilding : uint8
{
	/**
	 * Centro de Recuperação: cura o pet DE GRAÇA, e abriga a coleção.
	 *
	 * Curar e trocar o pet ativo são a mesma visita; duas portas para um gesto
	 * só é atrito sem ganho.
	 */
	CentroDeRecuperacao,

	/** Escola do treinador: tira `bs.Especializar` do console. */
	Escola,

	/** Arena da vila: um oponente com motivo para voltar. */
	Arena,

	/** Praça com o poste de anúncios — diz onde ficam as coisas do mapa. */
	Praca,

	/** Marco de retorno: viagem rápida e marca no mapa. */
	Marco,

	/**
	 * Casa. Não tem função, e ganha o lugar por uma razão só: a vila precisa
	 * se LER de fora — quem chega pela mata vê telhado antes de porta.
	 *
	 * E a regra que vem junto: casa sem função não tem porta. Porta que não
	 * abre é promessa quebrada.
	 */
	Casa
};

struct BATTLESQUARE_API FVillagePlacement
{
	EVillageBuilding Building = EVillageBuilding::Casa;

	/** Centro do prédio, relativo ao centro da vila. */
	FVector2D OffsetUnits = FVector2D::ZeroVector;

	/** Meio-lado da pegada quadrada. */
	FVector2D HalfExtentUnits = FVector2D(200.0f, 200.0f);

	/** Altura, para o telhado se ler de longe. */
	float HeightUnits = 400.0f;
};

/**
 * O traçado da vila — PURO, e é o ponto.
 *
 * Quem decide onde cada prédio fica não precisa de `UWorld`, de malha nem de
 * Editor: é aritmética sobre um lote. Assim "nenhum prédio invade o outro" e
 * "tudo cabe no lote" se verificam headless, que é onde este projeto consegue
 * verificar de verdade.
 *
 * Mesma separação que tornou `FArenaFromWorld` e `ScenaryClimate` testáveis.
 */
namespace VillageLayout
{
	/**
	 * O lote da vila ocupa uma FRAÇÃO do bloco.
	 *
	 * O bloco tem 6400 unidades (64 m). Vila que enche o bloco não deixa lugar
	 * para a floresta que vem em volta, e o jogador sairia de casa direto para
	 * o bloco vizinho.
	 */
	BATTLESQUARE_API float PlotHalfExtentUnits();

	/**
	 * A praça fica no MEIO, e os prédios em volta dela.
	 *
	 * Não é gosto: a praça é o poste de anúncios, e ele precisa ser a primeira
	 * coisa que se encontra ao entrar. Um poste num canto é um poste que
	 * ninguém lê.
	 */
	BATTLESQUARE_API TArray<FVillagePlacement> Plan();

	/** Os dois prédios que não podem faltar, porque tiram regra do console. */
	BATTLESQUARE_API bool HasBuilding(const TArray<FVillagePlacement>& Placements,
		EVillageBuilding Building);

	/** Duas pegadas se invadem. */
	BATTLESQUARE_API bool Overlaps(const FVillagePlacement& First, const FVillagePlacement& Second);

	/** A pegada inteira cabe no lote. */
	BATTLESQUARE_API bool FitsInPlot(const FVillagePlacement& Placement);
}

// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WorldDiscovery.generated.h"

/**
 * O que o jogador JÁ VIU do mundo.
 *
 * O mapa deixa de ser espelho do mundo e passa a ser registro do que se
 * andou. A diferença não é de apresentação: um mapa que já nasce completo
 * responde "onde fica o campo de Voo?" antes de a pergunta existir, e o mundo
 * aberto perde a única coisa que o faz valer a pena atravessar.
 *
 * GRADE GROSSA, e não posição contínua: a memória é "estive naquela região",
 * não "pisei naquele centímetro". Guardar posição exata gastaria save para
 * descrever um caminho que ninguém relê, e ainda revelaria o mapa em fatias
 * finas do tamanho do passo — o que se lê na tela como sujeira, não como
 * exploração.
 *
 * PURA: sem UWorld, sem ator, sem tempo. É o que permite verificar "andar
 * revela o entorno, e não só a casa onde estou" sem abrir o editor.
 */
USTRUCT()
struct BATTLESQUARE_API FWorldDiscovery
{
	GENERATED_BODY()

	/**
	 * Lado de uma região descoberta, em unidades de mundo.
	 *
	 * Grande o bastante para o mapa mostrar MANCHAS e não pontilhado. Com
	 * região do tamanho de um passo, o mapa vira um rastro de migalhas — bonito
	 * de perto e ilegível de longe, que é justamente quando se olha o mapa.
	 */
	static constexpr float RegionSizeUnits = 800.0f;

	/**
	 * O que uma passada revela, em REGIÕES de raio.
	 *
	 * Um revela o entorno imediato: quem passa numa clareira vê a clareira, e
	 * não só o chão sob o pé. Zero faria o mapa contar por onde se pisou em vez
	 * de o que se viu — e ninguém enxerga só para baixo.
	 */
	static constexpr int32 SightRadiusInRegions = 1;

	/**
	 * As regiões visitadas, empacotadas como (coluna, linha).
	 *
	 * `TSet` e não grade de bits: o mundo não tem tamanho declarado, e uma
	 * grade exigiria escolher um limite antes de saber qual é. O custo é uma
	 * entrada por região realmente visitada — quem nunca saiu do começo carrega
	 * meia dúzia.
	 */
	UPROPERTY()
	TSet<int64> VisitedRegions;

	/** Chave estável de uma região. Pública porque o teste precisa dela. */
	static int64 RegionKey(int32 Column, int32 Row)
	{
		return (static_cast<int64>(Column) << 32) | (static_cast<uint32>(Row));
	}

	static int32 RegionColumnOf(float WorldX)
	{
		return FMath::FloorToInt(WorldX / RegionSizeUnits);
	}

	static int32 RegionRowOf(float WorldY)
	{
		return FMath::FloorToInt(WorldY / RegionSizeUnits);
	}

	/**
	 * Registra que o jogador esteve aqui — e no que dá para ver daqui.
	 *
	 * Devolve quantas regiões eram NOVAS. Zero é o caso comum (andar dentro do
	 * que já se conhece), e é o que permite a quem chama só gravar o save
	 * quando algo mudou, em vez de a cada passo.
	 */
	int32 MarkSeenFrom(const FVector2D& WorldXY);

	/** Esta posição já foi descoberta? */
	bool IsDiscovered(const FVector2D& WorldXY) const;

	/** Quantas regiões o jogador conhece. */
	int32 DiscoveredCount() const { return VisitedRegions.Num(); }
};

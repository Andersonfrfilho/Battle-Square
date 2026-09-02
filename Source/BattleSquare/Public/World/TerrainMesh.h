// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "World/IslandBakedPlan.h"
#include "TerrainMesh.generated.h"

class UIslandBakedPlan;
class UProceduralMeshComponent;

/**
 * O RELEVO DA ILHA — o chão que tudo mais pisa.
 *
 * Vem primeiro porque sem terreno o rio flutua, a trilha não sobe nada e o
 * barranco não barra ninguém. Até aqui o chão era sondado por *line trace*
 * contra o que houvesse, e o relevo calculado nunca virava geometria: 679
 * testes verdes sobre um mundo que ninguém podia pisar.
 *
 * A geometria sai do ASSADO, nunca de `GroundHeightAt` chamado em runtime.
 * Duas fontes para a mesma altura concordam até a primeira edição, e aqui a
 * divergência seria invisível: o terreno subiria certo e a trilha assentada
 * ficaria alguns centímetros no ar.
 */
UCLASS()
class BATTLESQUARE_API ATerrainMesh : public AActor
{
	GENERATED_BODY()

public:
	ATerrainMesh();

	/**
	 * Constrói a superfície a partir do assado.
	 *
	 * Devolve quantos vértices ergueu. Devolver a conta é o que torna a
	 * construção verificável: malha de zero vértice existe como ator, passa em
	 * todo teste de lógica e não é chão nenhum.
	 */
	int32 BuildFrom(const UIslandBakedPlan& Baked);

	UProceduralMeshComponent* GetSurface() const { return Surface; }

	/** O lado da grade que foi erguida, em casas. Zero antes de construir. */
	int32 GetGridSide() const { return GridSide; }

	/** O tamanho de uma casa da grade, em unidades de mundo. */
	float GetCellSizeUnits() const { return CellSizeUnits; }

	/** A altura erguida na casa dada — o que a MALHA tem, não o que o plano diz. */
	float BuiltHeightAtCell(int32 Column, int32 Row) const;

	/**
	 * A altura da SUPERFÍCIE ERGUIDA numa posição qualquer da ilha.
	 *
	 * Interpola entre as quatro casas em volta, que é o que a malha de fato
	 * desenha entre os vértices. Perguntar a `GroundHeightAt` daria a altura do
	 * PLANO — e o que a trilha e o rio precisam saber é onde o chão está, não
	 * onde ele deveria estar. Entre os dois cabe justamente o erro que faria a
	 * trilha flutuar.
	 */
	float BuiltHeightAt(const FVector2D& PositionUnits) const;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	TObjectPtr<UProceduralMeshComponent> Surface;

	/** As alturas como foram erguidas, para a conferência não reler o plano. */
	UPROPERTY()
	TArray<float> BuiltHeightUnits;

	/** O assado de que esta superfície saiu — a fonte da altura que ela desenha. */
	UPROPERTY()
	TObjectPtr<const UIslandBakedPlan> SourcePlan;

	UPROPERTY()
	int32 GridSide = 0;

	UPROPERTY()
	float CellSizeUnits = 0.0f;

	/** Que seção da malha ficou com cada faixa. `INDEX_NONE` é faixa ausente. */
	UPROPERTY()
	TArray<int32> SectionOfBand;

public:
	/**
	 * A seção que ficou com a faixa dada, ou `INDEX_NONE` se ela não existe
	 * nesta ilha. É por aqui que se verifica que a cor de fato subiu.
	 */
	int32 GetSectionOfBand(ETerrainBand Band) const;
};

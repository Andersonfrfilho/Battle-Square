// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "Environment/CaveLabyrinth.h"

#include "CaveSystem.generated.h"

class UHierarchicalInstancedStaticMeshComponent;

/**
 * A caverna: a planta de `CaveLabyrinth` virada pedra que bloqueia.
 *
 * **A caverna é aberta em cima, e isso é decisão, não falta.** Um labirinto
 * com teto e sem fonte de luz é uma tela preta, e a câmera de terceira pessoa
 * dentro de um corredor de três metros fica dentro da rocha. Quem entra aqui
 * vê o caminho; de fora, a MURALHA da borda — mais alta e mais grossa que as
 * paredes de dentro — esconde o miolo e deixa só a boca aparecendo, que é o
 * que faz o lugar parecer caverna e não sebe de jardim.
 *
 * A boca ganha uma verga por cima: sem ela a entrada é um entalhe na muralha,
 * e com ela é um vão por onde se passa POR BAIXO da pedra.
 */
UCLASS()
class BATTLESQUARE_API ACaveSystem : public AActor
{
	GENERATED_BODY()

public:
	ACaveSystem();

	/** Escava e constrói. Grade inválida não constrói nada — nunca meia caverna. */
	void BuildCave(int32 Columns, int32 Rows, uint32 Seed);

	const CaveLabyrinth::FCaveGrid& GetGrid() const { return Grid; }

	/** O centro de uma casa, no espaço do ator. */
	FVector CellCenterLocal(int32 Column, int32 Row) const;

	/** Onde se entra — do lado de FORA da boca. */
	FVector EntranceLocal() const;

	/** A casa mais longe da entrada: onde vale a pena pôr o que se procura. */
	FVector DeepestLocal() const;

	/** O maior lado da caverna, para plantar duas sem uma comer a outra. */
	float FootprintUnits() const;

	/** A largura livre de um corredor. Menor que o jogador, ninguém anda. */
	float CorridorWidthUnits() const { return CellSizeUnits - WallThicknessUnits; }

	/** A altura das paredes de dentro. */
	float InnerWallHeightUnits() const { return WallHeightUnits; }

	/** A altura da muralha da borda — o que se vê de fora. */
	float OuterWallHeightUnits() const { return ShellHeightUnits; }

	UHierarchicalInstancedStaticMeshComponent* GetFloor() const { return Floor; }
	UHierarchicalInstancedStaticMeshComponent* GetWalls() const { return Walls; }
	UHierarchicalInstancedStaticMeshComponent* GetShell() const { return Shell; }

	/** Os tamanhos que o jogo usa — uma grande e uma pequena, de verdade. */
	static constexpr int32 LargeCaveSide = 11;
	static constexpr int32 MediumCaveSide = 7;
	static constexpr int32 SmallCaveSide = 5;

	/**
	 * As medidas com que uma caverna nasce.
	 *
	 * Estão aqui em cima, e não só como valor inicial do campo, porque QUEM
	 * ESCOLHE ONDE PLANTAR precisa saber o tamanho antes de existir uma
	 * caverna para medir. Transcrever os números no planejador faria as duas
	 * cópias concordarem até a primeira edição (L-032).
	 */
	static constexpr float DefaultCellSizeUnits = 240.0f;
	static constexpr float DefaultShellThicknessUnits = 120.0f;

	/** Lado do quadrado que uma caverna desse tamanho vai ocupar. */
	static float FootprintForSide(int32 Side)
	{
		return static_cast<float>(Side) * DefaultCellSizeUnits + 2.0f * DefaultShellThicknessUnits;
	}

protected:
	/** Largura de uma casa do labirinto. */
	UPROPERTY(EditAnywhere, Category = "Caverna")
	float CellSizeUnits = DefaultCellSizeUnits;

	UPROPERTY(EditAnywhere, Category = "Caverna")
	float WallHeightUnits = 340.0f;

	UPROPERTY(EditAnywhere, Category = "Caverna")
	float WallThicknessUnits = 40.0f;

	UPROPERTY(EditAnywhere, Category = "Caverna")
	float FloorThicknessUnits = 24.0f;

	/** A muralha da borda: o que se vê de fora. */
	UPROPERTY(EditAnywhere, Category = "Caverna")
	float ShellHeightUnits = 760.0f;

	UPROPERTY(EditAnywhere, Category = "Caverna")
	float ShellThicknessUnits = DefaultShellThicknessUnits;

private:
	void AddSlab(UHierarchicalInstancedStaticMeshComponent* Alvo,
		const FVector& Centro, const FVector& Tamanho);

	UPROPERTY()
	TObjectPtr<USceneComponent> CaveRoot;

	UPROPERTY()
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Floor;

	UPROPERTY()
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Walls;

	UPROPERTY()
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Shell;

	CaveLabyrinth::FCaveGrid Grid;
};

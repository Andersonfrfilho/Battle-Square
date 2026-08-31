// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "Environment/CaveFlavor.h"
#include "Environment/CaveLabyrinth.h"

#include "CaveSystem.generated.h"

class UHierarchicalInstancedStaticMeshComponent;
class UPointLightComponent;

/**
 * A receita de uma caverna: tudo o que decide como ela sai.
 *
 * Vira estrutura porque virou o quarto parâmetro. Quatro posicionais numa
 * chamada só produzem `BuildCave(5, 5, Semente, Sabor)`, em que trocar os dois
 * primeiros de lugar compila e sai errado — e a caverna errada só aparece
 * quando alguém anda até ela.
 */
struct BATTLESQUARE_API FCaveRecipe
{
	int32 Columns = 0;
	int32 Rows = 0;
	uint32 Seed = 0;

	/** Seco por omissão: é a caverna neutra, a que não promete nada. */
	ECaveFlavor Flavor = ECaveFlavor::Dry;
};

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
 *
 * **A boca também é o aviso.** Caverna que não se explora tem o vão TAPADO, e
 * a silhueta conta isso de longe. A alternativa seria deixar entrar e barrar na
 * porta — que é caminhar até lá para descobrir que não valia.
 */
UCLASS()
class BATTLESQUARE_API ACaveSystem : public AActor
{
	GENERATED_BODY()

public:
	ACaveSystem();

	/** Escava e constrói. Grade inválida não constrói nada — nunca meia caverna. */
	void BuildCave(const FCaveRecipe& Recipe);

	/** O que esta caverna tem dentro. */
	ECaveFlavor GetFlavor() const { return Flavor; }

	/** Se a boca ficou aberta. É `IsCaveExplorable(GetFlavor())`, e só. */
	bool IsExplorable() const { return IsCaveExplorable(Flavor); }

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

	/** As pontas de pedra: penduradas na verga, e de pé no chão. */
	UHierarchicalInstancedStaticMeshComponent* GetSpikes() const { return Spikes; }

	/** As poças — lava ou água, conforme o sabor. Vazio na caverna seca. */
	UHierarchicalInstancedStaticMeshComponent* GetPools() const { return Pools; }

	/**
	 * A brasa da caverna de lava.
	 *
	 * Existe sempre; só ACENDE no sabor de lava. Criar o componente conforme o
	 * sabor faria o ponteiro ser nulo em metade dos casos, e cada leitor teria
	 * de lembrar disso — lembrar é o que ninguém faz.
	 */
	UPointLightComponent* GetLavaGlow() const;

	/** Os tamanhos que o jogo usa — uma grande e uma pequena, de verdade. */
	static constexpr int32 LargeCaveSide = 11;
	static constexpr int32 MediumCaveSide = 7;
	static constexpr int32 SmallCaveSide = 5;

	/**
	 * A gruta: uma toca, não um labirinto.
	 *
	 * Menor que a pequena de propósito. Ela nasce ENCOSTADA numa cachoeira, e
	 * ali a ilha não tem folga: entre o lago que ainda alaga rio acima e a
	 * praia que começa mil e seiscentas unidades antes da orla sobra pouco mais
	 * de dois mil de terra. Um quadrado de lado cinco não cabe nessa faixa sem
	 * encostar a quina na água — e uma cavidade ao lado de uma queda d'água não
	 * deveria ser um labirinto de todo jeito.
	 */
	static constexpr int32 GrottoCaveSide = 3;

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

	/** Quantas pontas de pedra pendem da verga da boca. */
	UPROPERTY(EditAnywhere, Category = "Caverna")
	int32 MouthStalactiteCount = 5;

	/** A chance de uma casa qualquer ter uma ponta de pedra subindo do chão. */
	UPROPERTY(EditAnywhere, Category = "Caverna")
	float FloorStalagmiteChance = 0.34f;

	/** A chance de uma casa qualquer ter poça, nos sabores que têm poça. */
	UPROPERTY(EditAnywhere, Category = "Caverna")
	float PoolChance = 0.30f;

	/** A espessura da lâmina da poça. Poça funda vira piscina. */
	UPROPERTY(EditAnywhere, Category = "Caverna")
	float PoolThicknessUnits = 10.0f;

	/** A altura máxima de uma ponta de pedra, em fração da altura da parede. */
	UPROPERTY(EditAnywhere, Category = "Caverna")
	float StalactiteReachOfWall = 0.42f;

	/** A intensidade da brasa, quando o sabor é lava. */
	UPROPERTY(EditAnywhere, Category = "Caverna")
	float LavaGlowIntensity = 42000.0f;

private:
	void AddSlab(UHierarchicalInstancedStaticMeshComponent* Alvo,
		const FVector& Centro, const FVector& Tamanho);

	/**
	 * Pendura as pontas de pedra sob a verga da boca.
	 *
	 * É o único teto da caverna, e é onde quem chega decide se entra — o lugar
	 * em que a estalactite conta alguma coisa em vez de enfeitar.
	 */
	void HangMouthStalactites(const FVector& Boca, uint32 Seed);

	/** Uma ponta de pedra. `Apoio` é a base de quem sobe, o teto de quem pende. */
	void AddSpike(const FVector& Apoio, float Altura, bool bPendurada);

	UPROPERTY()
	TObjectPtr<USceneComponent> CaveRoot;

	UPROPERTY()
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Floor;

	UPROPERTY()
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Walls;

	UPROPERTY()
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Shell;

	/**
	 * As pontas de pedra, num componente só.
	 *
	 * Estalactite e estalagmite são o mesmo cone — uma de cabeça para baixo. Duas
	 * malhas para a mesma forma seria duplicar o que só difere por rotação.
	 *
	 * As penduradas ficam SÓ na boca, porque a boca é o único lugar com teto: a
	 * caverna é aberta em cima, e uma ponta pendurada no nada é uma pedra
	 * flutuando — o mesmo erro de usar a primitiva como se fosse a coisa.
	 */
	UPROPERTY()
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Spikes;

	/** As poças de lava ou de água, no chão dos corredores. */
	UPROPERTY()
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Pools;

	UPROPERTY()
	TObjectPtr<UPointLightComponent> LavaGlow;

	/** O sabor com que ela foi construída. */
	ECaveFlavor Flavor = ECaveFlavor::Dry;

	CaveLabyrinth::FCaveGrid Grid;
};

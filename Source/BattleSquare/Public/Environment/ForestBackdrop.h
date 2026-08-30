// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ForestBackdrop.generated.h"

class UHierarchicalInstancedStaticMeshComponent;
class UStaticMeshComponent;
class UMaterialInterface;

/**
 * A mata que fica ATRÁS da arena.
 *
 * O combate acontecia sobre o xadrez do template de mundo aberto, até o
 * horizonte: céu havia, chão havia, mas nada que dissesse ONDE a batalha se
 * passa. Este ator veste o entorno — chão de floresta, arbustos e pedras na
 * borda, e um paredão de árvores no fundo do enquadramento.
 *
 * Três decisões que não são de gosto:
 *
 * 1. **A escala sai da caixa medida da malha, nunca de um número por
 *    espécie.** Cada árvore do pacote tem uma altura diferente; pedir "esta
 *    árvore tem 6 casas de altura" e dividir pela caixa dela é o que faz o
 *    conjunto ficar coerente. Altura fixa por espécie foi o defeito que já
 *    afundou pet em laje três vezes neste projeto.
 * 2. **Nada nasce dentro do tabuleiro nem no caminho da câmera.** A mata é
 *    cenário; se ela entra na grade, vira regra de jogo por acidente.
 * 3. **A mesma semente dá a mesma mata.** Duas partidas com a mesma arena
 *    são a mesma cena, e uma captura de tela continua valendo amanhã.
 */
UCLASS()
class BATTLESQUARE_API AForestBackdrop : public AActor
{
	GENERATED_BODY()

public:
	AForestBackdrop();

	/**
	 * Espalha a mata em volta de um tabuleiro de `CellSize` por casa.
	 *
	 * Recebe o tamanho da casa em vez de guardar o seu: a arena é a fonte de
	 * verdade do espaçamento, e um segundo número aqui discordaria dela na
	 * primeira edição (L-032/L-033). Todos os raios e alturas da mata são
	 * múltiplos DESTA casa.
	 *
	 * `CameraGroundOffset` é onde a câmera pousa no chão, em espaço local:
	 * o vazio em volta dela é o que impede uma árvore de nascer colada na
	 * lente e tapar a batalha inteira.
	 */
	void BuildForest(float CellSize, uint32 Seed, const FVector2D& CameraGroundOffset);

	/** Espécies da mata, para o teste que exige asset atribuído em todas. */
	const TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>>& GetSpeciesClusters() const { return SpeciesClusters; }

	/** O disco de chão que cobre o xadrez do template. */
	UStaticMeshComponent* GetGroundMesh() const { return GroundMesh; }

	/**
	 * Altura LOCAL do topo do chão, em unidades.
	 *
	 * Existe para quem posiciona a mata poder encostá-la numa superfície
	 * conhecida sem repetir a espessura do disco — a mesma medida em dois
	 * lugares discorda na primeira edição (L-032/L-033).
	 */
	static float GroundTopLocalZ();

	/**
	 * Veste o chão com o material que o mundo emprestou.
	 *
	 * Nulo devolve o chão à paleta própria da mata. Quem empresta é a arena,
	 * ao sondar o terreno onde o encontro aconteceu.
	 */
	void SetGroundMaterialOverride(UMaterialInterface* Material);

	/** Quantas plantas foram de fato plantadas na última montagem. */
	int32 GetPlantedCount() const;

	/** Posição local de cada planta — o que o teste mede para saber se sobrou espaço. */
	TArray<FVector> GetPlantedLocations() const;

	/** Raio, em casas, que nenhuma planta invade — em volta do tabuleiro. */
	UPROPERTY(EditDefaultsOnly, Category = "Mata")
	float BoardClearanceInCells = 2.2f;

	/** Raio, em casas, que nenhuma planta invade — em volta da câmera. */
	UPROPERTY(EditDefaultsOnly, Category = "Mata")
	float CameraClearanceInCells = 3.0f;

private:
	/**
	 * Veste o chão: o material do mundo quando há um, a paleta quando não há.
	 *
	 * Num lugar só porque são dois momentos — a construção da mata e o
	 * empréstimo do chão, que chega depois — e duplicar a decisão faria as
	 * duas divergirem na primeira edição.
	 */
	void ApplyGroundMaterial();

	UPROPERTY()
	TObjectPtr<USceneComponent> ForestRoot;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> GroundMesh;

	/** Chão emprestado pelo mundo; vazio faz valer a paleta do cenário. */
	UPROPERTY()
	TObjectPtr<UMaterialInterface> AdoptedGroundMaterial;

	UPROPERTY()
	TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> SpeciesClusters;
};

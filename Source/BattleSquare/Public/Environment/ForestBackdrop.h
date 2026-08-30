// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Environment/ScenaryPalette.h"
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

	/**
	 * Este agrupamento tem CORPO — dá para esbarrar e para derrubar?
	 *
	 * Responde pela MESMA função que decide o que se derruba
	 * (`FWorldObstacleBreaking::StartingHealthFor`), e não por uma segunda
	 * lista: capim e flor não bloqueiam ninguém, e duas listas de "o que é
	 * sólido" concordariam até a primeira edição. O sintoma seria uma casa de
	 * arena bloqueada por um cogumelo.
	 */
	bool IsSolidSpecies(int32 SpeciesIndex) const;

	/**
	 * Os obstáculos EM PÉ perto de um ponto, com a vida que ainda têm.
	 *
	 * Só o que é obstáculo entra: capim e flor não são derrubáveis, e
	 * devolvê-los faria o golpe do jogador escolher grama por ela estar mais
	 * perto que a árvore.
	 *
	 * O ÍNDICE devolvido é opaco de propósito — quem bate não precisa saber
	 * que a mata guarda instâncias em agrupamentos por espécie, e depender
	 * disso amarraria o combate à forma como a mata é desenhada.
	 */
	TArray<struct FWorldObstacleCandidate> CollectObstaclesNear(
		const FVector& WorldLocation, float RadiusUnits, TArray<int32>& OutHandles) const;

	/**
	 * Tira vida de um obstáculo. Devolve true quando ele CAI neste golpe.
	 *
	 * Cair é sumir da tela e deixar de ser alvo; o que não pode acontecer é
	 * bater e nada mudar.
	 */
	bool DamageObstacle(int32 Handle, int32 Damage);

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

	/**
	 * Vida de cada obstáculo já golpeado, pela chave opaca.
	 *
	 * Só o que APANHOU entra no mapa: guardar a vida cheia das 400 instâncias
	 * desde o começo gastaria memória para dizer o que a tabela de papéis já
	 * diz. Ausente significa "inteiro".
	 */
	UPROPERTY()
	TMap<int32, int32> ObstacleHealthByHandle;

	/** O papel de cada agrupamento, na mesma ordem de SpeciesClusters. */
	TArray<EScenaryRole> SpeciesRoles;
};

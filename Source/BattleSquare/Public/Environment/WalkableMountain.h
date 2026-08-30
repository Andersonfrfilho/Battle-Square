// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "WalkableMountain.generated.h"

class UHierarchicalInstancedStaticMeshComponent;
class UStaticMeshComponent;

/**
 * Uma montanha da ILHA — a que se sobe, com trilha.
 *
 * Ela não é a serra do horizonte com outro nome. `AMountainRange` fica a seis
 * e onze quilômetros do centro, e a terra em que se pisa é um disco de sessenta
 * metros de raio cercado de água: aquela serra é fundo, e fundo não tem trilha
 * porque ninguém chega nele. Trilha só existe onde há chão para caminhar até a
 * base, e isso obrigou uma montanha NOVA, plantada dentro da ilha.
 *
 * O corpo é um cone que BLOQUEIA, e o bloqueio é o ponto: a encosta tem 58° de
 * inclinação contra os 44° que a engine deixa andar, então subir pela rocha não
 * é uma opção ruim, é uma opção que não existe. Quem quer o cume usa a trilha.
 *
 * A trilha é feita de PATAMARES, não de rampa. Rampa depende do ângulo caber no
 * limite de piso andável, e um ângulo que cabe na subida é um ângulo raso demais
 * para a montanha parecer montanha. Degrau de trinta unidades passa por baixo
 * dos quarenta e cinco de `MaxStepHeight` da engine, e o personagem sobe cada um
 * sem que ninguém precise mexer no movimento dele.
 */
UCLASS()
class BATTLESQUARE_API AWalkableMountain : public AActor
{
	GENERATED_BODY()

public:
	AWalkableMountain();

	/**
	 * Ergue a montanha e talha a trilha.
	 *
	 * A semente só desempata o que é aparência — o serpenteio da trilha em
	 * volta do eixo. A subida em si é geometria, não sorteio: uma trilha que
	 * às vezes sai íngreme demais é uma trilha que às vezes não é trilha.
	 */
	void BuildMountain(uint32 Seed);

	/** Altura do cume, em unidades de mundo. */
	float GetSummitUnits() const { return HeightUnits; }

	/** Raio da base, em unidades de mundo. */
	float GetBaseRadiusUnits() const { return BaseRadiusUnits; }

	/**
	 * Os patamares, na ordem em que se sobe.
	 *
	 * Exposto porque a subida precisa ser VERIFICÁVEL sem abrir o jogo: é aqui
	 * que o teste mede se o degrau cabe no passo e se um patamar alcança o
	 * seguinte. Trilha com um vão no meio passa em qualquer teste de contagem.
	 */
	const TArray<FTransform>& GetTrailSteps() const { return TrailSteps; }

	/** O patamar do cume — o último, e o único no eixo da montanha. */
	int32 GetSummitStepIndex() const { return TrailSteps.Num() - 1; }

	/** Raio do cone na altura dada. Zero acima do cume. */
	float RadiusAtHeight(float ZUnits) const;

	UStaticMeshComponent* GetBody() const { return Body; }
	UHierarchicalInstancedStaticMeshComponent* GetTrail() const { return Trail; }

	/**
	 * O degrau que a engine deixa subir andando.
	 *
	 * Vive aqui, e não solto no `.cpp`, porque é o número que o teste cobra:
	 * quem um dia dobrar a altura do patamar para "subir mais rápido" quebra o
	 * teste antes de quebrar o jogo.
	 */
	static constexpr float MaxWalkableRiseUnits = 40.0f;

private:
	/** Um patamar da trilha, em coordenadas locais da montanha. */
	void CarveStep(float ZUnits, float AngleRadians, float TangentialLengthUnits);

	UPROPERTY()
	TObjectPtr<USceneComponent> MountainRoot;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Body;

	UPROPERTY()
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Trail;

	/** Altura do cone. */
	UPROPERTY(EditAnywhere, Category = "Montanha")
	float HeightUnits = 2400.0f;

	/** Raio da base. */
	UPROPERTY(EditAnywhere, Category = "Montanha")
	float BaseRadiusUnits = 1500.0f;

	/** Até que fração da altura a trilha sobe. */
	UPROPERTY(EditAnywhere, Category = "Montanha")
	float TrailTopFraction = 0.86f;

	/** Quanto cada patamar sobe em relação ao anterior. */
	UPROPERTY(EditAnywhere, Category = "Montanha")
	float TrailRiseUnits = 30.0f;

	/** Quantas voltas a trilha dá em torno do eixo até o cume. */
	UPROPERTY(EditAnywhere, Category = "Montanha")
	float TrailTurns = 3.5f;

	/** Profundidade radial do patamar — metade entra na rocha, metade fica de fora. */
	UPROPERTY(EditAnywhere, Category = "Montanha")
	float TrailWidthUnits = 260.0f;

	/** Espessura do patamar. */
	UPROPERTY(EditAnywhere, Category = "Montanha")
	float TrailThicknessUnits = 24.0f;

	TArray<FTransform> TrailSteps;
};

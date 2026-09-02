// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FerryActor.generated.h"

class UStaticMeshComponent;

/**
 * A BALSA — geometria SÓLIDA que corre sobre a água e esbarra no que houver.
 *
 * Ela existe porque a travessia de balsa é INTERAÇÃO, e não obra. A ponte
 * resolve o rio ficando parada; a balsa resolve indo e voltando, e quem quer
 * atravessar espera por ela. Uma laje parada acima da lâmina não é balsa: é um
 * deck no meio do rio, e some justamente o que distingue os dois tipos que o
 * traçado separou.
 *
 * Três coisas fazem dela uma balsa, e nenhuma é a forma:
 *
 * 1. ela **anda** entre as duas margens;
 * 2. ela **flutua** — a lâmina manda na altura, não o leito;
 * 3. ela é **sólida** e ESBARRA no que estiver no caminho, em vez de
 *    atravessar por dentro.
 */
UCLASS()
class BATTLESQUARE_API AFerryActor : public AActor
{
	GENERATED_BODY()

public:
	AFerryActor();

	/**
	 * Põe a balsa na travessia dada.
	 *
	 * @param CenterUnits  o meio do percurso, onde a trilha cruza a água
	 * @param AxisUnits    o rumo da travessia, já normalizado
	 * @param SpanUnits    o vão a vencer — ela vai de meio vão a meio vão
	 * @param WaterZ       a altura da lâmina; a balsa flutua sobre ela
	 */
	void ConfigureFor(const FVector2D& CenterUnits, const FVector2D& AxisUnits,
		float SpanUnits, float WaterZ);

	UStaticMeshComponent* GetDeck() const { return Deck; }

	/**
	 * Avança a balsa por um tempo dado, e devolve se ela ANDOU.
	 *
	 * Recebe o tempo em vez de ler o relógio, e por isso é reproduzível: o
	 * teste anda a balsa em passos exatos, sem depender de quadro nem de
	 * quanto a máquina estava ocupada.
	 */
	bool AdvanceBy(float DeltaSeconds);

	/** Para que lado ela está indo: +1 ou -1 ao longo do eixo. */
	int32 GetHeading() const { return Heading; }

	/** Quanto ela já andou a partir do meio, ao longo do eixo. */
	float GetOffsetUnits() const { return OffsetUnits; }

	/** Quanto ela flutua acima da lâmina. */
	static float FreeboardUnits();

	/** A velocidade dela, em unidades por segundo. */
	static float SpeedUnitsPerSecond();

protected:
	virtual void Tick(float DeltaSeconds) override;

private:
	/** Leva a balsa ao ponto do percurso dado, VARRENDO — para ela esbarrar. */
	bool MoverPara(float NovoOffset);

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Deck;

	UPROPERTY()
	FVector2D Center = FVector2D::ZeroVector;

	UPROPERTY()
	FVector2D Axis = FVector2D(1.0f, 0.0f);

	UPROPERTY()
	float HalfSpanUnits = 0.0f;

	UPROPERTY()
	float SurfaceZ = 0.0f;

	UPROPERTY()
	float OffsetUnits = 0.0f;

	UPROPERTY()
	int32 Heading = 1;
};

// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Battle/FluidRegistry.h"
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
	/**
	 * Devolve `false` quando ela NÃO BOIA no fluido dado — e aí não há balsa.
	 *
	 * Uma balsa que afunda não é uma balsa parada: é uma travessia que não
	 * existe. Devolver a resposta é o que permite a travessia deixar de
	 * planejá-la, em vez de pôr no rio uma plataforma no fundo.
	 */
	bool ConfigureFor(const FVector2D& CenterUnits, const FVector2D& AxisUnits,
		float SpanUnits, float WaterZ, EFluidKind Fluid);

	/**
	 * A densidade do convés, em partes por mil.
	 *
	 * Madeira. É ela que decide onde a balsa pode existir, e por isso é um
	 * NÚMERO e não um booleano `bFloats`: o que boia depende dos dois lados, e
	 * a mesma pedra que afunda na água boia na lava.
	 */
	int32 GetDeckDensityPerMille() const { return DeckDensityPerMille; }

	/** Troca o material do convés. Balsa de pedra existe — só não em água. */
	void SetDeckDensityPerMille(int32 Density) { DeckDensityPerMille = Density; }

	/**
	 * A CORRENTE em que ela flutua: rumo já normalizado e força em partes por
	 * mil.
	 *
	 * Uma balsa que ignora a água em que boia é uma plataforma sobre trilhos.
	 * O que a acelera ou atrasa é a COMPONENTE da corrente ao longo do eixo da
	 * travessia — a parte perpendicular empurra de lado, e não muda quanto
	 * tempo ela leva para chegar do outro lado.
	 */
	void SetCurrent(const FVector2D& FlowDirection, int32 StrengthPerMille);

	/** Quanto o passo dela rende AGORA, contando a corrente e o rumo em que vai. */
	float CurrentSpeedUnitsPerSecond() const;

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

	/** Madeira: 600 por mil, contra os 1000 da água doce. */
	UPROPERTY()
	int32 DeckDensityPerMille = 600;

	UPROPERTY()
	FVector2D CurrentDirection = FVector2D::ZeroVector;

	UPROPERTY()
	int32 CurrentStrengthPerMille = 0;
};

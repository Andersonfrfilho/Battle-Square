// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "World/VillageResidents.h"

#include "VillagerActor.generated.h"

class UStaticMeshComponent;
class ABattleSceneLighting;

/**
 * O MORADOR NA RUA (decisão 66): quando não está em casa, está andando pela
 * vila — a janela de horário que já decidia quem atende agora decide quem se
 * VÊ.
 *
 * Corpo procedural, como todo o resto do mundo: cilindro de corpo, esfera de
 * cabeça, cor da roupa seedada. As malhas são atribuídas NO CONSTRUTOR — o
 * defeito três vezes pago deste projeto é o componente criado sem asset, que
 * passa em toda lógica e não existe na tela.
 *
 * O passeio é o MESMO componente dos encontros (`UEncounterRoamingComponent`):
 * um segundo andarilho escrito à mão divergiria do primeiro na primeira
 * edição.
 */
UCLASS()
class BATTLESQUARE_API AVillagerActor : public AActor
{
	GENERATED_BODY()

public:
	AVillagerActor();

	/**
	 * Quem é este morador, e onde ele passeia.
	 *
	 * `PlazaCenter`/`PlazaRadius` delimitam a rua dele: morador de vila anda
	 * pela vila — não pela mata, que é onde mora o encontro selvagem.
	 */
	void Configure(ESettlementKind Kind, int32 DoorIndex,
		const FVector& PlazaCenter, float PlazaRadius);

	const VillageResidents::FResident& GetResident() const { return Resident; }

	/**
	 * Aplica a hora do mundo: em casa, o corpo SOME da rua (ele está dentro,
	 * e dentro não se vê); fora da janela, ele anda. Separada do Tick para o
	 * teste decidir a hora sem esperar o céu girar.
	 */
	void ApplyHour(float Hour);

	UStaticMeshComponent* GetBody() const { return Body; }
	UStaticMeshComponent* GetHead() const { return Head; }

protected:
	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY()
	TObjectPtr<USceneComponent> VillagerRoot;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Body;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Head;

	UPROPERTY()
	TObjectPtr<class UEncounterRoamingComponent> Roaming;

	/** O céu, perguntado uma vez e guardado: varrer o mundo a cada tick não. */
	UPROPERTY(Transient)
	TObjectPtr<ABattleSceneLighting> CachedSky;

	VillageResidents::FResident Resident;
	bool bConfigured = false;
};

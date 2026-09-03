// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"
#include "World/VillageLayout.h"
#include "Village.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

/**
 * ALGUÉM ENTROU (ou saiu) da porta de um prédio.
 *
 * O evento carrega O QUE foi tocado e DE QUE assentamento, e nada mais. É de
 * propósito: quem escuta decide o que fazer, e o prédio não aprende a curar,
 * a cobrar nem a vender.
 *
 * Sem isso, a primeira interação escrita dentro do Centro de Recuperação
 * obrigaria as cinco seguintes a repetir o mesmo gatilho — o próprio
 * `tasks.md` avisa que escolher errado aqui custa seis vezes.
 */
/**
 * O ÍNDICE DA PORTA viaja junto (quarto parâmetro): dois prédios do mesmo tipo
 * — as quatro casas da vila — precisam ser distinguíveis, senão todo morador
 * seria o mesmo morador. É o índice em `GetDoors()`, estável para a mesma
 * vila erguida do mesmo traçado.
 */
DECLARE_MULTICAST_DELEGATE_FourParams(FOnVillageDoorCrossed,
	EVillageBuilding /*Predio*/, ESettlementKind /*DeQueVila*/, bool /*bEntrou*/,
	int32 /*DoorIndex*/);

/**
 * A vila em pé.
 *
 * Só CONSTRÓI: onde cada prédio fica é decisão de `VillageLayout`, que é puro e
 * testado. Aqui mora a parte que precisa de mundo — malha, material, colisão —
 * e que por isso não se verifica headless.
 *
 * Duas coisas são obrigatórias, e as duas por defeito que este projeto já
 * pagou:
 *
 * 1. **Toda malha é ATRIBUÍDA no construtor.** Componente criado sem asset
 *    passa em teste de lógica e não existe na tela — aconteceu três vezes, e
 *    hoje há sonda para isso.
 * 2. **Todo prédio BLOQUEIA.** O usuário jogou e relatou atravessar montanha e
 *    blocos. Cenário sólido sem colisão é a mesma família de defeito, e uma
 *    vila que se atravessa não é um lugar.
 */
UCLASS()
class BATTLESQUARE_API AVillage : public AActor
{
	GENERATED_BODY()

public:
	AVillage();

	/** Ergue a vila. Idempotente: chamar de novo refaz do zero. */
	void BuildVillage();

	/**
	 * Que assentamento este é. Decide o traçado, e por isso vem ANTES de
	 * `BuildVillage` — mudar depois de erguido não move prédio nenhum.
	 */
	void SetSettlementKind(ESettlementKind NewKind) { Kind = NewKind; }

	ESettlementKind GetSettlementKind() const { return Kind; }

	/** Quantos prédios existem em cena. */
	int32 GetBuiltCount() const { return BuiltMeshes.Num(); }

	const TArray<TObjectPtr<UStaticMeshComponent>>& GetBuiltMeshes() const
	{
		return BuiltMeshes;
	}

	/**
	 * As PORTAS: um volume de gatilho por prédio que tem função.
	 *
	 * Menos que os prédios, sempre — Casa, Palafita, Passarela e Chinampa não
	 * têm porta, e não tê-la é a regra, não uma falta.
	 */
	const TArray<TObjectPtr<UBoxComponent>>& GetDoors() const { return Doors; }

	/** De que prédio é a porta de índice `Qual`, na mesma ordem de `GetDoors`. */
	EVillageBuilding GetDoorBuilding(int32 Which) const;

	/** Alguém cruzou uma porta. Quem escuta decide o que isso significa. */
	FOnVillageDoorCrossed OnDoorCrossed;

	/**
	 * A cor de UM prédio — exposta para o teste ler.
	 *
	 * A tabela vive no `.cpp`, e o teste da invariante 16 precisa alcançá-la:
	 * Palafita, Passarela e Chinampa já caíram no cinza do `default` uma vez,
	 * em silêncio, e só o olho pegou. Sem esta janela o teste releria a tabela
	 * copiando-a — e duas cópias concordam até a primeira edição.
	 */
	static FLinearColor BuildingColor(EVillageBuilding Building);

	/** O cinza do `default` — a cor de "ninguém escreveu o case ainda". */
	static FLinearColor FallbackColor();

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	ESettlementKind Kind = ESettlementKind::VilaInicial;

	UPROPERTY()
	TObjectPtr<USceneComponent> VillageRoot;

	/** Um componente por prédio, na ordem do traçado. */
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> BuiltMeshes;

	/** O telhado de cada prédio — o que faz a vila se ler de longe. */
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> BuiltRoofs;

	UPROPERTY()
	TObjectPtr<UStaticMesh> BoxMesh;

	/**
	 * As portas, e elas NÃO BLOQUEIAM.
	 *
	 * Volume de gatilho que bloqueia é parede: emparedaria justamente o prédio
	 * que ele existe para deixar usar. Só `QueryOnly`, e só sobreposição.
	 */
	UPROPERTY()
	TArray<TObjectPtr<UBoxComponent>> Doors;

	/** De que prédio é cada porta, na mesma ordem. */
	TArray<EVillageBuilding> DoorBuildings;

	UFUNCTION()
	void HandleDoorBegin(UPrimitiveComponent* Porta, AActor* Quem,
		UPrimitiveComponent* Outro, int32 Corpo, bool bVarredura,
		const FHitResult& Batida);

	UFUNCTION()
	void HandleDoorEnd(UPrimitiveComponent* Porta, AActor* Quem,
		UPrimitiveComponent* Outro, int32 Corpo);

	/** O caminho único dos dois: só o SENTIDO muda. */
	void AnunciarPorta(UPrimitiveComponent* Porta, AActor* Quem, bool bEntrou);
};

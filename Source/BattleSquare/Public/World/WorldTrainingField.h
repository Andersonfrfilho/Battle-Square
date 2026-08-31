// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Environment/ScenaryPalette.h"
#include "GameFramework/Actor.h"
#include "WorldTrainingField.generated.h"

class UHierarchicalInstancedStaticMeshComponent;
class UStaticMeshComponent;

/**
 * As peças da mata que MARCAM um campo de treino.
 *
 * Uma estrutura e não três funções soltas porque o marco, o anel e o papel de
 * cor do marco são a mesma decisão vista de três lados: separá-los abriria a
 * porta para o campo de voo ganhar a árvore alta e continuar com o anel das
 * pedras do campo de força.
 */
struct FTrainingFieldMarker
{
	/** A peça grande do centro — é ela que se vê de longe. */
	FString MarkerSpecies;

	/** A peça pequena que se repete na BORDA, dizendo até onde o campo vai. */
	FString BorderSpecies;

	/** Com que cor da paleta o marco do centro é pintado. */
	EScenaryRole MarkerRole = EScenaryRole::Rock;
};

/**
 * Um lugar no mundo que treina UM atributo enquanto o jogador está nele.
 *
 * É o que dá DESTINO ao mundo aberto. Até aqui ele só servia para encontrar
 * inimigos, e caminhar até um lugar por um motivo é o que separa mapa de
 * corredor.
 *
 * O tempo que conta é o de PERMANÊNCIA COM O JOGO ABERTO, não o do relógio do
 * sistema — ver o comentário de ABattleSquareGameMode::TickTrainingFields.
 *
 * ELE NÃO É UM DISCO PINTADO. Era, e o disco marrom de 800 unidades de lado
 * deitado na grama foi a primeira coisa que o jogador apontou na tela como
 * "uma forma redonda marrom que pode sair, não tem porque". Uma elipse chapada
 * de cor lisa não pertence a mata nenhuma: ela anuncia que é interface, não
 * lugar. Agora o campo é uma CLAREIRA — uma peça grande no meio e um anel de
 * peças pequenas na borda, todas do mesmo pacote que planta a floresta em
 * volta. A cor do atributo continua existindo, e continua sendo o que ensina
 * o lugar antes de qualquer texto; ela só saiu do chão e foi para as pedras.
 */
UCLASS()
class BATTLESQUARE_API AWorldTrainingField : public AActor
{
	GENERATED_BODY()

public:
	AWorldTrainingField();

	/**
	 * Qual atributo este campo treina, no MESMO vocabulário do requisito de
	 * golpe ("musculature", "flight", …).
	 *
	 * Uma segunda lista de nomes produziria um campo que treina algo que
	 * nenhum golpe exige — treino que nunca destrava nada, e sem nada
	 * apontando a causa.
	 */
	UPROPERTY(EditAnywhere, Category = "Treino")
	FString TrainedAttribute = TEXT("musculature");

	/** Raio em que a permanência conta. */
	UPROPERTY(EditAnywhere, Category = "Treino")
	float FieldRadiusUnits = 400.0f;

	UStaticMeshComponent* GetFieldMesh() const { return FieldMesh; }

	UHierarchicalInstancedStaticMeshComponent* GetBorderProps() const { return BorderProps; }

	/**
	 * Monta o marco e o anel a partir do atributo que este campo treina.
	 *
	 * PÚBLICO e idempotente de propósito. Quem cria os campos atribui o
	 * atributo DEPOIS de pedir o ator, e `BeginPlay` já correu nessa hora —
	 * o resultado era cinco campos montados com o atributo padrão, ou seja
	 * cinco clareiras iguais dizendo que treinavam coisas diferentes. Depender
	 * da ordem do nascimento é depender de algo que não se vê no código de quem
	 * chama; poder remontar não depende de nada.
	 */
	void RebuildMarker();

	/** Cor do campo, escolhida pelo atributo — o mesmo atributo, a mesma cor. */
	static FLinearColor ColorForAttribute(const FString& Attribute);

	/** Que peças marcam o campo de um atributo. Uma tabela, um lugar. */
	static FTrainingFieldMarker MarkerForAttribute(const FString& Attribute);

	bool IsInside(const FVector& WorldLocation) const;

protected:
	virtual void BeginPlay() override;

private:
	/** Troca a malha do centro pela do atributo e a põe na escala do campo. */
	void ApplyCenterMarker(const FTrainingFieldMarker& Marker);

	/** Semeia o anel da borda, que é o que substituiu o contorno do disco. */
	void ApplyBorderRing(const FTrainingFieldMarker& Marker);

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> FieldMesh;

	UPROPERTY()
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> BorderProps;
};

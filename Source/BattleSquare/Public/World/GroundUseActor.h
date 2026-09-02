// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "World/LandUseLayout.h"
#include "GroundUseActor.generated.h"

class UStaticMeshComponent;

/**
 * O USO DO SOLO — um ator PARAMETRIZADO, e não quinze classes.
 *
 * Quinze classes para quinze usos que diferem em forma, cor e tamanho seriam
 * quinze construtores repetindo a mesma coisa, e a décima sexta nasceria por
 * cópia da mais parecida — junto com o defeito dela. O que muda entre um pomar
 * e um cemitério é DADO, e dado mora numa tabela.
 *
 * A tabela é a fonte ÚNICA: quem constrói o ator e quem conta o mundo leem a
 * mesma linha. Duas tabelas concordam até a primeira edição.
 */
UCLASS()
class BATTLESQUARE_API AGroundUseActor : public AActor
{
	GENERATED_BODY()

public:
	AGroundUseActor();

	/**
	 * Veste o ator com a mancha dada.
	 *
	 * Devolve `false` quando o uso não tem linha na tabela — e não silencia:
	 * uso sem forma é uma mancha invisível no mapa, que é o defeito que esta
	 * feature inteira existe para não repetir.
	 */
	bool ConfigureFor(const FGroundUsePatch& Patch);

	UStaticMeshComponent* GetBody() const { return Body; }

	EGroundUse GetUse() const { return Use; }

	/** De que deus, quando é templo ou ruína. */
	EDeity GetDeity() const { return Deity; }

	/** O poço deu água. Só faz sentido para `Poco`, e é falso no resto. */
	bool YieldsWater() const { return bYieldsWater; }

	/** O nome do uso, para o painel. */
	static const TCHAR* UseDebugName(EGroundUse Use);

	/** A malha que a tabela dá a este uso. Vazio quer dizer "sem linha". */
	static const TCHAR* MeshPathFor(EGroundUse Use);

private:
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Body;

	UPROPERTY()
	EGroundUse Use = EGroundUse::Nenhum;

	UPROPERTY()
	EDeity Deity = EDeity::MaeNatureza;

	UPROPERTY()
	bool bYieldsWater = false;
};

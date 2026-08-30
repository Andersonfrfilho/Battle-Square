// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UWorld;

/**
 * O que o mundo ainda está preparando, e o que já ficou pronto.
 *
 * Estado puro: quem preenche é o GameMode, quem desenha é a tela. Separar
 * permite testar "a tela some quando tudo fica pronto" sem abrir o editor.
 */
struct BATTLESQUARE_API FWorldLoadingProgress
{
	bool bTypeCatalogReady = false;
	bool bMirrorVerified = false;
	bool bSceneryBuilt = false;
	bool bEncountersPlaced = false;

	/** Preenchido quando a montagem falhou DE VEZ. */
	FString PermanentProblem;

	bool IsEverythingReady() const
	{
		return bTypeCatalogReady && bMirrorVerified && bSceneryBuilt && bEncountersPlaced;
	}

	bool HasFailed() const { return !PermanentProblem.IsEmpty(); }

	/** A linha que o jogador lê: o passo ATUAL, não a lista inteira. */
	FText DescribeCurrentStep() const;

	/** Quanto do caminho já foi, em porcentagem. */
	int32 PercentComplete() const;
};

/**
 * Cobre a tela enquanto o mundo se monta.
 *
 * Existe porque a montagem NÃO acontece no BeginPlay: ela depende do pawn do
 * jogador e roda no Tick, com repetição. Nesse intervalo o jogador nascia num
 * mundo vazio e cinza, e depois levava um engasgo quando a mata, o sol, os
 * encontros e os campos apareciam todos no mesmo quadro — junto com a
 * decifragem do espelho e a verificação de assinatura de cada pet.
 *
 * ELA ESPERA A COISA CERTA, e isso não é detalhe: uma tela de tempo fixo
 * esconderia a espera real e mostraria uma falsa. Esta só sai quando o mundo
 * está de fato montado — e, se a montagem falhar de vez, ela DIZ o motivo em
 * vez de girar para sempre.
 *
 * Slate por código, sem asset autorado: é o caminho que funciona neste projeto
 * hoje, e uma tela de carregamento que espera edição de asset não carrega nada.
 * Compilada TAMBÉM em Shipping — diferente da barra de depuração, esta é parte
 * do jogo.
 */
class BATTLESQUARE_API FWorldLoadingScreen
{
public:
	static void Show(UWorld* World);

	/** Atualiza o texto. Sem efeito se a tela não estiver visível. */
	static void Update(const FWorldLoadingProgress& Progress);

	static void Hide();

	static bool IsVisible();
};

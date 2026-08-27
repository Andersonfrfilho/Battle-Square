// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FBattleEvent;

/**
 * Traduz um evento do núcleo para uma frase que o jogador entende.
 *
 * DP-leg-02: isto TRADUZ, não reinterpreta. A narração nunca decide se um
 * ataque deveria ter acertado — ela conta o que o núcleo já resolveu. Decidir
 * de novo aqui criaria uma segunda fonte de verdade, e L-032/L-033 mostraram
 * o preço de duas cópias da mesma regra.
 */
class BATTLESQUARE_API FBattleNarration
{
public:
	/**
	 * DP-leg-01: função pura — sem estado, sem arena, sem efeito. É o que a
	 * torna testável sem abrir o editor.
	 *
	 * Devolve texto VAZIO para evento de contabilidade (início/fim de slot):
	 * feed que narra tudo não narra nada, porque a frase útil some no meio.
	 *
	 * FText e não FString: é o que a ferramenta de localização consegue
	 * coletar. String cravada não é traduzível por definição — não há de onde
	 * a coleta tirar a chave.
	 */
	static FText Describe(const FBattleEvent& Event, const FString& ActorName, const FString& TargetName);
};

/**
 * As últimas frases do combate, para o rodapé da tela.
 *
 * Produto, não depuração: compila em Shipping e não some com
 * `bs.ShowBattleDebug 0`. O painel de depuração vive ao lado, com outro
 * público — um explica a partida, o outro explica o programa.
 */
class BATTLESQUARE_API FBattleNarrationFeed
{
public:
	struct FLine
	{
		FText Text;
		FColor Color = FColor::White;
	};

	/** Teto fixo pelo mesmo motivo do painel: o que cresce sem limite não cabe. */
	static constexpr int32 MaxLines = 5;

	static void Push(const FText& Text, const FColor& Color);
	static void Clear();
	static const TArray<FLine>& GetLines();
};

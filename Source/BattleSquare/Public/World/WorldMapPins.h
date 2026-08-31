// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WorldMapPins.generated.h"

/**
 * O que uma marcação SIGNIFICA.
 *
 * Poucas, e com sentido declarado, em vez de "pino 1, 2, 3": um mapa cheio de
 * pontos idênticos volta a ser um mapa que não diz nada, e a marcação existe
 * justamente para o jogador escrever no mapa o que o jogo não sabe.
 *
 * Ao FIM do enum é onde entra qualquer significado novo: estes valores vão
 * para o SAVE, e inserir no meio reinterpretaria as marcações de quem já jogou.
 */
UENUM()
enum class EWorldPinKind : uint8
{
	/** Vale voltar: um campo, uma clareira, qualquer coisa que interessou. */
	Interesse,
	/** Perigo — o lugar onde a coisa deu errado. */
	Perigo,
	/** Ainda não fui até ali: o pino de intenção, e não de memória. */
	Destino
};

USTRUCT()
struct BATTLESQUARE_API FWorldMapPin
{
	GENERATED_BODY()

	UPROPERTY()
	FVector2D WorldXY = FVector2D::ZeroVector;

	UPROPERTY()
	EWorldPinKind Kind = EWorldPinKind::Interesse;
};

/**
 * As marcações do jogador no mapa.
 *
 * PURA e testável: a regra que importa aqui não é de desenho, é de gesto —
 * marcar em cima de uma marcação APAGA. Sem isso, apagar precisaria de um
 * segundo gesto (um modo, uma tecla, um menu), e cada gesto a mais é uma
 * chance de o caminho não ser encontrado. Este projeto já perdeu três rodadas
 * exatamente assim.
 */
USTRUCT()
struct BATTLESQUARE_API FWorldMapPins
{
	GENERATED_BODY()

	/**
	 * A que distância uma marcação nova conta como "a mesma".
	 *
	 * Do tamanho de uma região de descoberta: é a granularidade em que o mapa
	 * inteiro raciocina, e usar outra faria o jogador acertar o apagar numa
	 * escala e errar na outra.
	 */
	static constexpr float SamePlaceUnits = 800.0f;

	/**
	 * Teto de marcações.
	 *
	 * Existe porque o save é gravado a cada mudança e um mapa com centenas de
	 * pinos deixa de ser legível — vira o ruído que a legenda tenta desfazer.
	 * Ao bater no teto a marcação é RECUSADA com aviso, e não silenciosamente
	 * descartada nem trocada pela mais antiga: perder uma marcação que se pôs
	 * de propósito, sem nada dizer, é o pior dos três.
	 */
	static constexpr int32 MaxPins = 24;

	UPROPERTY()
	TArray<FWorldMapPin> Pins;

	/** O que aconteceu ao marcar — é o que a tela precisa anunciar. */
	enum class EResult : uint8
	{
		Posta,
		Apagada,
		Cheio
	};

	/**
	 * Marca aqui — ou apaga, se já houver marcação neste lugar.
	 *
	 * UM gesto para as duas coisas. Marcar duas vezes no mesmo lugar é o jeito
	 * natural de dizer "esquece", e exigir um modo de apagar seria construir a
	 * porta que ninguém acha.
	 */
	EResult ToggleAt(const FVector2D& WorldXY, EWorldPinKind Kind);

	/** A marcação neste lugar, ou nullptr. */
	const FWorldMapPin* FindAt(const FVector2D& WorldXY) const;
};

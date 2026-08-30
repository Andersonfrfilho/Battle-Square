// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * A forma do adorno na cabeça do pet. Só a FORMA: qual malha da engine
 * desenha cada uma é assunto de APetView, e misturar as duas coisas aqui
 * faria esta tabela depender de conteúdo carregado.
 */
enum class EPetCrestShape : uint8
{
	Orelha,
	OrelhaCaida,
	Chama,
	Barbatana,
	Folha,

	/**
	 * As quatro formas dos tipos acrescentados em 30/08/2026.
	 *
	 * Acrescentadas ao FIM, como todo enum deste projeto: os valores
	 * existentes viajam em dado salvo, e inserir no meio reinterpretaria o
	 * que já foi gravado.
	 *
	 * A SILHUETA passou a carregar o tipo, e a cor só reforça. Com três
	 * tipos, matiz bastava; com sete, o olho não separa sete matizes num
	 * bicho em movimento — mas separa quatro contornos.
	 */
	Antena,   // Inseto — cilindro fino e alto
	Orbe,     // Psíquico — esfera que não toca o corpo
	Cristal,  // Caverna — cubo, a única forma angular
	Ponta     // Mágico — cone, apontando ao contrário
};

/**
 * Como um pet de cada TIPO se parece.
 *
 * O tipo decide a skill (Config/PetTypes.json, DP-skill-01) e a vantagem
 * (Config/TypeEffectiveness.json) — e até aqui não decidia nada na tela.
 * Um pet cujo tipo só existe no texto do painel obriga o jogador a decorar
 * de quem é cada esfera.
 *
 * A cor do CORPO continua sendo do LADO (azul é meu, vermelho é do outro):
 * saber de quem é o pet vale mais, em combate, do que saber o tipo dele.
 *
 * O tipo entra SÓ pelo adorno, que fica acima da linha do corpo e por isso
 * continua legível no ângulo do diorama. A cauda já foi de cor de acento e
 * voltou a ser do lado: no tipo neutro o acento é quase branco, e uma cauda
 * clara saindo de um corpo azul não lia como cauda — lia como asa.
 */
struct BATTLESQUARE_API FPetAppearance
{
	FLinearColor AccentColor = FLinearColor(0.85f, 0.85f, 0.85f);
	EPetCrestShape CrestShape = EPetCrestShape::Orelha;

	/** Escala do adorno sobre a malha de 100uu da engine. */
	FVector CrestScale = FVector(0.16f, 0.16f, 0.26f);

	/** Inclinação do adorno esquerdo. O direito espelha o Roll. */
	FRotator CrestRotation = FRotator(0.0f, 0.0f, -18.0f);

	/**
	 * Tipo desconhecido devolve a aparência neutra em vez de falhar: o
	 * catálogo de pets é dado assinado que pode ganhar tipo novo sem
	 * recompilar, e um pet invisível seria pior que um pet genérico.
	 */
	static FPetAppearance ForType(const FString& PetType);
};

// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FPetAppearance;

/**
 * A fase de crescimento de um pet.
 *
 * Não é uma escala de tamanho: é um conjunto de proporções. Filhote não é o
 * adulto encolhido — tem cabeça grande, focinho curto, perna curta, adorno
 * pequeno e cor pálida. Isso é neotenia, e é o sinal que o olho reconhece
 * como "novo" sem precisar de legenda.
 */
enum class EPetGrowthStage : uint8
{
	Filhote,
	Adulto,
	Evoluido
};

/**
 * O PLANO DO CORPO de um pet, montado a partir da identidade dele.
 *
 * Nenhum bicho deste jogo é um animal real: cada um é inventado na hora, a
 * partir do id de catálogo. Dois pets diferentes têm proporções diferentes;
 * o MESMO pet tem sempre o mesmo corpo, em toda partida e em toda máquina —
 * é hash puro, sem relógio e sem FMath::Rand.
 *
 * O modelo é o do No Man's Sky, não o do Spore: as PEÇAS são fixas e feitas à
 * mão, e o que varia é proporção, encaixe e paleta. Spore gera o esqueleto e
 * reaproveita animação por cima do que sair, o que é pesquisa; isto aqui é
 * engenharia comum, e roda hoje.
 *
 * ── Variação COM SENTIDO ───────────────────────────────────────────────────
 *
 * Sortear cada medida por conta própria não produz criatura, produz colagem:
 * corpo comprido em perna curta arrasta a barriga no chão, adorno maior que a
 * cabeça vira chapéu, cabeça maior que o tronco vira boneco de mola. Por isso
 * os traços aqui são CORRELACIONADOS — dois eixos latentes (robustez e
 * alongamento) mandam nos demais — e cada limite é DERIVADO da geometria do
 * bicho, nunca um número escolhido a dedo:
 *
 *   - a perna nunca fica mais curta do que o necessário para a barriga passar;
 *   - a pata nunca se afasta para fora da elipse do corpo;
 *   - a cabeça nunca solta do tronco nem passa da largura dele;
 *   - o adorno nunca ultrapassa o tamanho da cabeça que o sustenta;
 *   - a barra de vida sempre sobra por cima da parte mais alta.
 *
 * Cada uma dessas frases é um teste que varre milhares de sementes. Limite que
 * não tem teste é limite que a primeira semente nova atravessa.
 *
 * ── Crescer é o MESMO plano, não outro ─────────────────────────────────────
 *
 * A fase entra como parâmetro do gerador, e não como um segundo gerador: o
 * mesmo id em Filhote, Adulto e Evoluido devolve o MESMO bicho em três
 * idades, com a mesma forma de adorno e a mesma família de cor. Duas funções
 * separadas concordariam até a primeira edição (L-032, L-033), e o desacordo
 * aqui seria o pet virar outro pet ao evoluir.
 *
 * Por isso a fase está na assinatura desde o primeiro chamador. Enfiá-la
 * depois obrigaria a mexer em todo mundo, e é a mesma lição de pôr a costura
 * antes do asset.
 *
 * ── O que NÃO varia ────────────────────────────────────────────────────────
 *
 * A cor do LADO (azul/vermelho) e a FORMA do adorno ficam de fora: a primeira
 * diz de quem é o pet, a segunda diz o tipo dele. Isso é informação, e
 * informação que varia por bicho — ou por idade — deixa de informar. Varia a
 * decoração, não o que a tela precisa comunicar.
 */
struct BATTLESQUARE_API FPetMorphology
{
	/** Escala do elipsoide do tronco, sobre a primitiva de 100uu da engine. */
	FVector BodyScale = FVector(0.66f, 0.52f, 0.48f);

	/** Altura do centro do tronco. Sai da perna, não é escolhida. */
	float BodyCenterUnits = 44.0f;

	/** Afastamento das patas, em unidades — sempre dentro da elipse do corpo. */
	float LegSpreadXUnits = 16.0f;
	float LegSpreadYUnits = 13.0f;

	/** Grossura da pata, escala da primitiva. Acompanha o porte. */
	float LegThicknessScale = 0.11f;

	/** Do chão à barriga, no ponto onde a pata encosta. */
	float LegClearanceUnits = 26.8f;

	float HeadScale = 0.36f;
	float HeadForwardUnits = 30.0f;
	float HeadCenterUnits = 58.0f;

	/** Focinho: tamanho e posição, ambos presos ao raio da cabeça. */
	float SnoutScale = 0.14f;
	FVector SnoutLocation = FVector(18.0f, 0.0f, -4.0f);

	FVector TailScale = FVector(0.16f, 0.16f, 0.40f);
	FVector TailLocation = FVector(-34.0f, 0.0f, 44.0f);
	FRotator TailRotation = FRotator(30.0f, 0.0f, 0.0f);

	/** Tamanho e tombo do adorno. A FORMA continua vindo do tipo. */
	FVector CrestScale = FVector(0.16f, 0.16f, 0.26f);
	FRotator CrestRotation = FRotator(0.0f, 0.0f, -18.0f);

	/** O tom do tipo, deslocado dentro da mesma família de cor. */
	FLinearColor AccentColor = FLinearColor(0.85f, 0.85f, 0.85f);

	/**
	 * Monta o corpo a partir de uma identidade estável e de uma fase.
	 *
	 * A semente é o id de catálogo do pet (FPetPresentationInfo::CatalogId),
	 * não o PetId da partida: dentro de uma batalha o PetId é só 1 ou 2, e
	 * semear por ele daria os MESMOS dois bichos em toda batalha do jogo.
	 *
	 * Semente vazia devolve o corpo neutro, que é o de hoje. Pet sem id ainda
	 * é um pet, e ele precisa aparecer na tela.
	 */
	static FPetMorphology FromSeed(
		const FString& Seed,
		const FPetAppearance& Appearance,
		EPetGrowthStage Stage = EPetGrowthStage::Adulto);

	/** Semieixos do elipsoide do tronco, já em unidades. */
	float BodySemiAxisXUnits() const;
	float BodySemiAxisYUnits() const;
	float BodySemiAxisZUnits() const;

	float HeadRadiusUnits() const;

	/** Altura da superfície de baixo do tronco no ponto da pata. */
	float BodyUnderSurfaceAtLegUnits() const;

	/** O ponto mais baixo do tronco — o meio da barriga, que pende mais. */
	float BodyLowestPointUnits() const;

	/** Comprimento da pata, já contando o pedaço que entra no corpo. */
	float LegHeightUnits() const;

	/**
	 * A parte mais alta do bicho, adorno incluído, mais a folga da barra.
	 *
	 * É o que a barra de vida usa para ficar SEMPRE por cima: com bichos de
	 * alturas diferentes, uma altura fixa de barra atravessa a cabeça dos
	 * altos e flutua longe dos baixos.
	 */
	float CrownUnits() const;
};

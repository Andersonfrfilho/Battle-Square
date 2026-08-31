// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Environment/IslandFeatureLayout.h"

/**
 * A água DOCE da ilha: os rios, os lagos e as quedas.
 *
 * O mar já existia — é o anel de `AWorldBoundaryWater` na borda. Faltava a
 * água que se bebe, e ela não é o mar em outro lugar: o mar é um limite (não
 * se entra nele, ele diz onde o mundo acaba), enquanto o rio é um caminho
 * (atravessa a ilha, se acompanha, se cruza).
 *
 * Um rio nasce numa MONTANHA e morre no mar, porque é essa a única versão que
 * não precisa ser explicada: água desce. A nascente não é inventada — sai do
 * mesmo `IslandFeatureLayout::Plan()` que planta os montes, e por isso um monte
 * que mudar de lugar leva o rio junto em vez de deixá-lo brotando do nada.
 *
 * O LAGO não é outra coisa: é o rio ficando largo. Fossem duas entidades,
 * bastaria alguém editar uma para o lago descolar do rio e virar uma poça no
 * meio do mato — e essa é exatamente a família de defeito que já custou L-032 e
 * L-033 aqui. Uma função de largura, um curso só.
 *
 * Tudo aqui é função pura sobre números: o curso não é ator, não é sorteio de
 * partida e não guarda estado. Quem desenha (o `AForestBackdrop` de cada
 * pedaço) pergunta "por onde passa o rio aqui dentro" e recebe a resposta.
 */
namespace FreshWater
{
	/**
	 * Um curso d'água inteiro, da nascente à foz.
	 *
	 * Ele é descrito em coordenadas POLARES — raio a partir do centro da ilha,
	 * e um rumo que varia com o raio — porque descer a montanha é literalmente
	 * afastar-se do centro. Em coordenadas cartesianas a mesma curva precisaria
	 * de um ponto de controle a cada dobra, e cada ponto seria uma chance de o
	 * rio subir de volta o morro.
	 */
	struct BATTLESQUARE_API FRiverCourse
	{
		/** Raio da nascente, na saia do monte. */
		float SourceRadiusUnits = 0.0f;

		/** Raio da foz, na linha da costa. */
		float MouthRadiusUnits = 0.0f;

		/** O rumo da nascente, em radianos, medido do centro da ilha. */
		float BearingRadians = 0.0f;

		/** Quanto o rumo abre para cada lado ao longo do percurso. */
		float MeanderRadians = 0.0f;

		/** Quantas curvas completas ele faz da nascente à foz. */
		float MeanderTurns = 0.0f;

		/** Em que raio ele alaga e vira lago. */
		float LakeRadiusUnits = 0.0f;

		/** Em que raio ele despenca. */
		float FallRadiusUnits = 0.0f;
	};

	/** Largura de meia calha do rio comum. */
	BATTLESQUARE_API float RiverHalfWidthUnits();

	/** Largura de meia calha no meio do lago. */
	BATTLESQUARE_API float LakeHalfWidthUnits();

	/** Até onde, para cada lado do raio da queda, a água ainda está caindo. */
	BATTLESQUARE_API float FallHalfLengthUnits();

	/** A que distância da margem corre a trilha que acompanha o rio. */
	BATTLESQUARE_API float TrailOffsetUnits();

	/** Largura de meia trilha. */
	BATTLESQUARE_API float TrailHalfWidthUnits();

	/** Todos os rios da ilha, um por montanha. */
	BATTLESQUARE_API TArray<FRiverCourse> Plan();

	/**
	 * Quantos rios desce cada monte.
	 *
	 * Existe para o teste perguntar em vez de transcrever. Um monte derrama
	 * para os dois flancos, e um rio por monte deixava a ilha com três fios
	 * de água que nenhuma trilha jamais cruzava.
	 */
	BATTLESQUARE_API int32 RiversPerMountain();

	/** Onde, no plano do mundo, o rio cruza este raio. */
	BATTLESQUARE_API FVector2D PointAt(const FRiverCourse& Course, float RadiusUnits);

	/**
	 * Quão largo o rio está neste raio.
	 *
	 * É AQUI que o lago existe: perto de `LakeRadiusUnits` a função sobe até
	 * `LakeHalfWidthUnits()` e volta, com transição suave. Lago é largura.
	 */
	BATTLESQUARE_API float HalfWidthAt(const FRiverCourse& Course, float RadiusUnits);

	/** Se neste raio a água está despencando. */
	BATTLESQUARE_API bool IsFallAt(const FRiverCourse& Course, float RadiusUnits);

	/**
	 * As grutas das cachoeiras — uma ao lado de cada queda.
	 *
	 * Ela sai como `FFeaturePlacement` de caverna, e não como um tipo novo,
	 * porque é UMA CAVERNA: quem planta já sabe plantar caverna, e `Overlaps` e
	 * `FitsOnLand` já sabem conferi-la. Um tipo próprio obrigaria a escrever de
	 * novo o mesmo despacho e as mesmas conferências, e a segunda cópia
	 * concorda com a primeira até alguém mexer numa delas (L-032).
	 *
	 * O caminho é de mão única: a gruta LÊ o plano da ilha, e o plano da ilha
	 * nunca lê a água. Acrescentar a gruta ao `IslandFeatureLayout::Plan()`
	 * fecharia o ciclo, já que é dele que os rios nascem.
	 *
	 * Ela fica ao LADO da queda: a água caindo ocupa o lugar imediatamente
	 * abaixo, e uma gruta ali seria uma boca com o rio entrando por dentro. Do
	 * lado, a queda fica à vista de quem está na boca — que é a única razão de a
	 * gruta estar ali e não em qualquer outro lugar.
	 *
	 * O lugar é PROCURADO, não calculado. Tentou-se afastar da margem por uma
	 * distância fixa, e não existe distância fixa que sirva: perto do degrau o
	 * rio vem torto de tanto serpentear, a perpendicular à corrente ganha
	 * componente radial e joga a gruta rio acima, dentro do lago. A busca anda
	 * ao redor da queda, do mais perto para o mais longe, e para no primeiro
	 * lugar que sobra da água, da terra, dos campos de treino e das outras
	 * peças da ilha — com folga, porque um lugar que apenas cabe sai de lugar
	 * na primeira mexida na serpentina do rio.
	 *
	 * Onde não houver lugar, não há gruta. A lista pode vir com menos grutas do
	 * que quedas, e vir vazia é resposta válida: plantar onde não cabe devolve
	 * exatamente a quina dentro da água que a busca existe para evitar.
	 */
	BATTLESQUARE_API TArray<IslandFeatureLayout::FFeaturePlacement> PlanGrottoes();
}

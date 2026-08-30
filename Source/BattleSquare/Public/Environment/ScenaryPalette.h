// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UMaterialInterface;
class UPrimitiveComponent;

/**
 * Papel de uma espécie na LEITURA do cenário.
 *
 * Não é o que a planta é, é o que ela precisa dizer na tela. Duas espécies
 * podem chegar do pacote com a mesma malha e o mesmo material e ainda assim
 * precisar de cores diferentes — foi exatamente o caso aqui: o arbusto e o
 * capim rasteiro têm ambos o slot `grass`, e enquanto a cor vinha do pacote
 * eram o mesmo verde. Quem olha não distingue arbusto de capim, e o cenário
 * inteiro lê como uma mancha só.
 */
enum class EScenaryRole : uint8
{
	/** Capim e folha rasteira, logo atrás da grade. */
	GroundCover,
	/** Arbusto e planta de volume baixo. */
	Undergrowth,
	/** Flor e cogumelo — o acento de cor. */
	Accent,
	/** Tronco caído e toco: madeira sem folha. */
	DeadWood,
	/** Pedra. */
	Rock,
	/** Árvore da mata, ainda inteira no quadro. */
	ForestTree,
	/** Árvore do dossel, que fecha o fundo. */
	CanopyTree
};

/**
 * A paleta do cenário — fonte de verdade ÚNICA da cor de tudo que é mata.
 *
 * Ela existe porque a cor que vinha nos assets não separava nada. O chão da
 * mata era vestido com `leafsGreen`, o MESMO material das folhas das árvores:
 * chão e copa eram literalmente a mesma cor, e o quadro inteiro lia como um
 * verde-água sem relevo. Trocar um material do pacote por outro só moveria o
 * problema de lugar — os dez materiais do kit são variações de uma faixa
 * estreita, e nenhum par deles se destaca do outro a três metros da tela.
 *
 * Por isso a cor passa a ser decisão NOSSA, e mora num lugar só: duas tabelas
 * de cor concordam até a primeira edição (L-032/L-033).
 */
namespace ScenaryPalette
{
	/**
	 * A cor de UM slot de material, dado o papel de quem o usa.
	 *
	 * Duas entradas, e não uma, porque nenhuma das duas basta sozinha. Só o
	 * slot não distingue o arbusto do capim (ambos chegam como `grass`); só o
	 * papel não distingue o tronco da copa na MESMA árvore (`woodBark` e
	 * `leafsGreen` convivem num asset). O par é que decide — e é o par que
	 * faz a pedra com musgo ter pedra cinza e musgo verde.
	 */
	BATTLESQUARE_API FLinearColor ColorFor(EScenaryRole Role, FName MaterialSlot);

	/**
	 * O verde do chão da mata.
	 *
	 * Fora de `ColorFor` de propósito: o chão não é espécie nenhuma, e é
	 * contra ELE que todo o resto precisa se destacar.
	 */
	BATTLESQUARE_API FLinearColor GroundColor();

	/**
	 * O material base que aceita cor — o mesmo que já pinta pets e donos.
	 *
	 * Nulo se o conteúdo da engine não carregar, e quem chama trata: cenário
	 * sem cor é feio, cenário que derruba o jogo é defeito.
	 */
	BATTLESQUARE_API UMaterialInterface* ColorableBaseMaterial();

	/**
	 * Pinta TODOS os slots de um componente com a paleta do papel dado, e
	 * devolve quantos pintou.
	 *
	 * Devolver a conta é o que torna a pintura verificável: componente sem
	 * malha atribuída pinta zero, passa em todo teste de lógica e não existe
	 * na tela — o padrão que já custou três defeitos neste projeto.
	 */
	BATTLESQUARE_API int32 PaintComponent(UPrimitiveComponent* Component, EScenaryRole Role);
}

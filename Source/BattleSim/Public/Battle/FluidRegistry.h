// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * O REGISTRO DOS FLUIDOS — de que a casa molhada é FEITA.
 *
 * ## O eixo que faltava
 *
 * `ECellProperty` já responde muito bem a uma pergunta: **quanta água há
 * aqui.** Gelo, poça, fundo, lama — com cadeia de secagem e tudo. Mas ela é a
 * pergunta da FUNDURA, e há uma segunda que nenhuma parte do jogo sabia fazer:
 * **de que essa água é feita.**
 *
 * Medido antes de escrever isto: `RequireTerrainForSkill(Submergir, Water)` —
 * uma água só. O mundo, enquanto isso, já tem água doce, água do mar, água
 * termal, água de pântano, água de caverna e lava, e a diferença entre elas
 * vivia só na COR da paleta. Submergir na lava passa hoje, se a casa for
 * marcada como água.
 *
 * Os dois eixos são independentes de propósito, e é isso que os torna úteis:
 * *fundura* diz se cabe nadar, *substância* diz o que acontece com quem nada.
 * Uma poça de lava e um rio de lava são a mesma substância em funduras
 * diferentes, e um poder que resiste a uma resiste à outra.
 *
 * ## Por que uma TABELA, e por que aqui
 *
 * Ela mora no `BattleSim` porque skill é regra de combate, e regra de combate
 * é determinística: **sem float, sem sorteio, sem relógio.** Densidade vai em
 * PARTES POR MIL, inteiro — a água doce é 1000, e tudo se lê contra ela.
 *
 * E é uma tabela só. As propriedades do fluido espalhadas por quem as consome
 * seriam a mesma lava valendo três coisas diferentes na terceira edição — é
 * L-032, e este projeto já pagou por ela três vezes.
 */
enum class EFluidKind : uint8
{
	/** Casa seca. Existe para a pergunta ter resposta em toda casa. */
	Nenhum = 0,

	/** A água dos rios, dos lagos e das fontes. A régua das outras. */
	AguaDoce,

	/** O mar que fecha a ilha. */
	AguaSalgada,

	/** A água quente do vulcão — a que sai fervendo perto da rocha queimada. */
	AguaTermal,

	/** A água parada do pântano: pesada de barro, e por isso mais densa. */
	AguaDePantano,

	/** A água que corre por dentro da pedra, nas galerias. */
	AguaDeCaverna,

	/**
	 * LAMA. Fluido, e não terreno molhado: nela se afunda.
	 *
	 * Ela já existe como `ECellProperty::Mud`, com escorregão e atraso. O que
	 * o registro acrescenta é ela ser uma SUBSTÂNCIA — para um poder poder
	 * dizer "atravesso lama" sem dizer "atravesso água".
	 */
	Lama,

	/** LAVA. O único fluido em que estar dentro custa caro por si só. */
	Lava,

	/** Sentinela. Sempre o último — o número dos outros viaja em save. */
	Count
};

/**
 * O que um fluido É, em números que o núcleo determinístico aceita.
 *
 * Campo novo entra no FIM: eles viajam em save.
 */
struct BATTLESIM_API FFluidTraits
{
	/**
	 * Densidade em PARTES POR MIL, com a água doce valendo 1000.
	 *
	 * Inteiro porque o `BattleSim` não tem float — e por mil, e não por cento,
	 * porque a diferença entre água doce e salgada é de 2,5%, e ela
	 * desapareceria arredondada em porcentagem.
	 *
	 * É ela que decide o que boia: a balsa flutua porque é menos densa que o
	 * que está embaixo dela.
	 */
	int32 DensityPerMille = 0;

	/**
	 * Dá para SUBMERGIR aqui.
	 *
	 * Substituto do `RequireTerrainForSkill(Submergir, Water)`, que aprovava
	 * qualquer casa marcada como água — inclusive uma de lava.
	 */
	bool bAllowsSubmerge = false;

	/**
	 * Dano por SLOT a quem está DENTRO, sem proteção.
	 *
	 * Zero na água: molhar não machuca. É o campo que faz da lava uma decisão
	 * em vez de um cenário colorido.
	 *
	 * Por SLOT, e não por turno — o nome foi corrigido quando a medição
	 * mostrou onde ele cairia. O dano de casa que já existe
	 * (`CellDamageAmount`) é cobrado uma vez por slot, e são três slots por
	 * turno. Dois danos de terreno em relógios diferentes seriam um defeito
	 * esperando: o jogador aprenderia o custo de um e erraria o do outro.
	 */
	int32 DamagePerSlot = 0;

	/**
	 * É ÁGUA, de qualquer procedência.
	 *
	 * Existe para um poder poder valer para "água" sem enumerar as cinco. Sem
	 * ele, cada skill nova de água nasceria com uma lista, e a sexta água
	 * ficaria de fora de todas elas — calada.
	 */
	bool bIsWater = false;

	/**
	 * QUANTO ELE CONDUZ, em partes por mil.
	 *
	 * Mil é conduzir por inteiro. Referência de fora, como as densidades: a
	 * água do mar conduz muito mais que a doce (sal), o pântano fica no meio
	 * (minerais), e a lava conduz porque rocha fundida é iônica.
	 *
	 * É NÚMERO e não booleano porque a corrente que chega ao outro lado da
	 * poça depende do que ela atravessou: um raio que corre pelo mar chega
	 * inteiro, e o mesmo raio numa poça de água doce chega fraco.
	 */
	int32 ConductivityPerMille = 0;

	/** O nome, para o painel. */
	const TCHAR* DebugName = TEXT("?");
};

/**
 * O array de resistência em `FPetState` tem tamanho fixo, porque a reflexão da
 * engine não aceita `EFluidKind::Count` como dimensão. Esta guarda é o que
 * impede o nono fluido de nascer sem lugar onde ser resistido — silenciosamente.
 */
static_assert(static_cast<int32>(EFluidKind::Count) <= 8,
	"Acrescentou um fluido? Aumente FPetState::FluidResistPercent junto.");

namespace FluidRegistry
{
	/** O que este fluido é. Fluido desconhecido devolve a linha de `Nenhum`. */
	BATTLESIM_API const FFluidTraits& TraitsOf(EFluidKind Fluid);

	/** Atalho: dá para submergir neste fluido? */
	BATTLESIM_API bool AllowsSubmerge(EFluidKind Fluid);

	/** Atalho: este fluido é água, de qualquer procedência? */
	BATTLESIM_API bool IsWater(EFluidKind Fluid);

	/** Atalho: quanto custa estar dentro dele por um slot, sem proteção. */
	BATTLESIM_API int32 DamagePerSlot(EFluidKind Fluid);

	/** Atalho: este fluido conduz corrente? */
	BATTLESIM_API bool Conducts(EFluidKind Fluid);

	/** Quanto de uma corrente de força dada chega, depois de atravessar este fluido. */
	BATTLESIM_API int32 ConductedShare(int32 Amount, EFluidKind Fluid);

	/**
	 * O que BOIA sobre o quê.
	 *
	 * Verdadeiro quando quem tem a densidade dada flutua neste fluido. É a
	 * pergunta que a balsa faz, e a razão de a densidade ser um número em vez
	 * de um booleano `bFloats`: o que boia depende dos DOIS lados.
	 */
	BATTLESIM_API bool FloatsOn(int32 BodyDensityPerMille, EFluidKind Fluid);

	/** A densidade da água doce, que é a régua das outras. */
	BATTLESIM_API int32 FreshWaterDensityPerMille();
}

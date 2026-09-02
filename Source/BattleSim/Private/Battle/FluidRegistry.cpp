// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/FluidRegistry.h"

namespace RegistroDosFluidos
{
	/**
	 * A régua: a água doce vale mil.
	 *
	 * Todas as outras densidades se leem contra ela, e é por isso que ela é uma
	 * constante nomeada em vez do literal 1000 espalhado pela tabela.
	 */
	constexpr int32 AguaDoce = 1000;

	/**
	 * A TABELA — uma linha por fluido, num lugar só.
	 *
	 * Os números de densidade são os do mundo real em partes por mil, porque
	 * ter uma referência de fora é o que impede a tabela de virar gosto: água
	 * do mar 1025, lama 1500, lava perto de 3000. Quando um deles for ajustado
	 * por equilíbrio de jogo, a mudança fica visível justamente por divergir
	 * da referência.
	 */
	const FFluidTraits Tabela[static_cast<int32>(EFluidKind::Count)] =
	{
		// Nenhum: casa seca. Densidade zero, e nada boia no que não existe.
		{ 0,           false, 0, false, TEXT("nenhum") },

		// Água doce: a régua.
		{ AguaDoce,    true,  0, true,  TEXT("agua doce") },

		// Água salgada: 2,5% mais densa — é o que faz boiar mais fácil no mar.
		{ 1025,        true,  0, true,  TEXT("agua salgada") },

		// Água termal: quente, e por isso um pouco MENOS densa que a fria.
		// O dano fica em zero: ela é morna, não fervente. Quem quiser que
		// escalde muda esta linha, e muda num lugar só.
		{ 988,         true,  0, true,  TEXT("agua termal") },

		// Água de pântano: carregada de barro, mais pesada que a doce.
		{ 1080,        true,  0, true,  TEXT("agua de pantano") },

		// Água de caverna: doce e parada. Igual à doce até alguém medir
		// diferença — e a linha existe para essa diferença ter onde morar.
		{ AguaDoce,    true,  0, true,  TEXT("agua de caverna") },

		// Lama: densa o bastante para segurar quem entra. NÃO se submerge
		// nela — afundar em lama não é mergulhar, é atolar.
		{ 1500,        false, 0, false, TEXT("lama") },

		// Lava: densa e cara. O dano é o que a torna uma decisão em vez de um
		// cenário colorido, e é o único fluido em que estar dentro custa por si.
		{ 3100,        false, 12, false, TEXT("lava") },
	};

	const FFluidTraits& LinhaDe(EFluidKind Fluido)
	{
		const int32 Qual = static_cast<int32>(Fluido);

		// Fora da tabela cai em `Nenhum`, e não em comportamento indefinido:
		// um valor novo do enum que alguém esqueça de registrar aqui vira casa
		// seca, que é o inócuo — nunca lava por acidente.
		if (Qual < 0 || Qual >= static_cast<int32>(EFluidKind::Count))
		{
			return Tabela[static_cast<int32>(EFluidKind::Nenhum)];
		}

		return Tabela[Qual];
	}
}

namespace FluidRegistry
{
	const FFluidTraits& TraitsOf(EFluidKind Fluido)
	{
		return RegistroDosFluidos::LinhaDe(Fluido);
	}

	bool AllowsSubmerge(EFluidKind Fluido)
	{
		return RegistroDosFluidos::LinhaDe(Fluido).bAllowsSubmerge;
	}

	bool IsWater(EFluidKind Fluido)
	{
		return RegistroDosFluidos::LinhaDe(Fluido).bIsWater;
	}

	int32 DamagePerTurn(EFluidKind Fluido)
	{
		return RegistroDosFluidos::LinhaDe(Fluido).DamagePerTurn;
	}

	bool FloatsOn(int32 DensidadeDoCorpo, EFluidKind Fluido)
	{
		const int32 DoFluido = RegistroDosFluidos::LinhaDe(Fluido).DensityPerMille;

		// Em casa seca nada boia: sem fluido não há empuxo. Sem esta linha, um
		// corpo de densidade zero "boiaria" no vazio.
		if (DoFluido <= 0)
		{
			return false;
		}

		// Estritamente menos denso. O igual AFUNDA devagar em vez de boiar, e
		// deixar o empate boiando faria um corpo de densidade exata da água
		// flutuar para sempre — que é o caso que mais aparece, porque é o
		// número redondo que alguém escreve primeiro.
		return DensidadeDoCorpo < DoFluido;
	}

	int32 FreshWaterDensityPerMille()
	{
		return RegistroDosFluidos::AguaDoce;
	}
}

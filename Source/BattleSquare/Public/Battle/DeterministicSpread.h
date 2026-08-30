// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Sorteio sem sorteador: a mesma semente dá o mesmo resultado em qualquer
 * máquina, hoje e no ano que vem.
 *
 * Mora aqui, e não dentro de quem usa, porque a morfologia do pet e a mata
 * do cenário precisam da MESMA função. Duas cópias de um hash concordam até
 * alguém mexer numa delas — e aí dois bichos com a mesma semente nascem
 * diferentes conforme quem os desenhou (L-032/L-033).
 *
 * Isto NÃO é o núcleo determinístico: `BattleSim` não pode ver float, e
 * `Fracao` devolve float de propósito. É semeadura de APARÊNCIA.
 */
namespace BattleSpread
{
	/** FNV-1a: hash puro, sem estado, sem relógio, igual em toda máquina. */
	inline uint32 SeedFromText(const FString& Texto)
	{
		uint32 Hash = 2166136261u;
		for (const TCHAR Caractere : Texto)
		{
			Hash ^= static_cast<uint32>(Caractere);
			Hash *= 16777619u;
		}
		return Hash;
	}

	/** Finalizador do murmur3: espalha os bits para o fluxo não ter padrão. */
	inline uint32 Scatter(uint32 Valor)
	{
		Valor ^= Valor >> 16;
		Valor *= 0x85EBCA6Bu;
		Valor ^= Valor >> 13;
		Valor *= 0xC2B2AE35u;
		Valor ^= Valor >> 16;
		return Valor;
	}

	/**
	 * Um fluxo POR ÍNDICE, entre 0 e 1.
	 *
	 * Fatiar um hash de 32 bits daria oito valores e acabou; pior, mexer na
	 * faixa de um mudaria os bits de todos os seguintes. Com um fluxo por
	 * índice, valor novo entra no fim sem tocar em quem já existe.
	 */
	inline float Fraction(uint32 Semente, int32 Indice)
	{
		const uint32 Fluxo = Scatter(Semente ^ (static_cast<uint32>(Indice) * 0x9E3779B9u));
		return static_cast<float>(Fluxo >> 8) / static_cast<float>(1u << 24);
	}

	/**
	 * Um inteiro em [0, Teto) do mesmo fluxo.
	 *
	 * Existe porque escolha não é medida: converter `Fraction` em índice por
	 * multiplicação e truncamento erra na borda (1.0 vira o índice que não
	 * existe), e labirinto com uma direção a menos deixa de ser labirinto.
	 */
	inline int32 Below(uint32 Semente, int32 Indice, int32 Teto)
	{
		if (Teto <= 1)
		{
			return 0;
		}

		const uint32 Fluxo = Scatter(Semente ^ (static_cast<uint32>(Indice) * 0x9E3779B9u));
		return static_cast<int32>(Fluxo % static_cast<uint32>(Teto));
	}

	/** Interpola dentro da faixa — o par natural de Fraction. */
	inline float Between(float Minimo, float Maximo, float Fracao)
	{
		return Minimo + (Maximo - Minimo) * Fracao;
	}
}

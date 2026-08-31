// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Environment/IslandGeography.h"

/**
 * O ELENCO de cada bioma: quais espécies da mata podem aparecer em cada um.
 *
 * Existe porque a proporção não bastava. `PresencaDe` já dizia QUANTO de cada
 * papel um bioma recebe — quanta pedra, quanto arbusto, quanta árvore —, mas
 * as espécies DENTRO do papel eram as mesmas em todo lugar. O resultado é o
 * que o jogador viu: geleira com carvalho, floresta com agulha de pedra de
 * vulcão, pântano com flor de jardim. Elenco único com proporção diferente não
 * faz lugar diferente; faz o mesmo lugar mais cheio ou mais vazio.
 *
 * É namespace, e não tabela dentro do `AForestBackdrop`, porque quem responde
 * "esta espécie é deste bioma?" precisa ser UM só: a mata da arena e o pedaço
 * do mundo plantam pelo mesmo elenco, e duas listas concordariam até a
 * primeira edição (L-032/L-033).
 *
 * O inventário por extenso, com o motivo de cada inclusão e de cada exclusão,
 * mora em `.specs/project/BIOMAS.md`. Aqui está só a decisão.
 */
namespace BiomeFlora
{
	/**
	 * Esta espécie entra neste bioma?
	 *
	 * Espécie fora do elenco não é plantada, qualquer que seja a proporção do
	 * papel dela. O filtro é por NOME de malha, e não por papel, porque é
	 * exatamente dentro do papel que a separação faltava: geleira e floresta
	 * têm as duas `CanopyTree`, e o que as distingue é a conífera contra a
	 * copa redonda.
	 *
	 * Nome desconhecido devolve `false`. É a escolha que faz o defeito
	 * aparecer: espécie nova sem linha de elenco simplesmente não nasce, e o
	 * teste que confere elenco contra a tabela de espécies acusa. Devolver
	 * `true` a espalharia calada por todos os seis biomas.
	 */
	BATTLESQUARE_API bool FitsBiome(const FString& SpeciesName, EIslandBiome Biome);

	/**
	 * O elenco inteiro de um bioma.
	 *
	 * Existe para o teste poder cobrar duas coisas que `FitsBiome` sozinha não
	 * revela: que todo nome escrito aqui EXISTE na tabela de espécies (nome
	 * errado nunca planta, e nada avisa), e que dois biomas não têm o mesmo
	 * elenco.
	 */
	BATTLESQUARE_API TArray<FString> AllowedSpecies(EIslandBiome Biome);
}

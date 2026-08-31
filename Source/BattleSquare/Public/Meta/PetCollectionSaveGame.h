// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "World/WorldDiscovery.h"
#include "World/WorldMapPins.h"
#include "GameFramework/SaveGame.h"
#include "PetCollectionSaveGame.generated.h"

// T1 (tasks.md, Coleção e Captura, DP-colecao-01/03): coleção local do
// jogador. Save independente de conta (M7 não existe ainda) — cada
// instalação guarda a própria, sem sincronização.
USTRUCT()
struct FOwnedPetInstance
{
	GENERATED_BODY()

	// Identidade de captura: o id do REGISTRO DE CATÁLOGO
	// (FLoadedPetRecord::Id), nunca o Type — dois pets do mesmo tipo com
	// ids diferentes são capturas independentes (spec.md, edge case).
	UPROPERTY()
	FString CatalogId;

	UPROPERTY()
	FString Name;

	UPROPERTY()
	FString Type;

	// Gancho para Níveis, Experiência e Evolução (próxima feature) — só
	// existe aqui e é incrementado no caso P2 (vitória redundante);
	// nenhuma lógica de nível/evolução nesta feature.
	UPROPERTY()
	int32 Experience = 0;

	/**
	 * Força bruta. Sobe com DANO CAUSADO, não com ataque desferido — atacar o
	 * vazio não fortalece ninguém (DP-atr-03).
	 */
	UPROPERTY()
	int32 Musculature = 0;

	/**
	 * Temperamento, num EIXO: negativo é cauteloso, positivo é agressivo.
	 *
	 * Não sobe, INCLINA. É o que permite golpe que exige um lado em vez de um
	 * mínimo — e um pet não pode ter os dois, o que faz personalidade virar
	 * decisão em vez de mais um número para maximizar.
	 */
	UPROPERTY()
	int32 Personality = 0;

	/**
	 * Proficiência POR SKILL, na ordem camuflagem, voo, subsolo.
	 *
	 * Separadas de propósito: voar muito destrava golpe aéreo, e não golpe de
	 * camuflagem — é o que torna o caminho do jogador legível no pet dele.
	 *
	 * Sobe com uso EFETIVO: a postura assumida só conta se o pet não tomou
	 * dano naquele slot. Contar toda postura recompensaria camuflar contra um
	 * bot parado.
	 */
	UPROPERTY()
	int32 SkillProficiency[3] = { 0, 0, 0 };
};

/**
 * O TREINADOR — a primeira coisa do jogo que pertence ao JOGADOR, e não a um pet.
 *
 * Atravessa a coleção inteira e sobrevive à troca de pet favorito. É o que
 * impede que dois treinadores veteranos sejam idênticos.
 */
USTRUCT()
struct BATTLESQUARE_API FTrainerProfile
{
	GENERATED_BODY()

	/**
	 * Em que atributos este treinador é especialista.
	 *
	 * LIMITADO de propósito (DP-atr-11): ninguém fica completo, e a escassez
	 * é o que transforma a escolha em identidade. Mesmo vocabulário do
	 * requisito de golpe e do campo de treino — uma terceira lista de nomes
	 * produziria uma especialidade que não casa com campo nenhum.
	 */
	UPROPERTY()
	TArray<FString> Specialties;
};

UCLASS()
class BATTLESQUARE_API UPetCollectionSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TArray<FOwnedPetInstance> OwnedPets;

	/**
	 * No MESMO save da coleção, e não num arquivo à parte: dois arquivos
	 * dessincronizam, e um treinador sem coleção (ou o contrário) é um estado
	 * que nada no jogo sabe interpretar.
	 *
	 * Save gravado antes disto existir carrega com o perfil vazio, que é
	 * "nenhuma especialidade" — o comportamento de antes desta feature.
	 */
	UPROPERTY()
	FTrainerProfile Trainer;

	/**
	 * O que o jogador já viu do mundo.
	 *
	 * No MESMO save, pelo mesmo motivo do treinador: dois arquivos
	 * dessincronizam. Save gravado antes disto existir carrega com o mapa em
	 * branco, que é o estado de quem nunca andou — e não o mapa completo, que
	 * seria devolver de graça o que a feature existe para cobrar.
	 */
	UPROPERTY()
	FWorldDiscovery Discovery;

	/**
	 * As marcações que o jogador escreveu no mapa.
	 *
	 * Ao lado da descoberta, e no mesmo save: as duas são a memória DELE sobre
	 * o mundo, e separá-las daria um mapa que lembra onde ele andou e esquece
	 * o que ele anotou.
	 */
	UPROPERTY()
	FWorldMapPins MapPins;
};

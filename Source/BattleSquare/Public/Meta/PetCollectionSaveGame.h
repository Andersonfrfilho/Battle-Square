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

	/**
	 * A BIOLOGIA do pet, nos quatro eixos, como texto.
	 *
	 * VAZIO é pet de coleção antiga, e o save tem de continuar carregando: um
	 * campo novo que invalidasse a coleção salva apagaria os pets de quem já
	 * jogava. `UPROPERTY` ausente no arquivo antigo desserializa como vazio,
	 * que é exatamente "não informado".
	 */
	/**
	 * A VIDA que sobrou da última batalha, em PORCENTAGEM (decisão 61).
	 *
	 * Porcentagem, e não valor absoluto: o teto muda com o nível, e a fração
	 * sobrevive à subida — um pet a meia-vida continua a meia-vida quando o
	 * teto crescer.
	 *
	 * **Negativo é "não informado", e não informado é CHEIO.** É o save
	 * antigo, gravado antes de a vida persistir — e é também o motivo de o
	 * padrão não ser zero: um campo novo que matasse os pets de quem já
	 * jogava seria o defeito que o comentário de `BiologySkin` acima existe
	 * para impedir, na pior forma possível.
	 */
	UPROPERTY()
	int32 HealthPercent = -1;

	UPROPERTY()
	FString BiologySkin;

	UPROPERTY()
	FString BiologyBuild;

	UPROPERTY()
	FString BiologyBreathing;

	UPROPERTY()
	FString BiologyLimbs;

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
 * O quanto o jogador JÁ CONHECE um morador (decisão 15).
 *
 * Mora no perfil do TREINADOR porque a relação é do jogador, não do mundo:
 * o morador é o mesmo para todos, mas quem já ouviu a história dele é você.
 */
USTRUCT()
struct BATTLESQUARE_API FResidentAcquaintance
{
	GENERATED_BODY()

	/** "vila-inicial-3": tipo da vila + porta — a mesma chave do morador. */
	UPROPERTY()
	FString ResidentKey;

	/** Quantas visitas ATENDIDAS. A história avança por aqui. */
	UPROPERTY()
	int32 Meetings = 0;
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

	/**
	 * O DINHEIRO do treinador. Nunca negativo — dívida seria mecânica própria.
	 *
	 * Mora no perfil, e não num save separado, porque é a mesma pergunta das
	 * especialidades: coisa que pertence ao JOGADOR e sobrevive à troca de pet.
	 */
	UPROPERTY()
	int32 Money = 0;

	/**
	 * A bolsa inicial já foi dada.
	 *
	 * Falso em save antigo — que é o certo: perfil gravado antes da carteira
	 * recebe a bolsa na primeira carga, igual a um treinador novo. E a marca é
	 * SEPARADA do saldo de propósito: zerar a carteira gastando não pode
	 * devolver a bolsa, senão falir vira renda.
	 */
	UPROPERTY()
	bool bWalletGranted = false;

	/**
	 * Os pontos de RANKING — vitórias de Arena, acumuladas.
	 *
	 * Só sobem. Perder não desconta, e é decisão do dono do mundo (58): o
	 * ranking é placar de progresso, não corda bamba — e o Portão que ele um
	 * dia trancaria não tranca ninguém, porque viajar é liberdade.
	 */
	UPROPERTY()
	int32 RankingPoints = 0;

	/**
	 * Quem o jogador já visitou, e quantas vezes foi atendido.
	 *
	 * Vazio em save antigo — ninguém conhecido, que é o certo: a história de
	 * cada morador começa do começo.
	 */
	UPROPERTY()
	TArray<FResidentAcquaintance> Acquaintances;

	/**
	 * Quantos pets o jogador JÁ VENDEU — o feito que o arco do Mercado
	 * escuta. Contador e não bool: histórias futuras podem querer "muitos".
	 */
	UPROPERTY()
	int32 PetsSold = 0;

	/**
	 * DE QUE CENTRO o jogador é LÍDER (decisão 15) — a chave do assentamento
	 * ("vila-inicial"), vazia quando não é de nenhum.
	 *
	 * UMA string, não uma lista: líder é de UM centro por vez — o posto
	 * prende (ver a tranca em `LeadershipRules`), e prender em dois lugares
	 * ao mesmo tempo é não estar em nenhum.
	 */
	UPROPERTY()
	FString LeaderOf;

	/** O último dia em que a renda do posto foi paga. -1 é "nunca". */
	UPROPERTY()
	int32 LastStipendDay = -1;

	/**
	 * A ESCOLHA do líder ausente (decisão 15, emendada): defesa automática
	 * ligada, o posto se defende sozinho — o BattleSim joga por ele. FALSO
	 * por padrão: nada é tirado (nem arriscado) sem o jogador pedir, e o
	 * desafiante ESPERA em vez de vencer por ausência.
	 */
	UPROPERTY()
	bool bAutoDefense = false;

	/** O dia em que um desafiante chegou e espera. -1 é "ninguém na fila". */
	UPROPERTY()
	int32 PendingDefenseDay = -1;
};

/**
 * Uma PILHA da mochila: um item, e quantos.
 *
 * A quantidade é da PILHA, não do item. Cinco poções são uma linha com
 * `Quantity = 5`, e não cinco linhas — senão a mochila de quem junta coisa vira
 * uma lista que não cabe na tela, e procurar um item nela deixa de ser
 * possível.
 */
USTRUCT()
struct BATTLESQUARE_API FBackpackStack
{
	GENERATED_BODY()

	/** O `Id` do catálogo. Estável: renomear quebra mochila já salva. */
	UPROPERTY()
	FString ItemId;

	UPROPERTY()
	int32 Quantity = 0;
};

/**
 * Um item VESTIDO num pet.
 *
 * Guarda o pet pelo `CatalogId` porque é o que a coleção já usa para achar um
 * pet. Guardar um índice amarraria o equipamento à ORDEM da lista, e capturar
 * um pet novo trocaria a bota de dono sem ninguém pedir.
 */
USTRUCT()
struct BATTLESQUARE_API FEquippedItem
{
	GENERATED_BODY()

	UPROPERTY()
	FString PetCatalogId;

	UPROPERTY()
	FString ItemId;
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
	 * A MOCHILA: o que o jogador carrega, e quantos de cada.
	 *
	 * No MESMO save, pelo mesmo motivo do treinador e do mapa: dois arquivos
	 * dessincronizam. Save gravado antes disto existir carrega com a mochila
	 * VAZIA — que é exatamente o estado de quem nunca pegou item nenhum, e não
	 * um erro.
	 */
	UPROPERTY()
	TArray<FBackpackStack> Backpack;

	/**
	 * O que está VESTIDO, e em quem.
	 *
	 * Separado da mochila porque são dois ESTADOS do mesmo item: equipar tira
	 * de lá e põe aqui. Guardar o item nos dois lugares o faria existir duas
	 * vezes, e a mesma bota vestiria cinco pets.
	 */
	UPROPERTY()
	TArray<FEquippedItem> Equipped;

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

// Copyright 2026 Anderson. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Environment/IslandGeography.h"
#include "Environment/ScenaryPalette.h"
#include "GameFramework/Actor.h"
#include "ForestBackdrop.generated.h"

class UHierarchicalInstancedStaticMeshComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UMaterialInterface;

/**
 * A mata que fica ATRÁS da arena.
 *
 * O combate acontecia sobre o xadrez do template de mundo aberto, até o
 * horizonte: céu havia, chão havia, mas nada que dissesse ONDE a batalha se
 * passa. Este ator veste o entorno — chão de floresta, arbustos e pedras na
 * borda, e um paredão de árvores no fundo do enquadramento.
 *
 * Três decisões que não são de gosto:
 *
 * 1. **A escala sai da caixa medida da malha, nunca de um número por
 *    espécie.** Cada árvore do pacote tem uma altura diferente; pedir "esta
 *    árvore tem 6 casas de altura" e dividir pela caixa dela é o que faz o
 *    conjunto ficar coerente. Altura fixa por espécie foi o defeito que já
 *    afundou pet em laje três vezes neste projeto.
 * 2. **Nada nasce dentro do tabuleiro nem no caminho da câmera.** A mata é
 *    cenário; se ela entra na grade, vira regra de jogo por acidente.
 * 3. **A mesma semente dá a mesma mata.** Duas partidas com a mesma arena
 *    são a mesma cena, e uma captura de tela continua valendo amanhã.
 */
UCLASS()
class BATTLESQUARE_API AForestBackdrop : public AActor
{
	GENERATED_BODY()

public:
	AForestBackdrop();

	/**
	 * Espalha a mata em volta de um tabuleiro de `CellSize` por casa.
	 *
	 * Recebe o tamanho da casa em vez de guardar o seu: a arena é a fonte de
	 * verdade do espaçamento, e um segundo número aqui discordaria dela na
	 * primeira edição (L-032/L-033). Todos os raios e alturas da mata são
	 * múltiplos DESTA casa.
	 *
	 * `CameraGroundOffset` é onde a câmera pousa no chão, em espaço local:
	 * o vazio em volta dela é o que impede uma árvore de nascer colada na
	 * lente e tapar a batalha inteira.
	 *
	 * O BIOMA é o do chão onde a luta começou, e filtra as espécies pela
	 * MESMA tabela que filtra o pedaço do mundo (`PresencaDe`). Sem ele,
	 * lutar na geleira dava mata de floresta — a geografia sabia onde o
	 * encontro foi, e só o cenário da briga não.
	 *
	 * O padrão é `Forest` porque é o que a tabela inteira, sem filtro nenhum,
	 * já era: arena que ninguém situou continua sendo a mata de sempre.
	 */
	void BuildForest(float CellSize, uint32 Seed, const FVector2D& CameraGroundOffset,
		EIslandBiome Biome = EIslandBiome::Forest);

	/**
	 * Monta um PEDAÇO do mundo: quadrado, sem tabuleiro e sem câmera a evitar.
	 *
	 * É outra entrada, e não mais parâmetros em `BuildForest`, porque a forma
	 * é outra. A mata da arena é um DISCO com duas folgas no meio — o
	 * tabuleiro e a lente. O pedaço do mundo é ladrilho: ele encosta nos
	 * vizinhos pelos quatro lados, e disco ladrilhado deixa buraco em todo
	 * canto. Um jogador andando em diagonal cairia exatamente ali.
	 *
	 * O BIOMA decide três coisas: a cor do chão, quais espécies aparecem, e
	 * em que proporção. Deserto não tem capim; geleira não tem flor; vulcão é
	 * pedra sobre pedra. A tabela de espécies continua sendo UMA — o bioma a
	 * filtra, não a duplica.
	 *
	 * A densidade acompanha a ÁREA: o mesmo adensamento por unidade quadrada
	 * que a mata da arena tem, para um pedaço não parecer mais cheio que o
	 * outro só por ser maior.
	 */
	void BuildRegion(float CellSize, uint32 Seed, EIslandBiome Biome, float SideUnits);

	/**
	 * Nomes das malhas de espécie, na ordem da tabela.
	 *
	 * Existe para o teste poder cobrar que todo nome escrito num elenco de
	 * bioma (`BiomeFlora`) EXISTE aqui. Nome errado no elenco nunca planta e
	 * nada avisa — é o modo de falhar silencioso que a lista de elenco
	 * introduz, e a única defesa contra ele é comparar as duas.
	 */
	static TArray<FString> SpeciesNames();

	/**
	 * Caminho do asset de uma espécie do pacote de mata.
	 *
	 * Uma função e não a pasta solta em cada lugar: o campo de treino também
	 * monta peças deste mesmo pacote, e duas montagens do caminho concordam
	 * até a primeira vez que o pacote muda de pasta (§16, L-032).
	 */
	static FString SpeciesAssetPath(const FString& SpeciesName);

	/** Espécies da mata, para o teste que exige asset atribuído em todas. */
	const TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>>& GetSpeciesClusters() const { return SpeciesClusters; }

	/**
	 * Este agrupamento tem CORPO — dá para esbarrar e para derrubar?
	 *
	 * Responde pela MESMA função que decide o que se derruba
	 * (`FWorldObstacleBreaking::StartingHealthFor`), e não por uma segunda
	 * lista: capim e flor não bloqueiam ninguém, e duas listas de "o que é
	 * sólido" concordariam até a primeira edição. O sintoma seria uma casa de
	 * arena bloqueada por um cogumelo.
	 */
	bool IsSolidSpecies(int32 SpeciesIndex) const;

	/**
	 * Os obstáculos EM PÉ perto de um ponto, com a vida que ainda têm.
	 *
	 * Só o que é obstáculo entra: capim e flor não são derrubáveis, e
	 * devolvê-los faria o golpe do jogador escolher grama por ela estar mais
	 * perto que a árvore.
	 *
	 * O ÍNDICE devolvido é opaco de propósito — quem bate não precisa saber
	 * que a mata guarda instâncias em agrupamentos por espécie, e depender
	 * disso amarraria o combate à forma como a mata é desenhada.
	 */
	TArray<struct FWorldObstacleCandidate> CollectObstaclesNear(
		const FVector& WorldLocation, float RadiusUnits, TArray<int32>& OutHandles) const;

	/**
	 * Tira vida de um obstáculo. Devolve true quando ele CAI neste golpe.
	 *
	 * Cair é sumir da tela e deixar de ser alvo; o que não pode acontecer é
	 * bater e nada mudar.
	 */
	bool DamageObstacle(int32 Handle, int32 Damage);

	/** O disco de chão que cobre o xadrez do template. */
	UStaticMeshComponent* GetGroundMesh() const { return GroundMesh; }

	/**
	 * Os montes de RELEVO deste pedaço: duna no deserto, gelo quebrado na
	 * geleira.
	 *
	 * Existe para o teste poder perguntar "o chão deste bioma tem forma?".
	 * Chão plano com pedra em cima passa em todo teste de densidade e continua
	 * sendo um pátio bege — foi exatamente o que o jogador viu.
	 */
	UHierarchicalInstancedStaticMeshComponent* GetReliefMounds() const;

	/**
	 * A faixa de areia molhada deste pedaço, na beira do mar.
	 *
	 * Ela acompanha o ARCO da ilha, não a borda do ladrilho: o pedaço tem 6400
	 * de lado e a praia tem 1600 de largura, então todo pedaço de praia é meio
	 * areia e meio interior. Faixa pintada no ladrilho inteiro molharia mata.
	 */
	UHierarchicalInstancedStaticMeshComponent* GetShoreWetSand() const;

	/** A espuma na linha d'água — o lado de FORA da areia molhada. */
	UHierarchicalInstancedStaticMeshComponent* GetShoreFoam() const;

	/**
	 * As árvores de beira: finas, altas e tombadas para o mar.
	 *
	 * Uma praia sem nada em pé é uma rampa bege — não há o que dê escala nem o
	 * que faça sombra, e o jogador chega na água sem ter visto que chegava.
	 *
	 * O pacote não traz coqueiro. `tree_thin` inclinada é o que mais perto
	 * chega da silhueta, e inclinar é metade do efeito: árvore de praia cresce
	 * torta porque o vento vem sempre do mesmo lado.
	 */
	UHierarchicalInstancedStaticMeshComponent* GetShoreTrees() const;

	UHierarchicalInstancedStaticMeshComponent* GetRiverSurface() const;
	UHierarchicalInstancedStaticMeshComponent* GetRiverFallFoam() const;
	UHierarchicalInstancedStaticMeshComponent* GetRiverTrail() const;

	/**
	 * As poças do brejo — vazias em todo bioma que não é pântano.
	 *
	 * Existe para o teste poder cobrar as duas metades da regra: que o pântano
	 * TEM água na tela, e que os outros biomas não herdam a poça do vizinho.
	 */
	UHierarchicalInstancedStaticMeshComponent* GetSwampPools() const;

	/**
	 * O papel de paleta do chão deste pedaço; `Count` quando não é pedaço.
	 *
	 * Existe para o teste poder cobrar que cada bioma pinta o SEU chão. A cor
	 * final mora numa instância dinâmica de material, que é o lugar mais caro
	 * de se ler e o mais fácil de se ler errado.
	 */
	EScenaryRole GetRegionGroundRole() const { return RegionGroundRole; }

	/**
	 * Altura LOCAL do topo do chão, em unidades.
	 *
	 * Existe para quem posiciona a mata poder encostá-la numa superfície
	 * conhecida sem repetir a espessura do disco — a mesma medida em dois
	 * lugares discorda na primeira edição (L-032/L-033).
	 */
	static float GroundTopLocalZ();

	/**
	 * O lado do chão de um pedaço, dado o lado do pedaço.
	 *
	 * MAIOR que o pedaço, de propósito. Dois pedaços de 6400 numa grade de
	 * 6400 apenas se ENCOSTAM, e encostar não é cobrir: na diagonal, onde
	 * quatro cantos se tocam, sobra uma fresta — e foi por uma fresta dessas
	 * que o jogador afundou perto da água.
	 *
	 * Existe como função, e não como número na montagem, para o teste poder
	 * cobrar a sobreposição sem instanciar mundo.
	 */
	static float RegionGroundSideUnits(float SideUnits);

	/**
	 * Veste o chão com o material que o mundo emprestou.
	 *
	 * Nulo devolve o chão à paleta própria da mata. Quem empresta é a arena,
	 * ao sondar o terreno onde o encontro aconteceu.
	 */
	void SetGroundMaterialOverride(UMaterialInterface* Material);

	/** Quantas plantas foram de fato plantadas na última montagem. */
	int32 GetPlantedCount() const;

	/** Posição local de cada planta — o que o teste mede para saber se sobrou espaço. */
	TArray<FVector> GetPlantedLocations() const;

	/**
	 * Raio do disco de chão, EM CASAS.
	 *
	 * Em casas, e não em unidades, porque a mata serve DOIS donos com casas de
	 * tamanhos diferentes: a arena (casa de 150) e o mundo aberto (casa de
	 * 200). O mesmo trinta dá clareira de 4500 num e terra de 6000 no outro, e
	 * é isso que se quer — o enquadramento da arena é um diorama, e ele não
	 * pode crescer junto com a ilha.
	 *
	 * O padrão é o do diorama. Quem tem ilha própria — o mundo — escreve aqui
	 * o seu raio antes de mandar construir.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Mata")
	float GroundRadiusInCells = 30.0f;

	/** Raio, em casas, que nenhuma planta invade — em volta do tabuleiro. */
	UPROPERTY(EditDefaultsOnly, Category = "Mata")
	float BoardClearanceInCells = 2.2f;

	/** Raio, em casas, que nenhuma planta invade — em volta da câmera. */
	UPROPERTY(EditDefaultsOnly, Category = "Mata")
	float CameraClearanceInCells = 3.0f;

private:
	/**
	 * Veste o chão: o material do mundo quando há um, a paleta quando não há.
	 *
	 * Num lugar só porque são dois momentos — a construção da mata e o
	 * empréstimo do chão, que chega depois — e duplicar a decisão faria as
	 * duas divergirem na primeira edição.
	 */
	void ApplyGroundMaterial();

	/**
	 * Apaga a orla inteira.
	 *
	 * Chamada em TODA montagem, antes de qualquer filtro de bioma: o mesmo
	 * ator vira praia, vira geleira e volta a ser a mata da arena, e uma laje
	 * esquecida não teria nada no mundo explicando de onde veio.
	 */
	void LimparOrla();

	/**
	 * Apaga rio, queda e trilha de beira.
	 *
	 * Mesmo motivo da orla: o ator é reaproveitado de um pedaço para o outro, e
	 * um trecho de rio esquecido correria por dentro de um pedaço por onde rio
	 * nenhum passa.
	 */
	void LimparAguaDoce();

	/**
	 * Apaga as poças do brejo.
	 *
	 * Em componente próprio, e não junto do rio, porque as duas águas têm
	 * vidas diferentes: o rio atravessa qualquer bioma que esteja no caminho
	 * dele, a poça só existe onde há pântano. No mesmo componente, limpar uma
	 * apagaria a outra.
	 */
	void LimparBrejo();

	UPROPERTY()
	TObjectPtr<USceneComponent> ForestRoot;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> GroundMesh;

	/**
	 * Os montes que dão FORMA ao chão do pedaço.
	 *
	 * Esferas meio enterradas, e esferas de propósito: o jogador pediu curva,
	 * e cone é o oposto disso — foi o cone que produziu as "montanhas pontudas
	 * extremamente esquisitas". Meia esfera acima do chão é a única curva que
	 * uma primitiva da engine entrega sem malha autorada.
	 *
	 * Sem colisão: duna que barra o passo vira cerca, e o deserto deixa de ser
	 * atravessável justamente por aquilo que deveria só desenhá-lo.
	 */
	UPROPERTY()
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> ReliefMounds;

	/**
	 * As três peças da ORLA, montadas só onde o bioma é praia.
	 *
	 * Elas são as únicas coisas deste ator que dependem de ONDE ele está no
	 * mundo, e não só do bioma que lhe mandaram: a linha d'água é um arco de
	 * raio conhecido, e sem consultar a posição do pedaço a faixa cairia no
	 * meio da areia num lugar e dentro do mar no outro.
	 */
	UPROPERTY()
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> ShoreWetSand;

	UPROPERTY()
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> ShoreFoam;

	UPROPERTY()
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> ShoreTrees;

	UPROPERTY()
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> RiverSurface;
	UPROPERTY()
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> RiverFallFoam;
	UPROPERTY()
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> RiverTrail;

	/**
	 * A ÁGUA PARADA do brejo: lâminas achatadas e sobrepostas, rentes ao chão.
	 *
	 * Sem colisão, como toda água deste ator. Poça que barra o passo é degrau,
	 * e degrau na água foi o que produziu "parte da agua ele afunda".
	 */
	UPROPERTY()
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> SwampPools;

	/**
	 * A malha QUADRADA do chão, para quem é ladrilho.
	 *
	 * Carregada no construtor junto com o cilindro, porque
	 * `ConstructorHelpers` só existe lá: escolher a forma na hora de montar
	 * exige as duas já na mão.
	 */
	UPROPERTY()
	TObjectPtr<UStaticMesh> SquareGroundAsset;

	/**
	 * O papel de paleta do chão deste pedaço; `Count` quer dizer "não sou
	 * pedaço".
	 *
	 * `Count` não é papel nenhum, e é justamente por isso que serve de
	 * ausência aqui: qualquer outro valor seria um bioma legítimo escolhido
	 * por engano.
	 */
	EScenaryRole RegionGroundRole = EScenaryRole::Count;

	/** Chão emprestado pelo mundo; vazio faz valer a paleta do cenário. */
	UPROPERTY()
	TObjectPtr<UMaterialInterface> AdoptedGroundMaterial;

	UPROPERTY()
	TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> SpeciesClusters;

	/**
	 * Vida de cada obstáculo já golpeado, pela chave opaca.
	 *
	 * Só o que APANHOU entra no mapa: guardar a vida cheia das 400 instâncias
	 * desde o começo gastaria memória para dizer o que a tabela de papéis já
	 * diz. Ausente significa "inteiro".
	 */
	UPROPERTY()
	TMap<int32, int32> ObstacleHealthByHandle;

	/** O papel de cada agrupamento, na mesma ordem de SpeciesClusters. */
	TArray<EScenaryRole> SpeciesRoles;
};

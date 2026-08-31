// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattleArena.h"
#include "IslandBiomeTestHelper.h"
#include "Battle/BattleTypes.h"
#include "Environment/ForestBackdrop.h"
#include "Environment/IslandGeography.h"
#include "Environment/ScenaryClimate.h"
#include "Environment/ScenaryPalette.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Misc/AutomationTest.h"
#include "UObject/SoftObjectPtr.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"

namespace ArenaCena
{
	UWorld* CriarMundoDaGeometria()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
		FWorldContext& Contexto = GEngine->CreateNewWorldContext(EWorldType::Game);
		Contexto.SetCurrentWorld(World);
		World->InitializeActorsForPlay(FURL());
		return World;
	}

	void DestruirMundoDaGeometria(UWorld* World)
	{
		if (!World)
		{
			return;
		}
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
	}

	/** Uma laje larga e sólida com material conhecido, para a sonda encontrar. */
	AActor* CriarChao(UWorld* World, const FVector& Onde, UMaterialInterface* Material)
	{
		AActor* Chao = World->SpawnActor<AActor>(AActor::StaticClass(), Onde, FRotator::ZeroRotator);
		UStaticMeshComponent* Malha = NewObject<UStaticMeshComponent>(Chao);
		Malha->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
		Malha->SetMaterial(0, Material);
		Malha->SetWorldScale3D(FVector(40.0f, 40.0f, 1.0f));
		Malha->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Malha->SetCollisionResponseToAllChannels(ECR_Block);
		Chao->SetRootComponent(Malha);
		Malha->RegisterComponent();
		// DEPOIS de registrar: componente que ainda não é raiz tem transformada
		// própria, e a laje nascia na origem por mais longe que se pedisse.
		Malha->SetWorldLocation(Onde);
		return Chao;
	}
}

// O tabuleiro precisa EXISTIR na tela, não só na lógica.
//
// Três vezes neste projeto um ator nasceu com componente visual e sem asset:
// os pets, os inimigos do mundo e o próprio jogador. Nos três casos toda a
// lógica passava e não havia nada para ver. A arena é o quarto candidato — e
// é o maior, porque é o fundo de tudo.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArenaMeshesHaveAssignedAssetsTest,
	"BattleSquare.Battle.Arena.MeshesHaveAssignedAssets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArenaMeshesHaveAssignedAssetsTest::RunTest(const FString& Parameters)
{
	const ABattleArena* Padrao = GetDefault<ABattleArena>();

	const TArray<TObjectPtr<UStaticMeshComponent>>& Lajes = Padrao->GetCellTileMeshes();
	TestEqual(TEXT("Uma laje por casa da grade configurada"), Lajes.Num(),
		Padrao->GridColumns * Padrao->GridRows);
	for (const UStaticMeshComponent* Laje : Lajes)
	{
		TestTrue(TEXT("Laje existe e tem malha ATRIBUÍDA"),
			Laje != nullptr && Laje->GetStaticMesh() != nullptr);
	}

	return true;
}

// Malha sem material é malha cinza: metade do defeito, e a metade que faz
// água, dano e bônus ficarem indistinguíveis.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArenaMaterialsArePointedAtTest,
	"BattleSquare.Battle.Arena.MaterialsArePointedAt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArenaMaterialsArePointedAtTest::RunTest(const FString& Parameters)
{
	const ABattleArena* Padrao = GetDefault<ABattleArena>();

	const TPair<const TCHAR*, const TSoftObjectPtr<UMaterialInterface>*> Materiais[] = {
		{ TEXT("neutro"), &Padrao->NeutralTileMaterial },
		{ TEXT("água"), &Padrao->WaterTileMaterial },
		{ TEXT("dano"), &Padrao->DamageTileMaterial },
		{ TEXT("bônus"), &Padrao->BuffTileMaterial },
		{ TEXT("bloqueada"), &Padrao->BlockedTileMaterial },
	};

	for (const TPair<const TCHAR*, const TSoftObjectPtr<UMaterialInterface>*>& Par : Materiais)
	{
		TestTrue(FString::Printf(TEXT("Material de %s aponta para um asset"), Par.Key),
			!Par.Value->IsNull());
	}

	return true;
}

// A altura vem de UMA função, e é ela que a laje e a grade desenhada usam.
// Se voltarem a ser dois números, a água afunda a laje e deixa o contorno
// boiando — foi o modo de falhar de L-032 e L-033.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArenaSurfaceHeightVariesByTerrainTest,
	"BattleSquare.Battle.Arena.SurfaceHeightVariesByTerrain",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArenaSurfaceHeightVariesByTerrainTest::RunTest(const FString& Parameters)
{
	const float Neutra = ABattleArena::GetCellSurfaceHeight(static_cast<uint8>(ECellProperty::None));
	const float Bloqueada = ABattleArena::GetCellSurfaceHeight(static_cast<uint8>(ECellProperty::Blocked));

	// A água ganhou DUAS alturas, e a pergunta "ela afunda?" passou a ser
	// sobre o PÉ. A intenção deste teste continua a mesma — submergir precisa
	// parecer submergir — só que agora ela mora no conceito certo: a lâmina é
	// o que se vê, o pé é onde se entra.
	//
	// Antes as duas eram uma só, e afundar as duas juntas punha a água DEBAIXO
	// do solo maciço da clareira: o jogador via terra numa casa de água.
	const float Agua = ABattleArena::GetCellFootingHeight(static_cast<uint8>(ECellProperty::Water));

	TestTrue(TEXT("Na água o PÉ afunda — é onde se submerge"), Agua < Neutra);
	TestTrue(TEXT("E a LÂMINA aparece, acima do solo"),
		ABattleArena::GetCellSurfaceHeight(static_cast<uint8>(ECellProperty::Water)) >= Neutra);
	TestTrue(TEXT("Bloqueada SOBE — ninguém pisa nela"), Bloqueada > Neutra);

	// Dano e bônus mudam de cor, não de altura: quem anda por cima delas anda
	// no mesmo plano do resto, e um degrau ali seria mentira sobre a regra.
	TestEqual(TEXT("Dano no plano de andar"),
		ABattleArena::GetCellSurfaceHeight(static_cast<uint8>(ECellProperty::Damage)), Neutra);
	TestEqual(TEXT("Bônus no plano de andar"),
		ABattleArena::GetCellSurfaceHeight(static_cast<uint8>(ECellProperty::Buff)), Neutra);

	return true;
}

// O pet fica NA SUPERFÍCIE da casa, seja ela qual for.
//
// Enquanto GetCellWorldLocation devolvia Z=0, a laje tinha relevo e o pet não:
// sobre a água ele boiava no ar, sobre a pedra ficava enterrado nela. A altura
// já tinha uma dona (GetCellSurfaceHeight) — o defeito foi esta função não
// perguntar a ela, que é o mesmo modo de falhar de L-032 e L-033.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArenaCellLocationFollowsTerrainTest,
	"BattleSquare.Battle.Arena.CellLocationFollowsTerrain",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArenaCellLocationFollowsTerrainTest::RunTest(const FString& Parameters)
{
	UWorld* World = ArenaCena::CriarMundoDaGeometria();
	ABattleArena* Arena = World->SpawnActor<ABattleArena>();

	FBattleState& Estado = Arena->GetMutableCurrentState();
	Estado.CellLayout[Estado.CellIndex(0, 0)] = static_cast<uint8>(ECellProperty::Water);
	Estado.CellLayout[Estado.CellIndex(2, 2)] = static_cast<uint8>(ECellProperty::Blocked);

	// Sem o redesenho o obstáculo ainda não tem tamanho nem assentamento, e
	// a casa bloqueada não teria topo para o pé encostar.
	Arena->RefreshTileVisuals();

	const float Base = Arena->GetActorLocation().Z;
	const float NaAgua = Arena->GetCellWorldLocation(0, 0).Z;
	const float NoPlano = Arena->GetCellWorldLocation(1, 1).Z;
	const float NaPedra = Arena->GetCellWorldLocation(2, 2).Z;

	TestTrue(TEXT("Na água o pé desce"), NaAgua < NoPlano);
	TestTrue(TEXT("Na pedra o pé sobe"), NaPedra > NoPlano);

	// Mesma fonte que posiciona o pet: dois números divergem na primeira
	// edição. Na água o pé NÃO é a superfície desenhada — a lâmina fica
	// visível acima do solo e o pé desce por dentro dela — então a fonte certa
	// é a do PÉ, e conferir contra a lâmina afirmaria que o pet fica de pé
	// sobre a água.
	TestEqual(TEXT("A altura é exatamente onde o PÉ pousa naquela casa"),
		NaAgua - Base,
		ABattleArena::GetCellFootingHeight(static_cast<uint8>(ECellProperty::Water)));

	// Na casa BLOQUEADA o pé não encosta na laje: encosta no TOPO do tronco,
	// porque quem está ali escalou. Antes disto o pet subia e renderizava por
	// dentro da pedra — de novo o defeito de afundar no tabuleiro.
	//
	// A medida sai da caixa do PRÓPRIO componente, e não de `altura * casa`:
	// a escala do obstáculo ainda é aparada para ele não invadir a casa
	// vizinha, então a conta refeita acharia um topo que a pedra larga não
	// tem.
	const TArray<TObjectPtr<UStaticMeshComponent>>& Obstaculos = Arena->GetCellObstacleMeshes();
	const int32 IndiceDaPedra = Estado.CellIndex(2, 2);
	if (!TestTrue(TEXT("a casa bloqueada tem componente de obstaculo"),
		Obstaculos.IsValidIndex(IndiceDaPedra) && Obstaculos[IndiceDaPedra] != nullptr))
	{
		ArenaCena::DestruirMundoDaGeometria(World);
		return false;
	}

	const FBoxSphereBounds CaixaDaPedra =
		Obstaculos[IndiceDaPedra]->CalcBounds(Obstaculos[IndiceDaPedra]->GetRelativeTransform());
	const float TopoDaPedra = static_cast<float>(CaixaDaPedra.Origin.Z + CaixaDaPedra.BoxExtent.Z);

	TestEqual(TEXT("Quem escalou fica em cima do obstaculo, nao dentro dele"),
		Arena->GetCellSurfaceHeightAt(2, 2), TopoDaPedra, 1.0f);
	TestTrue(TEXT("E isso e' bem acima da laje bloqueada"),
		Arena->GetCellSurfaceHeightAt(2, 2)
			> ABattleArena::GetCellSurfaceHeight(static_cast<uint8>(ECellProperty::Blocked))
				+ Arena->CellSize * 0.5f);

	ArenaCena::DestruirMundoDaGeometria(World);
	return true;
}

// A arena veste o lugar de onde o jogador veio.
//
// Ela nasce a um milhão de unidades do mundo (DP-enc-03): luz, céu e névoa são
// globais e já a alcançam, mas o CHÃO não — e era o chão que denunciava que a
// batalha se passava noutro lugar.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArenaAdoptsAmbienceFromWorldTest,
	"BattleSquare.Battle.Arena.AdoptsAmbienceFromWorld",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArenaAdoptsAmbienceFromWorldTest::RunTest(const FString& Parameters)
{
	UWorld* World = ArenaCena::CriarMundoDaGeometria();

	UMaterialInterface* MaterialDoLugar = LoadObject<UMaterialInterface>(
		nullptr, TEXT("/Engine/EngineMaterials/WorldGridMaterial.WorldGridMaterial"));
	if (!TestNotNull(TEXT("Material de referência carregou"), MaterialDoLugar))
	{
		ArenaCena::DestruirMundoDaGeometria(World);
		return false;
	}

	AActor* Terreno = ArenaCena::CriarChao(World, FVector::ZeroVector, MaterialDoLugar);
	TestNotNull(TEXT("Chão de teste criado"), Terreno);

	ABattleArena* Arena = World->SpawnActor<ABattleArena>(ABattleArena::StaticClass(),
		FVector(0.0f, 0.0f, 100000.0f), FRotator::ZeroRotator);

	TestTrue(TEXT("Adotou o ambiente do lugar do encontro"),
		Arena->AdoptAmbienceFromWorldLocation(FVector::ZeroVector));
	TestTrue(TEXT("E o chão da arena é o material DE LÁ"),
		Arena->GetAdoptedFloorMaterial() == MaterialDoLugar);

	ArenaCena::DestruirMundoDaGeometria(World);
	return true;
}

// Sem chão sondável a paleta autorada continua valendo: degrada, não quebra.
// A tela de batalha avulsa (BattleScreen) não nasce de encontro nenhum, e uma
// arena que recusasse existir sem mundo por baixo mataria justamente o caminho
// mais usado em desenvolvimento.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArenaKeepsOwnPaletteWithoutGroundTest,
	"BattleSquare.Battle.Arena.KeepsOwnPaletteWithoutGround",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArenaKeepsOwnPaletteWithoutGroundTest::RunTest(const FString& Parameters)
{
	UWorld* World = ArenaCena::CriarMundoDaGeometria();
	ABattleArena* Arena = World->SpawnActor<ABattleArena>();

	TestFalse(TEXT("Sem chão embaixo, não há o que adotar"),
		Arena->AdoptAmbienceFromWorldLocation(FVector(0.0f, 0.0f, 500000.0f)));
	TestNull(TEXT("E nada foi emprestado"), Arena->GetAdoptedFloorMaterial());

	ArenaCena::DestruirMundoDaGeometria(World);
	return true;
}

// O tabuleiro tem que ficar ACIMA do chão do nível.
//
// Medido em 2026-08-29: `Floor_0` de BattleScreen é um plano em Z=-0.5, e a
// laje de andar tem topo em -4. Sem elevação o tabuleiro nasce enterrado —
// o mesmo defeito de "existe na lógica, não existe na tela", só que por
// profundidade em vez de asset faltando.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArenaSitsAboveLevelGroundTest,
	"BattleSquare.Battle.Arena.SitsAboveLevelGround",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArenaSitsAboveLevelGroundTest::RunTest(const FString& Parameters)
{
	const ABattleArena* Padrao = GetDefault<ABattleArena>();

	// A água é o ponto mais fundo em que ainda se anda: se ela passa, todo o
	// resto do tabuleiro passa junto.
	const float TopoDaAgua = Padrao->BoardElevation
		+ ABattleArena::GetCellSurfaceHeight(static_cast<uint8>(ECellProperty::Water));

	TestTrue(TEXT("Até a casa mais funda fica acima do chão do nível"), TopoDaAgua > 0.0f);

	return true;
}

// A arena tem que ser PARTE DO CENÁRIO, não uma placa posta em cima dele.
//
// Duas vezes o usuário viu a mesma coisa na tela: primeiro "uns quadrados na
// arena marcados com bordas", depois "a arena está um redondo". As duas são o
// mesmo defeito — o tabuleiro trazia chão próprio. Achatar a casa neutra até
// o nível da terra escondeu a borda; o prato de terra escondeu o degrau. Nada
// disso tirava a placa de cima do cenário.
//
// Agora a casa sem regra NÃO É DESENHADA: o que se vê ali é o chão da mata.
// Só o que carrega regra ganha laje, e aí o relevo tem serventia, porque
// água, dano, bônus e bloqueio precisam ser achados de relance.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArenaNeutralCellIsTheGroundTest,
	"BattleSquare.Battle.Arena.NeutralCellIsTheGroundItself",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArenaNeutralCellIsTheGroundTest::RunTest(const FString& Parameters)
{
	UWorld* World = ArenaCena::CriarMundoDaGeometria();
	ABattleArena* Arena = World->SpawnActor<ABattleArena>();

	FBattleState& Estado = Arena->GetMutableCurrentState();
	Estado.CellLayout[Estado.CellIndex(0, 0)] = static_cast<uint8>(ECellProperty::Water);

	Arena->DispatchBeginPlay();

	const TArray<TObjectPtr<UStaticMeshComponent>>& Lajes = Arena->GetCellTileMeshes();
	const int32 Colunas = Arena->GetActiveGridColumns();

	const UStaticMeshComponent* ComRegra = Lajes[CellLayoutIndex(0, 0, Colunas)];
	const UStaticMeshComponent* SemRegra = Lajes[CellLayoutIndex(1, 1, Colunas)];

	if (TestNotNull(TEXT("A casa de água existe"), ComRegra)
		&& TestNotNull(TEXT("A casa neutra existe"), SemRegra))
	{
		TestFalse(TEXT("A casa SEM REGRA não é desenhada — ela é o chão da mata"),
			SemRegra->IsVisible());
		TestTrue(TEXT("E a casa COM REGRA é desenhada, senão a água some"),
			ComRegra->IsVisible());
	}

	ArenaCena::DestruirMundoDaGeometria(World);
	return true;
}

// A batalha acontece SOBRE O CHÃO DA MATA, não sobre um prato de terra.
//
// "A arena está um redondo, não precisa disso, deixe como se fosse o cenário
// real, estamos lutando em parte dele" — 2026-08-29, olhando a tela. A arena
// desenhava um disco de terra próprio porque a mata ficava BoardElevation
// abaixo dela, e alguma coisa tinha que tapar o degrau. Encostar o topo do
// chão da mata na superfície da casa neutra desfaz o degrau e a necessidade.
//
// O teste mede o ENCOSTE. Enquanto ele valer, não há degrau para tapar, e o
// prato não pode voltar por falta de chão.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArenaBoardSitsOnTheForestFloorTest,
	"BattleSquare.Battle.Arena.BoardSitsOnTheForestFloor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArenaBoardSitsOnTheForestFloorTest::RunTest(const FString& Parameters)
{
	UWorld* World = ArenaCena::CriarMundoDaGeometria();
	ABattleArena* Arena = World->SpawnActor<ABattleArena>();
	Arena->DispatchBeginPlay();

	AForestBackdrop* Mata = Arena->GetForestBackdrop();
	if (!TestNotNull(TEXT("A mata existe"), Mata))
	{
		ArenaCena::DestruirMundoDaGeometria(World);
		return false;
	}

	// Onde o pet pisa numa casa sem regra — a mesma altura que a arena usa
	// para posicionar o pet, e por isso a única referência honesta aqui.
	const float SuperficieDaCasa =
		Arena->GetCellWorldLocation(/*Column=*/1, /*Row=*/1).Z;
	const float TopoDoChaoDaMata =
		Mata->GetActorLocation().Z + AForestBackdrop::GroundTopLocalZ();

	TestEqual(TEXT("O topo do chão da mata É a superfície da casa"),
		TopoDoChaoDaMata, SuperficieDaCasa, 0.01f);

	ArenaCena::DestruirMundoDaGeometria(World);
	return true;
}

// A casa BLOQUEADA não pode ser só um piso um pouco mais alto.
//
// Era o que ela era: quem olhava via chão, e "não dá para andar aqui" ficava
// por conta de quem já sabia da regra. Um tronco ou uma pedra ocupando a casa
// diz isso sozinho — e é sobre ele que destruir e escalar vão se apoiar.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArenaBlockedCellCarriesAnObstacleTest,
	"BattleSquare.Battle.Arena.BlockedCellCarriesAnObstacle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArenaBlockedCellCarriesAnObstacleTest::RunTest(const FString& Parameters)
{
	UWorld* World = ArenaCena::CriarMundoDaGeometria();
	ABattleArena* Arena = World->SpawnActor<ABattleArena>();

	FBattleState& Estado = Arena->GetMutableCurrentState();
	for (uint8& Casa : Estado.CellLayout)
	{
		Casa = static_cast<uint8>(ECellProperty::None);
	}
	Estado.CellLayout[Estado.CellIndex(2, 2)] = static_cast<uint8>(ECellProperty::Blocked);
	Estado.CellLayout[Estado.CellIndex(0, 0)] = static_cast<uint8>(ECellProperty::Water);

	Arena->RefreshTileVisuals();

	const TArray<TObjectPtr<UStaticMeshComponent>>& Obstaculos = Arena->GetCellObstacleMeshes();
	TestEqual(TEXT("ha um obstaculo por casa, criado no construtor"),
		Obstaculos.Num(), Arena->GetCellTileMeshes().Num());

	const int32 NaPedra = Estado.CellIndex(2, 2);
	const int32 NaAgua = Estado.CellIndex(0, 0);
	const int32 NoPlano = Estado.CellIndex(1, 1);

	if (!TestTrue(TEXT("os indices das casas existem"),
		Obstaculos.IsValidIndex(NaPedra) && Obstaculos.IsValidIndex(NaAgua) && Obstaculos.IsValidIndex(NoPlano)))
	{
		ArenaCena::DestruirMundoDaGeometria(World);
		return false;
	}

	TestTrue(TEXT("a casa bloqueada MOSTRA o obstaculo"), Obstaculos[NaPedra]->IsVisible());
	TestFalse(TEXT("a agua nao ganha obstaculo"), Obstaculos[NaAgua]->IsVisible());
	TestFalse(TEXT("a casa sem regra nao ganha obstaculo"), Obstaculos[NoPlano]->IsVisible());

	// O mesmo defeito de sempre: componente visível sem asset atribuído passa
	// em toda lógica e não existe na tela.
	UStaticMesh* Malha = Obstaculos[NaPedra]->GetStaticMesh();
	if (!TestNotNull(TEXT("o obstaculo tem malha atribuida"), Malha))
	{
		ArenaCena::DestruirMundoDaGeometria(World);
		return false;
	}

	// E ele precisa TER volume. A medida sai da CAIXA DO COMPONENTE, não de
	// uma conta refeita aqui: refazer a conta é escrever a regra duas vezes,
	// e as duas cópias concordam até a primeira edição.
	const FBoxSphereBounds Caixa =
		Obstaculos[NaPedra]->CalcBounds(Obstaculos[NaPedra]->GetRelativeTransform());

	TestTrue(TEXT("ele sobe acima da laje, nao e' um piso"),
		Caixa.BoxExtent.Z * 2.0f > Arena->CellSize * 0.5f);
	TestTrue(TEXT("e nao invade a casa vizinha"),
		FMath::Max(Caixa.BoxExtent.X, Caixa.BoxExtent.Y) * 2.0f <= Arena->CellSize);

	// Ele ASSENTA no topo da casa bloqueada: nem flutuando sobre a laje, nem
	// com meio corpo dentro dela — o defeito que os pets já tiveram.
	TestEqual(TEXT("o pe do obstaculo encosta na superficie da casa bloqueada"),
		static_cast<float>(Caixa.Origin.Z - Caixa.BoxExtent.Z),
		ABattleArena::GetCellSurfaceHeight(static_cast<uint8>(ECellProperty::Blocked)),
		1.0f);

	ArenaCena::DestruirMundoDaGeometria(World);
	return true;
}

// O que ocupa a casa sai das COORDENADAS dela: o mesmo campo dá sempre a
// mesma cena. Sorteio com relógio faria a arena mudar de cara a cada abertura
// e tornaria impossível reproduzir o que o usuário viu na tela.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArenaObstacleIsDeterministicPerCellTest,
	"BattleSquare.Battle.Arena.ObstacleIsDeterministicPerCell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArenaObstacleIsDeterministicPerCellTest::RunTest(const FString& Parameters)
{
	UWorld* World = ArenaCena::CriarMundoDaGeometria();

	const auto CenaDaCasaBloqueada = [](ABattleArena* Arena)
	{
		FBattleState& Estado = Arena->GetMutableCurrentState();
		Estado.CellLayout[Estado.CellIndex(2, 2)] = static_cast<uint8>(ECellProperty::Blocked);
		Arena->RefreshTileVisuals();

		UStaticMeshComponent* Obstaculo = Arena->GetCellObstacleMeshes()[Estado.CellIndex(2, 2)];
		return TPair<UStaticMesh*, FRotator>(Obstaculo->GetStaticMesh(), Obstaculo->GetRelativeRotation());
	};

	ABattleArena* Primeira = World->SpawnActor<ABattleArena>();
	ABattleArena* Segunda = World->SpawnActor<ABattleArena>();

	const TPair<UStaticMesh*, FRotator> Uma = CenaDaCasaBloqueada(Primeira);
	const TPair<UStaticMesh*, FRotator> Outra = CenaDaCasaBloqueada(Segunda);

	TestEqual(TEXT("a mesma casa recebe a mesma materia"), Uma.Key, Outra.Key);
	TestTrue(TEXT("e o mesmo giro"), Uma.Value.Equals(Outra.Value, 0.01f));

	ArenaCena::DestruirMundoDaGeometria(World);
	return true;
}

// A casa BLOQUEADA é volume, não superfície.
//
// Ela desenhava laje E pedra: a laje lia como piso de relevo retangular — um
// chão elevado, que é o oposto do que ela quer dizer. Quem explica a casa
// bloqueada é a pedra que está nela.
//
// Este teste fixa a REGRA, e não a aparência: casa bloqueada carrega
// obstáculo, e obstáculo dispensa laje.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBlockedCellIsVolumeNotSurfaceTest,
	"BattleSquare.Battle.Arena.BlockedCellIsVolumeNotSurface",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBlockedCellIsVolumeNotSurfaceTest::RunTest(const FString& Parameters)
{
	UWorld* Mundo = ArenaCena::CriarMundoDaGeometria();
	if (!Mundo)
	{
		AddError(TEXT("não foi possível criar mundo de teste"));
		return false;
	}

	ABattleArena* Arena = Mundo->SpawnActor<ABattleArena>();

	FBattleState Estado;
	FPetState Meu;
	Meu.PetId = 1; Meu.Side = 0; Meu.Health = 50; Meu.MaxHealth = 50;
	FPetState Dele;
	Dele.PetId = 2; Dele.Side = 1; Dele.Health = 50; Dele.MaxHealth = 50;
	Estado.Pets.Add(Meu);
	Estado.Pets.Add(Dele);
	Estado.PlaceDuelistsAtStartingCells();

	// Uma casa BLOQUEADA longe dos pets, para a montagem não ser recusada.
	Estado.CellLayout[Estado.CellIndex(0, 0)] = static_cast<uint8>(ECellProperty::Blocked);

	TArray<FPetPresentationInfo> Apresentacoes;
	Apresentacoes.AddDefaulted(2);
	Apresentacoes[0].PetId = 1;
	Apresentacoes[1].PetId = 2;

	if (!Arena->BeginBattle(Estado, Apresentacoes))
	{
		AddError(TEXT("BeginBattle recusou a montagem"));
		ArenaCena::DestruirMundoDaGeometria(Mundo);
		return false;
	}

	const int32 Indice = Estado.CellIndex(0, 0);
	const TArray<TObjectPtr<UStaticMeshComponent>>& Lajes = Arena->GetCellTileMeshes();
	const TArray<TObjectPtr<UStaticMeshComponent>>& Obstaculos = Arena->GetCellObstacleMeshes();

	if (Lajes.IsValidIndex(Indice) && Lajes[Indice])
	{
		TestFalse(TEXT("A casa bloqueada NÃO desenha laje — ela não é piso"),
			Lajes[Indice]->IsVisible());
	}

	if (Obstaculos.IsValidIndex(Indice) && Obstaculos[Indice])
	{
		TestTrue(TEXT("Ela desenha OBSTÁCULO — é ele que a explica"),
			Obstaculos[Indice]->IsVisible());
	}

	ArenaCena::DestruirMundoDaGeometria(Mundo);
	return true;
}

// A ÁGUA PRECISA APARECER, e para isso ela tem de ficar acima do solo.
//
// Ela afundava dezoito unidades abaixo do neutro — o que funcionava enquanto a
// arena tinha chão próprio com vão. Desde que a clareira da mata virou o piso,
// o solo é um disco MACIÇO: a água ficava debaixo dele e nunca aparecia. O
// jogador via terra numa casa que a regra chamava de água, e submergir ali
// parecia defeito.
//
// Este teste fixa a RELAÇÃO, e não o número: qualquer terreno que se veja
// precisa estar na superfície do chão ou acima dela. Assim ele continua
// valendo quando as alturas forem ajustadas.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FVisibleTerrainIsNeverBuriedTest,
	"BattleSquare.Battle.Arena.VisibleTerrainIsNeverBuried",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FVisibleTerrainIsNeverBuriedTest::RunTest(const FString& Parameters)
{
	// O topo do chão da clareira é a superfície da casa neutra: é ali que a
	// mata é posicionada, e é o que o jogador enxerga como solo.
	const float TopoDoSolo =
		ABattleArena::GetCellSurfaceHeight(static_cast<uint8>(ECellProperty::None));

	const uint8 QueSeVeem[] = {
		static_cast<uint8>(ECellProperty::Water),
		static_cast<uint8>(ECellProperty::Damage),
		static_cast<uint8>(ECellProperty::Buff),
		static_cast<uint8>(ECellProperty::Blocked),
	};

	for (const uint8 Terreno : QueSeVeem)
	{
		const float Superficie = ABattleArena::GetCellSurfaceHeight(Terreno);
		TestTrue(
			*FString::Printf(TEXT("O terreno %d não fica enterrado (%.1f contra solo %.1f)"),
				static_cast<int32>(Terreno), Superficie, TopoDoSolo),
			Superficie >= TopoDoSolo);
	}

	// E a ÁGUA tem DUAS alturas, que é o ponto: a lâmina se vê, o pé afunda.
	//
	// Confundi-las foi o defeito dos dois lados. Subir as duas juntas mostra a
	// água e apaga a profundidade; descer as duas dá profundidade e enterra a
	// água sob o solo maciço da clareira.
	const float Lamina = ABattleArena::GetCellSurfaceHeight(
		static_cast<uint8>(ECellProperty::Water));
	const float Pe = ABattleArena::GetCellFootingHeight(
		static_cast<uint8>(ECellProperty::Water));

	TestTrue(TEXT("A lâmina da água aparece acima do solo"), Lamina >= TopoDoSolo);
	TestTrue(TEXT("E o pé desce por dentro dela"), Pe < Lamina);

	// Nos outros terrenos não há por onde entrar: pé e superfície coincidem.
	for (const uint8 Terreno : QueSeVeem)
	{
		if (static_cast<ECellProperty>(Terreno) == ECellProperty::Water)
		{
			continue;
		}
		TestEqual(
			*FString::Printf(TEXT("No terreno %d o pé pousa na superfície"),
				static_cast<int32>(Terreno)),
			ABattleArena::GetCellFootingHeight(Terreno),
			ABattleArena::GetCellSurfaceHeight(Terreno));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFunduraSeVeNaGeometriaTest,
	"BattleSquare.Arena.Geometry.FunduraSeVeNaGeometria",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFunduraSeVeNaGeometriaTest::RunTest(const FString&)
{
	// Poça e água funda VESTEM o mesmo material — não há asset novo, e slot
	// que ninguém atribui é casa invisível, defeito que este projeto já pagou
	// três vezes. Então quem tem de separá-las é a GEOMETRIA, e é isto que
	// este teste trava: uma RELAÇÃO, não um número.
	const float PeNaPoca = ABattleArena::GetCellFootingHeight(
		static_cast<uint8>(ECellProperty::ShallowWater));
	const float PeNaAgua = ABattleArena::GetCellFootingHeight(
		static_cast<uint8>(ECellProperty::Water));
	const float PeNoSeco = ABattleArena::GetCellFootingHeight(
		static_cast<uint8>(ECellProperty::None));

	TestTrue(TEXT("Na poça o pé afunda menos que na água funda"),
		PeNaPoca > PeNaAgua);
	TestTrue(TEXT("Mas ainda afunda — poça é água, não chão"),
		PeNaPoca < PeNoSeco);

	// O GELO é sólido: pisa-se EM CIMA. Fosse o pé dentro dele como na água,
	// congelar não mudaria nada na tela e o jogador não veria a jogada
	// acontecer — que é a única forma de ele entender por que não submergiu.
	const float PeNoGelo = ABattleArena::GetCellFootingHeight(
		static_cast<uint8>(ECellProperty::Ice));
	TestTrue(TEXT("No gelo o pé fica ACIMA da água que ele cobre"),
		PeNoGelo > PeNaAgua);
	TestEqual(TEXT("E pousa na própria superfície, como todo chão sólido"),
		PeNoGelo, ABattleArena::GetCellSurfaceHeight(
			static_cast<uint8>(ECellProperty::Ice)));

	// A lâmina da poça fica ABAIXO da lâmina da água funda: menos água,
	// menos volume visível. Invertido, a poça pareceria a mais funda das duas.
	TestTrue(TEXT("A lâmina da poça é mais baixa que a da água funda"),
		ABattleArena::GetCellSurfaceHeight(static_cast<uint8>(ECellProperty::ShallowWater))
			< ABattleArena::GetCellSurfaceHeight(static_cast<uint8>(ECellProperty::Water)));

	return true;
}

namespace ArenaBiomaDaLuta
{
	/**
	 * Um ponto no setor de geleira, longe do miolo de mata e da faixa de praia.
	 *
	 * Escrito em ângulo e raio, e não em X e Y à mão: o que importa é CAIR no
	 * setor: um par de coordenadas mágicas silenciaria o teste no dia em que
	 * os setores mudassem de largura, em vez de reprovar.
	 */
	FVector2D PontoNaGeleira()
	{
		// FRAÇÃO do raio, e não 12.000 unidades.
		//
		// O número absoluto caía na geleira quando a ilha tinha 20.000, e caiu
		// dentro do MIOLO — que é sempre mata — quando ela cresceu para
		// 140.000. O teste passou a afirmar "a geleira é mata", e a culpa não
		// era do código: era de uma posição escolhida quando só existia um
		// tamanho de ilha. Mesmo padrão dos anéis das peças e da região de
		// descoberta, agora nos testes.
		const float Angulo = FMath::DegreesToRadians(252.0f);
		const float Raio = IslandGeography::LandRadiusUnits() * 0.60f;
		return FVector2D(Raio * FMath::Cos(Angulo), Raio * FMath::Sin(Angulo));
	}
}

// A luta se passa no lugar onde ela começou.
//
// A arena lia o clima do `.ini`, um só para o mapa inteiro: lutar na geleira
// dava mata de floresta e serra com a neve do arquivo. A geografia SABIA onde
// o encontro foi — só o cenário da briga não sabia.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArenaWearsEncounterBiomeTest,
	"BattleSquare.Battle.Arena.WearsEncounterBiome",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArenaWearsEncounterBiomeTest::RunTest(const FString& Parameters)
{
	// UMA ILHA, UM BIOMA: "o ponto na geleira" deixou de ser uma direção e
	// passou a ser uma ILHA de geleira. O teste continua querendo geleira —
	// ele só pede de outro jeito.
	const IlhaDeTeste::FBiomaTemporario IlhaDeGeleira(TEXT("Glacier"));

	UWorld* World = ArenaCena::CriarMundoDaGeometria();

	const FVector2D NaGeleira = ArenaBiomaDaLuta::PontoNaGeleira();
	if (!TestEqual(TEXT("O ponto escolhido é geleira mesmo"),
		static_cast<int32>(IslandGeography::BiomeAt(NaGeleira)),
		static_cast<int32>(EIslandBiome::Glacier)))
	{
		ArenaCena::DestruirMundoDaGeometria(World);
		return false;
	}

	UMaterialInterface* MaterialDoLugar = LoadObject<UMaterialInterface>(
		nullptr, TEXT("/Engine/EngineMaterials/WorldGridMaterial.WorldGridMaterial"));
	const FVector Chao(NaGeleira.X, NaGeleira.Y, 0.0f);
	ArenaCena::CriarChao(World, Chao, MaterialDoLugar);

	ABattleArena* Arena = World->SpawnActor<ABattleArena>(ABattleArena::StaticClass(),
		FVector(NaGeleira.X, NaGeleira.Y, 100000.0f), FRotator::ZeroRotator);

	// A arena nasce e monta o cenário ANTES de alguém contar de onde a luta
	// veio — é essa ordem que obriga a mata a ser refeita, e é ela que o
	// teste precisa reproduzir.
	Arena->DispatchBeginPlay();

	TestTrue(TEXT("Adotou o ambiente do lugar do encontro"),
		Arena->AdoptAmbienceFromWorldLocation(Chao));

	if (!TestTrue(TEXT("E aprendeu de onde a luta veio"),
		Arena->GetEncounterBiome().IsSet()))
	{
		ArenaCena::DestruirMundoDaGeometria(World);
		return false;
	}
	TestEqual(TEXT("O bioma da luta é o do chão do encontro"),
		static_cast<int32>(Arena->GetEncounterBiome().GetValue()),
		static_cast<int32>(EIslandBiome::Glacier));

	// O chão da mata pela MESMA tabela que veste o pedaço do mundo. Uma
	// segunda, escrita para a arena, concordaria com esta até a primeira
	// edição — e o sintoma seria geleira com capim numa tela e sem na outra.
	if (TestNotNull(TEXT("A mata da arena existe"), Arena->GetForestBackdrop()))
	{
		TestEqual(TEXT("E veste o chão do bioma da luta"),
			static_cast<int32>(Arena->GetForestBackdrop()->GetRegionGroundRole()),
			static_cast<int32>(EScenaryRole::GlacierIce));
	}

	// A serra do fundo pelo mesmo lugar: gelo no horizonte é o que faz a
	// geleira ter tamanho. Com o `.ini` mandando, ela saía temperada.
	TestEqual(TEXT("A serra sai no clima do lugar, não no do arquivo"),
		static_cast<int32>(Arena->ResolveScenaryClimate()),
		static_cast<int32>(EScenaryClimate::Cold));

	ArenaCena::DestruirMundoDaGeometria(World);
	return true;
}

// Onde a luta foi é uma pergunta, e o chão herdado é outra.
//
// Vão sem colisão sobre a geleira ainda é geleira. Aprender o bioma DEPOIS da
// sondagem faria a batalha sobre um buraco voltar a ser floresta — e as duas
// recusas de herança continuam recusando, porque elas falam do material.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArenaLearnsBiomeWithoutGroundTest,
	"BattleSquare.Battle.Arena.LearnsBiomeWithoutGround",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArenaLearnsBiomeWithoutGroundTest::RunTest(const FString& Parameters)
{
	// UMA ILHA, UM BIOMA: "o ponto na geleira" deixou de ser uma direção e
	// passou a ser uma ILHA de geleira. O teste continua querendo geleira —
	// ele só pede de outro jeito.
	const IlhaDeTeste::FBiomaTemporario IlhaDeGeleira(TEXT("Glacier"));

	UWorld* World = ArenaCena::CriarMundoDaGeometria();
	ABattleArena* Arena = World->SpawnActor<ABattleArena>();

	const FVector2D NaGeleira = ArenaBiomaDaLuta::PontoNaGeleira();
	TestFalse(TEXT("Sem chão embaixo, não há material a herdar"),
		Arena->AdoptAmbienceFromWorldLocation(
			FVector(NaGeleira.X, NaGeleira.Y, 500000.0f)));
	TestNull(TEXT("E nada foi emprestado"), Arena->GetAdoptedFloorMaterial());

	if (TestTrue(TEXT("Mas o lugar da luta foi aprendido"),
		Arena->GetEncounterBiome().IsSet()))
	{
		TestEqual(TEXT("E é a geleira"),
			static_cast<int32>(Arena->GetEncounterBiome().GetValue()),
			static_cast<int32>(EIslandBiome::Glacier));
	}
	TestEqual(TEXT("A serra acompanha o lugar mesmo sem chão herdado"),
		static_cast<int32>(Arena->ResolveScenaryClimate()),
		static_cast<int32>(EScenaryClimate::Cold));

	ArenaCena::DestruirMundoDaGeometria(World);
	return true;
}

// Arena que ninguém situou continua sendo a mata de sempre.
//
// A tela de batalha avulsa e os testes não nascem de encontro nenhum. O `.ini`
// existe para alcançar deserto e clima bom sem recompilar, e ele continua
// sendo a resposta — o encontro só passa na frente quando existe.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArenaWithoutEncounterKeepsConfiguredClimateTest,
	"BattleSquare.Battle.Arena.WithoutEncounterKeepsConfiguredClimate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArenaWithoutEncounterKeepsConfiguredClimateTest::RunTest(const FString& Parameters)
{
	UWorld* World = ArenaCena::CriarMundoDaGeometria();
	ABattleArena* Arena = World->SpawnActor<ABattleArena>();
	Arena->DispatchBeginPlay();

	TestFalse(TEXT("Ninguém contou de onde a luta veio"),
		Arena->GetEncounterBiome().IsSet());
	TestEqual(TEXT("Então a serra sai no clima do arquivo"),
		static_cast<int32>(Arena->ResolveScenaryClimate()),
		static_cast<int32>(ScenaryClimate::ConfiguredClimate()));

	// `Count` quer dizer "a cor de chão de sempre", que é a da mata. Guardar
	// `Forest` no lugar do vazio apagaria a diferença entre não saber e saber.
	if (TestNotNull(TEXT("A mata da arena existe"), Arena->GetForestBackdrop()))
	{
		TestEqual(TEXT("E o chão é o da mata, sem papel de bioma"),
			static_cast<int32>(Arena->GetForestBackdrop()->GetRegionGroundRole()),
			static_cast<int32>(EScenaryRole::Count));
	}

	ArenaCena::DestruirMundoDaGeometria(World);
	return true;
}

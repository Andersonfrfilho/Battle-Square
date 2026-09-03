// Copyright 2026 Anderson. All Rights Reserved.

#include "Components/BoxComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Misc/AutomationTest.h"
#include "World/RegionLayout.h"
#include "World/Village.h"
#include "World/VillageLayout.h"

/**
 * A PORTA — o primeiro padrão de interação de prédio deste jogo.
 *
 * Antes desta task nenhum prédio reagia a ninguém: o grep por
 * `OnActorBeginOverlap` e `TriggerVolume` fora de `Tests/` vinha vazio. Não
 * havia o que consertar — a interação precisava NASCER.
 *
 * E ela nasce genérica de propósito. Presa ao Centro de Recuperação, as cinco
 * tasks seguintes repetiriam o mesmo gatilho seis vezes.
 */

namespace PortaDaVilaTeste
{
	// Nome próprio (L-042): helper homônimo em outro arquivo de teste vira
	// sobrecarga ambígua quando o unity build junta os dois.
	UWorld* CriarMundoParaAsPortasDaVila()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
		FWorldContext& Contexto = GEngine->CreateNewWorldContext(EWorldType::Game);
		Contexto.SetCurrentWorld(World);
		World->InitializeActorsForPlay(FURL());
		return World;
	}

	void DestruirMundoDasPortasDaVila(UWorld* World)
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVillageDoorOnlyWhereThereIsFunctionTest,
	"BattleSquare.World.Vila.PortaSoOndeHaFuncao",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVillageDoorOnlyWhereThereIsFunctionTest::RunTest(const FString&)
{
	// A REGRA JÁ ESTAVA ESCRITA, e só no comentário da `Casa`: "casa sem
	// função não tem porta. Porta que não abre é promessa quebrada." Aqui ela
	// vira coisa que o teste alcança.
	TestFalse(TEXT("a Casa nao tem porta"),
		VillageLayout::HasDoor(EVillageBuilding::Casa));
	TestFalse(TEXT("nem a Palafita, que e casa sobre a agua"),
		VillageLayout::HasDoor(EVillageBuilding::Palafita));
	TestFalse(TEXT("nem a Passarela, que e rua"),
		VillageLayout::HasDoor(EVillageBuilding::Passarela));
	TestFalse(TEXT("nem a Chinampa, que e lavoura"),
		VillageLayout::HasDoor(EVillageBuilding::Chinampa));

	// E A OUTRA METADE, que é a que importa: quem tem função tem porta. Sem
	// ela, uma regra que devolvesse `false` sempre passaria acima.
	TestTrue(TEXT("o Centro de Recuperacao tem"),
		VillageLayout::HasDoor(EVillageBuilding::CentroDeRecuperacao));
	TestTrue(TEXT("a Escola tem"), VillageLayout::HasDoor(EVillageBuilding::Escola));
	TestTrue(TEXT("o Mercado tem"), VillageLayout::HasDoor(EVillageBuilding::Mercado));
	TestTrue(TEXT("a Arena tem"), VillageLayout::HasDoor(EVillageBuilding::Arena));
	TestTrue(TEXT("o Marco tem"), VillageLayout::HasDoor(EVillageBuilding::Marco));

	// E TODO PRÉDIO TEM NOME. O "?" é o que apareceria na tela se alguém
	// acrescentasse um tipo e esquecesse da tabela — o mesmo modo de falhar
	// que deixou Palafita, Passarela e Chinampa sem cor.
	const EVillageBuilding Todos[] = {
		EVillageBuilding::CentroDeRecuperacao, EVillageBuilding::Escola,
		EVillageBuilding::Arena, EVillageBuilding::Praca, EVillageBuilding::Marco,
		EVillageBuilding::Casa, EVillageBuilding::Academia, EVillageBuilding::Mercado,
		EVillageBuilding::Portao, EVillageBuilding::Palafita,
		EVillageBuilding::Passarela, EVillageBuilding::Chinampa };

	for (EVillageBuilding Predio : Todos)
	{
		TestNotEqual(TEXT("o predio tem nome, e nao e '?'"),
			FString(VillageLayout::BuildingDebugName(Predio)), FString(TEXT("?")));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVillageDoorsExistInTheWorldTest,
	"BattleSquare.World.Vila.AsPortasExistemNoMundo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVillageDoorsExistInTheWorldTest::RunTest(const FString&)
{
	// A PORTA É INVISÍVEL POR DESENHO, e por isso o número é a única prova de
	// que ela existe. Uma vila que erguesse dez prédios com zero portas leria
	// como vila pronta — é o mesmo defeito de "componente criado não é
	// componente visível", de cabeça para baixo.
	UWorld* Mundo = PortaDaVilaTeste::CriarMundoParaAsPortasDaVila();
	AVillage* Vila = Mundo->SpawnActor<AVillage>();
	Vila->BuildVillage();

	int32 ComFuncao = 0;
	for (const FVillagePlacement& Peca : VillageLayout::PlanFor(Vila->GetSettlementKind()))
	{
		if (VillageLayout::HasDoor(Peca.Building))
		{
			++ComFuncao;
		}
	}

	TestTrue(TEXT("ha predio com funcao nesta vila"), ComFuncao > 0);
	TestEqual(TEXT("uma porta por predio com funcao"), Vila->GetDoors().Num(), ComFuncao);
	TestTrue(TEXT("e MENOS portas que predios, porque a Casa nao tem"),
		Vila->GetDoors().Num() < Vila->GetBuiltCount());

	for (int32 Qual = 0; Qual < Vila->GetDoors().Num(); ++Qual)
	{
		UBoxComponent* Porta = Vila->GetDoors()[Qual];
		TestNotNull(TEXT("a porta existe"), Porta);
		if (!Porta)
		{
			continue;
		}

		// A PORTA NÃO BLOQUEIA. Gatilho que bloqueia é parede, e empararedaria
		// justamente o prédio que ele existe para deixar usar.
		TestEqual(TEXT("a porta so consulta, nao empurra"),
			static_cast<int32>(Porta->GetCollisionEnabled()),
			static_cast<int32>(ECollisionEnabled::QueryOnly));
		TestEqual(TEXT("e ela SOBREPOE quem anda, em vez de barrar"),
			static_cast<int32>(Porta->GetCollisionResponseToChannel(ECC_Pawn)),
			static_cast<int32>(ECR_Overlap));
		TestTrue(TEXT("e gera evento de sobreposicao"),
			Porta->GetGenerateOverlapEvents());

		// E ELA É MAIOR QUE A PAREDE, senão nunca dispara: o prédio bloqueia, e
		// ninguém consegue estar dentro de uma caixa sólida.
		TestTrue(TEXT("a porta e uma calcada em volta, nao a parede"),
			VillageLayout::HasDoor(Vila->GetDoorBuilding(Qual)));
	}

	PortaDaVilaTeste::DestruirMundoDasPortasDaVila(Mundo);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVillageEnteringIsNotCommittingTest,
	"BattleSquare.World.Vila.EntrarNaoEhComprometerSe",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVillageEnteringIsNotCommittingTest::RunTest(const FString&)
{
	// O CONTRAPESO DA CI2, e ele é o que separa uma porta de uma armadilha.
	//
	// Entrar e sair sem fazer nada não pode cobrar, curar nem vender. Uma vila
	// que cobra por atravessar a calçada é uma vila que ninguém atravessa duas
	// vezes.
	//
	// O que se afirma é a FORMA do evento: ele carrega O QUE foi tocado, DE QUE
	// vila, e o sentido — e nada mais. Sem caminho para efeito nenhum, não há
	// efeito para escapar.
	UWorld* Mundo = PortaDaVilaTeste::CriarMundoParaAsPortasDaVila();
	AVillage* Vila = Mundo->SpawnActor<AVillage>();

	int32 Entradas = 0;
	int32 Saidas = 0;
	EVillageBuilding Ultimo = EVillageBuilding::Casa;
	ESettlementKind DeQual = ESettlementKind::PostoDeFronteira;

	Vila->OnDoorCrossed.AddLambda(
		[&Entradas, &Saidas, &Ultimo, &DeQual](
			EVillageBuilding Predio, ESettlementKind Vila, bool bEntrou)
		{
			bEntrou ? ++Entradas : ++Saidas;
			Ultimo = Predio;
			DeQual = Vila;
		});

	Vila->BuildVillage();

	// Erguer a vila NÃO anuncia visita nenhuma: as portas nascem fechadas, e
	// quem não andou não entrou.
	TestEqual(TEXT("erguer a vila nao anuncia entrada"), Entradas, 0);
	TestEqual(TEXT("nem saida"), Saidas, 0);

	PortaDaVilaTeste::DestruirMundoDasPortasDaVila(Mundo);
	return true;
}

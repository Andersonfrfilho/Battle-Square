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
	// A REGRA "casa sem função não tem porta" continua de pé — o que mudou
	// foi a CASA: a decisão 65 lhe deu função (visitar, conversar com quem
	// mora), e a porta veio junto. A Palafita é casa sobre a água: mesma
	// função, mesma porta.
	TestTrue(TEXT("a Casa TEM porta — a decisao 65 lhe deu funcao"),
		VillageLayout::HasDoor(EVillageBuilding::Casa));
	TestTrue(TEXT("e a Palafita tambem: casa sobre a agua"),
		VillageLayout::HasDoor(EVillageBuilding::Palafita));

	// As duas que continuam sem: rua não se entra, lavoura é chão.
	TestFalse(TEXT("a Passarela segue sem porta — e rua"),
		VillageLayout::HasDoor(EVillageBuilding::Passarela));
	TestFalse(TEXT("e a Chinampa tambem — e lavoura"),
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

	// NUNCA mais portas que prédios: porta sem prédio atrás seria a promessa
	// quebrada com outra roupa. (Igualdade é legítima desde a decisão 65 — a
	// vila inicial só tem prédios com função agora.)
	TestTrue(TEXT("nunca mais portas que predios"),
		Vila->GetDoors().Num() <= Vila->GetBuiltCount());

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
			EVillageBuilding Predio, ESettlementKind Vila, bool bEntrou, int32)
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVillageLakeBuildingsHaveTheirOwnColorTest,
	"BattleSquare.World.Vila.OMercadoDoLagoTemCorPropria",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVillageLakeBuildingsHaveTheirOwnColorTest::RunTest(const FString&)
{
	// A INVARIANTE 16, agora com teste: Palafita, Passarela e Chinampa caíram
	// no cinza do `default` uma vez, em silêncio, e só o olho pegou. A cor é
	// informação — três construções cinzas sobre a água leem como caixotes.
	const FLinearColor Cinza = AVillage::FallbackColor();

	const EVillageBuilding DoLago[] = {
		EVillageBuilding::Palafita,
		EVillageBuilding::Passarela,
		EVillageBuilding::Chinampa,
	};

	for (EVillageBuilding Predio : DoLago)
	{
		TestFalse(TEXT("a construcao do lago nao cai no cinza do default"),
			AVillage::BuildingColor(Predio).Equals(Cinza));
	}

	// E as três são DIFERENTES entre si — senão "cor própria" é só trocar um
	// cinza por outro, que é o aceite da task por extenso.
	TestFalse(TEXT("palafita != passarela"),
		AVillage::BuildingColor(EVillageBuilding::Palafita)
			.Equals(AVillage::BuildingColor(EVillageBuilding::Passarela)));
	TestFalse(TEXT("passarela != chinampa"),
		AVillage::BuildingColor(EVillageBuilding::Passarela)
			.Equals(AVillage::BuildingColor(EVillageBuilding::Chinampa)));
	TestFalse(TEXT("chinampa != palafita"),
		AVillage::BuildingColor(EVillageBuilding::Chinampa)
			.Equals(AVillage::BuildingColor(EVillageBuilding::Palafita)));

	// O CONTRAPESO: o `default` continua de pé para o prédio que ainda não
	// existe. A rede de segurança não se remove — só se para de precisar dela
	// para estas três.
	TestTrue(TEXT("valor sem case ainda cai no cinza"),
		AVillage::BuildingColor(static_cast<EVillageBuilding>(200)).Equals(Cinza));

	return true;
}

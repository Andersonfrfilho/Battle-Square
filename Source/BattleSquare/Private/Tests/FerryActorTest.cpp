// Copyright 2026 Anderson. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "World/FerryActor.h"
#include "World/TrailLayout.h"

/**
 * A BALSA É INTERAÇÃO, e não obra — e é isso que a T13 pedia.
 *
 * Eu tinha entregado uma laje parada acima da lâmina, e testado a ALTURA dela.
 * Altura certa, contagem certa, material certo: 25 decks no meio dos rios. Uma
 * plataforma que não leva ninguém a lugar nenhum não é balsa, e a distinção
 * que o traçado fez entre "largo demais para ponte" e "ponte" tinha sumido.
 *
 * As três coisas que fazem dela balsa, e nenhuma é a forma: ela ANDA, ela
 * FLUTUA, e ela é SÓLIDA — esbarra no que estiver no caminho em vez de
 * atravessar por dentro.
 */

namespace ProvaDaBalsa
{
	constexpr float VaoDeTeste = 2000.0f;
	constexpr float LaminaDeTeste = 500.0f;

	UWorld* MundoDeTeste()
	{
		return UWorld::CreateWorld(EWorldType::Game, false);
	}

	AFerryActor* NoRio(UWorld* Mundo)
	{
		AFerryActor* Balsa = Mundo->SpawnActor<AFerryActor>();
		Balsa->ConfigureFor(FVector2D::ZeroVector, FVector2D(1.0f, 0.0f),
			VaoDeTeste, LaminaDeTeste);
		return Balsa;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFerryActorAssignsItsMeshInTheConstructorTest,
	"BattleSquare.FerryActor.AssignsItsMeshInTheConstructor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFerryActorAssignsItsMeshInTheConstructorTest::RunTest(const FString& Parameters)
{
	const AFerryActor* Padrao = GetDefault<AFerryActor>();
	if (!TestNotNull(TEXT("a classe da balsa tem default"), Padrao))
	{
		return false;
	}

	const UStaticMeshComponent* Conves = Padrao->GetDeck();
	if (!TestNotNull(TEXT("a balsa tem conves"), Conves))
	{
		return false;
	}

	TestTrue(TEXT("o conves nasce com MALHA atribuida"),
		Conves->GetStaticMesh() != nullptr);
	TestTrue(TEXT("o conves nasce com material atribuido"),
		Conves->GetMaterial(0) != nullptr);

	// SÓLIDA e MÓVEL, e as duas juntas são o que faz dela balsa: sólida para
	// carregar e esbarrar, móvel para poder andar. Um convés estático
	// atravessaria a geometria por dentro em vez de bater nela.
	TestEqual(TEXT("o conves e solido"),
		static_cast<int32>(Conves->GetCollisionEnabled()),
		static_cast<int32>(ECollisionEnabled::QueryAndPhysics));
	TestEqual(TEXT("o conves e movel"),
		static_cast<int32>(Conves->Mobility),
		static_cast<int32>(EComponentMobility::Movable));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFerryActorGoesAndComesBackTest,
	"BattleSquare.FerryActor.GoesAndComesBack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFerryActorGoesAndComesBackTest::RunTest(const FString& Parameters)
{
	UWorld* Mundo = ProvaDaBalsa::MundoDeTeste();
	AFerryActor* Balsa = ProvaDaBalsa::NoRio(Mundo);

	// ELA ANDA. Uma balsa parada é um deck no meio do rio.
	const FVector Comecou = Balsa->GetActorLocation();
	TestTrue(TEXT("a balsa andou"), Balsa->AdvanceBy(1.0f));
	TestTrue(TEXT("a posicao mudou"),
		!Balsa->GetActorLocation().Equals(Comecou, 1.0f));

	// E ELA VOLTA. Uma balsa que só vai deixa a outra margem sem travessia
	// para sempre — e o teste de "andou" acima passaria feliz.
	const int32 RumoInicial = Balsa->GetHeading();

	bool bVirou = false;
	for (int32 Passo = 0; Passo < 400; ++Passo)
	{
		Balsa->AdvanceBy(0.1f);
		if (Balsa->GetHeading() != RumoInicial)
		{
			bVirou = true;
			break;
		}
	}

	TestTrue(TEXT("a balsa vira na margem"), bVirou);

	// E NUNCA PASSA DAS MARGENS. Sem esta, "virar" poderia acontecer depois de
	// ela ter saído nadando rio afora.
	for (int32 Passo = 0; Passo < 400; ++Passo)
	{
		Balsa->AdvanceBy(0.1f);

		if (FMath::Abs(Balsa->GetOffsetUnits())
			> ProvaDaBalsa::VaoDeTeste * 0.5f + 1.0f)
		{
			AddError(FString::Printf(
				TEXT("a balsa saiu do vao: %.1f de um meio-vao de %.1f"),
				Balsa->GetOffsetUnits(), ProvaDaBalsa::VaoDeTeste * 0.5f));
			Mundo->DestroyWorld(false);
			return false;
		}
	}

	Mundo->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFerryActorFloatsOnTheSurfaceTest,
	"BattleSquare.FerryActor.FloatsOnTheSurface",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFerryActorFloatsOnTheSurfaceTest::RunTest(const FString& Parameters)
{
	UWorld* Mundo = ProvaDaBalsa::MundoDeTeste();
	AFerryActor* Balsa = ProvaDaBalsa::NoRio(Mundo);

	// A LÂMINA manda na altura, não o leito. A balsa anda SOBRE a água, e
	// mantém isso ao longo de todo o percurso — uma altura que só valesse no
	// começo a deixaria afundando ou subindo enquanto atravessa.
	for (int32 Passo = 0; Passo < 60; ++Passo)
	{
		Balsa->AdvanceBy(0.1f);

		TestEqual(TEXT("a balsa flutua na lamina"),
			static_cast<float>(Balsa->GetActorLocation().Z),
			ProvaDaBalsa::LaminaDeTeste + AFerryActor::FreeboardUnits(), 1.0f);
	}

	// E ela flutua BAIXO: na altura de um tabuleiro de ponte, ela viraria uma
	// ponte que se move, e a distincao que o tracado fez entre "largo demais
	// para ponte" e "ponte" desapareceria.
	TestTrue(TEXT("a borda livre e pequena"),
		AFerryActor::FreeboardUnits() > 0.0f && AFerryActor::FreeboardUnits() < 100.0f);

	Mundo->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFerryActorBumpsIntoWhatIsInTheWayTest,
	"BattleSquare.FerryActor.BumpsIntoWhatIsInTheWay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFerryActorBumpsIntoWhatIsInTheWayTest::RunTest(const FString& Parameters)
{
	UWorld* Mundo = ProvaDaBalsa::MundoDeTeste();
	AFerryActor* Balsa = ProvaDaBalsa::NoRio(Mundo);

	// ELA ESBARRA. Geometria sólida que corre sobre a água não atravessa o que
	// houver pelo caminho — atravessar por dentro é exatamente o que um sólido
	// não pode fazer, e nenhum dos testes acima notaria: ela andaria, voltaria
	// e flutuaria, passando por dentro de tudo.
	//
	// Põe-se uma pedra no meio do percurso e anda-se a balsa contra ela.
	AStaticMeshActor* Pedra = Mundo->SpawnActor<AStaticMeshActor>();
	if (!Pedra)
	{
		AddError(TEXT("nao foi possivel plantar o obstaculo"));
		Mundo->DestroyWorld(false);
		return false;
	}

	UStaticMeshComponent* Corpo = Pedra->GetStaticMeshComponent();
	Corpo->SetMobility(EComponentMobility::Movable);
	Corpo->SetStaticMesh(LoadObject<UStaticMesh>(
		nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
	Corpo->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Corpo->SetCollisionResponseToAllChannels(ECR_Block);
	Corpo->SetWorldScale3D(FVector(6.0f, 12.0f, 6.0f));

	// No meio do vão, na altura em que a balsa passa.
	Pedra->SetActorLocation(FVector(0.0f, 0.0f,
		ProvaDaBalsa::LaminaDeTeste + AFerryActor::FreeboardUnits()));

	const float ComecouEm = Balsa->GetOffsetUnits();

	// Anda contra a pedra por tempo de sobra para atravessá-la, se fosse
	// atravessar.
	for (int32 Passo = 0; Passo < 120; ++Passo)
	{
		Balsa->AdvanceBy(0.05f);
	}

	// Ela NÃO pode ter chegado do outro lado da pedra. Partiu de -meio vão; se
	// terminar com um deslocamento claramente positivo, passou por dentro.
	if (Balsa->GetOffsetUnits() > 0.0f)
	{
		AddError(FString::Printf(
			TEXT("a balsa atravessou o obstaculo: comecou em %.1f e esta em %.1f"),
			ComecouEm, Balsa->GetOffsetUnits()));
		Mundo->DestroyWorld(false);
		return false;
	}

	Mundo->DestroyWorld(false);
	return true;
}

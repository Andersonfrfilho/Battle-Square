// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/PetView.h"
#include "Battle/BattleResolver.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Misc/AutomationTest.h"

namespace
{
	FBattleState MakePetViewDuelState()
	{
		FBattleState State;
		FPetState Left;
		Left.PetId = 1; Left.Side = 0; Left.Column = 1; Left.Row = 1;
		Left.Health = 50; Left.MaxHealth = 50; Left.Attack = 20; Left.Defense = 0;
		FPetState Right;
		Right.PetId = 2; Right.Side = 1; Right.Column = 2; Right.Row = 1;
		Right.Health = 50; Right.MaxHealth = 50; Right.Attack = 10; Right.Defense = 5;
		State.Pets.Add(Left);
		State.Pets.Add(Right);
		return State;
	}
}

// T8: SetInitialState + ApplyEvent reagem a um trace REAL de uma
// resolução completa — nunca recalculam dano/posição, só leem os campos
// que o núcleo já preencheu.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPetViewAppliesEventsFromRealTraceTest,
	"BattleSquare.PetView.AppliesEventsFromRealTrace",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPetViewAppliesEventsFromRealTraceTest::RunTest(const FString& Parameters)
{
	const FBattleState State = MakePetViewDuelState();

	FTurnCommit LeftCommit;
	LeftCommit.Actions[0] = { EActionType::Atacar, EBattleDirection::Direita };
	FTurnCommit RightCommit;
	RightCommit.Actions[0] = { EActionType::Aguardar, EBattleDirection::Nenhuma };

	const FBattleResolveResult Result = FBattleResolver::ResolveTurn(State, LeftCommit, RightCommit);

	bool bFoundDamageEvent = false;
	int32 ExpectedDamage = 0;
	for (const FBattleEvent& Event : Result.Trace)
	{
		// DanoAplicado (F5) carrega quem SOFREU o dano em ActorId — não em
		// TargetId (esse campo é usado por eventos de ataque, F4).
		if (Event.Type == EBattleEventType::DanoAplicado && Event.ActorId == State.Pets[1].PetId)
		{
			bFoundDamageEvent = true;
			ExpectedDamage = Event.Value;
			break;
		}
	}
	if (!TestTrue(TEXT("Trace real contém DanoAplicado contra o alvo"), bFoundDamageEvent))
	{
		return false;
	}

	APetView* View = NewObject<APetView>();

	FPetPresentationInfo Presentation;
	Presentation.PetId = State.Pets[1].PetId;
	Presentation.Name = TEXT("AlvoTeste");

	View->SetInitialState(State.Pets[1], Presentation);
	TestEqual(TEXT("Vida cheia após SetInitialState"), View->GetHealthRatio(), 1.0f);
	TestFalse(TEXT("Não derrotado após SetInitialState"), View->IsDefeated());

	for (const FBattleEvent& Event : Result.Trace)
	{
		if (Event.TargetId == State.Pets[1].PetId || Event.ActorId == State.Pets[1].PetId)
		{
			View->ApplyEvent(Event);
		}
	}

	const float ExpectedRatio = 1.0f - (static_cast<float>(ExpectedDamage) / static_cast<float>(State.Pets[1].MaxHealth));
	TestTrue(TEXT("HealthRatio reduzido exatamente pelo Value do evento, sem recalcular"), FMath::IsNearlyEqual(View->GetHealthRatio(), ExpectedRatio, 0.001f));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPetViewHasVisibleBodyTest,
	"BattleSquare.PetView.HasVisibleBodyAndFollowsItsCell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPetViewHasVisibleBodyTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
	WorldContext.SetCurrentWorld(World);
	World->InitializeActorsForPlay(FURL());

	APetView* View = World->SpawnActor<APetView>();

	// Até 2026-08-26 esta classe não tinha componente visual nenhum: era
	// invisível E ficava presa na origem, porque AActor sem RootComponent
	// ignora SetActorLocation em silêncio (mesmo modo de falha de L-018).
	TestNotNull(TEXT("o pet tem corpo"), View->BodyMesh.Get());
	TestNotNull(TEXT("e o corpo é a raiz — sem isso o ator não se move"), View->GetRootComponent());

	View->SetActorLocation(FVector(300.0, -150.0, 20.0));
	TestEqual(TEXT("agora ele obedece a SetActorLocation"),
		View->GetActorLocation(), FVector(300.0, -150.0, 20.0));

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}

// TODA parte do bicho precisa de malha ATRIBUÍDA.
//
// É o padrão que já custou três aparições neste projeto (APetView, os inimigos
// do mundo, o próprio jogador): componente criado passa em qualquer teste de
// lógica e não existe na tela. Com a silhueta montada de várias peças, esquecer
// UMA delas produz um bicho sem cabeça — e nada acusa.
//
// O teste varre os componentes em vez de listar cada peça à mão, de propósito:
// peça nova entra coberta, sem ninguém lembrar de vir aqui.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPetViewSilhouetteHasAssignedMeshesTest,
	"BattleSquare.PetView.SilhouetteHasAssignedMeshes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPetViewSilhouetteHasAssignedMeshesTest::RunTest(const FString& Parameters)
{
	const APetView* Padrao = GetDefault<APetView>();

	TInlineComponentArray<UStaticMeshComponent*> Malhas(Padrao);
	TestTrue(TEXT("a silhueta tem várias peças, não uma bola só"), Malhas.Num() >= 8);

	for (const UStaticMeshComponent* Malha : Malhas)
	{
		TestTrue(FString::Printf(TEXT("%s tem malha atribuída — sem isto é invisível"),
			*Malha->GetName()), Malha->GetStaticMesh() != nullptr);
	}

	TestEqual(TEXT("quatro patas"), Padrao->Legs.Num(), 4);
	TestNotNull(TEXT("tem cabeça"), Padrao->HeadMesh.Get());
	TestNotNull(TEXT("tem cauda"), Padrao->TailMesh.Get());
	TestNotNull(TEXT("tem adorno esquerdo"), Padrao->CrestLeft.Get());
	TestNotNull(TEXT("tem adorno direito"), Padrao->CrestRight.Get());

	// O focinho fica À FRENTE da cabeça: é ele que diz para onde o bicho
	// está virado, e centrado não diria nada.
	TestTrue(TEXT("o focinho fica à frente da cabeça"),
		Padrao->GazeMarker->GetRelativeLocation().X > 1.0f);

	// Os adornos são simétricos. Se os dois tivessem o mesmo Y, ficariam um
	// dentro do outro e o bicho pareceria ter um só.
	TestTrue(TEXT("os adornos ficam em lados opostos"),
		Padrao->CrestLeft->GetRelativeLocation().Y * Padrao->CrestRight->GetRelativeLocation().Y < 0.0f);

	return true;
}

// O tipo do pet chega à silhueta, e o LADO continua mandando na cor do corpo.
//
// Saber de quem é o pet vale mais, em combate, do que saber o tipo dele: o tipo
// entra pelo adorno, acima da linha do corpo, onde continua legível no ângulo
// do diorama.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPetViewShowsItsTypeTest,
	"BattleSquare.PetView.ShowsItsType",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPetViewShowsItsTypeTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
	WorldContext.SetCurrentWorld(World);
	World->InitializeActorsForPlay(FURL());

	const FBattleState State = MakePetViewDuelState();

	APetView* Fogo = World->SpawnActor<APetView>();
	FPetPresentationInfo Apresentacao;
	Apresentacao.PetId = State.Pets[0].PetId;
	Apresentacao.Name = TEXT("Chaminha");
	Apresentacao.Type = TEXT("Fogo");
	Fogo->SetInitialState(State.Pets[0], Apresentacao);

	// O tipo vinha na apresentação e era DESCARTADO: chegava até aqui e não
	// mudava nada na tela.
	TestEqual(TEXT("o tipo da apresentação chega ao pet"), Fogo->GetPetType(), FString(TEXT("Fogo")));

	APetView* Agua = World->SpawnActor<APetView>();
	Apresentacao.PetId = State.Pets[1].PetId;
	Apresentacao.Type = TEXT("Agua");
	Agua->SetInitialState(State.Pets[1], Apresentacao);

	TestFalse(TEXT("tipos diferentes não ficam com o mesmo adorno"),
		Fogo->CrestLeft->GetRelativeScale3D().Equals(Agua->CrestLeft->GetRelativeScale3D(), 0.001f));

	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}

// Olhar para cima levanta A CABEÇA, e virar-se gira O CORPO.
//
// Antes havia um cubo solto orbitando a esfera: a "direção do olhar" não era do
// bicho, era de um adereço. Com o corpo tendo frente, girar precisa girar o
// corpo — e inclinar precisa NÃO girar, ou o pet deitaria de costas para olhar
// para o céu.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPetViewTurnsAndTiltsSeparatelyTest,
	"BattleSquare.PetView.TurnsAndTiltsSeparately",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPetViewTurnsAndTiltsSeparatelyTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
	WorldContext.SetCurrentWorld(World);
	World->InitializeActorsForPlay(FURL());

	APetView* Pet = World->SpawnActor<APetView>();
	Pet->SetActorLocation(FVector::ZeroVector);

	Pet->LookAtLocation(FVector(0.0, 300.0, 0.0));
	TestTrue(TEXT("o corpo vira para o alvo"),
		FMath::IsNearlyEqual(Pet->BodyPivot->GetComponentRotation().Yaw, 90.0f, 0.5f));

	// A barra de vida fica FORA do pivô do corpo: girada junto, ela ficaria de
	// perfil para a câmera e sumiria.
	TestTrue(TEXT("a barra de vida não gira com o corpo"),
		FMath::IsNearlyZero(Pet->HealthBarBackground->GetComponentRotation().Yaw, 0.5f));

	const FRotator CorpoAntes = Pet->BodyPivot->GetComponentRotation();
	Pet->LookUp();
	TestTrue(TEXT("olhar para cima inclina a cabeça"),
		Pet->HeadPivot->GetRelativeRotation().Pitch > 1.0f);
	TestTrue(TEXT("e não deita o corpo"),
		Pet->BodyPivot->GetComponentRotation().Equals(CorpoAntes, 0.5f));

	Pet->LookDown();
	TestTrue(TEXT("olhar para baixo abaixa a cabeça"),
		Pet->HeadPivot->GetRelativeRotation().Pitch < -1.0f);

	// Em cima do outro não há direção: escolher uma daria a impressão de que
	// o pet se distraiu justamente quando o adversário chegou.
	const FRotator AntesDaSobreposicao = Pet->BodyPivot->GetComponentRotation();
	Pet->LookAtLocation(Pet->GetActorLocation());
	TestTrue(TEXT("alvo em cima dele não gira nada"),
		Pet->BodyPivot->GetComponentRotation().Equals(AntesDaSobreposicao, 0.01f));

	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}

// As patas terminavam ~7uu ABAIXO da barriga e o corpo pairava sobre quatro
// tocos soltos — visível na tela, invisível para todo teste de lógica.
//
// A causa não era o número 20: era ser um número FIXO. O corpo é um elipsoide,
// e sua face de baixo sobe conforme se afasta do centro; a pata não fica no
// centro. Por isso a altura agora é derivada da barriga, e é isto que o teste
// prende — mexer na escala do corpo não pode reabrir o vão.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPetViewLegsReachTheBodyTest,
	"BattleSquare.PetView.LegsReachTheBody",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPetViewLegsReachTheBodyTest::RunTest(const FString& Parameters)
{
	const APetView* Padrao = GetDefault<APetView>();
	const float Barriga = APetView::BodyUnderSurfaceAtLegUnits();

	TestEqual(TEXT("quatro patas"), Padrao->Legs.Num(), 4);

	for (const UStaticMeshComponent* Pata : Padrao->Legs)
	{
		const float Centro = Pata->GetRelativeLocation().Z;
		const float Altura = Pata->GetRelativeScale3D().Z * 100.0f;
		const float Pe = Centro - Altura * 0.5f;
		const float Topo = Centro + Altura * 0.5f;

		TestTrue(FString::Printf(TEXT("%s pisa no chão da casa (pé em %.1f)"), *Pata->GetName(), Pe),
			FMath::IsNearlyEqual(Pe, 0.0f, 0.5f));
		TestTrue(FString::Printf(
				TEXT("%s ENCOSTA no corpo: topo %.1f contra barriga %.1f"), *Pata->GetName(), Topo, Barriga),
			Topo >= Barriga);
	}

	// A barriga na posição da pata é mais alta que o ponto mais baixo do
	// corpo. Se estes dois valores coincidissem, o cálculo teria virado
	// "fundo do corpo" e o vão voltaria na próxima mudança de escala.
	TestTrue(TEXT("a barriga sob a pata é mais alta que o fundo do corpo"),
		Barriga > APetView::BodyLowestPointUnits() + 1.0f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPetViewCrestsEmergeFromTheHeadTest,
	"BattleSquare.PetView.CrestsEmergeFromTheHead",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPetViewCrestsEmergeFromTheHeadTest::RunTest(const FString& Parameters)
{
	// Os quatro tipos, porque cada um tem escala e inclinação próprias e o
	// adorno mais tombado é justamente o que mais afundava.
	const TArray<FString> Tipos = {TEXT("Cat"), TEXT("Fogo"), TEXT("Agua"), TEXT("Planta")};
	const float Raio = APetView::HeadRadiusUnits();
	const float Encaixe = APetView::CrestEmbedUnits();

	for (const FString& Tipo : Tipos)
	{
		const FPetAppearance Aparencia = FPetAppearance::ForType(Tipo);

		for (const float Lado : {-1.0f, 1.0f})
		{
			const FVector Centro = APetView::CrestRelativeLocation(Aparencia, Lado);
			const FRotator Rotacao = APetView::CrestRotationForSide(Aparencia.CrestRotation, Lado);
			const float MeiaAltura = Aparencia.CrestScale.Z * 100.0f * 0.5f;
			const FVector DaBaseAoCentro = Rotacao.RotateVector(FVector(0.0f, 0.0f, MeiaAltura));

			const float Base = (Centro - DaBaseAoCentro).Size();
			const float Ponta = (Centro + DaBaseAoCentro).Size();

			TestTrue(FString::Printf(
					TEXT("%s lado %.0f: a base encosta na cabeça, sem afundar (base a %.1f, raio %.1f)"),
					*Tipo, Lado, Base, Raio),
				FMath::IsNearlyEqual(Base, Raio - Encaixe, 0.5f));
			TestTrue(FString::Printf(
					TEXT("%s lado %.0f: a ponta sai da cabeça (ponta a %.1f, raio %.1f)"),
					*Tipo, Lado, Ponta, Raio),
				Ponta > Raio + MeiaAltura * 0.5f);

			const float BaseParaFora = Lado * (Centro - DaBaseAoCentro).Y;
			const float PontaParaFora = Lado * (Centro + DaBaseAoCentro).Y;
			TestTrue(FString::Printf(
					TEXT("%s lado %.0f: o adorno tomba para FORA (base %.1f, ponta %.1f)"),
					*Tipo, Lado, BaseParaFora, PontaParaFora),
				PontaParaFora > BaseParaFora);
		}
	}

	return true;
}

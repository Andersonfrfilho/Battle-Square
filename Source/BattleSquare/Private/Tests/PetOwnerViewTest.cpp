// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattleArena.h"
#include "Battle/BattleState.h"
#include "Battle/PetOwnerView.h"
#include "Components/StaticMeshComponent.h"
#include "Data/BattleDataTranslator.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/AutomationTest.h"

namespace DonosEmCampo
{
	UWorld* CriarMundoDoDono()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
		FWorldContext& Contexto = GEngine->CreateNewWorldContext(EWorldType::Game);
		Contexto.SetCurrentWorld(World);
		World->InitializeActorsForPlay(FURL());
		return World;
	}

	void DestruirMundoDoDono(UWorld* World)
	{
		if (!World)
		{
			return;
		}
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
	}

	FPetState Pet(uint8 Id, uint8 Lado, uint8 Coluna, uint8 Linha)
	{
		FPetState Estado;
		Estado.PetId = Id;
		Estado.Side = Lado;
		Estado.Column = Coluna;
		Estado.Row = Linha;
		Estado.Health = 10;
		Estado.MaxHealth = 10;
		Estado.Attack = 3;
		return Estado;
	}
}

// A quarta vez que este projeto teria criado um ator visível sem nada para ver.
//
// Pets, inimigos do mundo e o próprio jogador nasceram com componente visual e
// SEM asset atribuído: toda a lógica passava e a tela ficava vazia. O treinador
// é feito de quatro peças, e basta uma sem malha para ele aparecer mutilado.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPetOwnerViewAssignsMeshesTest,
	"BattleSquare.Battle.PetOwnerView.AssignsMeshToEveryBodyPart",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPetOwnerViewAssignsMeshesTest::RunTest(const FString& Parameters)
{
	const APetOwnerView* Padrao = GetDefault<APetOwnerView>();

	const TArray<UStaticMeshComponent*> Pecas = Padrao->GetBodyParts();
	TestEqual(TEXT("Pernas, tronco, cabeça e chapéu"), Pecas.Num(), 4);

	for (const UStaticMeshComponent* Peca : Pecas)
	{
		TestTrue(TEXT("Peça existe e tem malha ATRIBUÍDA"),
			Peca != nullptr && Peca->GetStaticMesh().Get() != nullptr);
	}

	TestTrue(TEXT("O boneco tem altura de gente, não de ponto"),
		APetOwnerView::GetStandingHeight() > 100.0f);

	return true;
}

// Dois treinadores iguais não dizem de quem é o pet. A cor do lado é a única
// coisa que liga o boneco de fora ao bicho de dentro do tabuleiro.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPetOwnerViewPaintsBySideTest,
	"BattleSquare.Battle.PetOwnerView.PaintsBySide",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPetOwnerViewPaintsBySideTest::RunTest(const FString& Parameters)
{
	UWorld* World = DonosEmCampo::CriarMundoDoDono();

	APetOwnerView* Esquerda = World->SpawnActor<APetOwnerView>();
	APetOwnerView* Direita = World->SpawnActor<APetOwnerView>();
	Esquerda->SetSide(0);
	Direita->SetSide(1);

	TestEqual(TEXT("O lado fica guardado"), static_cast<int32>(Esquerda->GetSide()), 0);
	TestEqual(TEXT("E o do outro também"), static_cast<int32>(Direita->GetSide()), 1);

	UMaterialInstanceDynamic* TroncoEsquerda =
		Cast<UMaterialInstanceDynamic>(Esquerda->GetTorsoMesh()->GetMaterial(0));
	UMaterialInstanceDynamic* TroncoDireita =
		Cast<UMaterialInstanceDynamic>(Direita->GetTorsoMesh()->GetMaterial(0));

	if (TestNotNull(TEXT("O tronco da esquerda foi pintado"), TroncoEsquerda)
		&& TestNotNull(TEXT("E o da direita também"), TroncoDireita))
	{
		const FLinearColor CorEsquerda = TroncoEsquerda->K2_GetVectorParameterValue(TEXT("Color"));
		const FLinearColor CorDireita = TroncoDireita->K2_GetVectorParameterValue(TEXT("Color"));
		TestFalse(TEXT("Os dois lados NÃO vestem a mesma cor"),
			CorEsquerda.Equals(CorDireita, 0.01f));
	}

	DonosEmCampo::DestruirMundoDoDono(World);
	return true;
}

// "Onde estão os players donos dos pets?" — a pergunta que o usuário fez
// olhando a tela em 2026-08-29. Não havia ninguém: só os bichos brigando
// sozinhos num descampado.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArenaOwnersStandOnBothSidesTest,
	"BattleSquare.Battle.Arena.OwnersStandOnBothSides",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArenaOwnersStandOnBothSidesTest::RunTest(const FString& Parameters)
{
	UWorld* World = DonosEmCampo::CriarMundoDoDono();
	ABattleArena* Arena = World->SpawnActor<ABattleArena>();

	FBattleState Estado;
	Estado.Pets.Add(DonosEmCampo::Pet(1, 0, 0, 1));
	Estado.Pets.Add(DonosEmCampo::Pet(2, 1, 2, 1));

	Arena->SpawnPetViews(Estado, TArray<FPetPresentationInfo>());

	const TArray<TObjectPtr<APetOwnerView>>& Donos = Arena->GetOwnerViews();
	TestEqual(TEXT("Um dono de cada lado"), Donos.Num(), 2);

	float MenorY = TNumericLimits<float>::Max();
	float MaiorY = TNumericLimits<float>::Lowest();
	for (const TObjectPtr<APetOwnerView>& Dono : Donos)
	{
		if (!Dono)
		{
			continue;
		}
		const float Y = Dono->GetActorLocation().Y;
		MenorY = FMath::Min(MenorY, Y);
		MaiorY = FMath::Max(MaiorY, Y);

		// Do lado de FORA do tabuleiro: dono em cima de casa disputa espaço
		// com o pet, e o olho lê como uma terceira peça em jogo.
		TestTrue(TEXT("Fora da grade, não em cima dela"),
			FMath::Abs(Y - Arena->GetActorLocation().Y)
				> Arena->CellSize * static_cast<float>(Arena->GetActiveGridColumns()) * 0.5f);
	}

	TestTrue(TEXT("Um em cada margem, não os dois do mesmo lado"), MenorY < MaiorY);

	DonosEmCampo::DestruirMundoDoDono(World);
	return true;
}

// Dois pets do mesmo treinador são dois bichos, não dois donos. Um dono por
// PET encheria a margem de clones e desfaria justamente a leitura de "aquele
// ali é o meu".
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArenaOneOwnerPerSideTest,
	"BattleSquare.Battle.Arena.OneOwnerPerSideNotPerPet",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArenaOneOwnerPerSideTest::RunTest(const FString& Parameters)
{
	UWorld* World = DonosEmCampo::CriarMundoDoDono();
	ABattleArena* Arena = World->SpawnActor<ABattleArena>();

	FBattleState Estado;
	Estado.Pets.Add(DonosEmCampo::Pet(1, 0, 0, 0));
	Estado.Pets.Add(DonosEmCampo::Pet(2, 0, 0, 2));
	Estado.Pets.Add(DonosEmCampo::Pet(3, 1, 2, 1));

	Arena->SpawnPetViews(Estado, TArray<FPetPresentationInfo>());

	TestEqual(TEXT("Três pets, dois lados, DOIS donos"), Arena->GetOwnerViews().Num(), 2);

	DonosEmCampo::DestruirMundoDoDono(World);
	return true;
}

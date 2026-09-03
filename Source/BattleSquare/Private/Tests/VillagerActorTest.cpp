// Copyright 2026 Anderson. All Rights Reserved.

#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "World/VillagerActor.h"

/**
 * O MORADOR NA RUA (decisão 66) — as duas provas que importam.
 *
 * A primeira é A regra três vezes paga: corpo com malha atribuída, senão o
 * morador passa em toda lógica e a vila continua deserta na tela. A segunda é
 * a janela: em casa o corpo SOME da rua, fora dela aparece — a MESMA janela
 * que decide quem atende a porta, uma fonte só.
 */

namespace MoradorNaRuaTeste
{
	// Nome próprio (L-042): helper homônimo em outro arquivo de teste vira
	// sobrecarga ambígua quando o unity build junta os dois.
	UWorld* CriarMundoParaOMoradorNaRua()
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
		FWorldContext& Contexto = GEngine->CreateNewWorldContext(EWorldType::Game);
		Contexto.SetCurrentWorld(World);
		World->InitializeActorsForPlay(FURL());
		return World;
	}

	void DestruirMundoDoMoradorNaRua(UWorld* World)
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVillagerExistsOnScreenTest,
	"BattleSquare.World.Moradores.OMoradorExisteNaTela",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVillagerExistsOnScreenTest::RunTest(const FString&)
{
	UWorld* Mundo = MoradorNaRuaTeste::CriarMundoParaOMoradorNaRua();
	AVillagerActor* Morador = Mundo->SpawnActor<AVillagerActor>();

	TestNotNull(TEXT("o corpo tem componente"), Morador->GetBody());
	TestNotNull(TEXT("a cabeca tem componente"), Morador->GetHead());

	// A REGRA TRÊS VEZES PAGA: componente sem malha passa em toda bateria e
	// não existe na tela — pets, inimigos do mundo e o próprio jogador já
	// nasceram invisíveis assim.
	TestNotNull(TEXT("o corpo tem MALHA atribuida"),
		ToRawPtr(Morador->GetBody()->GetStaticMesh()));
	TestNotNull(TEXT("e a cabeca tambem"),
		ToRawPtr(Morador->GetHead()->GetStaticMesh()));

	const FVector Escala = Morador->GetBody()->GetRelativeScale3D();
	TestTrue(TEXT("e a escala nao e zero"),
		Escala.X > KINDA_SMALL_NUMBER && Escala.Z > KINDA_SMALL_NUMBER);

	// E a cabeça fica ACIMA do corpo — de cabeça para baixo ele passa em toda
	// contagem e vira defeito que só o olho pega.
	TestTrue(TEXT("a cabeca fica acima do corpo"),
		Morador->GetHead()->GetRelativeLocation().Z
			> Morador->GetBody()->GetRelativeLocation().Z);

	MoradorNaRuaTeste::DestruirMundoDoMoradorNaRua(Mundo);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FVillagerHomeHoursHideTheBodyTest,
	"BattleSquare.World.Moradores.EmCasaOCorpoSomeDaRua",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVillagerHomeHoursHideTheBodyTest::RunTest(const FString&)
{
	// A MESMA janela decide a porta e a rua: se a Dona Iraci atende em casa,
	// ela NÃO está na praça — e vice-versa. Duas fontes da mesma verdade
	// divergiriam na primeira edição, e o sintoma seria a vizinha bilocada.
	UWorld* Mundo = MoradorNaRuaTeste::CriarMundoParaOMoradorNaRua();
	AVillagerActor* Morador = Mundo->SpawnActor<AVillagerActor>();
	Morador->Configure(ESettlementKind::VilaInicial, 3, FVector::ZeroVector, 800.0f);

	const VillageResidents::FResident& Quem = Morador->GetResident();

	// Uma hora DENTRO da janela e uma FORA — as duas existem para todo
	// morador, porque a janela nunca é o dia inteiro nem dia nenhum (já
	// afirmado no teste da janela).
	float HoraEmCasa = -1.0f;
	float HoraNaRua = -1.0f;
	for (int32 Hora = 0; Hora < 24; ++Hora)
	{
		(VillageResidents::IsHomeAtHour(Quem, static_cast<float>(Hora))
			? HoraEmCasa : HoraNaRua) = static_cast<float>(Hora);
	}

	TestTrue(TEXT("ha hora em casa e hora na rua"),
		HoraEmCasa >= 0.0f && HoraNaRua >= 0.0f);

	Morador->ApplyHour(HoraEmCasa);
	TestTrue(TEXT("em casa, o corpo some da rua"), Morador->IsHidden());

	Morador->ApplyHour(HoraNaRua);
	TestFalse(TEXT("fora da janela, ele esta na rua"), Morador->IsHidden());

	MoradorNaRuaTeste::DestruirMundoDoMoradorNaRua(Mundo);
	return true;
}

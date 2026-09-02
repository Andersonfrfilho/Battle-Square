// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/BattleEvent.h"
#include "Battle/BattlePhases.h"
#include "Battle/BattleState.h"
#include "Battle/FluidRegistry.h"
#include "Misc/AutomationTest.h"

/**
 * A ÁGUA CONDUZ — o primeiro poder que o registro de fluidos tornou possível.
 *
 * "Se ele soltar um poder molhado, quem estiver no molhado e em um CONDUTOR
 * DIRETO vai sofrer o dano." Condutor direto é componente conexo, não raio de
 * distância: uma poça a duas casas ligada por um fio de água leva a corrente;
 * uma encostada e separada por chão seco, não.
 */

namespace ProvaDaConducao
{
	/** Um tabuleiro 3x3 com uma faixa de água na linha do meio. */
	FBattleState CampoComPocaNaLinhaDoMeio(EFluidKind Fluido)
	{
		FBattleState Estado;
		Estado.GridColumns = 3;
		Estado.GridRows = 3;
		Estado.CellLayout.SetNumZeroed(9);
		Estado.Random.State = 11;
		Estado.ApplyDefaultTerrainRequirements();

		for (int32 Coluna = 0; Coluna < 3; ++Coluna)
		{
			Estado.CellLayout[Estado.CellIndex(Coluna, 1)] =
				static_cast<uint8>(ECellProperty::Water);
			Estado.SetFluidAt(Coluna, 1, Fluido);
		}

		return Estado;
	}

	/** Um pet com um golpe que conduz, de força dada. */
	FPetState ComGolpeQueConduz(uint8 Id, uint8 Lado, int32 Coluna, int32 Linha, int32 Forca)
	{
		FPetState Pet;
		Pet.PetId = Id; Pet.Side = Lado; Pet.Column = Coluna; Pet.Row = Linha;
		Pet.Health = 200; Pet.MaxHealth = 200; Pet.Speed = 50;
		Pet.Attack = 20; Pet.Defense = 0;
		Pet.MovePowers[0] = Forca;
		Pet.MoveConducts[0] = 1;
		return Pet;
	}

	FPetState Comum(uint8 Id, uint8 Lado, int32 Coluna, int32 Linha)
	{
		FPetState Pet;
		Pet.PetId = Id; Pet.Side = Lado; Pet.Column = Coluna; Pet.Row = Linha;
		Pet.Health = 200; Pet.MaxHealth = 200; Pet.Speed = 10;
		Pet.Attack = 20; Pet.Defense = 0;
		return Pet;
	}

	FPetState* Achar(FBattleState& Estado, uint8 Id)
	{
		for (FPetState& Pet : Estado.Pets)
		{
			if (Pet.PetId == Id)
			{
				return &Pet;
			}
		}
		return nullptr;
	}

	/** O lado esquerdo ataca; o direito não faz nada. */
	void UmSlotDeAtaque(FBattleState& Estado)
	{
		FBattleAction Ataque;
		Ataque.Type = EActionType::Atacar;

		const FBattleAction Nada;
		TArray<FBattleEvent> Traco;
		BattlePhases::ApplyCombat(Estado,
		BattlePhases::DuelSlotActions(Estado, Ataque, Nada), 0, Traco);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConductionReachesEveryoneInTheSamePoolTest,
	"BattleSim.Conduction.ReachesEveryoneInTheSamePool",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FConductionReachesEveryoneInTheSamePoolTest::RunTest(const FString& Parameters)
{
	// O ACEITE: o raio cai num alvo molhado, e quem mais está na MESMA poça
	// leva a corrente. Sem isso, a água é enfeite e o poder é um golpe comum.
	FBattleState Estado =
		ProvaDaConducao::CampoComPocaNaLinhaDoMeio(EFluidKind::AguaSalgada);

	Estado.Pets.Add(ProvaDaConducao::ComGolpeQueConduz(1, 0, 0, 1, 60));
	Estado.Pets.Add(ProvaDaConducao::Comum(2, 1, 1, 1));  // alvo, na poça
	Estado.Pets.Add(ProvaDaConducao::Comum(3, 1, 2, 1));  // terceiro, na poça

	ProvaDaConducao::UmSlotDeAtaque(Estado);

	const FPetState* Terceiro = ProvaDaConducao::Achar(Estado, 3);
	if (!Terceiro)
	{
		AddError(TEXT("o terceiro pet sumiu do estado"));
		return false;
	}

	TestTrue(TEXT("quem esta na mesma poca leva a corrente"),
		Terceiro->PendingDamage > 0);

	// A água salgada conduz por inteiro: a corrente que chega é a força do
	// golpe. Numa água doce chegaria menos, e é o que a torna outra coisa.
	TestEqual(TEXT("a agua salgada conduz a forca inteira"),
		Terceiro->PendingDamage,
		FluidRegistry::ConductedShare(60, EFluidKind::AguaSalgada));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConductionStopsAtDryGroundTest,
	"BattleSim.Conduction.StopsAtDryGround",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FConductionStopsAtDryGroundTest::RunTest(const FString& Parameters)
{
	// O CONTRAPESO, e é ele que dá sentido a "condutor DIRETO": uma poça
	// separada por chão seco é outro circuito. Sem esta prova, uma regra que
	// atingisse o tabuleiro inteiro passaria no teste acima.
	FBattleState Estado =
		ProvaDaConducao::CampoComPocaNaLinhaDoMeio(EFluidKind::AguaSalgada);

	// Corta a poça ao meio: a casa do centro vira chão seco.
	Estado.CellLayout[Estado.CellIndex(1, 1)] =
		static_cast<uint8>(ECellProperty::None);
	Estado.SetFluidAt(1, 1, EFluidKind::Nenhum);

	Estado.Pets.Add(ProvaDaConducao::ComGolpeQueConduz(1, 0, 0, 1, 60));
	Estado.Pets.Add(ProvaDaConducao::Comum(2, 1, 0, 1));  // alvo, na poça esquerda
	Estado.Pets.Add(ProvaDaConducao::Comum(3, 1, 2, 1));  // do OUTRO lado do corte

	ProvaDaConducao::UmSlotDeAtaque(Estado);

	const FPetState* DoOutroLado = ProvaDaConducao::Achar(Estado, 3);
	if (!DoOutroLado)
	{
		AddError(TEXT("o terceiro pet sumiu do estado"));
		return false;
	}

	TestEqual(TEXT("a poca separada por chao seco NAO leva"),
		DoOutroLado->PendingDamage, 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConductionNeedsAConductingMoveTest,
	"BattleSim.Conduction.NeedsAConductingMove",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FConductionNeedsAConductingMoveTest::RunTest(const FString& Parameters)
{
	// Um golpe QUALQUER numa poça não eletrifica nada. Sem isto, toda pancada
	// dentro d'água viraria dano em área, e a água deixaria de ser escolha
	// para virar armadilha permanente.
	FBattleState Estado =
		ProvaDaConducao::CampoComPocaNaLinhaDoMeio(EFluidKind::AguaSalgada);

	FPetState SemConduzir = ProvaDaConducao::ComGolpeQueConduz(1, 0, 0, 1, 60);
	SemConduzir.MoveConducts[0] = 0;

	Estado.Pets.Add(SemConduzir);
	Estado.Pets.Add(ProvaDaConducao::Comum(2, 1, 1, 1));
	Estado.Pets.Add(ProvaDaConducao::Comum(3, 1, 2, 1));

	ProvaDaConducao::UmSlotDeAtaque(Estado);

	const FPetState* Terceiro = ProvaDaConducao::Achar(Estado, 3);
	TestEqual(TEXT("golpe que nao conduz nao eletrifica a poca"),
		Terceiro ? Terceiro->PendingDamage : -1, 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConductionWhoGeneratesItWithstandsItTest,
	"BattleSim.Conduction.WhoGeneratesItWithstandsIt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FConductionWhoGeneratesItWithstandsItTest::RunTest(const FString& Parameters)
{
	// QUEM GERA, AGUENTA — e é isto que responde, SEM REGRA À PARTE, se o
	// lançador se machuca com a própria corrente: ela nunca supera o que ele
	// sabe produzir. A resistência não é um número escolhido; é a capacidade.
	FBattleState Estado =
		ProvaDaConducao::CampoComPocaNaLinhaDoMeio(EFluidKind::AguaSalgada);

	Estado.Pets.Add(ProvaDaConducao::ComGolpeQueConduz(1, 0, 1, 1, 60)); // na poça
	Estado.Pets.Add(ProvaDaConducao::Comum(2, 1, 2, 1));
	// Outro elétrico, MAIS FRACO: aguenta 20 dos 60 que chegam.
	Estado.Pets.Add(ProvaDaConducao::ComGolpeQueConduz(3, 1, 0, 1, 20));

	ProvaDaConducao::UmSlotDeAtaque(Estado);

	const FPetState* Lancador = ProvaDaConducao::Achar(Estado, 1);
	const FPetState* Fraco = ProvaDaConducao::Achar(Estado, 3);
	if (!Lancador || !Fraco)
	{
		AddError(TEXT("faltou pet no estado"));
		return false;
	}

	TestEqual(TEXT("o lancador nao se machuca com a propria corrente"),
		Lancador->PendingDamage, 0);

	// O mais fraco absorve a PRÓPRIA capacidade e leva só o excedente. É a
	// resposta a "teria que ser mais forte que o poder dele" — literalmente.
	const int32 Corrente = FluidRegistry::ConductedShare(60, EFluidKind::AguaSalgada);
	TestEqual(TEXT("o eletrico mais fraco leva so o excedente"),
		Fraco->PendingDamage, Corrente - 20);

	// E o excedente é MENOR que a corrente cheia: sem isto, a capacidade não
	// estaria absorvendo nada e o teste acima passaria por coincidência.
	TestTrue(TEXT("a capacidade absorveu parte"), Fraco->PendingDamage < Corrente);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FConductionTheFluidDecidesHowMuchArrivesTest,
	"BattleSim.Conduction.TheFluidDecidesHowMuchArrives",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FConductionTheFluidDecidesHowMuchArrivesTest::RunTest(const FString& Parameters)
{
	// O REGISTRO DECIDE QUANTO CHEGA. Se todo fluido conduzisse igual, a
	// separação entre eles — que é a razão de o registro existir — não
	// mudaria nada no jogo.
	FBattleState NoMar =
		ProvaDaConducao::CampoComPocaNaLinhaDoMeio(EFluidKind::AguaSalgada);
	NoMar.Pets.Add(ProvaDaConducao::ComGolpeQueConduz(1, 0, 0, 1, 60));
	NoMar.Pets.Add(ProvaDaConducao::Comum(2, 1, 1, 1));
	NoMar.Pets.Add(ProvaDaConducao::Comum(3, 1, 2, 1));
	ProvaDaConducao::UmSlotDeAtaque(NoMar);

	FBattleState NoRio =
		ProvaDaConducao::CampoComPocaNaLinhaDoMeio(EFluidKind::AguaDoce);
	NoRio.Pets.Add(ProvaDaConducao::ComGolpeQueConduz(1, 0, 0, 1, 60));
	NoRio.Pets.Add(ProvaDaConducao::Comum(2, 1, 1, 1));
	NoRio.Pets.Add(ProvaDaConducao::Comum(3, 1, 2, 1));
	ProvaDaConducao::UmSlotDeAtaque(NoRio);

	const FPetState* NaSalgada = ProvaDaConducao::Achar(NoMar, 3);
	const FPetState* NaDoce = ProvaDaConducao::Achar(NoRio, 3);
	if (!NaSalgada || !NaDoce)
	{
		AddError(TEXT("faltou pet no estado"));
		return false;
	}

	TestTrue(TEXT("a agua salgada conduz mais que a doce"),
		NaSalgada->PendingDamage > NaDoce->PendingDamage);

	// E a doce conduz ALGO: zero faria a linha acima passar com a água doce
	// sendo isolante, que não é o que a tabela diz.
	TestTrue(TEXT("a agua doce ainda conduz"), NaDoce->PendingDamage > 0);

	return true;
}

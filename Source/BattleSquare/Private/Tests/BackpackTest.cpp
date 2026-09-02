// Copyright 2026 Anderson. All Rights Reserved.

#include "Balance/ItemCatalog.h"
#include "Battle/FluidRegistry.h"
#include "Data/BattleDataTranslator.h"
#include "Data/PetDataLoader.h"
#include "Meta/BackpackService.h"
#include "Meta/PetCollectionSaveGame.h"
#include "Misc/AutomationTest.h"

// Namespace NOMEADO (L-042).
namespace MochilaTeste
{
	FItemCatalog MontarCatalogo()
	{
		FItemCatalog Catalogo;

		FItemDefinition Bota;
		Bota.Id = TEXT("bota_de_lava");
		Bota.Name = TEXT("Bota de lava");
		Bota.Nature = EItemNature::Equipamento;
		Bota.FluidResists.Add(TEXT("lava"), 60);
		Catalogo.Items.Add(Bota);

		FItemDefinition Grampo;
		Grampo.Id = TEXT("grampo_de_rocha");
		Grampo.Nature = EItemNature::Equipamento;
		Grampo.FootingPerMille = 250;
		Catalogo.Items.Add(Grampo);

		FItemDefinition Unguento;
		Unguento.Id = TEXT("unguento_termal");
		Unguento.Nature = EItemNature::Consumivel;
		Catalogo.Items.Add(Unguento);

		return Catalogo;
	}

	struct FCatalogoInstalado
	{
		explicit FCatalogoInstalado(const FItemCatalog& Catalogo)
		{
			FItemCatalog::OverrideForTesting(&Catalogo);
		}
		~FCatalogoInstalado() { FItemCatalog::OverrideForTesting(nullptr); }
	};

	/**
	 * QUANTOS deste item existem no save inteiro — mochila mais vestidos.
	 *
	 * É o invariante da feature: equipar move, não cria nem destrói. Contar só
	 * a mochila deixaria "equipar apagou o item" passar despercebido.
	 */
	int32 TotalNoMundo(const UPetCollectionSaveGame* Save, const FString& ItemId)
	{
		int32 Total = FBackpackService::CountInBackpack(Save, ItemId);
		for (const FEquippedItem& Vestido : Save->Equipped)
		{
			if (Vestido.ItemId.Equals(ItemId, ESearchCase::IgnoreCase))
			{
				++Total;
			}
		}
		return Total;
	}
}

// ---------------------------------------------------------------------------
// I3 — EQUIPAR MOVE, NÃO CRIA.
//
// Este é o invariante da feature, e vem direto da decisão do usuário:
// "carregamos os itens na mochila, a não ser que seja um item equipado com o
// pet". Um item que ficasse nos dois lugares existiria duas vezes, e a mesma
// bota vestiria cinco pets.
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBackpackEquipMovesItTest,
	"BattleSquare.Backpack.EquipMovesIt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBackpackEquipMovesItTest::RunTest(const FString& Parameters)
{
	const FItemCatalog Catalogo = MochilaTeste::MontarCatalogo();
	const MochilaTeste::FCatalogoInstalado Instalado(Catalogo);

	UPetCollectionSaveGame* Save = NewObject<UPetCollectionSaveGame>();
	FBackpackService::Add(Save, TEXT("bota_de_lava"), 1);

	TestEqual(TEXT("uma bota na mochila"),
		FBackpackService::CountInBackpack(Save, TEXT("bota_de_lava")), 1);

	TestTrue(TEXT("equipar funciona"),
		FBackpackService::Equip(Save, TEXT("pet-um"), TEXT("bota_de_lava")));

	TestEqual(TEXT("e a mochila FICOU SEM ela"),
		FBackpackService::CountInBackpack(Save, TEXT("bota_de_lava")), 0);
	TestEqual(TEXT("mas o total do mundo nao mudou"),
		MochilaTeste::TotalNoMundo(Save, TEXT("bota_de_lava")), 1);

	TestTrue(TEXT("desequipar funciona"),
		FBackpackService::Unequip(Save, TEXT("pet-um"), TEXT("bota_de_lava")));
	TestEqual(TEXT("e a mochila recebeu de volta"),
		FBackpackService::CountInBackpack(Save, TEXT("bota_de_lava")), 1);
	TestEqual(TEXT("total continua um"),
		MochilaTeste::TotalNoMundo(Save, TEXT("bota_de_lava")), 1);

	return true;
}

// CONTRAPESO — EQUIPAR O QUE NÃO SE TEM FALHA, E NÃO CONSOME NADA.
//
// Meio equipar perderia o item sem vesti-lo, e o jogador não teria como saber
// que o perdeu.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBackpackCannotEquipWhatYouLackTest,
	"BattleSquare.Backpack.CannotEquipWhatYouLack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBackpackCannotEquipWhatYouLackTest::RunTest(const FString& Parameters)
{
	const FItemCatalog Catalogo = MochilaTeste::MontarCatalogo();
	const MochilaTeste::FCatalogoInstalado Instalado(Catalogo);

	UPetCollectionSaveGame* Save = NewObject<UPetCollectionSaveGame>();

	TestTrue(TEXT("equipar sem ter FALHA"),
		!FBackpackService::Equip(Save, TEXT("pet-um"), TEXT("bota_de_lava")));
	TestEqual(TEXT("e ninguem veste nada"),
		FBackpackService::EquippedOn(Save, TEXT("pet-um")).Num(), 0);

	// Desequipar o que não se veste também falha, e não inventa item na
	// mochila — seria uma fábrica de itens de graça.
	TestTrue(TEXT("desequipar o que nao se veste FALHA"),
		!FBackpackService::Unequip(Save, TEXT("pet-um"), TEXT("bota_de_lava")));
	TestEqual(TEXT("e nao apareceu bota nenhuma"),
		FBackpackService::CountInBackpack(Save, TEXT("bota_de_lava")), 0);

	return true;
}

// OS SLOTS TÊM TETO, e o item recusado FICA NA MOCHILA.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBackpackSlotsAreLimitedTest,
	"BattleSquare.Backpack.SlotsAreLimited",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBackpackSlotsAreLimitedTest::RunTest(const FString& Parameters)
{
	const FItemCatalog Catalogo = MochilaTeste::MontarCatalogo();
	const MochilaTeste::FCatalogoInstalado Instalado(Catalogo);

	const int32 Slots = FBackpackService::SlotsPerPet();
	TestTrue(TEXT("ha ao menos um slot"), Slots >= 1);

	UPetCollectionSaveGame* Save = NewObject<UPetCollectionSaveGame>();
	FBackpackService::Add(Save, TEXT("bota_de_lava"), Slots + 1);

	for (int32 Qual = 0; Qual < Slots; ++Qual)
	{
		TestTrue(TEXT("cabe ate encher"),
			FBackpackService::Equip(Save, TEXT("pet-um"), TEXT("bota_de_lava")));
	}

	TestTrue(TEXT("o de depois do teto NAO cabe"),
		!FBackpackService::Equip(Save, TEXT("pet-um"), TEXT("bota_de_lava")));

	// E O RECUSADO CONTINUA NA MOCHILA: uma recusa que consumisse seria pior
	// que a recusa, porque o jogador perde sem nada avisar.
	TestEqual(TEXT("o recusado ficou na mochila"),
		FBackpackService::CountInBackpack(Save, TEXT("bota_de_lava")), 1);

	// OUTRO PET ainda pode vestir: o teto é por pet, não do mundo.
	TestTrue(TEXT("outro pet ainda veste"),
		FBackpackService::Equip(Save, TEXT("pet-dois"), TEXT("bota_de_lava")));

	return true;
}

// I5 — O CONSUMÍVEL GASTA; O EQUIPAMENTO, NÃO.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBackpackConsumableIsSpentTest,
	"BattleSquare.Backpack.ConsumableIsSpent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBackpackConsumableIsSpentTest::RunTest(const FString& Parameters)
{
	const FItemCatalog Catalogo = MochilaTeste::MontarCatalogo();
	const MochilaTeste::FCatalogoInstalado Instalado(Catalogo);

	UPetCollectionSaveGame* Save = NewObject<UPetCollectionSaveGame>();
	FBackpackService::Add(Save, TEXT("unguento_termal"), 1);

	TestTrue(TEXT("usar uma vez funciona"),
		FBackpackService::Consume(Save, TEXT("unguento_termal")));
	TestEqual(TEXT("e a pilha zerou"),
		FBackpackService::CountInBackpack(Save, TEXT("unguento_termal")), 0);

	// A SEGUNDA FALHA, e falhar é o aceite: gastar em silêncio um item que não
	// existe é o jogador achando que usou.
	TestTrue(TEXT("usar de novo FALHA"),
		!FBackpackService::Consume(Save, TEXT("unguento_termal")));

	// CONTRAPESO: equipamento não se gasta. Gastar uma bota seria a mecânica
	// de um sistema respondendo pela do outro.
	FBackpackService::Add(Save, TEXT("bota_de_lava"), 1);
	TestTrue(TEXT("equipamento nao se gasta"),
		!FBackpackService::Consume(Save, TEXT("bota_de_lava")));
	TestEqual(TEXT("e continua na mochila"),
		FBackpackService::CountInBackpack(Save, TEXT("bota_de_lava")), 1);

	return true;
}

// A PILHA É UMA LINHA SÓ. Duas linhas do mesmo id fariam a contagem depender de
// quantas vezes alguém pegou, e não de quantos há.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBackpackStacksTest,
	"BattleSquare.Backpack.QuantityIsOfTheStack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBackpackStacksTest::RunTest(const FString& Parameters)
{
	UPetCollectionSaveGame* Save = NewObject<UPetCollectionSaveGame>();

	FBackpackService::Add(Save, TEXT("unguento_termal"), 2);
	FBackpackService::Add(Save, TEXT("unguento_termal"), 3);

	TestEqual(TEXT("cinco unguentos"),
		FBackpackService::CountInBackpack(Save, TEXT("unguento_termal")), 5);
	TestEqual(TEXT("numa LINHA so"), Save->Backpack.Num(), 1);

	// Quantidade zero ou negativa não faz nada — e não cria pilha vazia, que
	// apareceria na tela como um item que o jogador não tem.
	FBackpackService::Add(Save, TEXT("bota_de_lava"), 0);
	FBackpackService::Add(Save, TEXT("bota_de_lava"), -5);
	TestEqual(TEXT("nada foi criado"), Save->Backpack.Num(), 1);

	return true;
}

// ---------------------------------------------------------------------------
// I4 — ESTE É O ACEITE ESCRITO DA TAREFA.
//
// O MESMO pet atravessa a lava com a bota e não atravessa sem. O mesmo pet — e
// não dois pets diferentes, que provariam apenas que dois cadastros diferem.
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBackpackLavaBootWorksTest,
	"BattleSquare.Backpack.TheLavaBootWorks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBackpackLavaBootWorksTest::RunTest(const FString& Parameters)
{
	const FItemCatalog Catalogo = MochilaTeste::MontarCatalogo();
	const MochilaTeste::FCatalogoInstalado Instalado(Catalogo);

	FLoadedPetRecord Pet;
	Pet.Id = TEXT("um");
	Pet.Name = TEXT("Um");
	// ESCOLA/ELEMENTO, nesta ordem. Escrito ao contrário, o elemento não é
	// achado, a resistência inteira é pulada, e os dois lados dão ZERO — o
	// teste reprova com "a bota não faz diferença", que é verdade e não é o
	// defeito. Foi o que aconteceu na primeira rodada, e o que salvou foi o
	// `AddError` do contrapeso dizendo "o elemento sumiu do catálogo".
	Pet.Type = TEXT("Fisica/Agua");
	Pet.Attack = 50; Pet.Defense = 50; Pet.Speed = 50; Pet.MaxHealth = 100;

	FPetState SemBota;
	FPetState ComBota;
	FPetPresentationInfo Fora;

	FBattleDataTranslator::TranslatePet(Pet, 1, 0, 0, 1, SemBota, Fora);
	FBattleDataTranslator::TranslatePetWithItems(Pet,
		TArray<FString>{ TEXT("bota_de_lava") }, 1, 0, 0, 1, ComBota, Fora);

	const int32 Descalco = SemBota.ResistPercentFor(EFluidKind::Lava);
	const int32 Calcado = ComBota.ResistPercentFor(EFluidKind::Lava);

	AddInfo(FString::Printf(TEXT("lava: descalco %d%%, calcado %d%%"),
		Descalco, Calcado));

	TestTrue(TEXT("A BOTA FAZ DIFERENCA na lava"), Calcado > Descalco);

	// E o item mexe na FIRMEZA também — o grampo existe para provar que não é
	// só resistência.
	FPetState ComGrampo;
	FBattleDataTranslator::TranslatePetWithItems(Pet,
		TArray<FString>{ TEXT("grampo_de_rocha") }, 1, 0, 0, 1, ComGrampo, Fora);
	TestTrue(TEXT("o grampo firma contra a corrente"),
		ComGrampo.FootingPerMille > SemBota.FootingPerMille);

	// CONTRAPESO 1 — item DESCONHECIDO soma zero e não derruba: um erro de
	// digitação no cadastro não pode tirar o pet da partida.
	FPetState ComLixo;
	FBattleDataTranslator::TranslatePetWithItems(Pet,
		TArray<FString>{ TEXT("item que ninguem cadastrou") }, 1, 0, 0, 1, ComLixo, Fora);
	TestEqual(TEXT("item desconhecido nao muda nada"),
		ComLixo.ResistPercentFor(EFluidKind::Lava), Descalco);

	// CONTRAPESO 2 — a bota de lava não resiste a ÁGUA. Um item que somasse em
	// tudo seria um bônus geral com nome de bota.
	TestEqual(TEXT("a bota nao ajuda na agua doce"),
		ComBota.ResistPercentFor(EFluidKind::AguaDoce),
		SemBota.ResistPercentFor(EFluidKind::AguaDoce));

	return true;
}

// O CATÁLOGO DE VERDADE CARREGA, e os fluidos dele casam com o registro.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FItemRealCatalogLoadsTest,
	"BattleSquare.Backpack.RealCatalogLoads",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FItemRealCatalogLoadsTest::RunTest(const FString& Parameters)
{
	const FItemCatalog& Real = FItemCatalog::Get();
	TestTrue(TEXT("ha itens cadastrados"), Real.Items.Num() > 0);

	// A BOTA DE LAVA existe pelo id que o aceite da tarefa nomeia. Sem esta
	// linha, renomear o id passaria e o aceite viraria letra morta.
	const FItemDefinition* Bota = Real.Find(TEXT("bota_de_lava"));
	TestTrue(TEXT("a bota de lava existe"), Bota != nullptr);

	int32 Conferidas = 0;
	for (const FItemDefinition& Item : Real.Items)
	{
		for (const auto& Par : Item.FluidResists)
		{
			bool bAchou = false;
			for (int32 Qual = 1; Qual < static_cast<int32>(EFluidKind::Count); ++Qual)
			{
				if (FString(FluidRegistry::TraitsOf(static_cast<EFluidKind>(Qual)).DebugName)
					.Equals(Par.Key, ESearchCase::IgnoreCase))
				{
					bAchou = true;
					++Conferidas;
					break;
				}
			}

			TestTrue(*FString::Printf(TEXT("'%s' em '%s' e um fluido do registro"),
				*Par.Key, *Item.Id), bAchou);
		}
	}

	TestTrue(TEXT("ao menos uma resistencia foi conferida"), Conferidas > 0);

	// E HÁ UM CONSUMÍVEL: sem nenhum, a regra de gasto não teria como ser
	// exercitada pelo jogo de verdade.
	TestTrue(TEXT("ha ao menos um consumivel cadastrado"),
		Real.Items.ContainsByPredicate([](const FItemDefinition& Item)
		{
			return Item.Nature == EItemNature::Consumivel;
		}));

	return true;
}

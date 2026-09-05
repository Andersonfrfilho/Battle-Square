// Copyright 2026 Anderson. All Rights Reserved.

#include "Battle/PetModelArt.h"
#include "Battle/PetView.h"
#include "Misc/AutomationTest.h"
#include "Misc/ConfigCacheIni.h"

/**
 * AR6 — o pet veste a malha adotada, achada pelo CATÁLOGO. Os contrapesos no
 * centro: sem override cai no caminho antigo (e daí na silhueta, que é visível),
 * e id com caractere estranho nunca vira caminho.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPetModelArtIsDataTest,
	"BattleSquare.Battle.PetArte.MalhaVemDoCatalogo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPetModelArtIsDataTest::RunTest(const FString&)
{
	using namespace PetModelArt;

	const TCHAR* Secao = TEXT("/Script/BattleSquare.Art");
	const FString Chave = ConfigKeyFor(TEXT("Faisca"));
	TestEqual(TEXT("a chave e por NOME"), Chave, FString(TEXT("Mesh_Pet_Faisca")));

	FString Original;
	const bool bTinha = GConfig->GetString(Secao, *Chave, Original, GGameIni);

	// SEM override: vazio — o pet cai no caminho por tipo, e daí na silhueta.
	GConfig->RemoveKey(Secao, *Chave, GGameIni);
	TestTrue(TEXT("sem chave, caminho vazio"), MeshPathFor(TEXT("Faisca")).IsEmpty());

	// COM override: o caminho do dado.
	GConfig->SetString(Secao, *Chave,
		TEXT("/Game/Quaternius/Monsters/SK_Dragon.SK_Dragon"), GGameIni);
	TestEqual(TEXT("com chave, devolve o asset adotado"),
		MeshPathFor(TEXT("Faisca")),
		FString(TEXT("/Game/Quaternius/Monsters/SK_Dragon.SK_Dragon")));

	// A PRECEDENCIA: o catalogo ganha do tipo.
	TestEqual(TEXT("o nome tem prioridade sobre o tipo"),
		APetView::CharacterMeshPathFor(TEXT("Faisca"), TEXT("Fogo")),
		FString(TEXT("/Game/Quaternius/Monsters/SK_Dragon.SK_Dragon")));

	// CONTRAPESO: sem catalogo atribuido, cai no caminho por TIPO — o que
	// existia antes desta feature continua valendo.
	const FString SemModelo = APetView::CharacterMeshPathFor(TEXT("NaoAtribuido"), TEXT("Fogo"));
	TestTrue(TEXT("sem atribuicao, cai no caminho por tipo"),
		SemModelo.Contains(TEXT("/Game/Pets/Characters/")));

	// CONTRAPESO DE SEGURANCA: id com caractere estranho nunca vira chave —
	// caminho montado com texto de fora e travessia de diretorio esperando.
	TestTrue(TEXT("nome com barra nao vira chave"), ConfigKeyFor(TEXT("../../etc")).IsEmpty());
	TestTrue(TEXT("nome com ponto nao vira chave"), ConfigKeyFor(TEXT("a.b")).IsEmpty());
	TestTrue(TEXT("nome vazio nao vira chave"), ConfigKeyFor(TEXT("")).IsEmpty());

	// E o ACENTO PASSA: metade do catalogo tem acento no nome (Faisca, Vigilia,
	// Vagalhao). Recusa-lo deixaria esses pets sem poder vestir modelo nenhum.
	TestFalse(TEXT("nome com acento vira chave"),
		ConfigKeyFor(TEXT("Fa\u00edsca")).IsEmpty());

    if (bTinha) { GConfig->SetString(Secao, *Chave, *Original, GGameIni); }
    else { GConfig->RemoveKey(Secao, *Chave, GGameIni); }

	return true;
}

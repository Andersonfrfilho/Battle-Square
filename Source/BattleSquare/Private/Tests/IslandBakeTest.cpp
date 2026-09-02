#include "Misc/AutomationTest.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

#include "World/IslandBakedPlan.h"

/**
 * ASSA o traçado e grava o `UDataAsset`.
 *
 * Vive como teste de automação porque é assim que este projeto roda coisa
 * headless — o mesmo motivo do despejo do mapa. Não é teste de regra: é a
 * FERRAMENTA de assar, chamada por `Tools/bake_island.sh`.
 *
 * Ela não pode ser um passo manual no Editor. Capacidade que espera edição de
 * asset é capacidade que não existe hoje — e um assado que depende de alguém
 * lembrar de reassar é exatamente o assado velho de que a spec fala.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FIslandBakeTest,
	"BattleSquare.IslandMap.Bake",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIslandBakeTest::RunTest(const FString& Parameters)
{
	const FString CaminhoDoObjeto = IslandBakedPlan::AssetPath();
	FString CaminhoDoPacote;
	FString NomeDoObjeto;
	CaminhoDoObjeto.Split(TEXT("."), &CaminhoDoPacote, &NomeDoObjeto);

	UPackage* Pacote = CreatePackage(*CaminhoDoPacote);
	if (!TestNotNull(TEXT("o pacote do assado foi criado"), Pacote))
	{
		return false;
	}

	UIslandBakedPlan* Plano = NewObject<UIslandBakedPlan>(
		Pacote, UIslandBakedPlan::StaticClass(), *NomeDoObjeto,
		RF_Public | RF_Standalone);
	IslandBakedPlan::BakeInto(*Plano);

	// Assado vazio grava sem erro e derruba o mundo inteiro depois, longe daqui.
	TestTrue(TEXT("o assado tem relevo"), Plano->GroundHeightUnits.Num() > 0);
	TestTrue(TEXT("o assado tem rios"), Plano->Rivers.Num() > 0);
	TestTrue(TEXT("o assado tem trilhas"), Plano->Trails.Num() > 0);
	TestTrue(TEXT("o assado tem uso do solo"), Plano->GroundUses.Num() > 0);

	Plano->MarkPackageDirty();

	FSavePackageArgs Argumentos;
	Argumentos.TopLevelFlags = RF_Public | RF_Standalone;
	Argumentos.SaveFlags = SAVE_NoError;

	const FString CaminhoNoDisco = FPackageName::LongPackageNameToFilename(
		CaminhoDoPacote, FPackageName::GetAssetPackageExtension());

	TestTrue(TEXT("o assado foi gravado"),
		UPackage::SavePackage(Pacote, Plano, *CaminhoNoDisco, Argumentos));

	return true;
}

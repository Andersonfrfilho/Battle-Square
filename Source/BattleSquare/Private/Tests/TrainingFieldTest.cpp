// Copyright 2026 Anderson. All Rights Reserved.

#include "Meta/PetAttributeProgression.h"
#include "Meta/PetMoveRequirements.h"
#include "Misc/AutomationTest.h"
#include "Balance/PetTypeCatalog.h"
#include "Misc/Paths.h"
#include "World/TrainingFieldRules.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "World/WorldTrainingField.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Environment/BiomeFlora.h"
#include "Environment/ForestBackdrop.h"

// O RESTO acumulado atravessa as visitas.
//
// Sem devolvê-lo, dez visitas de cinco segundos renderiam ZERO enquanto uma de
// cinquenta renderia oito — e duas formas de fazer a mesma coisa não podem ter
// resultados diferentes. É o defeito que faria o jogador aprender a ficar
// parado em vez de andar pelo mundo.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTrainingCarriesLeftoverTimeTest,
	"BattleSquare.World.Training.CarriesLeftoverTime",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTrainingCarriesLeftoverTimeTest::RunTest(const FString& Parameters)
{
	// Meio segundo por passagem, como o temporizador do mundo.
	float Resto = 0.0f;
	int32 Total = 0;
	for (int32 Passagem = 0; Passagem < 24; ++Passagem)
	{
		Total += FTrainingFieldRules::PointsForTime(0.5f, /*bEspecialista=*/false, Resto);
	}

	// 24 passagens de 0,5s = 12 segundos = 2 pontos a 6s cada.
	TestEqual(TEXT("Passos pequenos somam igual a um passo grande"), Total, 2);

	float RestoDeUmaVez = 0.0f;
	TestEqual(TEXT("E 12 segundos de uma vez rendem o mesmo"),
		FTrainingFieldRules::PointsForTime(12.0f, /*bEspecialista=*/false, RestoDeUmaVez), 2);

	return true;
}

// O estudo do dono MULTIPLICA (DP-atr-09): mesmo tempo, mais rendimento —
// nunca treino que acontece sem o pet.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSpecialistTrainsFasterForTheSameTimeTest,
	"BattleSquare.World.Training.SpecialistTrainsFasterForTheSameTime",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSpecialistTrainsFasterForTheSameTimeTest::RunTest(const FString& Parameters)
{
	float RestoComum = 0.0f;
	const int32 Comum = FTrainingFieldRules::PointsForTime(60.0f, /*bEspecialista=*/false, RestoComum);

	float RestoEspecialista = 0.0f;
	const int32 Especialista = FTrainingFieldRules::PointsForTime(60.0f, /*bEspecialista=*/true, RestoEspecialista);

	TestTrue(TEXT("O especialista rende mais no mesmo tempo"), Especialista > Comum);
	TestEqual(TEXT("E rende exatamente o multiplicador declarado"),
		Especialista, (Comum * FTrainingFieldRules::SpecialistBonusPercent) / 100);

	return true;
}

// Tempo zero ou negativo não rende nada, e não mexe no resto.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FNoTimeNoTrainingTest,
	"BattleSquare.World.Training.NoTimeNoTraining",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FNoTimeNoTrainingTest::RunTest(const FString& Parameters)
{
	float Resto = 3.0f;
	TestEqual(TEXT("Zero segundos não rende"),
		FTrainingFieldRules::PointsForTime(0.0f, false, Resto), 0);
	TestEqual(TEXT("E não mexe no que já estava acumulado"), Resto, 3.0f);

	TestEqual(TEXT("Tempo negativo também não rende"),
		FTrainingFieldRules::PointsForTime(-5.0f, false, Resto), 0);
	TestEqual(TEXT("E também não mexe no resto"), Resto, 3.0f);

	return true;
}

// O campo treina O ATRIBUTO QUE DIZ TREINAR, e nome desconhecido não mexe em
// nada — um campo mal configurado não pode subir o atributo errado calado.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTrainingHitsTheNamedAttributeTest,
	"BattleSquare.World.Training.HitsTheNamedAttribute",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTrainingHitsTheNamedAttributeTest::RunTest(const FString& Parameters)
{
	FOwnedPetInstance Pet;
	FTrainingFieldRules::ApplyPoints(TEXT("flight"), 3, Pet);

	TestEqual(TEXT("Voo subiu"), Pet.SkillProficiency[FPetAttributeProgression::Flight], 3);
	TestEqual(TEXT("E a musculatura não"), Pet.Musculature, 0);

	FTrainingFieldRules::ApplyPoints(TEXT("musculature"), 2, Pet);
	TestEqual(TEXT("Musculatura subiu"), Pet.Musculature, 2);

	FOwnedPetInstance Intocado;
	FTrainingFieldRules::ApplyPoints(TEXT("telepatia"), 99, Intocado);
	TestEqual(TEXT("Nome desconhecido não mexe na musculatura"), Intocado.Musculature, 0);
	TestEqual(TEXT("Nem na personalidade"), Intocado.Personality, 0);
	TestEqual(TEXT("Nem no voo"),
		Intocado.SkillProficiency[FPetAttributeProgression::Flight], 0);

	return true;
}

// O vocabulário do campo é o MESMO do requisito de golpe.
//
// Duas listas de nomes produziriam um campo que treina algo que nenhum golpe
// exige — treino que nunca destrava nada, e sem nada apontando a causa.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTrainingSharesVocabularyWithMoveRequirementsTest,
	"BattleSquare.World.Training.SharesVocabularyWithMoveRequirements",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTrainingSharesVocabularyWithMoveRequirementsTest::RunTest(const FString& Parameters)
{
	const TCHAR* Atributos[] = {
		TEXT("musculature"), TEXT("personality"),
		TEXT("camouflage"), TEXT("flight"), TEXT("underground"),
	};

	for (const TCHAR* Atributo : Atributos)
	{
		// Rótulo conhecido: GetAttributeLabel devolve o nome CRU quando não
		// conhece, então "traduziu" prova que os dois lados falam o mesmo.
		const FString Rotulo = FPetMoveRequirements::GetAttributeLabel(Atributo).ToString();
		TestNotEqual(
			*FString::Printf(TEXT("'%s' é reconhecido pelo requisito de golpe"), Atributo),
			Rotulo, FString(Atributo));

		// E o treino sabe aplicá-lo.
		FOwnedPetInstance Pet;
		FTrainingFieldRules::ApplyPoints(Atributo, 1, Pet);
		const bool bMudouAlgo = Pet.Musculature != 0 || Pet.Personality != 0
			|| Pet.SkillProficiency[0] != 0 || Pet.SkillProficiency[1] != 0
			|| Pet.SkillProficiency[2] != 0;
		TestTrue(*FString::Printf(TEXT("'%s' treina alguma coisa"), Atributo), bMudouAlgo);

		// E o campo tem cor própria para ele — cinza é o "não conheço".
		TestNotEqual(*FString::Printf(TEXT("'%s' tem cor própria"), Atributo),
			AWorldTrainingField::ColorForAttribute(Atributo), FLinearColor(0.5f, 0.5f, 0.5f));
	}

	return true;
}

// Estar no campo é distância NO PLANO: pular dentro dele continua dentro.
// Exigir contato com o chão faria o treino piscar a cada salto.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTrainingFieldIgnoresHeightTest,
	"BattleSquare.World.Training.FieldIgnoresHeight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTrainingFieldIgnoresHeightTest::RunTest(const FString& Parameters)
{
	const AWorldTrainingField* Padrao = GetDefault<AWorldTrainingField>();

	TestTrue(TEXT("Campo nasce com malha ATRIBUÍDA"),
		Padrao->GetFieldMesh() != nullptr && Padrao->GetFieldMesh()->GetStaticMesh() != nullptr);

	UWorld* Mundo = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/false);
	if (!Mundo)
	{
		AddError(TEXT("não foi possível criar mundo de teste"));
		return false;
	}

	AWorldTrainingField* Campo = Mundo->SpawnActor<AWorldTrainingField>(
		AWorldTrainingField::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
	Campo->FieldRadiusUnits = 400.0f;

	TestTrue(TEXT("No centro está dentro"), Campo->IsInside(FVector::ZeroVector));
	TestTrue(TEXT("Pulando alto continua dentro"), Campo->IsInside(FVector(0.0f, 0.0f, 5000.0f)));
	TestTrue(TEXT("Na borda está dentro"), Campo->IsInside(FVector(400.0f, 0.0f, 0.0f)));
	TestFalse(TEXT("Um passo além está fora"), Campo->IsInside(FVector(401.0f, 0.0f, 0.0f)));

	Mundo->DestroyWorld(false);
	return true;
}

// CRIATURA É VIVA, TERRENO É TERRA — e isto é medível, não opinião.
//
// A primeira paleta dos campos não obedecia à regra, e o resultado era
// concreto: o verde da camuflagem ficava a cinco pontos do verde do pet de
// Planta. Um pet parado no próprio campo virava duas coisas da mesma cor
// dizendo coisas diferentes.
//
// O teste compara SATURAÇÃO: todo campo precisa ser mais apagado que o
// elemento mais apagado do elenco de criaturas. Assim a regra sobrevive ao
// próximo campo e ao próximo elemento, em vez de proteger só os cinco de hoje.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTerrainIsAlwaysDullerThanCreaturesTest,
	"BattleSquare.World.Training.TerrainIsAlwaysDullerThanCreatures",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FTerrainIsAlwaysDullerThanCreaturesTest::RunTest(const FString& Parameters)
{
	FPetTypeCatalog Tipos;
	const FString Caminho = FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("PetTypes.json"));
	if (!FPetTypeCatalog::LoadFromJson(Caminho, Tipos))
	{
		AddError(TEXT("Config/PetTypes.json não carregou"));
		return false;
	}

	float SaturacaoMinimaDeCriatura = 1.0f;
	for (const FPetElementDefinition& Elemento : Tipos.GetElements())
	{
		SaturacaoMinimaDeCriatura = FMath::Min(
			SaturacaoMinimaDeCriatura, Elemento.Color.LinearRGBToHSV().G);
	}

	const TCHAR* Atributos[] = {
		TEXT("musculature"), TEXT("personality"),
		TEXT("camouflage"), TEXT("flight"), TEXT("underground"),
	};

	for (const TCHAR* Atributo : Atributos)
	{
		const float SaturacaoDoCampo =
			AWorldTrainingField::ColorForAttribute(Atributo).LinearRGBToHSV().G;

		TestTrue(
			*FString::Printf(TEXT("O campo de %s (sat %.2f) é mais apagado que qualquer criatura (mín %.2f)"),
				Atributo, SaturacaoDoCampo, SaturacaoMinimaDeCriatura),
			SaturacaoDoCampo < SaturacaoMinimaDeCriatura);
	}

	return true;
}


namespace
{
	/** Os cinco atributos que existem, na grafia do requisito de golpe. */
	TArray<FString> AtributosDoCampoDeTreino()
	{
		return {
			TEXT("musculature"), TEXT("personality"),
			TEXT("camouflage"), TEXT("flight"), TEXT("underground"),
		};
	}
}

// AS PEÇAS DO CAMPO SAEM DO ELENCO DA FLORESTA.
//
// O campo era um disco chapado de cor lisa deitado na grama, e foi a primeira
// coisa que o jogador apontou na tela: "uma forma redonda marrom, isso pode
// sair, não tem porque". Agora ele é feito de peças do MESMO pacote que planta
// a mata em volta — e o modo de falhar novo é escrever aqui o nome de uma peça
// que aquele elenco não planta, ou que nem existe no pacote. Nenhum dos dois
// avisa: o campo simplesmente nasceria sem marco, ou com a única peça que a
// mata em volta não tem, voltando a parecer colado de fora.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTrainingMarkerComesFromForestCastTest,
	"BattleSquare.World.Training.MarcoSaiDoElencoDaFloresta",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTrainingMarkerComesFromForestCastTest::RunTest(const FString& Parameters)
{
	const TArray<FString> Pacote = AForestBackdrop::SpeciesNames();
	TSet<FString> MarcosVistos;

	for (const FString& Atributo : AtributosDoCampoDeTreino())
	{
		const FTrainingFieldMarker Marco = AWorldTrainingField::MarkerForAttribute(Atributo);

		TestFalse(FString::Printf(TEXT("%s tem marco"), *Atributo), Marco.MarkerSpecies.IsEmpty());
		TestFalse(FString::Printf(TEXT("%s tem peça de borda"), *Atributo), Marco.BorderSpecies.IsEmpty());

		TestTrue(FString::Printf(TEXT("o marco de %s existe no pacote"), *Atributo),
			Pacote.Contains(Marco.MarkerSpecies));
		TestTrue(FString::Printf(TEXT("a borda de %s existe no pacote"), *Atributo),
			Pacote.Contains(Marco.BorderSpecies));

		TestTrue(FString::Printf(TEXT("o marco de %s é da floresta"), *Atributo),
			BiomeFlora::FitsBiome(Marco.MarkerSpecies, EIslandBiome::Forest));
		TestTrue(FString::Printf(TEXT("a borda de %s é da floresta"), *Atributo),
			BiomeFlora::FitsBiome(Marco.BorderSpecies, EIslandBiome::Forest));

		MarcosVistos.Add(Marco.MarkerSpecies);
	}

	// CINCO MARCOS DIFERENTES. Se dois atributos partilhassem a peça do centro,
	// as duas clareiras ficariam iguais de longe — e é justamente de longe que
	// o jogador decide para qual delas andar.
	TestEqual(TEXT("Cada atributo tem o SEU marco"), MarcosVistos.Num(), 5);

	// O desconhecido ganha peça, e não o nada: marco invisível seria campo que
	// não está na tela. Quem denuncia o erro é a cor cinza.
	const FTrainingFieldMarker Torto = AWorldTrainingField::MarkerForAttribute(TEXT("nao_existe"));
	TestFalse(TEXT("Atributo torto ainda ganha um marco"), Torto.MarkerSpecies.IsEmpty());
	TestEqual(TEXT("E a cor dele denuncia"),
		AWorldTrainingField::ColorForAttribute(TEXT("nao_existe")), FLinearColor(0.5f, 0.5f, 0.5f));

	return true;
}

// O ANEL É QUEM DIZ ATÉ ONDE O CAMPO VAI, e é o que substituiu o contorno do
// disco. Sem ele o campo teria um marco no meio e nenhuma borda — o jogador
// veria a pedra e continuaria sem saber onde parar de andar.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTrainingRingReplacesTheDiscTest,
	"BattleSquare.World.Training.AnelSubstituiODisco",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTrainingRingReplacesTheDiscTest::RunTest(const FString& Parameters)
{
	const AWorldTrainingField* Padrao = GetDefault<AWorldTrainingField>();
	TestTrue(TEXT("O anel nasce com malha ATRIBUÍDA no CDO"),
		Padrao->GetBorderProps() != nullptr && Padrao->GetBorderProps()->GetStaticMesh() != nullptr);

	UWorld* Mundo = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld=*/false);
	if (!Mundo)
	{
		AddError(TEXT("não foi possível criar mundo de teste"));
		return false;
	}

	AWorldTrainingField* Campo = Mundo->SpawnActor<AWorldTrainingField>(
		AWorldTrainingField::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
	Campo->FieldRadiusUnits = 400.0f;
	Campo->TrainedAttribute = TEXT("flight");
	Campo->RebuildMarker();

	UHierarchicalInstancedStaticMeshComponent* Anel = Campo->GetBorderProps();
	const int32 Plantadas = Anel ? Anel->GetInstanceCount() : 0;
	TestTrue(TEXT("O anel tem peças"), Plantadas > 0);

	int32 ForaDaBorda = 0;
	for (int32 Indice = 0; Indice < Plantadas; ++Indice)
	{
		FTransform Pouso;
		Anel->GetInstanceTransform(Indice, Pouso, /*bWorldSpace=*/false);

		const FVector Local = Pouso.GetLocation();
		const float Distancia = FVector2D(Local.X, Local.Y).Size();
		if (Distancia < Campo->FieldRadiusUnits - 60.0f || Distancia > Campo->FieldRadiusUnits + 60.0f)
		{
			++ForaDaBorda;
		}
	}
	TestEqual(TEXT("Toda peça do anel fica NA borda"), ForaDaBorda, 0);

	// O marco segue o atributo: pedir voo e receber a pedra da musculatura era
	// o defeito de ordem que fez as cinco clareiras nascerem iguais.
	UStaticMesh* MalhaDoMarco = Campo->GetFieldMesh()->GetStaticMesh();
	TestTrue(TEXT("O marco de voo é a árvore alta"),
		MalhaDoMarco != nullptr && MalhaDoMarco->GetName() == TEXT("tree_tall"));

	// E A PROVA DE QUE O DISCO SAIU: a escala do marco é UNIFORME. O disco era
	// 8 x 8 x 0,12 — uma elipse chapada —, e nenhuma peça da mata é isso.
	const FVector Escala = Campo->GetFieldMesh()->GetRelativeScale3D();
	TestTrue(TEXT("O marco não é um disco achatado"),
		FMath::IsNearlyEqual(Escala.X, Escala.Z, 0.01) && FMath::IsNearlyEqual(Escala.X, Escala.Y, 0.01));

	// REMONTAR NÃO DUPLICA. Quem cria os campos atribui o atributo depois do
	// nascimento, então esta função corre duas vezes por campo, sempre.
	Campo->TrainedAttribute = TEXT("personality");
	Campo->RebuildMarker();

	TestEqual(TEXT("Remontar mantém a conta do anel"), Anel->GetInstanceCount(), Plantadas);

	UStaticMesh* MalhaNova = Campo->GetFieldMesh()->GetStaticMesh();
	TestTrue(TEXT("E troca o marco junto com o atributo"),
		MalhaNova != nullptr && MalhaNova->GetName() == TEXT("stump_round"));

	Mundo->DestroyWorld(false);
	return true;
}
